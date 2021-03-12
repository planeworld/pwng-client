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

PwngClient::PwngClient(const Arguments& arguments): Magnum::Platform::Application{arguments, Magnum::NoCreate}
{
    this->setupWindow();
    this->setupNetwork();
}

void PwngClient::setupNetwork()
{
    entt::registry Reg;

    Reg.set<MessageHandler>();
    Reg.set<NetworkManager>(Reg);

    auto& Messages = Reg.ctx<MessageHandler>();
    auto& Network = Reg.ctx<NetworkManager>();

    Messages.registerSource("net", "net");
    Messages.registerSource("prg", "prg");
    Messages.setColored(true);
    Messages.setLevel(MessageHandler::DEBUG_L3);

    std::string Uri = "ws://localhost:9002/?id=1";

    moodycamel::ConcurrentQueue<std::string> InputQueue;
    moodycamel::ConcurrentQueue<std::string> OutputQueue;

    Network.init(&InputQueue, &OutputQueue, Uri);

    while (Network.isRunning())
    {
        std::string Message;
        bool NewMessageFound = InputQueue.try_dequeue(Message);
        if (NewMessageFound)
        {
            DBLK(Messages.report("prg", "Incoming Message: \n" + Message, MessageHandler::DEBUG_L3);)

            // json j = json::parse(Message);

            // if (j["Message"] == "stop")
            // {
            //     Network.stop();
            //     // Simulation.stop();
            // }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    Network.stop();

    Messages.report("prg", "Exit program");
}


void PwngClient::setupWindow()
{
    /* Try 8x MSAA, fall back to zero samples if not possible. Enable only 2x
       MSAA if we have enough DPI. */
    const Magnum::Vector2 dpiScaling = this->dpiScaling({});

    Configuration conf;
    conf.setSize({1000, 1000});

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
    ImGUI_ = Magnum::ImGuiIntegration::Context({1000.0, 1000.0},
    windowSize(), framebufferSize());
    UIStyle_ = &ImGui::GetStyle();
    UIStyleDefault_ = *UIStyle_;
    UIStyleSubStats_ = *UIStyle_;
    UIStyleSubStats_.WindowRounding = 0.0f;

    Magnum::GL::Renderer::setBlendEquation(Magnum::GL::Renderer::BlendEquation::Add,
    Magnum::GL::Renderer::BlendEquation::Add);
    Magnum::GL::Renderer::setBlendFunction(Magnum::GL::Renderer::BlendFunction::SourceAlpha,
    Magnum::GL::Renderer::BlendFunction::OneMinusSourceAlpha);
}

MAGNUM_APPLICATION_MAIN(PwngClient)
