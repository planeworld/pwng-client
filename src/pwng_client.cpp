#include <atomic>
#include <chrono>
#include <ctime>
#include <iostream>
#include <thread>

#include <Magnum/GL/Renderer.h>
#include <Magnum/ImGuiIntegration/Context.hpp>

#include <argagg/argagg.hpp>
#include <concurrentqueue/concurrentqueue.h>
#include <entt/entity/registry.hpp>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "components.hpp"
#include "message_handler.hpp"
#include "name_system.hpp"
#include "network_manager.hpp"
#include "pwng_client.hpp"
#include "render_system.hpp"
#include "ui_manager.hpp"

PwngClient::PwngClient(const Arguments& arguments): Platform::Application{arguments, NoCreate}

{
    Reg_.set<MessageHandler>();
    Reg_.set<NameSystem>(Reg_);
    Reg_.set<NetworkManager>(Reg_);
    Reg_.set<RenderSystem>(Reg_);
    Reg_.set<UIManager>(Reg_, ImGUI_);

    auto& Messages = Reg_.ctx<MessageHandler>();

    Messages.registerSource("col", "col");
    Messages.registerSource("net", "net");
    Messages.registerSource("prg", "prg");
    Messages.registerSource("gfx", "gfx");
    Messages.registerSource("ui", "ui");

    this->setupWindow();
    this->setupNetwork();
    Reg_.ctx<RenderSystem>().setupCamera();
    Reg_.ctx<RenderSystem>().setupGraphics();
    setSwapInterval(1);
    setMinimalLoopPeriod(1.0f/60.0f * 1000.0f);
}

void PwngClient::drawEvent()
{
    auto& Renderer = Reg_.ctx<RenderSystem>();

    GL::defaultFramebuffer.clearColor(Color4(0.0f, 0.0f, 0.0f, 1.0f));

    this->getObjectsFromQueue();
    Renderer.renderScene();
    Renderer.renderScale();

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
            const entt::entity Camera = Reg_.ctx<RenderSystem>().getCamera();

            auto& SysPos = Reg_.get<SystemPositionComponent>(Camera);
            auto& Pos = Reg_.get<PositionComponent>(Camera);
            auto& Zoom = Reg_.get<ZoomComponent>(Camera);
            if (Event.modifiers() & MouseMoveEvent::Modifier::Shift)
            {
                if (Zoom.z - 0.01*Event.relativePosition().y()*Zoom.z > 0.0)
                    Zoom.z -= 0.01*Event.relativePosition().y()*Zoom.z;
                if (Zoom.z < 1.0e-22) Zoom.z = 1.0e-22;
                else if (Zoom.z > 100.0) Zoom.z = 100.0;
            }
            else
            {
                if (Zoom.z < 1.0e12)
                {
                    Pos.x -= Event.relativePosition().x() / Zoom.z;
                    Pos.y += Event.relativePosition().y() / Zoom.z;
                }
                else
                {
                    SysPos.x -= Event.relativePosition().x() / Zoom.z;
                    SysPos.y += Event.relativePosition().y() / Zoom.z;
                }
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

void PwngClient::mouseScrollEvent(MouseScrollEvent& Event)
{
    if (!ImGUI_.handleMouseScrollEvent(Event))
    {
        const entt::entity Camera = Reg_.ctx<RenderSystem>().getCamera();
        auto& Zoom = Reg_.get<ZoomComponent>(Camera);

        double ZoomSpeed = std::pow(10.0, Event.offset().y());
        if (Event.modifiers() & MouseScrollEvent::Modifier::Ctrl)
        {
            ZoomSpeed = std::pow(10.0, Event.offset().y()*0.1);
        }

        Zoom.t = Zoom.z * ZoomSpeed;
        Zoom.i = (Zoom.t - Zoom.z) / Zoom.s;
        Zoom.c = 0;
    }
}

void PwngClient::textInputEvent(TextInputEvent& Event)
{
    ImGUI_.handleTextInputEvent(Event);
}

void PwngClient::viewportEvent(ViewportEvent& Event)
{
    GL::defaultFramebuffer.setViewport({{}, Event.framebufferSize()});

    Reg_.ctx<RenderSystem>().setWindowSize(Event.windowSize().x(), Event.windowSize().y());

    ImGUI_.relayout(Vector2(Event.windowSize()), Event.windowSize(), Event.framebufferSize());
}

void PwngClient::getObjectsFromQueue()
{
    Timers_.Queue.start();
    auto& Messages = Reg_.ctx<MessageHandler>();
    auto& Names = Reg_.ctx<NameSystem>();

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
                Reg_.emplace_or_replace<RadiusComponent>(ci->second, r);
                Reg_.emplace_or_replace<MassComponent>(ci->second, m);
                Reg_.emplace_or_replace<SystemPositionComponent>(ci->second, x, y);
                Reg_.emplace_or_replace<StarDataComponent>(ci->second, SpectralClassE(SC), t);
                Reg_.emplace_or_replace<NameComponent>(ci->second);
                Names.setName(ci->second, n);
                DBLK(Messages.report("prg", "Entity components updated", MessageHandler::DEBUG_L3);)
            }
            else
            {
                auto e = Reg_.create();
                Reg_.emplace<RadiusComponent>(e, r);
                Reg_.emplace<MassComponent>(e, m);
                Reg_.emplace<SystemPositionComponent>(e, x, y);
                Reg_.emplace<StarDataComponent>(e, SpectralClassE(SC), t);
                Reg_.emplace<NameComponent>(e);
                Names.setName(e, n);
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
                Reg_.emplace_or_replace<MassComponent>(ci->second, m);
                Reg_.emplace_or_replace<PositionComponent>(ci->second, px, py);
                Reg_.emplace_or_replace<RadiusComponent>(ci->second, r);
                Reg_.emplace_or_replace<SystemPositionComponent>(ci->second, spx, spy);
                Reg_.emplace_or_replace<NameComponent>(ci->second);
                Names.setName(ci->second, n);
                DBLK(Messages.report("prg", "Entity components updated", MessageHandler::DEBUG_L3);)
            }
            else
            {
                auto e = Reg_.create();
                Reg_.emplace<MassComponent>(e, m);
                Reg_.emplace<PositionComponent>(e, px, py);
                Reg_.emplace<RadiusComponent>(e, r);
                Reg_.emplace<SystemPositionComponent>(e, spx, spy);
                Reg_.emplace<NameComponent>(e);
                Names.setName(e, n);
                Id2EntityMap_[Id] = e;
                DBLK(Messages.report("prg", "Entity created", MessageHandler::DEBUG_L2);)
            }
        }
        else if (j["method"] == "sim_stats")
        {
            Timers_.ServerPhysicsFrameTimeAvg.addValue(j["params"]["t_phy"].GetDouble());
            Timers_.ServerQueueInFrameTimeAvg.addValue(j["params"]["t_queue_in"].GetDouble());
            Timers_.ServerQueueOutFrameTimeAvg.addValue(j["params"]["t_queue_out"].GetDouble());
            Timers_.ServerSimFrameTimeAvg.addValue(j["params"]["t_sim"].GetDouble());
        }
    }
    Timers_.Queue.stop();
    Timers_.QueueAvg.addValue(Timers_.Queue.elapsed());
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

    // ImGuiIO& io = ImGui::GetIO();
    // io.Fonts->AddFontFromFileTTF("/home/bfeld/projects/pwng/client/3rdparty/imgui/misc/fonts/MonospaceTypewriter.ttf", 16.0f);
    // UIFont_ = io.Fonts->AddFontFromFileTTF("/home/bfeld/projects/pwng/client/3rdparty/imgui/misc/fonts/MonospaceTypewriter.ttf", 18.0f);
    // io.Fonts->AddFontFromFileTTF("/home/bfeld/projects/pwng/client/3rdparty/imgui/misc/fonts/MonospaceTypewriter.ttf", 20.0f);
    // io.Fonts->AddFontFromFileTTF("/home/bfeld/projects/pwng/client/3rdparty/imgui/misc/fonts/Roboto-Light.ttf", 20.0f);
    // io.Fonts->Build();

    GL::Renderer::setBlendEquation(GL::Renderer::BlendEquation::Add,
    GL::Renderer::BlendEquation::Add);
    GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::SourceAlpha,
    GL::Renderer::BlendFunction::OneMinusSourceAlpha);
}

void PwngClient::updateUI()
{
    auto& Messages = Reg_.ctx<MessageHandler>();
    auto& Network = Reg_.ctx<NetworkManager>();
    auto Camera = Reg_.ctx<RenderSystem>().getCamera();

    GL::Renderer::enable(GL::Renderer::Feature::Blending);
    GL::Renderer::enable(GL::Renderer::Feature::ScissorTest);
    GL::Renderer::disable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::disable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::disable(GL::Renderer::Feature::StencilTest);

    ImGUI_.newFrame();
    {
        // ImGui::ShowDemoWindow();
        // ImGui::ShowFontSelector("Font");

        auto& UI = Reg_.ctx<UIManager>();
        ImGui::Begin("PwNG Desktop Client");

            UI.displayPerformance(Timers_);

            ImGui::TextColored(ImVec4(1,1,0,1), "Client control");
            ImGui::Indent();

                UI.processHelp();
                UI.processConnections();

                if (ImGui::Button("Get Static Galaxy Data"))
                {
                    this->sendJsonRpcMessage("get_data", "c000");
                }
                if (ImGui::Button("Subscribe: Dynamic Data"))
                {
                    this->sendJsonRpcMessage("sub_dynamic_data", "c001");
                }
                if (ImGui::Button("Subscribe: Server Stats"))
                {
                    this->sendJsonRpcMessage("sub_server_stats", "c001");
                }

                if (ImGui::Button("Quit Client"))
                {
                    Network.quit();
                    Platform::Application::Sdl2Application::exit();
                }

                UI.processVerbosity();

            ImGui::Unindent();
            ImGui::Indent();
                UI.processCameraHooks(Camera);
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
                auto& Zoom = Reg_.get<ZoomComponent>(Camera);
                // ImGui::SliderFloat("Stars: Minimum Display Size", &StarsDisplaySizeMin_, 0.1, 20.0);
                // ImGui::SliderFloat("Stars: Display Scale Factor", &StarsDisplayScaleFactor_, 1.0, std::clamp(1.0e-7/Zoom.z, 5.0, 1.0e11));
                // if (StarsDisplayScaleFactor_ >  1.0e-7/Zoom.z) StarsDisplayScaleFactor_= 1.0e-7/Zoom.z;
                // if (StarsDisplayScaleFactor_ <  1.0) StarsDisplayScaleFactor_= 1.0;
            ImGui::Unindent();
            UI.processObjectLabels();
        ImGui::End();
        UI.displayObjectLabels(Camera);
        UI.displayHelp();
        UI.displayScale(Scale_, ScaleUnit_);
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
