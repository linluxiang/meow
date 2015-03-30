//
// Created by Luxiang Lin on 24/3/15.
// linluxiang@gmail.com
// This is the main TCP server implementation
//

#include <iostream>
#include <memory>
#include <algorithm>

#include "Server.h"
#include "Channel.h"

#ifdef __WINDOWS__
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <signal.h>

#endif

using namespace std;

namespace simpleserver {

bool Server::isStart_;

Server::Server(string& addr, string& port):
        addr_(addr), port_(port),
        channelIdCount_(1),
        maxConnectionWhenListen_(MAX_CONNECTION_WHEN_LISTEN),
        connectionCallback_(nullptr) {
    // handle singals
    isStart_ = false;
    ::signal(SIGHUP, Server::handleSignal);
    ::signal(SIGINT, Server::handleSignal);
    ::signal(SIGQUIT, Server::handleSignal);
    ::signal(SIGTERM, Server::handleSignal);
}

Server::~Server() {
    this->shutdown();
}

void Server::shutdown() {
    isStart_ = false;
    ::shutdown(sock_, SHUT_RDWR);
    ::close(sock_);
    //closesocket(sock_);
    //WSACLEANUP();
}

int Server::start() {

    /*
    WSADATA wsaData;

    int result {};

    result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != NO_ERROR) {
        wprintf(L"Error at WSAStartup()\n");
        return 1;
    }
    */

    struct addrinfo *result;
    struct addrinfo hint;
    int error;


    memset(&hint,0, sizeof(hint));

    hint.ai_family = AF_INET;
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_protocol = IPPROTO_TCP;

    error = getaddrinfo(addr_.c_str(), port_.c_str(), &hint, &result);
    if (error != 0) {
        cerr << "Error for getaddrinfo: " << error << " addr: " << addr_ << " port " << port_ << endl;
        //WSACLEANUP();
        return 1;
    }

    int sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

    //if (sock == INVALID_SOCKET)
    if (sock < 0) {
        cerr << "Error for creating socket";
        //WSACLEANUP();
        return 1;
    }

    sock_ = sock;

    int bindResult = 0;

    bindResult = ::bind(sock, result->ai_addr, result->ai_addrlen);

    //if (bindResult == SOCKET_ERROR)
    if (bindResult < 0) {
        struct sockaddr_in *info= reinterpret_cast<struct sockaddr_in*>(result->ai_addr);
        cerr << "Error for bind socket: " << info->sin_addr.s_addr << ":" << htons(info->sin_port) << endl;
        //WSACLEANUP();
        return 1;
    }

    freeaddrinfo(result);

    int listenResult = listen(sock, maxConnectionWhenListen_);

    if (listenResult < 0) {
        cerr << "Error for listen" << endl;
        //WSACLEANUP();
        return 1;
    }
    cout << "Start listening on " << addr_ << ":" << port_ << endl;
    isStart_ = true;
    setSocketNonblock(sock_);

    pollfd serverPollFd;
    serverPollFd.fd = sock_;
    serverPollFd.events = POLLIN;
    pollfds_.push_back(serverPollFd);

    // This part is main io loop, it should be in an sepereated class
    while (isStart_) {
        // real remove channel
        realRemoveChannels();
        if (poll(pollfds_.data(), pollfds_.size(), -1) > 0) {
            for (auto const &pfd: pollfds_) {
                if ((pfd.fd == sock_) && (pfd.revents & POLLIN)) {
                    // need accept
                    struct sockaddr addr;
                    socklen_t addrlen;
                    int acceptFd = accept(sock_, &addr, &addrlen);
                    if (acceptFd < 0) {
                        cerr << "accept error" << endl;
                        return 1;
                    }
                    setSocketNonblock(acceptFd);
                    setSocketNoDelay(acceptFd);
                    auto thisSharedPtr = shared_from_this();
                    auto channel = make_shared<Channel>(channelIdCount_, acceptFd, thisSharedPtr);
                    channelIdCount_ += 1;
                    channel->setSockAddr(addr);
                    channels_[channel->getId()] = channel;
                    fdToChannelIdMap_[acceptFd] = channel->getId();
                    struct pollfd acceptPollFd;
                    acceptPollFd.fd = acceptFd;
                    acceptPollFd.events = POLLIN | POLLOUT;
                    pollfds_.push_back(acceptPollFd);
                    if (connectionCallback_ != nullptr) {
                        connectionCallback_(*channel);
                    }
                } else {
                    if (pfd.revents & POLLIN) {
                        // fd can read
                        auto iter = fdToChannelIdMap_.find(pfd.fd);
                        if (iter != fdToChannelIdMap_.end()) {
                            auto channelIter = channels_.find(iter->second);
                            if (channelIter != channels_.end()) {
                                auto channel = channelIter->second;
                                if (channel != nullptr) {
                                    if (channel->waitReadBytesCount() > 0) {
                                        channel->read();
                                    }
                                    if (readCallback_ != nullptr) {
                                        readCallback_(*channel);
                                    }
                                }
                            }
                        }
                    }

                    if (pfd.revents & POLLOUT) {
                        // fd can write
                        auto iter = fdToChannelIdMap_.find(pfd.fd);
                        if (iter != fdToChannelIdMap_.end()) {
                            auto channelIter = channels_.find(iter->second);
                            if (channelIter != channels_.end()) {
                                auto channel = channelIter->second;
                                if (channel != nullptr) {
                                    if (channel != nullptr) {
                                        if (channel->waitWriteBytesCount() > 0) {
                                            channel->write();
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

}

int Server::sendMessage(int sock, const vector<uint8_t>& buffer) {
    auto channel = channels_[sock];
    if (channel != nullptr) {
        channel->write(buffer);
    }
}

int Server::sendMessageToAllChannel(const vector<uint8_t>& buffer) {
    for (auto &x: channels_) {
        auto channel = x.second;
        channel->write(buffer);
    }
    return 0;
}

void Server::removeChannel(Channel& channel) {
    channel.shutdown();
    if (disconnectCallback_ != nullptr) {
        disconnectCallback_(channel);
    }
    readyToRemoveChannels_.push_back(channel.getId());
}

void Server::realRemoveChannels() {
    for (auto id: readyToRemoveChannels_) {
        for (auto x: channels_) {
            cout << x.first << ": " << (x.second)->getId() << endl;
        }
        auto iter = channels_.find(id);
        if (iter != channels_.end()) {
            auto channel = iter->second;
            auto fd = channel->getSocket();
            auto pollfdIter = find_if(pollfds_.begin(), pollfds_.end(), [&fd](const struct pollfd &x){
                return x.fd == fd;
            });
            pollfds_.erase(pollfdIter);

            auto socketToChanneldIdIter = fdToChannelIdMap_.find(fd);
            if (socketToChanneldIdIter != fdToChannelIdMap_.end()) {
                if (socketToChanneldIdIter->second == id) {
                    fdToChannelIdMap_.erase(fd);
                }
            }
            channels_.erase(id);
        }

    }
    readyToRemoveChannels_.resize(0);
}
}
