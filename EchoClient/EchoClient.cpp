//
// Created by Luxiang Lin on 25/3/15.
//

#include <string>
#include <vector>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <thread>
#include <future>
#include <iostream>
#include <fstream>
#include <limits>
#include <chrono>

#include "EchoClient.h"
#include "opcode.h"

using namespace std;

int EchoClient::connect(int index) {
    start_ = false;
    currentIndex_ = index;
    auto addr = hosts_[index];
    auto port = ports_[index];
    struct addrinfo *result;
    struct addrinfo hint;
    int error;

    memset(&hint,0, sizeof(hint));

    hint.ai_family = AF_INET;
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_protocol = IPPROTO_TCP;
    error = ::getaddrinfo(addr.c_str(), port.c_str(), &hint, &result);
    if (error != 0) {
        // maybe reconnect?
        cerr << "Error for getaddrinfo: " << gai_strerror(error) << endl;
        return -1;
    }

    int sock = ::socket(result->ai_family, result->ai_socktype, result->ai_protocol);

    if (sock < 0) {
        cerr << "Error for creating socket" << endl;
        freeaddrinfo(result);
        return -1;
    }

    sock_ = sock;
    setSocketNoDelay(sock);

    int connectResult = ::connect(sock_, result->ai_addr, result->ai_addrlen);
    if (connectResult < 0) {
        cerr << "Error for connect: "<< connectResult << " addr: " << addr << " port: " << port << endl;
        return -1;
    }

    freeaddrinfo(result);

    start_ = true;
    future_ = async(launch::async, std::bind(&EchoClient::receiveInThread, this));
    return 0;
}

void EchoClient::shutdown() {
    if (sock_ > 0) {
        start_ = false;
        ::shutdown(sock_, SHUT_RDWR);
        ::close(sock_);
    }
}

void EchoClient::insertHost(string &host, string &port) {
    hosts_.push_back(host);
    ports_.push_back(port);
}

void EchoClient::send(const string& buffer) {
    if (buffer.size() == 0) {
        return;
    }
    vector<uint8_t> header(4, 0);
    short length = ntohs(buffer.size());
    short opcode = ntohs(static_cast<short>(Opcode::ECHO_REQUEST));
    short* lengthP = reinterpret_cast<short*>(header.data());
    short* opcodeP = reinterpret_cast<short*>(header.data()+2);
    *lengthP = length;
    *opcodeP = opcode;
    int sendResult = ::send(sock_, header.data(), header.size(), 0);
    if (sendResult == 0) {
        reconnect();
        return;
    }
    sendResult = ::send(sock_, buffer.c_str(), buffer.size(), 0);
    if (sendResult == 0) {
        reconnect();
        return;
    }
}


/*
* The strategy of choosing next server is easy.
* Just choose the next one, if reaching the end, then choose first one.
*/
void EchoClient::reconnect() {
    int reconnectResult = -1;
    int retry = 0;
    while ((reconnectResult < 0) && (retry < MAX_RECONNECT_RETRY)) {
        if (currentIndex_ == (hosts_.size() - 1)) {
            // last one
            shutdown();
            reconnectResult = connect(0);
        } else {
            shutdown();
            reconnectResult = connect(currentIndex_+1);
        }
        this_thread::sleep_for(chrono::seconds(1));
        retry += 1;
    }
}

void EchoClient::receiveInThread() {
    vector<uint8_t> header(4, 0);
    short* lengthP;
    short* opcodeP;
    short length;
    short opcodeShort;
    int recvResult;
    Opcode opcode;
    vector<uint8_t> body;
    while (start_) {
        recvResult = recv(sock_, header.data(), 4, 0);
        if (recvResult <= 0) {
            if (start_ == true) {
                reconnect();
            }
            break;
        }
        lengthP = reinterpret_cast<short*>(header.data());
        opcodeP = reinterpret_cast<short*>(header.data()+2);
        length = ntohs(*lengthP);
        opcodeShort = ntohs(*opcodeP);
        opcode = static_cast<Opcode>(opcodeShort);
        body.resize(length);
        recvResult = recv(sock_, body.data(), length, 0);
        if (recvResult <= 0) {
            if (start_ == true) {
                reconnect();
            }
            break;
        }
        if (opcode == Opcode::ECHO_RESPONSE) {
            string result(body.data(), body.data()+length);
            cout << "received: " << result << endl;
        }
    }
}

int EchoClient::selectServer() {
    ifstream inf(serverListConf_.c_str());
    if (!inf) {
        cerr << "cannot open server list config file: " << serverListConf_ << endl;
        return -1;
    }
    string serverInfo;
    while (getline(inf, serverInfo)) {
        auto iter = serverInfo.find(":");
        if (iter == -1) {
            cerr << "error item in server list config file: " << serverInfo << endl;
            return -1;
        }
        string host = serverInfo.substr(0, iter);
        string port = serverInfo.substr(iter+1, serverInfo.size());
        insertHost(host, port);
    }

    cout << "server list: " << endl;
    cout << "id host port" << endl;
    int hostSize = hosts_.size();
    for (int i=0; i < hostSize; i++) {
        cout << i << ": " << hosts_[i] << " " <<ports_[i] << endl;
    }
    int selectedId = -1;
    string dummy;
    while ((selectedId < 0) || (selectedId >= hostSize)) {
        cout << "input id to select:";
        cin >> selectedId;
        cin.clear();
        cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
    return selectedId;
}

void EchoClient::loop() {
    int serverIndex = selectServer();
    if (serverIndex < 0) {
        return;
    }
    int connectResult = connect(serverIndex);
    if (connectResult < 0) {
        return;
    }
    while (true) {
        cout << "input message: " << endl;
        string buffer;
        getline(cin, buffer);
        send(buffer);
    }
}
