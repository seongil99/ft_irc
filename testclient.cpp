#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

void receiveData(int clientSocket) {
    char buf[1024];
    while (true) {
        int n = read(clientSocket, buf, 1023);
        if (n > 0) {
            buf[n] = '\0';
            std::cout << "Received: " << buf << std::endl;
        }
    }
}

int main() {
    // creating socket
    int clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    // specifying address
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    // sending connection request
    connect(clientSocket, (struct sockaddr *)&serverAddress,
            sizeof(serverAddress));

    std::thread recvThread(receiveData, clientSocket);
    recvThread.detach(); // Detach the thread to run independently

    // sending data
    std::string s;
    while (getline(std::cin, s)) {
        s += " \r\n";
        send(clientSocket, s.c_str(), s.size(), 0);
    }

    // closing socket
    close(clientSocket);
    return 0;
}

// compile option //
// c++ testclient.cpp -o client -std=c++11 -pthread
////////////////////
