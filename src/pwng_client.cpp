#include <iostream>
#include <thread>

#include <argagg/argagg.hpp>
#include <entt/entity/registry.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>

typedef websocketpp::client<websocketpp::config::asio_client> ClientType;

void on_message(websocketpp::connection_hdl, ClientType::message_ptr msg)
{
    std::cout << msg->get_payload() << std::endl;
}

void sendMessage(ClientType::connection_ptr _Connection)
{
    std::string Msg;
    int i=0;
    while (true)
    {
        Msg = "12345678901234567890123456789012345678901234567890"
                "12345678901234567890123456789012345678901234567890"
                "12345678901234567890123456789012345678901234567890"
                "12345678901234567890123456789012345678901234567890"
                "12345678901234567890123456789012345678901234567890"
                "12345678901234567890123456789012345678901234567890"
                "12345678901234567890123456789012345678901234567890"
                "12345678901234567890123456789012345678901234567890"
                "12345678901234567890123456789012345678901234567890"
                "12345678901234567890123456789012345678901234567890"
                "12345678901234567890123456789012345678901234567890"
                "12345678901234567890123456789012345678901234567890"
                "12345678901234567890123456789012345678901234567890"
                "12345678901234567890123456789012345678901234567890"
                "12345678901234567890123456789012345678901234567890"
                "12345678901234567890123456789012345678901234567890"
                "12345678901234567890123456789012345678901234567890"
                "12345678901234567890123456789012345678901234567890"
                "12345678901234567890123456789012345678901234567890"
                "12345678901234567890123456789012345678901234567890"
                "12345678901234567890123456789012345678901234567890"
                "12345678901234567890123456789012345678901234567890"
                "12345678901234567890123456789012345678901234567890"
                "12345678901234567890123456789012345678901234567890"
                "12345678901234567890123456789012345678901234567890_"+std::to_string(i++);
        // std::cin >> Msg;
        _Connection->send(Msg);
    }
}

int main(int argc, char *argv[])
{

    ClientType Client;
    entt::registry Reg_;

    // argagg::parser ArgParser
    //     {{
    //         {"help", {"-h", "--help"},
    //          "Shows this help message", 0},
    //         {"port", {"-p", "--port"},
    //          "Port to listen to", 1}
    //     }};

    // argagg::parser_results Args;
    // try
    // {
    //     Args = ArgParser.parse(argc, argv);
    // }
    // catch (const std::exception& e)
    // {
    //     std::cerr << e.what() << '\n';
    //     return EXIT_FAILURE;
    // }
    // if (Args["help"])
    // {
    //     std::cerr << ArgParser;
    //     return EXIT_SUCCESS;
    // }
    // int Port = Args["port"];

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
        Client.set_message_handler(&on_message);

        websocketpp::lib::error_code ec;
        ClientType::connection_ptr con = Client.get_connection(uri, ec);
        if (ec) {
            std::cout << "could not create connection because: " << ec.message() << std::endl;
            return 0;
        }

        std::thread t(&sendMessage, con);

        // Note that connect here only requests a connection. No network messages are
        // exchanged until the event loop starts running in the next line.
        Client.connect(con);
        // Start the ASIO io_service run loop
        // this will cause a single connection to be made to the server. Client.run()
        // will exit when this connection is closed.
        Client.run();
    }
    catch (websocketpp::exception const & e)
    {
        std::cout << e.what() << std::endl;
    }

    return EXIT_SUCCESS;
}
