//
// Created by Luxiang Lin on 25/3/15.
//

#include <vector>
#include "EchoServer.h"

void EchoServer::OnRead(simpleserver::Channel &channel) {
    auto iter = handlers_.find(channel.getId());
    if (iter != handlers_.end()) {
        auto handler = iter->second;
        handler->OnRead(channel);
    }
}

void EchoServer::sendMessageToAllChannel(const vector<uint8_t>& buffer) {
    server_->sendMessageToAllChannel(buffer);
}
