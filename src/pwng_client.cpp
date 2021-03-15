#include <atomic>
#include <chrono>
#include <ctime>
#include <iostream>
#include <string>
#include <thread>

#include <argagg/argagg.hpp>
#include <concurrentqueue/concurrentqueue.h>
#include <entt/entity/registry.hpp>

#include <Magnum/GL/Renderer.h>

#include <nlohmann/json.hpp>

#include "message_handler.hpp"
#include "network_manager.hpp"
#include "pwng_client.hpp"

using json = nlohmann::json;

PwngClient::PwngClient(const Arguments& arguments): Platform::Application{arguments, NoCreate}
{
    Reg_.set<MessageHandler>();
    Reg_.set<NetworkManager>(Reg_);

    this->setupWindow();
    this->setupNetwork();
    setSwapInterval(0);
    setMinimalLoopPeriod(1.0f/60.0f * 1000.0f);
}

void PwngClient::drawEvent()
{
    GL::defaultFramebuffer.clearColor(Color4(0.0f, 0.0f, 0.0f, 1.0f));
    this->updateUI();
    swapBuffers();
    redraw();
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

void PwngClient::viewportEvent(ViewportEvent& Event)
{
    GL::defaultFramebuffer.setViewport({{}, Event.framebufferSize()});

    ImGUI_.relayout(Vector2(Event.windowSize()), Event.windowSize(), Event.framebufferSize());
}

void PwngClient::setupNetwork()
{
    auto& Messages = Reg_.ctx<MessageHandler>();
    auto& Network = Reg_.ctx<NetworkManager>();

    Messages.registerSource("net", "net");
    Messages.registerSource("prg", "prg");
    Messages.setColored(true);
    Messages.setLevel(MessageHandler::DEBUG_L3);

    std::string Uri = "ws://localhost:9002/?id=1";

    Network.init(&InputQueue_, &OutputQueue_, Uri);
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
    GL::Renderer::enable(GL::Renderer::Feature::Blending);
    GL::Renderer::enable(GL::Renderer::Feature::ScissorTest);
    GL::Renderer::disable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::disable(GL::Renderer::Feature::DepthTest);

    ImGUI_.newFrame();
    {
        ImGui::Begin("Test");
            ImGui::TextColored(ImVec4(1,1,0,1), "Performance");
            ImGui::Indent();
                ImGui::Text("Frame Time:  %.3f ms; (%.1f FPS)",
                            1000.0/Double(ImGui::GetIO().Framerate), Double(ImGui::GetIO().Framerate));
            ImGui::Unindent();
        ImGui::End();
    }
    ImGUI_.drawFrame();

    GL::Renderer::setBlendEquation(GL::Renderer::BlendEquation::Add,
    GL::Renderer::BlendEquation::Add);
    GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::SourceAlpha,
    GL::Renderer::BlendFunction::OneMinusSourceAlpha);
}

MAGNUM_APPLICATION_MAIN(PwngClient)
