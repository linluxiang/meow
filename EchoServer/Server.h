//
// Created by Luxiang Lin on 24/3/15.
//

#ifndef _ECHOSERVER_SERVER_H_
#define _ECHOSERVER_SERVER_H_

#define MAX_CONNECTION_WHEN_LISTEN 1024

#include <string>
#include <algorithm>
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>
#include <sys/poll.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/tcp.h>
#include <netdb.h>

using namespace std;
namespace simpleserver {
class Channel;

class Server: public std::enable_shared_from_this<Server> {
public:
    Server(string& addr, string& port);
    ~Server();

    /*
    * Start the server
    * create and register channel when accept
    */
    int start();

    void setConnectionCallback(function<void (Channel&)> callback) {
        connectionCallback_ = callback;
    }

    void setReadCallback(function<void (Channel&)> callback) {
        readCallback_ = callback;
    }

    void setDisconnectCallback(function<void (Channel&)> callback) {
        disconnectCallback_ = callback;
    }

    int sendMessageToAllChannel(const vector<uint8_t>&);

    int sendMessage(int, const vector<uint8_t>&);

    // Just add channel to ready to remove vector
    void removeChannel(Channel& channel);

private:
    static bool isStart_;
    int maxConnectionWhenListen_;
    string addr_;
    string port_;
    int sock_;
    long channelIdCount_;
    unordered_map<long, shared_ptr<Channel>> channels_;
    unordered_map<int, long> fdToChannelIdMap_;
    function<void (Channel&)> connectionCallback_;
    function<void (Channel&)> readCallback_;
    function<void (Channel&)> disconnectCallback_;
    vector<struct pollfd> pollfds_;
    vector<long> readyToRemoveChannels_;

    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    void setSocketNonblock(int sock) {
        int opts;

        opts = fcntl(sock, F_GETFL);
        if (opts < 0) {
        }
        opts = (opts | O_NONBLOCK);
        if (fcntl(sock, F_SETFL, opts) < 0) {
            cerr << "set sock nonblocking error: " << sock << endl;
        }
    }

    void setSocketNoDelay(int sock) {
        int on = 1;
        if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char*)&on, sizeof(int)) < 0) {
            cerr << "set sock nodelay error: " << sock << endl;
        }
    }

    void shutdown();

    void realRemoveChannels();

    static void handleSignal(int singnal) {
        Server::isStart_ = false;
    }
};
}

#endif //_ECHOSERVER_SERVER_H_
