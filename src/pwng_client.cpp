#include <atomic>
#include <chrono>
#include <ctime>
#include <iostream>
#include <string>
#include <thread>

#include <argagg/argagg.hpp>
#include <concurrentqueue/concurrentqueue.h>
#include <entt/entity/registry.hpp>

#include <nlohmann/json.hpp>

#include "message_handler.hpp"
#include "network_manager.hpp"

using json = nlohmann::json;

int main(int argc, char *argv[])
{

    entt::registry Reg;

    Reg.set<MessageHandler>();
    Reg.set<NetworkManager>(Reg);

    auto& Messages = Reg.ctx<MessageHandler>();
    auto& Network = Reg.ctx<NetworkManager>();

    Messages.registerSource("net", "net");
    Messages.registerSource("prg", "prg");
    Messages.setColored(true);
    Messages.setLevel(MessageHandler::DEBUG_L1);

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

    return EXIT_SUCCESS;
}
