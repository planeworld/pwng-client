#include <atomic>
#include <chrono>
#include <ctime>
#include <iostream>
#include <thread>

#include <Magnum/GL/Renderer.h>
#include <Magnum/Math/Vector2.h>
#include <Magnum/MeshTools/Compile.h>
#include <Magnum/Primitives/Line.h>
#include <Magnum/Trade/MeshData.h>

#include <argagg/argagg.hpp>
#include <concurrentqueue/concurrentqueue.h>
#include <entt/entity/registry.hpp>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "components.hpp"
#include "message_handler.hpp"
#include "network_manager.hpp"
#include "pwng_client.hpp"
#include "ui_manager.hpp"

PwngClient::PwngClient(const Arguments& arguments): Platform::Application{arguments, NoCreate},
                                                    TemperaturePalette_(Reg_, 256, {0.95, 0.58, 0.26}, {0.47, 0.56, 1.0})

{
    Reg_.set<MessageHandler>();
    Reg_.set<NetworkManager>(Reg_);
    Reg_.set<UIManager>(Reg_, ImGUI_);

    auto& Messages = Reg_.ctx<MessageHandler>();

    Messages.registerSource("col", "col");
    Messages.registerSource("net", "net");
    Messages.registerSource("prg", "prg");
    Messages.registerSource("ui", "ui");

    this->setupCamera();
    this->setupWindow();
    this->setupNetwork();
    this->setupGraphics();
    setSwapInterval(1);
    setMinimalLoopPeriod(1.0f/60.0f * 1000.0f);
}

void PwngClient::drawEvent()
{
    GL::defaultFramebuffer.clearColor(Color4(0.0f, 0.0f, 0.0f, 1.0f));

    this->getObjectsFromQueue();
    this->updateCameraHook();
    this->renderScene();
    this->renderScale();

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
            auto& Pos = Reg_.get<SystemPositionComponent>(Camera_);
            auto& Zoom = Reg_.get<ZoomComponent>(Camera_);
            if (Event.modifiers() & MouseMoveEvent::Modifier::Shift)
            {
                if (Zoom.z - 0.01*Event.relativePosition().y()*Zoom.z > 0.0)
                    Zoom.z -= 0.01*Event.relativePosition().y()*Zoom.z;
                if (Zoom.z < 1.0e-22) Zoom.z = 1.0e-22;
                else if (Zoom.z > 100.0) Zoom.z = 100.0;
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
        rapidjson::Document j;
        j.Parse(Data.c_str());

        if (j["method"] == "galaxy_data")
        {

            std::string n = j["params"]["name"].GetString();
            double      m = j["params"]["m"].GetDouble();
            double      x = j["params"]["spx"].GetDouble();
            double      y = j["params"]["spy"].GetDouble();
            double      r = j["params"]["r"].GetDouble();
            int        SC = j["params"]["sc"].GetInt();
            double      t = j["params"]["t"].GetDouble();

            entt::id_type Id = j["params"]["eid"].GetInt();

            auto ci = Id2EntityMap_.find(Id);
            if (ci != Id2EntityMap_.end())
            {
                Reg_.replace<RadiusComponent>(ci->second, r);
                Reg_.replace<NameComponent>(ci->second, n);
                Reg_.replace<MassComponent>(ci->second, m);
                Reg_.replace<SystemPositionComponent>(ci->second, x, y);
                Reg_.replace<StarDataComponent>(ci->second, SpectralClassE(SC), t);
                DBLK(Messages.report("prg", "Entity components updated", MessageHandler::DEBUG_L3);)
            }
            else
            {
                auto e = Reg_.create();
                Reg_.emplace<RadiusComponent>(e, r);
                Reg_.emplace<NameComponent>(e, n);
                Reg_.emplace<MassComponent>(e, m);
                Reg_.emplace<SystemPositionComponent>(e, x, y);
                Reg_.emplace<StarDataComponent>(e, SpectralClassE(SC), t);
                Id2EntityMap_[Id] = e;
                DBLK(Messages.report("prg", "Entity created", MessageHandler::DEBUG_L2);)
            }
        }
        if (j["method"] == "bc_dynamic_data")
        {
            std::string n = j["params"]["name"].GetString();
            double      m = j["params"]["m"].GetDouble();
            double    spx = j["params"]["spx"].GetDouble();
            double    spy = j["params"]["spy"].GetDouble();
            double     px = j["params"]["px"].GetDouble();
            double     py = j["params"]["py"].GetDouble();
            double      r = j["params"]["r"].GetDouble();

            entt::id_type Id = j["params"]["eid"].GetInt();

            auto ci = Id2EntityMap_.find(Id);
            if (ci != Id2EntityMap_.end())
            {
                Reg_.replace<NameComponent>(ci->second, n);
                Reg_.replace<MassComponent>(ci->second, m);
                Reg_.replace<PositionComponent>(ci->second, px, py);
                Reg_.replace<RadiusComponent>(ci->second, r);
                Reg_.replace<SystemPositionComponent>(ci->second, spx, spy);
                DBLK(Messages.report("prg", "Entity components updated", MessageHandler::DEBUG_L3);)
            }
            else
            {
                auto e = Reg_.create();
                Reg_.emplace<NameComponent>(e, n);
                Reg_.emplace<MassComponent>(e, m);
                Reg_.emplace<PositionComponent>(e, px, py);
                Reg_.emplace<RadiusComponent>(e, r);
                Reg_.emplace<SystemPositionComponent>(e, spx, spy);
                Id2EntityMap_[Id] = e;
                DBLK(Messages.report("prg", "Entity created", MessageHandler::DEBUG_L2);)
            }
        }
        else if (j["method"] == "sim_stats")
        {
            Timers_.ServerPhysicsFrameTimeAvg.addValue(j["params"]["t_phy"].GetDouble());
            Timers_.ServerQueueOutFrameTimeAvg.addValue(j["params"]["t_queue_out"].GetDouble());
            Timers_.ServerSimFrameTimeAvg.addValue(j["params"]["t_sim"].GetDouble());
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
        auto& p = Reg_.get<SystemPositionComponent>(h.e);
        h.x = p.x;
        h.y = p.y;
    }
}

void PwngClient::renderScale()
{
    auto& Zoom = Reg_.get<ZoomComponent>(Camera_);

    constexpr double SCALE_BAR_SIZE_MAX = 0.2; // 20% of display width

    double BarLength{1.0};
    // double Ratio = windowSize().x() * SCALE_BAR_SIZE_MAX / (BarLength*Zoom.z);
    double Ratio = windowSize().x() * SCALE_BAR_SIZE_MAX / Zoom.z;
    if (Ratio > 9.46e21)
    {
        BarLength = 9.46e21; // One million lightyears
        ScaleUnit_ = ScaleUnitE::MLY;
    }
    else if (Ratio > 9.46e15)
    {
        BarLength = 9.46e15; // One lightyear
        ScaleUnit_ = ScaleUnitE::LY;
    }
    else if (Ratio > 1.0e9)
    {
        BarLength = 1.0e9;
        ScaleUnit_ = ScaleUnitE::MKM;
    }
    else if (Ratio > 1.0e3)
    {
        BarLength = 1.0e3;
        ScaleUnit_ = ScaleUnitE::KM;
    }
    else
    {
        BarLength = 1.0;
        ScaleUnit_ = ScaleUnitE::M;
    }

    double Scale = windowSize().x() * SCALE_BAR_SIZE_MAX / (BarLength*Zoom.z);

    Scale_ = int(std::log10(Scale));

    Shader_.setTransformationProjectionMatrix(
        Projection_ * Matrix3::scaling(
            Vector2(BarLength*0.5*std::pow(10, Scale_)*Zoom.z,
                    0.5 * windowSize().y() - 10)
    ));

    Shader_.setColor({0.8, 0.8, 1.0})
           .draw(ScaleLineShapeH_);

    Shader_.setTransformationProjectionMatrix(
        Projection_ *
        Matrix3::translation(
            Vector2(- BarLength*0.5*std::pow(10, Scale_)*Zoom.z,
                    0.5 * windowSize().y() - 10)) *
        Matrix3::scaling(
            Vector2(1.0, 7.0))
    );
    Shader_.draw(ScaleLineShapeV_);

    Shader_.setTransformationProjectionMatrix(
        Projection_ *
        Matrix3::translation(
            Vector2(BarLength*0.5*std::pow(10, Scale_)*Zoom.z,
                    0.5 * windowSize().y() - 10)) *
        Matrix3::scaling(
            Vector2(1.0, 7.0))
    );
    Shader_.draw(ScaleLineShapeV_);
}

void PwngClient::renderScene()
{
    GL::defaultFramebuffer.setViewport({{}, windowSize()});

    auto& Hook = Reg_.get<HookComponent>(Camera_);
    auto& Pos = Reg_.get<SystemPositionComponent>(Camera_);
    auto& Zoom = Reg_.get<ZoomComponent>(Camera_);

    // Remove all "outside" tags from objects
    // After that, test all objects for camera viewport and tag
    // appropriatly
    Timers_.ViewportTest.start();
    Reg_.view<SystemPositionComponent, entt::tag<"is_outside"_hs>>().each(
        [this](auto _e, const auto& _p)
        {
            Reg_.remove<entt::tag<"is_outside"_hs>>(_e);
        });

    const double ScreenX = windowSize().x();
    const double ScreenY = windowSize().y();

    Reg_.view<SystemPositionComponent>().each(
        [this, &Hook, &Pos, &ScreenX, &ScreenY, &Zoom](auto _e, const auto& _p)
        {
            auto x = _p.x;
            auto y = _p.y;

            x -= Pos.x + Hook.x;
            y += Pos.y - Hook.y;

            // std::cout << "PRE_LOCAL" << std::endl;
            // const PositionComponent* PosLocal = Reg_.try_get<PositionComponent>(_e);
            // if (PosLocal != nullptr)
            // {
            //     std::cout << "LOCAL" << std::endl;
            //     x -= PosLocal->x;
            //     y += PosLocal->y;
            // }

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
    Reg_.view<SystemPositionComponent, RadiusComponent, StarDataComponent>(entt::exclude<entt::tag<"is_outside"_hs>>).each(
        [&](auto _e, const auto& _p, const auto& _r, const auto& _s)
    {
        auto x = _p.x;
        auto y = _p.y;

        x -= Pos.x + Hook.x;
        y += Pos.y - Hook.y;
        x *= Zoom.z;
        y *= Zoom.z;

        auto r = _r.r;
        r *= Zoom.z * StarsDisplayScaleFactor_;
        if (r < StarsDisplaySizeMin_) r=StarsDisplaySizeMin_;

        Shader_.setTransformationProjectionMatrix(
            Projection_ *
            Matrix3::translation(Vector2(x, y)) *
            Matrix3::scaling(Vector2(r, r))
        );

        Shader_.setColor(TemperaturePalette_.getColorClip((_s.Temperature-2000.0)/45000.0));
        Shader_.draw(CircleShape_);
    });

    Reg_.view<SystemPositionComponent, RadiusComponent, PositionComponent>(entt::exclude<entt::tag<"is_outside"_hs>>).each(
        [&](auto _e, const auto& _sp, const auto& _r, const auto& _p)
    {
        auto x = _sp.x;
        auto y = _sp.y;

        x -= Pos.x + Hook.x;
        y += Pos.y - Hook.y;
        x -= _p.x;
        y += _p.y;
        x *= Zoom.z;
        y *= Zoom.z;

        auto r = _r.r;
        r *= Zoom.z * StarsDisplayScaleFactor_;
        if (r < StarsDisplaySizeMin_) r=StarsDisplaySizeMin_;

        Shader_.setTransformationProjectionMatrix(
            Projection_ *
            Matrix3::translation(Vector2(x, y)) *
            Matrix3::scaling(Vector2(r, r))
        );

        Shader_.draw(CircleShape_);
    });

    Timers_.Render.stop();
    Timers_.RenderAvg.addValue(Timers_.Render.elapsed());
}

void PwngClient::setupCamera()
{
    Camera_ = Reg_.create();
    Reg_.emplace<HookComponent>(Camera_);
    Reg_.emplace<SystemPositionComponent>(Camera_);
    Reg_.emplace<ZoomComponent>(Camera_);
}

void PwngClient::setupGraphics()
{
    Shader_ = Shaders::Flat2D{};
    CircleShape_ = MeshTools::compile(Primitives::circle2DSolid(100));
    ScaleLineShapeH_ = MeshTools::compile(Primitives::line2D({-1.0, 1.0},
                                                             { 1.0, 1.0}));
    ScaleLineShapeV_ = MeshTools::compile(Primitives::line2D({ 1.0, -1.0},
                                                             { 1.0,  1.0}));

    TemperaturePalette_.addSupportPoint(0.03, {1.0, 0.8, 0.21});
    TemperaturePalette_.addSupportPoint(0.11, {1.0, 0.93, 0.27});
    TemperaturePalette_.addSupportPoint(0.15, {1.0, 0.97, 0.7});
    TemperaturePalette_.addSupportPoint(0.19, {0.82, 0.92, 1.0});
    TemperaturePalette_.addSupportPoint(0.42, {0.4, 0.74, 1.0});
    TemperaturePalette_.buildLuT();
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
                if (ImGui::Button("Get Static Galaxy Data"))
                {
                    this->sendJsonRpcMessage("get_data", "c000");
                }
                if (ImGui::Button("Subscribe: Dynamic Data"))
                {
                    this->sendJsonRpcMessage("sub_dynamic_data", "c001");
                }
                if (ImGui::Button("Start Simulation"))
                {
                    this->sendJsonRpcMessage("start_simulation", "c002");
                }
                if (ImGui::Button("Stop Simulation"))
                {
                    this->sendJsonRpcMessage("stop_simulation", "c003");
                }
                if (ImGui::Button("Shutdown Server"))
                {
                    this->sendJsonRpcMessage("shutdown", "c0004");
                }
            ImGui::Unindent();
            ImGui::TextColored(ImVec4(1,1,0,1), "Display");
            ImGui::Indent();
                auto& Zoom = Reg_.get<ZoomComponent>(Camera_);
                ImGui::SliderFloat("Stars: Minimum Display Size", &StarsDisplaySizeMin_, 0.1, 20.0);
                // ImGui::SliderFloat("Stars: Display Scale Factor", &StarsDisplayScaleFactor_, 1.0, std::max(5.0, std::min(1.0e-7/Zoom.z, 1.0e11)));
                ImGui::SliderFloat("Stars: Display Scale Factor", &StarsDisplayScaleFactor_, 1.0, std::clamp(1.0e-7/Zoom.z, 5.0, 1.0e11));
                if (StarsDisplayScaleFactor_ >  1.0e-7/Zoom.z) StarsDisplayScaleFactor_= 1.0e-7/Zoom.z;
                if (StarsDisplayScaleFactor_ <  1.0) StarsDisplayScaleFactor_= 1.0;
            ImGui::Unindent();
            UI.processObjectLabels();
        ImGui::End();
        UI.displayObjectLabels(Camera_);
        ImGuiWindowFlags WindowFlags =  ImGuiWindowFlags_NoDecoration |
                                        ImGuiWindowFlags_AlwaysAutoResize |
                                        ImGuiWindowFlags_NoSavedSettings |
                                        ImGuiWindowFlags_NoFocusOnAppearing |
                                        ImGuiWindowFlags_NoInputs |
                                        ImGuiWindowFlags_NoNav |
                                        ImGuiWindowFlags_NoMove;
        bool CloseButton{false};

        ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x / 2, 40), ImGuiCond_Always, ImVec2(0.5f,0.5f));
        ImGui::Begin("Scale", &CloseButton, WindowFlags);
            int l = std::pow(10,Scale_);
            if (ScaleUnit_ == ScaleUnitE::MLY)
                ImGui::Text("Scale: %d million ly", l);
            else if (ScaleUnit_ == ScaleUnitE::LY)
                ImGui::Text("Scale: %d ly", l);
            else if (ScaleUnit_ == ScaleUnitE::MKM)
                ImGui::Text("Scale: %d million km", l);
            else if (ScaleUnit_ == ScaleUnitE::KM)
                ImGui::Text("Scale: %d km", l);
            else if (ScaleUnit_ == ScaleUnitE::M)
                ImGui::Text("Scale: %d m", l);
        ImGui::End();
    }
    ImGUI_.drawFrame();

    GL::Renderer::setBlendEquation(GL::Renderer::BlendEquation::Add,
    GL::Renderer::BlendEquation::Add);
    GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::SourceAlpha,
    GL::Renderer::BlendFunction::OneMinusSourceAlpha);
}

void PwngClient::sendJsonRpcMessage(const std::string& _Msg, const std::string& _ID)
{
    using namespace rapidjson;
    StringBuffer s;
    Writer<StringBuffer> w(s);

    w.StartObject();
    w.Key("jsonrpc"); w.String("2.0");
    w.Key("method"); w.String("send");
    w.Key("params");
        w.StartObject();
        w.Key("Message"); w.String(_Msg.c_str());
        w.EndObject();
    w.Key("id"); w.String(_ID.c_str());
    w.EndObject();
    OutputQueue_.enqueue(s.GetString());
}

MAGNUM_APPLICATION_MAIN(PwngClient)
