#include "network_manager.hpp"

#include "error_handler.hpp"
#include "message_handler.hpp"

bool NetworkManager::init(moodycamel::ConcurrentQueue<std::string>* const _InputQueue,
                          moodycamel::ConcurrentQueue<std::string>* const _OutputQueue,
                          const std::string& _Uri)
{
    InputQueue_ = _InputQueue;
    OutputQueue_ = _OutputQueue;

    auto& Errors = Reg_.ctx<ErrorHandler>();

    Client_.set_access_channels(websocketpp::log::alevel::all);
    Client_.set_error_channels(websocketpp::log::elevel::all);

    Client_.set_close_handler(std::bind(&NetworkManager::onClose, this,
                              std::placeholders::_1));
    Client_.set_message_handler(std::bind(&NetworkManager::onMessage, this,
                                std::placeholders::_1, std::placeholders::_2));
    Client_.set_validate_handler(std::bind(&NetworkManager::onValidate, this,
                                 std::placeholders::_1));

    Client_.init_asio();

    websocketpp::lib::error_code ErrorCode;
    ClientType::connection_ptr Con = Client_.get_connection(_Uri, ErrorCode);
    if (ErrorCode)
    {
        Errors.report("Couldn't start client: " + ErrorCode.message());
        std::cerr << ErrorCode.message() << std::endl;
        return false;
    }
    Client_.connect(Con);

    ThreadClient_ = std::thread(std::bind(&ClientType::run, &Client_));
    // ThreadSender_ = std::thread(&NetworkManager::run, this);

    return true;
}

void NetworkManager::onClose(websocketpp::connection_hdl _Connection)
{
    auto& Messages = Reg_.ctx<MessageHandler>();
    Messages.report("Connection to client closed");
    IsRunning_ = false;
}

void NetworkManager::onMessage(websocketpp::connection_hdl _Connection, ClientType::message_ptr _Msg)
{
    auto& Messages = Reg_.ctx<MessageHandler>();
    DBLK(Messages.report("Incoming message enqueued", MessageHandler::DEBUG_L2);)
    DBLK(Messages.report("Content: " + _Msg->get_payload(), MessageHandler::DEBUG_L3);)
    InputQueue_->enqueue(_Msg->get_payload());
}

bool NetworkManager::onValidate(websocketpp::connection_hdl _Connection)
{
    auto& Messages = Reg_.ctx<MessageHandler>();

    websocketpp::client<websocketpp::config::asio_client>::connection_ptr
        Connection = Client_.get_con_from_hdl(_Connection);
    websocketpp::uri_ptr Uri = Connection->get_uri();

    DBLK(Messages.report("Query string: " + Uri->get_query(), MessageHandler::DEBUG_L1);)

    std::string ID = "1";
    Messages.report("Connection validated");
    Connection_ = _Connection;

    return true;
}

void NetworkManager::run()
{
    // auto& Messages = Reg_.ctx<MessageHandler>();

    // while (IsRunning_)
    // {
    //     std::string Message;
    //     while (OutputQueue_->try_dequeue(Message))
    //     {
    //         std::string ID = "1";

    //         auto it = Connections_.find(ID);
    //         if (it != Connections_.end())
    //         {
    //             auto Connection = it->second;
    //             websocketpp::lib::error_code ErrorCode;
    //             Client_.send(Connection, Message, websocketpp::frame::opcode::text, ErrorCode);
    //             if (ErrorCode)
    //             {
    //                 Reg_.ctx<ErrorHandler>().report("Sending failed: " + ErrorCode.message());
    //             }
    //         }
    //         else
    //         {
    //             // std::cout << "Socket unknown, message dropped" << std::endl;
    //         }

    //         // std::cout << Message << std::endl;
    //     }
    //     std::this_thread::sleep_for(std::chrono::milliseconds(10));
    // }
    // DBLK(Messages.report("Sender thread stopped successfully", MessageHandler::DEBUG_L1);)
}

bool NetworkManager::stop()
{
    auto& Errors = Reg_.ctx<ErrorHandler>();
    auto& Messages = Reg_.ctx<MessageHandler>();

    Messages.report("Stopping client");

    // websocketpp::lib::error_code ErrorCode;
    // Client_.stop_listening(ErrorCode);
    // if (ErrorCode)
    // {
    //     Errors.report("Stopping client failed: " + ErrorCode.message());
    //     return false;
    // }

    // {
    //     websocketpp::lib::error_code ErrorCode;
    //     Client_.close(Connection_, websocketpp::close::status::normal,
    //                   "Client shutting down, closing connection.", ErrorCode);
    //     if (ErrorCode)
    //     {
    //         Errors.report("Closing connection failed: " + ErrorCode.message());
    //     }
    // }

    Client_.stop();

    ThreadClient_.join();

    Messages.report("Client stopped");

    return true;
}
