#include "network_manager.hpp"

#include "message_handler.hpp"

bool NetworkManager::init(moodycamel::ConcurrentQueue<std::string>* const _InputQueue,
                          moodycamel::ConcurrentQueue<std::string>* const _OutputQueue,
                          const std::string& _Uri)
{
    InputQueue_ = _InputQueue;
    OutputQueue_ = _OutputQueue;

    auto& Messages = Reg_.ctx<MessageHandler>();

    Client_.set_access_channels(websocketpp::log::alevel::all);
    Client_.clear_access_channels(websocketpp::log::alevel::frame_header);
    Client_.clear_access_channels(websocketpp::log::alevel::frame_payload);
    Client_.set_error_channels(websocketpp::log::elevel::all);
    Client_.get_elog().set_ostream(&ErrorStream_);
    Client_.get_alog().set_ostream(&MessageStream_);
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
        Messages.report("net", "Couldn't start client: " + ErrorCode.message(), MessageHandler::ERROR);
        return false;
    }
    Client_.connect(Con);

    ThreadClient_ = std::thread(std::bind(&ClientType::run, &Client_));
    ThreadSender_ = std::thread(&NetworkManager::run, this);

    return true;
}

void NetworkManager::onClose(websocketpp::connection_hdl _Connection)
{
    auto& Messages = Reg_.ctx<MessageHandler>();
    Messages.report("net", "Connection to client closed", MessageHandler::INFO);
    IsRunning_ = false;
}

void NetworkManager::onMessage(websocketpp::connection_hdl _Connection, ClientType::message_ptr _Msg)
{
    auto& Messages = Reg_.ctx<MessageHandler>();
    DBLK(Messages.report("net", "Incoming message enqueued", MessageHandler::DEBUG_L2);)
    // DBLK(Messages.report("net", "Content: " + _Msg->get_payload(), MessageHandler::DEBUG_L3);)
    InputQueue_->enqueue(_Msg->get_payload());
}

bool NetworkManager::onValidate(websocketpp::connection_hdl _Connection)
{
    auto& Messages = Reg_.ctx<MessageHandler>();

    websocketpp::client<websocketpp::config::asio_client>::connection_ptr
        Connection = Client_.get_con_from_hdl(_Connection);
    websocketpp::uri_ptr Uri = Connection->get_uri();

    DBLK(Messages.report("net", "Query string: " + Uri->get_query(), MessageHandler::DEBUG_L1);)

    std::string ID = "1";
    Messages.report("net", "Connection validated", MessageHandler::INFO);
    Connection_ = _Connection;

    return true;
}

void NetworkManager::run()
{
    auto& Messages = Reg_.ctx<MessageHandler>();

    while (IsRunning_)
    {
        // Check, if there are any errors or messages from
        // websocketpp.
        // Since output is transferred to a (string-)stream,
        // it's content is frequently checked
        if (!ErrorStream_.str().empty())
        {
            // Extract line by line in case of multiple messages
            std::string Line;
            while (std::getline(ErrorStream_, Line, '\n'))
            {
                Messages.report("net", "WebSocket++: "+Line);
            }
            ErrorStream_.str({});
        }
        DBLK(
            if (!MessageStream_.str().empty())
            {
                std::string Line;
                while(std::getline(MessageStream_, Line, '\n'))
                {
                    Messages.report("net", "WebSocket++: "+Line, MessageHandler::DEBUG_L1);
                }
                MessageStream_.str({});
            }
        )

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
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    // DBLK(Messages.report("Sender thread stopped successfully", MessageHandler::DEBUG_L1);)
}

bool NetworkManager::stop()
{
    auto& Messages = Reg_.ctx<MessageHandler>();

    Messages.report("net", "Stopping client", MessageHandler::INFO);

    // websocketpp::lib::error_code ErrorCode;
    // Client_.stop_listening(ErrorCode);
    // if (ErrorCode)
    // {
    //     Messages.report("Stopping client failed: " + ErrorCode.message());
    //     return false;
    // }

    // {
    //     websocketpp::lib::error_code ErrorCode;
    //     Client_.close(Connection_, websocketpp::close::status::normal,
    //                   "Client shutting down, closing connection.", ErrorCode);
    //     if (ErrorCode)
    //     {
    //         Messages.report("Closing connection failed: " + ErrorCode.message());
    //     }
    // }

    Client_.stop();

    ThreadClient_.join();

    Messages.report("net", "Client stopped", MessageHandler::INFO);

    return true;
}
