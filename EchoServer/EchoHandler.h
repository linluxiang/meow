//
// Created by Luxiang Lin on 26/3/15.
//

#ifndef _ECHOSERVER_ECHOHANDLER_H_
#define _ECHOSERVER_ECHOHANDLER_H_

#include <iostream>
#include <functional>
#include <memory>
#include <map>
#include <vector>

#include "opcode.h"
#include "server.h"
#include "channel.h"

enum class EchoHandlerStatus {
    START = 0,
    ON_HEAD,
    ON_BODY,
    END,
    CLOSE,
    ERROR,
};

class EchoServer;

class EchoHandler {
public:
    EchoHandler(EchoServer& server):
            server_(server),
            status_(EchoHandlerStatus::START),
            opcode_(-1),
            bodyLength_(0) {
        statusHandlerMap_[EchoHandlerStatus::START] = bind(&EchoHandler::OnStart, this, placeholders::_1);
        statusHandlerMap_[EchoHandlerStatus::ON_HEAD] = bind(&EchoHandler::OnHead, this, placeholders::_1);
        statusHandlerMap_[EchoHandlerStatus::ON_BODY] = bind(&EchoHandler::OnBody, this, placeholders::_1);
        statusHandlerMap_[EchoHandlerStatus::END] = bind(&EchoHandler::OnEnd, this, placeholders::_1);
        statusHandlerMap_[EchoHandlerStatus::ERROR] = bind(&EchoHandler::OnError, this, placeholders::_1);

        handlers_[Opcode::ECHO_REQUEST] = bind(&EchoHandler::Echo, this, placeholders::_1, placeholders::_2);
    }

    /*
    * The protocol is 2 bytes length, 2 bytes opcode and body
    * dispatch the handler
    */
    void OnRead(simpleserver::Channel& channel);

    void OnStart(simpleserver::Channel& channel);
    void OnHead(simpleserver::Channel& channel);
    void OnBody(simpleserver::Channel& channel);
    void OnEnd(simpleserver::Channel& channel);
    void OnError(simpleserver::Channel& channel) {
        channel.shutdown();
    }

    void Echo(simpleserver::Channel& channel, vector<uint8_t>&);

    void disconnect() {
        status_ = EchoHandlerStatus::CLOSE;
    }
private:
    EchoServer& server_;
    EchoHandlerStatus status_;
    map<EchoHandlerStatus, function<void (simpleserver::Channel&)>> statusHandlerMap_;
    map<Opcode, function<void (simpleserver::Channel&, vector<uint8_t>&)>> handlers_;
    short opcode_;
    long bodyLength_;
};


#endif //_ECHOSERVER_ECHOHANDLER_H_
