#include <chrono>
#include <ctime>
#include <iostream>
#include <string>
#include <thread>

#include <argagg/argagg.hpp>
#include <entt/entity/registry.hpp>

#define ASIO_STANDALONE
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>

typedef websocketpp::client<websocketpp::config::asio_client> ClientType;

#include <nlohmann/json.hpp>

// for convenience
using json = nlohmann::json;

void onMessage(websocketpp::connection_hdl, ClientType::message_ptr msg)
{
    std::cout << msg->get_payload() << std::endl;
}

void sendMessage(ClientType::connection_ptr _Connection)
{
    std::string Msg;
    while (true)
    {
        std::cin >> Msg;

        using sc = std::chrono::system_clock ;
        std::time_t t = sc::to_time_t(sc::now());
        char buf[20];
        strftime(buf, 20, "%d.%m.%Y %H:%M:%S", localtime(&t));
        std::string s(buf);

        json j =
        {
            {"Message", Msg},
            {"Time", s}
        };

        _Connection->send(j.dump(4));
    }
}

int main(int argc, char *argv[])
{

    ClientType Client;
    entt::registry Reg_;

    std::string uri = "ws://localhost:9002";

	try
    {
        // Set logging to be pretty verbose (everything except message payloads)
        Client.set_access_channels(websocketpp::log::alevel::all);
        Client.clear_access_channels(websocketpp::log::alevel::frame_payload);
        Client.set_error_channels(websocketpp::log::elevel::all);

        // Initialize ASIO
        Client.init_asio();

        // Register our message handler
        Client.set_message_handler(&onMessage);

        websocketpp::lib::error_code ec;
        ClientType::connection_ptr con = Client.get_connection(uri, ec);
        if (ec) {
            std::cout << "could not create connection because: " << ec.message() << std::endl;
            return 0;
        }

        std::thread t(&sendMessage, con);

        Client.connect(con);
        Client.run();
    }
    catch (websocketpp::exception const & e)
    {
        std::cout << e.what() << std::endl;
    }

    return EXIT_SUCCESS;
}
