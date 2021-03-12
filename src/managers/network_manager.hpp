#ifndef NETWORK_MANAGER_HPP
#define NETWORK_MANAGER_HPP

#include <map>
#include <mutex>
#include <string>
#include <thread>

#include <concurrentqueue/concurrentqueue.h>
#include <entt/entity/registry.hpp>

#define ASIO_STANDALONE
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>

class NetworkManager
{

    public:

        typedef websocketpp::client<websocketpp::config::asio_client> ClientType;

        NetworkManager(entt::registry& _Reg) : Reg_(_Reg) {}

        bool isRunning() const {return IsRunning_;}

        bool init(moodycamel::ConcurrentQueue<std::string>* const _InputQueue,
                  moodycamel::ConcurrentQueue<std::string>* const _OutputQueue,
                  const std::string& _Uri);
        bool stop();

    private:

        void onClose(websocketpp::connection_hdl);
        void onMessage(websocketpp::connection_hdl, ClientType::message_ptr _Msg);
        bool onValidate(websocketpp::connection_hdl);
        void run();

        entt::registry& Reg_;

        std::stringstream ErrorStream_;
        std::stringstream MessageStream_;

        moodycamel::ConcurrentQueue<std::string>* InputQueue_;
        moodycamel::ConcurrentQueue<std::string>* OutputQueue_;

        ClientType Client_;
        websocketpp::connection_hdl Connection_;

        std::thread ThreadClient_;
        std::thread ThreadSender_;

        bool IsRunning_{true};
};

#endif // NETWORK_MANAGER_HPP
