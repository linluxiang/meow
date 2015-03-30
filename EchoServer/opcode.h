//
// Created by Luxiang Lin on 25/3/15.
//

#ifndef _ECHOSERVER_OPCODE_H_
#define _ECHOSERVER_OPCODE_H_

enum class Opcode {
    // start of request opcode
    REQUEST_START = 1000,
    ECHO_REQUEST = 1001,
    REQUEST_END = 1999,

    // start of response opcode
    RESPONSE_START = 2000,
    ECHO_RESPONSE = 2001,
    RESPONSE_END = 2999,
};

#endif //_ECHOSERVER_OPCODE_H_
