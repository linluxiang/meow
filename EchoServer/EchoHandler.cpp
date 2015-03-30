//
// Created by Luxiang Lin on 26/3/15.
//

#include "EchoHandler.h"
#include "EchoServer.h"

/*
* The protocol is 2 bytes length, 2 bytes opcode and body
*/
void EchoHandler::OnRead(simpleserver::Channel &channel) {
    auto handler = statusHandlerMap_[status_];
    if (handler != nullptr) {
        handler(channel);
    }
}

void EchoHandler::OnStart(simpleserver::Channel &channel) {
    status_ = EchoHandlerStatus::ON_HEAD;
}

void EchoHandler::OnHead(simpleserver::Channel& channel){
    auto buffer = channel.readIntoBuffer(4);
    if (buffer.size() != 0) {
        short *lengthP = reinterpret_cast<short *>(buffer.data());
        short *opcodeP = reinterpret_cast<short *>(buffer.data() + 2);
        auto length = ntohs(*lengthP);
        auto opcode = ntohs(*opcodeP);
        bodyLength_ = length;
        opcode_ = opcode;
        if (length == 0) {
            status_ = EchoHandlerStatus::ERROR;
            return;
        }
        status_ = EchoHandlerStatus::ON_BODY;
    }
}

void EchoHandler::OnBody(simpleserver::Channel& channel) {
    auto buffer = channel.readIntoBuffer(bodyLength_);
    if (buffer.size() != 0) {
        Opcode opcode = static_cast<Opcode>(opcode_);
        auto handlerIter = handlers_.find(opcode);
        if (handlerIter != handlers_.end()) {
            auto handler = handlerIter->second;
            handler(channel, buffer);
        }
        status_ = EchoHandlerStatus::END;
    }
}

void EchoHandler::OnEnd(simpleserver::Channel& channel) {
    bodyLength_ = 0;
    opcode_ = 0;
    status_ = EchoHandlerStatus::START;
}

void EchoHandler::Echo(simpleserver::Channel& channel, vector<uint8_t>& buffer) {
    vector<uint8_t> header(4, 0);
    short length = htons(buffer.size());
    short opcode = htons(static_cast<short>(Opcode::ECHO_RESPONSE));
    short* lengthP = reinterpret_cast<short*>(header.data());
    short* opcodeP = reinterpret_cast<short*>(header.data()+2);
    *lengthP = length;
    *opcodeP = opcode;
    //channel.write(header);
    //channel.write(buffer);
    server_.sendMessageToAllChannel(header);
    server_.sendMessageToAllChannel(buffer);
}