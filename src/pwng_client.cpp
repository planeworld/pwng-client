#include <atomic>
#include <chrono>
#include <ctime>
#include <iostream>
#include <thread>

#include <Magnum/GL/Renderer.h>
#include <Magnum/Math/Vector2.h>
#include <Magnum/MeshTools/Compile.h>
#include <Magnum/Trade/MeshData.h>

#include <argagg/argagg.hpp>
#include <concurrentqueue/concurrentqueue.h>
#include <entt/entity/registry.hpp>
#include <nlohmann/json.hpp>

#include "components.hpp"
#include "message_handler.hpp"
#include "network_manager.hpp"
#include "pwng_client.hpp"
#include "ui_manager.hpp"

using json = nlohmann::json;

PwngClient::PwngClient(const Arguments& arguments): Platform::Application{arguments, NoCreate}
{
    Reg_.set<MessageHandler>();
    Reg_.set<NetworkManager>(Reg_);
    Reg_.set<UIManager>(Reg_, ImGUI_);

    auto& Messages = Reg_.ctx<MessageHandler>();

    Messages.registerSource("net", "net");
    Messages.registerSource("prg", "prg");
    Messages.registerSource("ui", "ui");

    this->setupCamera();
    this->setupWindow();
    this->setupNetwork();
    setSwapInterval(1);
    setMinimalLoopPeriod(1.0f/60.0f * 1000.0f);

    Shader_ = Shaders::Flat2D{};
    CircleShape_ = MeshTools::compile(Primitives::circle2DSolid(100));
}

void PwngClient::drawEvent()
{
    GL::defaultFramebuffer.clearColor(Color4(0.0f, 0.0f, 0.0f, 1.0f));

    this->getObjectsFromQueue();
    this->updateCameraHook();
    this->renderScene();

    this->updateUI();
    swapBuffers();
    redraw();
}

void PwngClient::keyPressEvent(KeyEvent& Event)
{
    ImGUI_.handleKeyPressEvent(Event);
}

void PwngClient::keyReleaseEvent(KeyEvent& Event)
{
    ImGUI_.handleKeyReleaseEvent(Event);
}

void PwngClient::mouseMoveEvent(MouseMoveEvent& Event)
{
    if (!ImGUI_.handleMouseMoveEvent(Event))
    {
        if (Event.modifiers() & MouseMoveEvent::Modifier::Ctrl)
        {
            auto& Pos = Reg_.get<PositionComponent>(Camera_);
            auto& Zoom = Reg_.get<ZoomComponent>(Camera_);
            if (Event.modifiers() & MouseMoveEvent::Modifier::Shift)
            {
                if (Zoom.z - 0.01*Event.relativePosition().y()*Zoom.z > 0.0)
                    Zoom.z -= 0.01*Event.relativePosition().y()*Zoom.z;
            }
            else
            {
                Pos.x += Event.relativePosition().x() / Zoom.z;
                Pos.y += Event.relativePosition().y() / Zoom.z;
            }
        }
    }

}

void PwngClient::mousePressEvent(MouseEvent& Event)
{
    ImGUI_.handleMousePressEvent(Event);
}

void PwngClient::mouseReleaseEvent(MouseEvent& Event)
{
    ImGUI_.handleMouseReleaseEvent(Event);
}

void PwngClient::textInputEvent(TextInputEvent& Event)
{
    ImGUI_.handleTextInputEvent(Event);
}

void PwngClient::viewportEvent(ViewportEvent& Event)
{
    GL::defaultFramebuffer.setViewport({{}, Event.framebufferSize()});

    Projection_= Magnum::Matrix3::projection(Magnum::Vector2(windowSize()));

    ImGUI_.relayout(Vector2(Event.windowSize()), Event.windowSize(), Event.framebufferSize());
}

void PwngClient::getObjectsFromQueue()
{
    Timers_.Queue.start();
    auto& Messages = Reg_.ctx<MessageHandler>();

    std::string Data;
    while (InputQueue_.try_dequeue(Data))
    {
        json j = json::parse(Data);

        if (j["method"] == "sim_broadcast")
        {

            std::string s{j["params"]["name"]};
            double m = j["params"]["m"];
            double x = j["params"]["px"];
            double y = j["params"]["py"];
            double r = j["params"]["r"];
            int    SC = j["params"]["sc"];
            double t = j["params"]["t"];
            double Vx = j["params"]["vx"];
            double Vy = j["params"]["vy"];

            std::uint32_t Id = j["params"]["eid"];

            auto ci = Id2EntityMap_.find(Id);
            if (ci != Id2EntityMap_.end())
            {
                Reg_.replace<RadiusComponent>(ci->second, r);
                Reg_.replace<MassComponent>(ci->second, m);
                Reg_.replace<PositionComponent>(ci->second, x, y);
                Reg_.replace<StarDataComponent>(ci->second, SpectralClassE(SC), t);
                Reg_.replace<VelocityComponent>(ci->second, Vx, Vy);
                DBLK(Messages.report("prg", "Entity components updated", MessageHandler::DEBUG_L3);)
            }
            else
            {
                auto e = Reg_.create();
                Reg_.emplace<RadiusComponent>(e, r);
                Reg_.emplace<NameComponent>(e, s);
                Reg_.emplace<MassComponent>(e, m);
                Reg_.emplace<PositionComponent>(e, x, y);
                Reg_.emplace<StarDataComponent>(e, SpectralClassE(SC), t);
                Reg_.emplace<VelocityComponent>(e, Vx, Vy);
                Id2EntityMap_[Id] = e;
                DBLK(Messages.report("prg", "Entity created", MessageHandler::DEBUG_L2);)
            }
        }
    }
    Timers_.Queue.stop();
    Timers_.QueueAvg.addValue(Timers_.Queue.elapsed());
}

void PwngClient::setCameraHook(entt::entity _e)
{
    auto& h = Reg_.get<HookComponent>(Camera_);
    h.e = _e;
}

void PwngClient::updateCameraHook()
{
    auto& h = Reg_.get<HookComponent>(Camera_);

    if (Reg_.valid(h.e))
    {
        auto& p = Reg_.get<PositionComponent>(h.e);
        h.x = p.x;
        h.y = p.y;
    }
}

void PwngClient::renderScene()
{
    GL::defaultFramebuffer.setViewport({{}, windowSize()});

    auto& Hook = Reg_.get<HookComponent>(Camera_);
    auto& Pos = Reg_.get<PositionComponent>(Camera_);
    auto& Zoom = Reg_.get<ZoomComponent>(Camera_);

    // Remove all "outside" tags from objects
    // After that, test all objects for camera viewport and tag
    // appropriatly
    Timers_.ViewportTest.start();
    Reg_.view<PositionComponent, entt::tag<"is_outside"_hs>>().each(
        [this](auto _e, const auto& _p)
        {
            Reg_.remove<entt::tag<"is_outside"_hs>>(_e);
        });

    const double ScreenX = windowSize().x();
    const double ScreenY = windowSize().y();

    Reg_.view<PositionComponent>().each(
        [this, &Hook, &Pos, &ScreenX, &ScreenY, &Zoom](auto _e, const auto& _p)
        {
            auto x = _p.x;
            auto y = _p.y;

            x -= Pos.x + Hook.x;
            y += Pos.y - Hook.y;
            x *= Zoom.z;
            y *= Zoom.z;

            if (( x+0.5*ScreenX < 0) || (x >= 0.5*ScreenX) ||
                (-y+0.5*ScreenY < 0) || (-y >= 0.5*ScreenY))
            {
                Reg_.emplace<entt::tag<"is_outside"_hs>>(_e);
            }
        });
    Timers_.ViewportTest.stop();
    Timers_.ViewportTestAvg.addValue(Timers_.ViewportTest.elapsed());

    Timers_.Render.start();
    Reg_.view<PositionComponent, RadiusComponent, StarDataComponent>(entt::exclude<entt::tag<"is_outside"_hs>>).each(
        [this, &Hook, &Pos, &Zoom](auto _e, const auto& _p, const auto& _r, const auto& _s)
    {
        auto x = _p.x;
        auto y = _p.y;

        x -= Pos.x + Hook.x;
        y += Pos.y - Hook.y;
        x *= Zoom.z;
        y *= Zoom.z;

        auto r = _r.r;
        if (RealObjectSizes_)
        {
            r *= Zoom.z;
            if (r < 2.0) r=2.0;
        }
        else
        {
            r = r * Zoom.z * 10.0 + 10.0;
        }

        Shader_.setTransformationProjectionMatrix(
            Projection_ *
            Matrix3::translation(Vector2(x, y)) *
            Matrix3::scaling(Vector2(r, r))
        );

        // Shader_.setColor({1.0-_t.t/10000.0, 1.0-_t.t/10000.0, 1.0-_t.t/10000.0});
        Shader_.setColor({1.0-_s.Temperature/30000.0, 0.0, _s.Temperature/30000.0});
        Shader_.draw(CircleShape_);
    });

    Timers_.Render.stop();
    Timers_.RenderAvg.addValue(Timers_.Render.elapsed());
}

void PwngClient::setupCamera()
{
    Camera_ = Reg_.create();
    Reg_.emplace<HookComponent>(Camera_);
    Reg_.emplace<PositionComponent>(Camera_);
    Reg_.emplace<ZoomComponent>(Camera_);
}

void PwngClient::setupNetwork()
{
    auto& Messages = Reg_.ctx<MessageHandler>();
    auto& Network = Reg_.ctx<NetworkManager>();

    Messages.setColored(true);
    Messages.setLevel(MessageHandler::DEBUG_L3);

    Network.init(&InputQueue_, &OutputQueue_);
}

void PwngClient::setupWindow()
{
    /* Try 8x MSAA, fall back to zero samples if not possible. Enable only 2x
       MSAA if we have enough DPI. */
    const Vector2 dpiScaling = this->dpiScaling({});

    Configuration conf;
    conf.setSize({500, 500});

    conf.setTitle("PWNG Desktop Client")
        .setSize(conf.size(), dpiScaling)
        .setWindowFlags(Configuration::WindowFlag::Resizable);

    GLConfiguration glConf;
    glConf.setSampleCount(dpiScaling.max() < 2.0f ? 8 : 2);

    if (!tryCreate(conf, glConf))
    {
        create(conf, glConf.setSampleCount(0));
    }

    // Prepare ImGui
    ImGUI_ = ImGuiIntegration::Context({500.0, 500.0},
    windowSize(), framebufferSize());
    UIStyle_ = &ImGui::GetStyle();
    UIStyleDefault_ = *UIStyle_;
    UIStyleSubStats_ = *UIStyle_;
    UIStyleSubStats_.WindowRounding = 0.0f;

    GL::Renderer::setBlendEquation(GL::Renderer::BlendEquation::Add,
    GL::Renderer::BlendEquation::Add);
    GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::SourceAlpha,
    GL::Renderer::BlendFunction::OneMinusSourceAlpha);
}

void PwngClient::updateUI()
{
    auto& Messages = Reg_.ctx<MessageHandler>();
    auto& Network = Reg_.ctx<NetworkManager>();

    GL::Renderer::enable(GL::Renderer::Feature::Blending);
    GL::Renderer::enable(GL::Renderer::Feature::ScissorTest);
    GL::Renderer::disable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::disable(GL::Renderer::Feature::DepthTest);

    ImGUI_.newFrame();
    {
        auto& UI = Reg_.ctx<UIManager>();
        ImGui::Begin("PwNG Desktop Client");

            UI.displayPerformance(Timers_);

            ImGui::TextColored(ImVec4(1,1,0,1), "Client control");
            ImGui::Indent();
                UI.processConnections();

                if (ImGui::Button("Quit Client"))
                {
                    Network.quit();
                    Platform::Application::Sdl2Application::exit();
                }

                UI.processVerbosity();

            ImGui::Unindent();
            ImGui::Indent();
                UI.processCameraHooks(Camera_);
            ImGui::Unindent();
            ImGui::TextColored(ImVec4(1,1,0,1), "Server control");
            ImGui::Indent();
                static char Id[10] = "1";
                ImGui::Text("ID     ");
                ImGui::SameLine();
                ImGui::InputText("##ID", Id, IM_ARRAYSIZE(Id));
                static char Msg[128] = "Hello Server";
                ImGui::Text("Message");
                ImGui::SameLine();
                ImGui::InputText("##Message", Msg, IM_ARRAYSIZE(Msg));
                ImGui::SameLine();
                if (ImGui::Button("Send"))
                {
                    this->sendJsonRpcMessage(Msg, Id);
                }
                if (ImGui::Button("Start Simulation"))
                {
                    this->sendJsonRpcMessage("start_simulation", "c001");
                }
                if (ImGui::Button("Stop Simulation"))
                {
                    this->sendJsonRpcMessage("stop_simulation", "c001");
                }
                if (ImGui::Button("Shutdown Server"))
                {
                    this->sendJsonRpcMessage("shutdown", "c0002");
                }
            ImGui::Unindent();
            ImGui::TextColored(ImVec4(1,1,0,1), "Display");
            ImGui::Indent();
                ImGui::Checkbox("Real Object Sizes", &RealObjectSizes_);
            ImGui::Unindent();
            UI.processObjectLabels();
        ImGui::End();
        UI.displayObjectLabels(Camera_);
    }
    ImGUI_.drawFrame();

    GL::Renderer::setBlendEquation(GL::Renderer::BlendEquation::Add,
    GL::Renderer::BlendEquation::Add);
    GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::SourceAlpha,
    GL::Renderer::BlendFunction::OneMinusSourceAlpha);
}

void PwngClient::sendJsonRpcMessage(const std::string& _Msg, const std::string& _ID)
{
    json j =  {{"jsonrpc", "2.0"},
               {"method", "send"},
               {"params", {{"Message", _Msg}}},
               {"id", _ID}};
    OutputQueue_.enqueue(j.dump(4));
}

MAGNUM_APPLICATION_MAIN(PwngClient)
