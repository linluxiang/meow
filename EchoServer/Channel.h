//
// Created by Luxiang Lin on 24/3/15.
//

#ifndef _ECHOSERVER_CHANNEL_H_
#define _ECHOSERVER_CHANNEL_H_

#define DEFAULT_BUFFER_SIZE 1024

#include <functional>
#include <vector>
#include <sys/socket.h>
#include <unistd.h>

using namespace std;

namespace simpleserver {

class Server;

/*
* Channel is used for holding the client information such as socket, addrinfo etc.
* This implementation combines buffer and channel together, this is not so elegent but directly.
*
*/
class Channel {
friend class Server;
public:
    Channel(long, int, shared_ptr<Server>);

    int getId() {
        return id_;
    }

    int getSocket() {
        return socket_;
    }

    void setSockAddr(struct sockaddr& addr) {
        sockaddr_ = addr;
    }

    long write(const vector<uint8_t>& buffer);

    /*
    * This function will always return empty vector for the first time.
    * After it was first called, the ioloop will try to read length bytes
    * Only if it successfully read length bytes, it will return the whole buffer and reset internal buffe to empty
    */
    vector<uint8_t> readIntoBuffer(long length);

    void shutdown() {
        ::shutdown(socket_, SHUT_RDWR);
        close(socket_);
        waitReadBytesCount_ = 0;
        waitWriteBytesCount_ = 0;
        readBuffer_.resize(0);
        writeBuffer_.resize(0);
    }

private:
    long id_;
    int socket_;
    struct sockaddr sockaddr_;
    shared_ptr<Server> server_;
    long waitReadBytesCount_;
    long waitWriteBytesCount_;
    vector<uint8_t> readBuffer_;
    vector<uint8_t> writeBuffer_;

    Channel(const Channel&) = delete;
    Channel& operator=(const Channel&) = delete;

    /*
    * this is to read from socket to inner buffer
    * only should be called in server
    */
    void read();

    /*
    * Server will use this one to verify whether it should call read
    */
    long waitReadBytesCount() {
        return waitReadBytesCount_;
    }

    /*
    * this is to write to socket from inner buffer
    * only should be called in server
    */
    void write();

    /*
    * Server will use this one to verify whether it should call write
    */
    long waitWriteBytesCount() {
        return waitWriteBytesCount_;
    }
};

}

#endif //_ECHOSERVER_CHANNEL_H_
