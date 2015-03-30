//
// Created by Luxiang Lin on 24/3/15.
//

#include <iostream>
#include <sys/socket.h>
#include "Channel.h"
#include "Server.h"

using namespace std;
namespace simpleserver {

Channel::Channel(long id, int socket, shared_ptr<Server> server):
    id_(id), socket_(socket), server_(server),
    waitReadBytesCount_(0),
    waitWriteBytesCount_(0) {
}

void Channel::read() {
    if (waitReadBytesCount_ > 0) {
        long nbyte = recv(socket_, readBuffer_.data()+(readBuffer_.size() - waitReadBytesCount_), waitReadBytesCount_, 0);
        if (nbyte <= 0) {
            // socket closed
            waitReadBytesCount_ = 0;
            server_->removeChannel(*this);
            return;
        }
        waitReadBytesCount_ -= nbyte;
    }
}

void Channel::write() {
    if (waitWriteBytesCount_ > 0) {
        long nbyte = send(socket_, writeBuffer_.data()+(writeBuffer_.size() - waitWriteBytesCount_), waitWriteBytesCount_, 0);
        if (nbyte <= 0) {
            waitWriteBytesCount_ = 0;
            server_->removeChannel(*this);
            return;
        }
        waitWriteBytesCount_ -= nbyte;
        if (waitWriteBytesCount_ == 0) {
            writeBuffer_.resize(0);
        }
    }
}

long Channel::write(const vector<uint8_t>& buffer) {
    // copy buffer once more, bad!!
    writeBuffer_.insert(writeBuffer_.end(), buffer.begin(), buffer.end());
    waitWriteBytesCount_ = writeBuffer_.size();

    return 0;
}

vector<uint8_t> Channel::readIntoBuffer(long length) {
    vector<uint8_t> data;
    if (length == 0) {
        // invalid length
        return std::move(data);
    }
    if (waitReadBytesCount_ == 0) {
        if (readBuffer_.size() == 0) {
            // first call, should start read process
            waitReadBytesCount_ = length;
            readBuffer_.resize(length);
        } else {
            // return buffered data
            data = std::move(readBuffer_);
            return std::move(data);
        }
    }
    // still need to wait for length data to filled into readBuffer
    return std::move(data);
}
}
