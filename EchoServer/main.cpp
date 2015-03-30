#include <iostream>
#include "EchoServer.h"

using namespace std;


int main(int argc, char *argv[]) {
    if (argc != 3) {
        cout << "Echo server usage:" << endl;
        cout << "echo_server.exe 0.0.0.0 6666" << endl;
        return 1;
    }
    EchoServer server(argv[1], argv[2]);
    server.start();
    return 0;
}