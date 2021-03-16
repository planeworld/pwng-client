#include "network_manager.hpp"

#include "message_handler.hpp"

bool NetworkManager::init(moodycamel::ConcurrentQueue<std::string>* const _InputQueue,
                          moodycamel::ConcurrentQueue<std::string>* const _OutputQueue)
{
    InputQueue_ = _InputQueue;
    OutputQueue_ = _OutputQueue;

    Client_.set_access_channels(websocketpp::log::alevel::all);
    Client_.clear_access_channels(websocketpp::log::alevel::frame_header);
    Client_.clear_access_channels(websocketpp::log::alevel::frame_payload);
    Client_.set_error_channels(websocketpp::log::elevel::all);
    Client_.get_elog().set_ostream(&ErrorStream_);
    Client_.get_alog().set_ostream(&MessageStream_);
    Client_.set_close_handler(std::bind(&NetworkManager::onClose, this,
                              std::placeholders::_1));
    Client_.set_fail_handler(std::bind(&NetworkManager::onFail, this));
    Client_.set_message_handler(std::bind(&NetworkManager::onMessage, this,
                                std::placeholders::_1, std::placeholders::_2));
    Client_.set_open_handler(std::bind(&NetworkManager::onOpen, this,
                             std::placeholders::_1));

    auto& Messages = Reg_.ctx<MessageHandler>();
    try
    {
        Client_.init_asio();
    }
    catch (const websocketpp::exception& e)
    {
        Messages.report("net", "Couldn't initialise network: " + std::string(e.what()), MessageHandler::ERROR);
        return false;
    }

    ThreadSender_ = std::thread(&NetworkManager::run, this);

    return true;
}

bool NetworkManager::connect(const std::string& _Uri)
{
    if (!IsConnected_)
    {
        // Maybe there's still a stale connection due to server-side disconnect
        if (ThreadClient_.joinable()) this->reset();

        auto& Messages = Reg_.ctx<MessageHandler>();
        DBLK(Messages.report("net", "Connecting", MessageHandler::DEBUG_L1);)

        websocketpp::lib::error_code ErrorCode;
        ClientType::connection_ptr Con = Client_.get_connection(_Uri, ErrorCode);
        if (ErrorCode)
        {
            Messages.report("net", "Couldn't start client: " + ErrorCode.message(), MessageHandler::ERROR);
            return false;
        }
        Client_.connect(Con);

        ThreadClient_ = std::thread(std::bind(&ClientType::run, &Client_));
    }
    return true;
}

void NetworkManager::onClose(websocketpp::connection_hdl _Connection)
{
    auto& Messages = Reg_.ctx<MessageHandler>();
    Messages.report("net", "Connection closed", MessageHandler::INFO);
    IsConnected_ = false;
    IsRunning_ = false;
}

void NetworkManager::onFail()
{
    auto& Messages = Reg_.ctx<MessageHandler>();
    Messages.report("net", "Connection failed", MessageHandler::ERROR);
    IsConnected_ = false;
}

void NetworkManager::onMessage(websocketpp::connection_hdl _Connection, ClientType::message_ptr _Msg)
{
    auto& Messages = Reg_.ctx<MessageHandler>();
    DBLK(Messages.report("net", "Incoming message enqueued", MessageHandler::DEBUG_L2);)
    // DBLK(Messages.report("net", "Content: " + _Msg->get_payload(), MessageHandler::DEBUG_L3);)
    InputQueue_->enqueue(_Msg->get_payload());
}

bool NetworkManager::onOpen(websocketpp::connection_hdl _Connection)
{
    auto& Messages = Reg_.ctx<MessageHandler>();

    Messages.report("net", "Connection opened", MessageHandler::INFO);
    Connection_ = _Connection;
    IsConnected_ = true;

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
        if (!IsConnected_ && ThreadClient_.joinable())
        {
            DBLK(Messages.report("net", "Terminating stale connection", MessageHandler::DEBUG_L1);)
            this->reset();
        }

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
}

bool NetworkManager::disconnect()
{
    if (IsConnected_)
    {
        auto& Messages = Reg_.ctx<MessageHandler>();

        DBLK(Messages.report("net", "Disconnecting", MessageHandler::DEBUG_L1);)

        auto Con = Client_.get_con_from_hdl(Connection_);
        websocketpp::lib::error_code ErrorCode;
        Con->close(websocketpp::close::status::normal, "Client closing connection.", ErrorCode);
        if (ErrorCode)
        {
            Messages.report("net", "Disconnecting failed: " + ErrorCode.message(), MessageHandler::ERROR);
        }
        IsConnected_ = false;

        this->reset();
    }

    return true;
}

void NetworkManager::reset()
{
    ThreadClient_.join();
    Client_.reset();
}

void NetworkManager::quit()
{
    this->disconnect();
    if (ThreadClient_.joinable()) this->reset();
    IsRunning_ = false;
    ThreadSender_.join();
}
