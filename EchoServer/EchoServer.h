//
// Created by Luxiang Lin on 25/3/15.
//

#ifndef _ECHOSERVER_ECHOSERVER_H_
#define _ECHOSERVER_ECHOSERVER_H_

#define HEAD_LENGTH 4

#include <iostream>
#include <functional>
#include <memory>
#include <map>
#include <vector>

#include "opcode.h"
#include "server.h"
#include "channel.h"
#include "EchoHandler.h"

using namespace std;

/*
* TODO devide into handlers
*/

class EchoHandler;

class EchoServer {
public:
    EchoServer(string addr, string port):
            server_(make_shared<simpleserver::Server>(addr, port)){
        server_->setConnectionCallback(bind(&EchoServer::OnConnection, this, placeholders::_1));
        server_->setReadCallback(bind(&EchoServer::OnRead, this, placeholders::_1));
        server_->setDisconnectCallback(bind(&EchoServer::OnDisconnect, this, placeholders::_1));

    }

    void start() {
        server_->start();
    }

    shared_ptr<simpleserver::Server> getServer() {
        return server_;
    }

    void sendMessageToAllChannel(const vector<uint8_t>& buffer);


private:
    shared_ptr<simpleserver::Server> server_;
    unordered_map<long, shared_ptr<EchoHandler>> handlers_;

    void OnRead(simpleserver::Channel& channel);

    // send server list when first connect
    void OnConnection(simpleserver::Channel& channel) {
        auto handler = make_shared<EchoHandler>(*this);
        handlers_[channel.getId()] = handler;
    }

    // handle disconnect
    void OnDisconnect(simpleserver::Channel& channel) {
        auto iter = handlers_.find(channel.getId());
        if (iter != handlers_.end()) {
            auto handler = iter->second;
            handler->disconnect();
            handlers_.erase(iter);
        }
    }
};


#endif //_ECHOSERVER_ECHOSERVER_H_
