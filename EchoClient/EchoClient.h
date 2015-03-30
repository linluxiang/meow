//
// Created by Luxiang Lin on 25/3/15.
//

#ifndef _ECHOCLIENT_ECHOCLIENT_H_
#define _ECHOCLIENT_ECHOCLIENT_H_

#define MAX_RECONNECT_RETRY 10

#include <string>
#include <vector>
#include <future>
#include <iostream>
#include <netinet/tcp.h>
#include <netdb.h>
#include <errno.h>

using namespace std;

class EchoClient {
public:
    EchoClient(const string& serverListConf):
        serverListConf_(serverListConf),
        currentIndex_(-1),
        sock_(-1),
        start_(false) {

    }

    /*
    * 1. choose server
    * 2. read from stdin and send to server
    */
    void loop();

private:
    void receiveInThread();
    int selectServer();
    int connect(int index);
    void insertHost(string& host, string& port);
    void send(const string&);
    void shutdown();
    void reconnect();
    void setSocketNoDelay(int sock) {
        int on = 1;
        if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char*)&on, sizeof(int)) < 0) {
            cerr << "set sock nodelay error: " << sock << endl;
        }
    }

    string serverListConf_;
    bool start_;
    vector<string> hosts_;
    vector<string> ports_;
    int currentIndex_;
    int sock_;
    future<void> future_;
};


#endif //_ECHOCLIENT_ECHOCLIENT_H_
