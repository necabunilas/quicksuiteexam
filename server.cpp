#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>

const int PORT = 8080;
const int BUFFER_SIZE = 1024;

bool performLogin(int clientSocket) {
    char username[50];
    char password[50];

    // Receive username and password from the client
    recv(clientSocket, username, sizeof(username), 0);
    recv(clientSocket, password, sizeof(password), 0);

    // Perform a simple login check (replace this with your authentication logic)
    if (strcmp(username, "user") == 0 && strcmp(password, "pass") == 0) {
        send(clientSocket, "Login successful", strlen("Login successful"), 0);
        return true;
    } else {
        send(clientSocket, "Login failed", strlen("Login failed"), 0);
        return false;
    }
}

int main() {
    int serverSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    char buffer[BUFFER_SIZE];

    // Create socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        std::cerr << "Error creating socket." << std::endl;
        return -1;
    }

    // Set up server address structure
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    // Bind socket
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        std::cerr << "Error binding socket." << std::endl;
        close(serverSocket);
        return -1;
    }

    // Listen for incoming connections
    if (listen(serverSocket, 10) == -1) {
        std::cerr << "Error listening for connections." << std::endl;
        close(serverSocket);
        return -1;
    }

    std::cout << "Server listening on port " << PORT << "..." << std::endl;

    // Accept incoming connections and perform login
    while (true) {
        socklen_t clientAddrLen = sizeof(clientAddr);
        clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);

        if (clientSocket == -1) {
            std::cerr << "Error accepting connection." << std::endl;
            continue;
        }

        std::cout << "Connection accepted from " << inet_ntoa(clientAddr.sin_addr) << ":" << ntohs(clientAddr.sin_port) << std::endl;

        // Perform login
        if (performLogin(clientSocket)) {
            // Echo back data from authenticated clients
            while (true) {
                int bytesRead = recv(clientSocket, buffer, BUFFER_SIZE, 0);
                if (bytesRead <= 0) {
                    // Connection closed or error
                    break;
                }

                send(clientSocket, buffer, bytesRead, 0);
            }
        }

        std::cout << "Connection closed by client." << std::endl;
        close(clientSocket);
    }

    // Close the server socket
    close(serverSocket);

    return 0;
}
