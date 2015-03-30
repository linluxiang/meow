#include <iostream>
#include "EchoClient.h"

using namespace std;

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cout << "Echo client usage:" << endl;
        cout << "echo_client.exe server_list.conf" << endl;
        return 1;
    }
    EchoClient client(argv[1]);
    client.loop();
    cout << "finish" << endl;
    return 0;
}