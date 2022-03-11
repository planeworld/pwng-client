#ifndef NETWORK_MANAGER_HPP
#define NETWORK_MANAGER_HPP

#include <atomic>
#include <map>
#include <string>
#include <thread>
#include <vector>

#include <concurrentqueue/concurrentqueue.h>
#include <entt/entity/registry.hpp>

#define ASIO_STANDALONE
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>

#include "avg_filter.hpp"

class NetworkManager
{

    public:

        typedef websocketpp::client<websocketpp::config::asio_client> ClientType;

        NetworkManager(entt::registry& _Reg) : Reg_(_Reg) {}

        double getFrameTime() const {return TimerNetwork_.getAvg();}

        bool isConnected() const {return IsConnected_;}
        bool isRunning() const {return IsRunning_;}

        bool init(moodycamel::ConcurrentQueue<std::string>* const _InputQueue,
                  moodycamel::ConcurrentQueue<std::string>* const _OutputQueue);

                void addListenerDisconnect(std::function<void(void)> f)
                {
                        ListenersDisconnect.push_back(f);
                }
        bool connect(const std::string& _Uri);
        bool disconnect();
        void quit();

    private:

        void onClose(websocketpp::connection_hdl);
        void onFail();
        void onMessage(websocketpp::connection_hdl, ClientType::message_ptr _Msg);
        bool onOpen(websocketpp::connection_hdl);
        void run();
        void reset();

        entt::registry& Reg_;

        std::stringstream ErrorStream_;
        std::stringstream MessageStream_;

        moodycamel::ConcurrentQueue<std::string>* InputQueue_;
        moodycamel::ConcurrentQueue<std::string>* OutputQueue_;

        std::uint32_t NetworkingStepSize_{10};
        AvgFilter<double> TimerNetwork_{50};

        std::vector<std::function<void(void)>> ListenersDisconnect;

        ClientType Client_;
        websocketpp::connection_hdl Connection_;

        std::thread ThreadClient_;
        std::thread ThreadSender_;

        std::atomic<bool> IsConnected_{false};
        std::atomic<bool> IsRunning_{true};
};

#endif // NETWORK_MANAGER_HPP
