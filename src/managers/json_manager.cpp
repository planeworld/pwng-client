#include "json_manager.hpp"

#include <iostream>

void JsonManager::addParamDouble(const std::string& _Name, double _v)
{

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

    // StringBuffer_.Clear();
    // Writer_.Reset(StringBuffer_);
    // Writer_.StartObject();

    return RequestID_;
}
