// C++ program to illustrate the client application in the
// socket programming
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

int main() {
    // creating socket
    int clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    char buf[1024];

    // specifying address
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    // sending connection request
    connect(clientSocket, (struct sockaddr *)&serverAddress,
            sizeof(serverAddress));

    // sending data
    std::string s;
    while (getline(std::cin, s)) {
        send(clientSocket, s.c_str(), s.size(), 0);
        int n = read(clientSocket, buf, 1023);
        buf[n] = 0;
        std::cout << buf << std::endl;
    }

    // closing socket
    close(clientSocket);

    return 0;
}
