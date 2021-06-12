#include "json_manager.hpp"

#include <iostream>

JsonManager& JsonManager::addParam(const std::string& _Name, double _v)
{
    Writer_.Key(_Name.c_str());
    Writer_.Double(_v);

    return *this;
}

JsonManager& JsonManager::addParam(const std::string& _Name, std::uint32_t _v)
{
    Writer_.Key(_Name.c_str());
    Writer_.Uint(_v);

    return *this;
}

JsonManager& JsonManager::addParam(const std::string& _Name, const std::string& _v)
{
    Writer_.Key(_Name.c_str());
    Writer_.String(_v.c_str());

    return *this;
}

JsonManager& JsonManager::createRequest(const std::string &_Req)
{
    Writer_.StartObject();
    Writer_.Key("jsonrpc"); Writer_.String("2.0");
    Writer_.Key("method"); Writer_.String(_Req.c_str());
    Writer_.Key("params");
    Writer_.StartObject();

    return *this;
}

JsonManager::RequestIDType JsonManager::send()
{
    Writer_.EndObject();
    Writer_.Key("id"); Writer_.Uint(++RequestID_);
    Writer_.EndObject();
    OutputQueue_->enqueue(Buffer_.GetString());

    Buffer_.Clear();
    Writer_.Reset(Buffer_);

    return RequestID_;
}

JsonManager::RequestIDType JsonManager::sendJsonRpcRequest(const std::string& _Req)
{
    using namespace rapidjson;
    StringBuffer s;
    Writer<StringBuffer> w(s);

    w.StartObject();
    w.Key("jsonrpc"); w.String("2.0");
    w.Key("method"); w.String(_Req.c_str());
    w.Key("params");
        w.StartObject();
        // w.Key("Message"); w.String(_Msg.c_str());
        w.EndObject();
    w.Key("id"); w.Uint(++RequestID_);
    w.EndObject();
    OutputQueue_->enqueue(s.GetString());

    return RequestID_;
}

JsonManager::RequestIDType JsonManager::sendJsonRpcRequest(const std::string& _Req, int _NrParams)
{
    using namespace rapidjson;
    StringBuffer s;
    Writer<StringBuffer> w(s);

    w.StartObject();
    w.Key("jsonrpc"); w.String("2.0");
    w.Key("method"); w.String(_Req.c_str());
    w.Key("params");
        w.StartObject();
        w.EndObject();
        // w.Key("Message"); w.String(_Msg.c_str());
        // w.Key("Params"); w.String(_Params.c_str());
    w.Key("id"); w.Uint(++RequestID_);
    w.EndObject();
    OutputQueue_->enqueue(s.GetString());

    return RequestID_;
}
