#ifndef JSON_MANAGER_HPP
#define JSON_MANAGER_HPP

#include <string>

#include <concurrentqueue/concurrentqueue.h>
#include <entt/entity/registry.hpp>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

using namespace rapidjson;

class JsonManager
{

    public:

        explicit JsonManager(entt::registry& _Reg,
                             moodycamel::ConcurrentQueue<std::string>* _InputQueue,
                             moodycamel::ConcurrentQueue<std::string>* _OutputQueue) :
                             Reg_(_Reg),
                             InputQueue_(_InputQueue),
                             OutputQueue_(_OutputQueue){}

        JsonManager& createRequest(const std::string& _Req);
        JsonManager& addParam(const std::string& _Name, double _v);
        JsonManager& addParam(const std::string& _Name, std::uint32_t _v);
        JsonManager& addParam(const std::string& _Name, const std::string& _v);
        std::uint32_t send();



        std::uint32_t sendJsonRpcRequest(const std::string& _Req);
        std::uint32_t sendJsonRpcRequest(const std::string& _Req, int _NrParams);

    private:

        entt::registry& Reg_;

        moodycamel::ConcurrentQueue<std::string>* InputQueue_;
        moodycamel::ConcurrentQueue<std::string>* OutputQueue_;

        StringBuffer Buffer_;
        Writer<StringBuffer> Writer_{Buffer_};

        std::string Params_;

        // Request ID should be unique, but is simply an increased integer
        // So after running more than a year with 100 requests per second
        // there will occur an overflow
        // If changed to another type such as int64, another method from
        // rapidjson's writer has to be chosen
        using RequestIDType = std::uint32_t;
        RequestIDType RequestID_{0};
};

#endif // JSON_MANAGER_HPP
