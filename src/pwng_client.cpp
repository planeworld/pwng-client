#include <atomic>
#include <chrono>
#include <ctime>
#include <iostream>
#include <thread>

// #include <Magnum/GL/Mesh.h>
// #include <Magnum/GL/Renderer.h>
#include <Magnum/Math/Vector2.h>
#include <Magnum/Math/Matrix3.h>
#include <Magnum/MeshTools/Compile.h>
// #include <Magnum/MeshTools/Transform.h>
#include <Magnum/Primitives/Circle.h>
// #include <Magnum/Primitives/Square.h>
#include <Magnum/Shaders/Flat.h>
#include <Magnum/Trade/MeshData.h>

#include <argagg/argagg.hpp>
#include <concurrentqueue/concurrentqueue.h>
#include <entt/entity/registry.hpp>
#include <nlohmann/json.hpp>

#include <Magnum/GL/Renderer.h>

#include "message_handler.hpp"
#include "network_manager.hpp"
#include "pwng_client.hpp"

using json = nlohmann::json;

struct PositionComponent
{
    double x{0.0};
    double y{0.0};
};

struct CircleComponent
{
    double r{1.0};
};

PwngClient::PwngClient(const Arguments& arguments): Platform::Application{arguments, NoCreate}
{
    Reg_.set<MessageHandler>();
    Reg_.set<NetworkManager>(Reg_);

    this->setupWindow();
    this->setupNetwork();
    setSwapInterval(0);
    setMinimalLoopPeriod(1.0f/60.0f * 1000.0f);

    // auto e = Reg_.create();
    // Reg_.emplace<PositionComponent>(e);
    // Reg_.emplace<CircleComponent>(e, 20.0);

}

void PwngClient::drawEvent()
{
    GL::defaultFramebuffer.clearColor(Color4(0.0f, 0.0f, 0.0f, 1.0f));
    this->getObjectsFromQueue();
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
    ImGUI_.handleMouseMoveEvent(Event);
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

    ImGUI_.relayout(Vector2(Event.windowSize()), Event.windowSize(), Event.framebufferSize());
}

void PwngClient::getObjectsFromQueue()
{
    std::string Data;
    while (InputQueue_.try_dequeue(Data))
    {
        json j = json::parse(Data);

        double x = j["params"]["px"];
        double y = j["params"]["py"];
        x *= 3.0e-9;
        y *= 3.0e-9;

        std::uint32_t Id = j["params"]["eid"];

        auto ci = Id2EntityMap_.find(Id);
        if (ci != Id2EntityMap_.end())
        {
            Reg_.replace<PositionComponent>(ci->second, x, y);
        }
        else
        {
            auto e = Reg_.create();
            Reg_.emplace<PositionComponent>(e, x, y);
            Reg_.emplace<CircleComponent>(e, 20.0);
            Id2EntityMap_[Id] = e;
        }
    }
}

void PwngClient::renderScene()
{
    GL::defaultFramebuffer.setViewport({{}, windowSize()});

    auto shape = MeshTools::compile(Primitives::circle2DSolid(20));
    auto shader = Shaders::Flat2D{};

    auto projection = Magnum::Matrix3::projection(Magnum::Vector2(windowSize()));

    Reg_.view<PositionComponent, CircleComponent>().each([&](const auto& _p, const auto& _r)
    {
        shader.setTransformationProjectionMatrix(
            projection *
            Matrix3::translation(Vector2(_p.x, _p.y)) *
            Matrix3::scaling(Vector2(_r.r, _r.r))
        );

        shader.setColor({1.0, 1.0, 1.0});
        shader.draw(shape);
    });
}

void PwngClient::setupNetwork()
{
    auto& Messages = Reg_.ctx<MessageHandler>();
    auto& Network = Reg_.ctx<NetworkManager>();

    Messages.registerSource("net", "net");
    Messages.registerSource("prg", "prg");
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

    GL::Renderer::enable(GL::Renderer::Feature::Blending);
    GL::Renderer::enable(GL::Renderer::Feature::ScissorTest);
    GL::Renderer::disable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::disable(GL::Renderer::Feature::DepthTest);

    auto& Network = Reg_.ctx<NetworkManager>();

    ImGUI_.newFrame();
    {
        ImGui::Begin("PwNG Desktop Client");
            ImGui::TextColored(ImVec4(1,1,0,1), "Performance");
            ImGui::Indent();
                ImGui::Text("Frame Time:  %.3f ms; (%.1f FPS)",
                            1000.0/Double(ImGui::GetIO().Framerate), Double(ImGui::GetIO().Framerate));
            ImGui::Unindent();

            ImGui::TextColored(ImVec4(1,1,0,1), "Client control");
            ImGui::Indent();
                std::string Uri = "ws://localhost:9002/?id=1";
                if (Network.isConnected())
                {
                    if (ImGui::Button("Disconnect")) Network.disconnect();
                }
                else
                {
                    if (ImGui::Button("Connect")) Network.connect(Uri);
                }
                if (ImGui::Button("Quit Client"))
                {
                    Network.quit();
                    Platform::Application::Sdl2Application::exit();
                }

                // If compiled debug, let the user choose debug level to avoid
                // excessive console spamming
                DBLK(
                    static int DebugLevel = 5;
                    ImGui::Text("Verbosity:");
                    ImGui::Indent();
                        ImGui::RadioButton("Info", &DebugLevel, 2);
                        ImGui::RadioButton("Debug Level 1", &DebugLevel, 3);
                        ImGui::RadioButton("Debug Level 2", &DebugLevel, 4);
                        ImGui::RadioButton("Debug Level 3", &DebugLevel, 5);
                    ImGui::Unindent();
                    Messages.setLevel(MessageHandler::ReportLevelType(DebugLevel));
                )

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
    json j =  {{"jsonrpc", "2.0"},
               {"method", "send"},
               {"params", {{"Message", _Msg}}},
               {"id", _ID}};
    OutputQueue_.enqueue(j.dump(4));
}

MAGNUM_APPLICATION_MAIN(PwngClient)
