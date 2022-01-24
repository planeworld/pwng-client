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
#include <rapidjson/error/en.h>

#include "components.hpp"
#include "json_manager.hpp"
#include "message_handler.hpp"
#include "name_system.hpp"
#include "network_manager.hpp"
#include "pwng_client.hpp"
#include "render_system.hpp"
#include "ui_manager.hpp"

PwngClient::PwngClient(const Arguments& arguments): Platform::GlfwApplication{arguments, NoCreate}

{
    Reg_.set<JsonManager>(Reg_);
    Reg_.set<MessageHandler>();
    Reg_.set<NameSystem>(Reg_);
    Reg_.set<NetworkManager>(Reg_);
    Reg_.set<RenderSystem>(Reg_, Timers_);
    Reg_.set<UIManager>(Reg_, ImGUI_, &InputQueue_, &OutputQueue_);

    auto& Messages = Reg_.ctx<MessageHandler>();

    Messages.registerSource("col", "col");
    Messages.registerSource("jsn", "jsn");
    Messages.registerSource("net", "net");
    Messages.registerSource("prg", "prg");
    Messages.registerSource("gfx", "gfx");
    Messages.registerSource("ui", "ui");

    this->setupWindow();
    this->setupNetwork();
    Reg_.ctx<RenderSystem>().setupCamera();
    Reg_.ctx<RenderSystem>().setupGraphics();
    setSwapInterval(1);
    // setMinimalLoopPeriod(1.0f/60.0f * 1000.0f);
}

void PwngClient::drawEvent()
{
    auto& Renderer = Reg_.ctx<RenderSystem>();

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
        DBLK(
            // Use stringstream instead of to_string because of precision/scientific format
            // E.g. to_string shows 0.000000 for 1e-20
            std::ostringstream oss;
            oss << "Zoom: " << Zoom.z;
            Reg_.ctx<MessageHandler>().report("prg", oss.str(), MessageHandler::DEBUG_L2);
            )
    }
}

void PwngClient::textInputEvent(TextInputEvent& Event)
{
    ImGUI_.handleTextInputEvent(Event);
}

void PwngClient::viewportEvent(ViewportEvent& Event)
{
    Reg_.ctx<RenderSystem>().setWindowSize(Event.windowSize().x(), Event.windowSize().y());

    ImGUI_.relayout(Vector2(Event.windowSize()), Event.windowSize(), Event.framebufferSize());
}

void PwngClient::getObjectsFromQueue()
{
    Timers_.Queue.start();
    auto& Messages = Reg_.ctx<MessageHandler>();
    auto& Names = Reg_.ctx<NameSystem>();
    auto& Renderer = Reg_.ctx<RenderSystem>();
    auto& UI = Reg_.ctx<UIManager>();

    bool NewHooks = false;
    std::string Data;
    while (InputQueue_.try_dequeue(Data))
    {
        rapidjson::Document j;
        rapidjson::ParseResult r = j.Parse(Data.c_str());

        if (!r)
        {
            // std::cerr << "JSON parse error: %s (%u)", rapidjson::GetParseError_En(r.Code()), r.Offset();
            Messages.report("prg", "Parse error: "+std::string(rapidjson::GetParseError_En(r.Code())), MessageHandler::ERROR);
            continue;
        }

        rapidjson::Value::ConstMemberIterator it = j.FindMember("method");
        if (it != j.MemberEnd())
        {
            if (j["method"] == "galaxy_data_stars")
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
                    // DBLK(Messages.report("prg", "Entity components updated", MessageHandler::DEBUG_L3);)
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
                    UI.addCamHook(e, n);
                    Id2EntityMap_[Id] = e;
                    // DBLK(Messages.report("prg", "Entity created", MessageHandler::DEBUG_L2);)
                }
            }
            else if (j["method"] == "galaxy_data_systems")
            {
                std::string n = j["params"]["name"].GetString();
                entt::id_type Id = j["params"]["eid"].GetInt();

                auto ci = Id2EntityMap_.find(Id);
                if (ci != Id2EntityMap_.end())
                {
                    Reg_.emplace_or_replace<StarSystemTag>(ci->second);
                    Reg_.emplace_or_replace<NameComponent>(ci->second);
                    Names.setName(ci->second, n);
                    // DBLK(Messages.report("prg", "Entity components updated", MessageHandler::DEBUG_L3);)
                }
                else
                {
                    auto e = Reg_.create();
                    Reg_.emplace<StarSystemTag>(e);
                    Reg_.emplace<NameComponent>(e);
                    Names.setName(e, n);
                    UI.addSystem(e, n);
                    Id2EntityMap_[Id] = e;
                    // DBLK(Messages.report("prg", "Entity created", MessageHandler::DEBUG_L2);)
                }
            }
            else if (j["method"] == "bc_dynamic_data")
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
                    // DBLK(Messages.report("prg", "Entity components updated", MessageHandler::DEBUG_L3);)
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
                    UI.addCamHook(e, n);
                    NewHooks = true;
                    Id2EntityMap_[Id] = e;
                    // DBLK(Messages.report("prg", "Entity created", MessageHandler::DEBUG_L2);)
                }
            }
            else if (j["method"] == "perf_stats")
            {
                Timers_.ServerPhysicsFrameTimeAvg.addValue(j["params"]["t_phy"].GetDouble());
                Timers_.ServerQueueInFrameTimeAvg.addValue(j["params"]["t_queue_in"].GetDouble());
                Timers_.ServerQueueOutFrameTimeAvg.addValue(j["params"]["t_queue_out"].GetDouble());
                Timers_.ServerSimFrameTimeAvg.addValue(j["params"]["t_sim"].GetDouble());
            }
            else if (j["method"] == "sim_stats")
            {
                SimTime_.fromStamp(j["params"]["ts"].GetString());
                SimTime_.setAcceleration(j["params"]["ts_f"].GetDouble());
            }
            else if (j["method"] == "tire_data")
            {
                double RimX = j["params"]["rim_xy"][0].GetDouble();
                double RimY = j["params"]["rim_xy"][1].GetDouble();
                double RimR = j["params"]["rim_r"].GetDouble();

                entt::id_type Id = j["params"]["eid"].GetInt();

                auto ci = Id2EntityMap_.find(Id);
                if (ci != Id2EntityMap_.end())
                {
                    Reg_.emplace_or_replace<PositionComponent>(ci->second, RimX, RimY);

                    auto& Tire = Reg_.emplace_or_replace<TireComponent>(ci->second, RimR);
                    for (auto i=0u; i<TireComponent::SEGMENTS; ++i)
                    {
                        Tire.RubberX[i] = j["params"]["rubber"][i*2].GetDouble();
                        Tire.RubberY[i] = j["params"]["rubber"][i*2+1].GetDouble();
                    }
                }
                else
                {
                    auto e = Reg_.create();
                    auto& Tire = Reg_.emplace<TireComponent>(e, RimX, RimY, RimR);

                    Reg_.emplace<SystemPositionComponent>(e, 0.0, 0.0);
                    Reg_.emplace<PositionComponent>(e, RimX, RimY);

                    for (auto i=0u; i<TireComponent::SEGMENTS; ++i)
                    {
                        Tire.RubberX[i] = j["params"]["rubber"][i*2].GetDouble();
                        Tire.RubberY[i] = j["params"]["rubber"][i*2+1].GetDouble();
                    }
                    UI.addCamHook(e, "Tire");
                    NewHooks = true;
                    Id2EntityMap_[Id] = e;
                }
            }
        }
        it = j.FindMember("result");
        if (it != j.MemberEnd())
        {
            if (j["result"] == "success")
            {
                NewHooks = true;
                UI.finishSystemsTransfer();
                DBLK(Messages.report("prg", "Receiving systems successful", MessageHandler::DEBUG_L1);)
                Renderer.buildGalaxyMesh();
            }
        }
        if (NewHooks)
        {
            UI.finishSystemsTransfer();
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
    GL::Renderer::enable(GL::Renderer::Feature::Blending);
    GL::Renderer::enable(GL::Renderer::Feature::ScissorTest);
    GL::Renderer::disable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::disable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::disable(GL::Renderer::Feature::StencilTest);

    GL::Renderer::setBlendEquation(GL::Renderer::BlendEquation::Add,
    GL::Renderer::BlendEquation::Add);
    GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::SourceAlpha,
    GL::Renderer::BlendFunction::OneMinusSourceAlpha);
}

void PwngClient::updateUI()
{
    auto& Messages = Reg_.ctx<MessageHandler>();
    auto& Network = Reg_.ctx<NetworkManager>();
    auto& Renderer = Reg_.ctx<RenderSystem>();
    auto Camera = Renderer.getCamera();

    // GL::Renderer::enable(GL::Renderer::Feature::Blending);
    // GL::Renderer::enable(GL::Renderer::Feature::ScissorTest);
    // GL::Renderer::disable(GL::Renderer::Feature::FaceCulling);
    // GL::Renderer::disable(GL::Renderer::Feature::DepthTest);
    // GL::Renderer::disable(GL::Renderer::Feature::StencilTest);

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
                UI.processClientControl();

                if (ImGui::Button("Quit Client"))
                {
                    Network.quit();
                    Platform::Application::GlfwApplication::exit();
                }
                UI.processVerbosity();

                static float RenderResolutionFactor = 2.0f;
                if (ImGui::SliderFloat("Render resolution factor", &RenderResolutionFactor, 0.1f, 4.0f))
                    Renderer.setRenderResFactor(RenderResolutionFactor);

            ImGui::Unindent();

            ImGui::TextColored(ImVec4(1,1,0,1), "Subscriptions");
            ImGui::Indent();
                UI.processSubscriptions();
                UI.processStarSystems();
            ImGui::Unindent();

            ImGui::TextColored(ImVec4(1,1,0,1), "Camera Hooks");
            ImGui::Indent();
                UI.processCameraHooks(Camera);
            ImGui::Unindent();

            ImGui::TextColored(ImVec4(1,1,0,1), "Server control");
            ImGui::Indent();
                UI.processServerControl(SimTime_.getAcceleration());
            ImGui::Unindent();
            ImGui::TextColored(ImVec4(1,1,0,1), "Display");
            ImGui::Indent();
                // auto& Zoom = Reg_.get<ZoomComponent>(Camera);
                // ImGui::SliderFloat("Stars: Minimum Display Size", &StarsDisplaySizeMin_, 0.1, 20.0);
                // ImGui::SliderFloat("Stars: Display Scale Factor", &StarsDisplayScaleFactor_, 1.0, std::clamp(1.0e-7/Zoom.z, 5.0, 1.0e11));
                // if (StarsDisplayScaleFactor_ >  1.0e-7/Zoom.z) StarsDisplayScaleFactor_= 1.0e-7/Zoom.z;
                // if (StarsDisplayScaleFactor_ <  1.0) StarsDisplayScaleFactor_= 1.0;
            ImGui::Unindent();
            UI.processObjectLabels();
        ImGui::End();
        UI.displayObjectLabels(Camera);
        UI.displayHelp();
        UI.displayScaleAndTime(Renderer.getScale(), Renderer.getScaleUnit(), SimTime_);
    }
    ImGUI_.drawFrame();

    GL::Renderer::setBlendEquation(GL::Renderer::BlendEquation::Add,
    GL::Renderer::BlendEquation::Add);
    GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::SourceAlpha,
    GL::Renderer::BlendFunction::OneMinusSourceAlpha);
}

MAGNUM_GLFWAPPLICATION_MAIN(PwngClient)
