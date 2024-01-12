#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>

const char* SERVER_IP = "127.0.0.1";
const int PORT = 8080;
const int BUFFER_SIZE = 1024;

bool login(int clientSocket) {
    char username[50];
    char password[50];

    std::cout << "Enter username: ";
    std::cin >> username;
    std::cout << "Enter password: ";
    std::cin >> password;

    // Send login information to the server
    send(clientSocket, username, strlen(username), 0);
    send(clientSocket, password, strlen(password), 0);

    // Wait for the server's response
    char response[BUFFER_SIZE];
    recv(clientSocket, response, BUFFER_SIZE, 0);

    // Check if login was successful
    if (strcmp(response, "Login successful") == 0) {
        std::cout << "Login successful!" << std::endl;
        return true;
    } else {
        std::cout << "Login failed. Server says: " << response << std::endl;
        return false;
    }
}

int main() {
    int clientSocket;
    struct sockaddr_in serverAddr;
    char buffer[BUFFER_SIZE];

    // Create socket
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1) {
        std::cerr << "Error creating socket." << std::endl;
        return -1;
    }

    // Set up server address structure
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);
    serverAddr.sin_port = htons(PORT);

    // Connect to the server
    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        std::cerr << "Error connecting to server." << std::endl;
        close(clientSocket);
        return -1;
    }

    // Perform login
    if (!login(clientSocket)) {
        // Login failed, close the connection
        close(clientSocket);
        return -1;
    }

    // Echo back data from the server
    while (true) {
        std::cout << "Enter message (or type 'exit' to quit): ";
        std::cin.getline(buffer, BUFFER_SIZE);

        if (strcmp(buffer, "exit") == 0) {
            break;
        }

        send(clientSocket, buffer, strlen(buffer), 0);

        // Receive and display the echoed message
        recv(clientSocket, buffer, BUFFER_SIZE, 0);
        std::cout << "Server says: " << buffer << std::endl;
    }

    // Close the client socket
    close(clientSocket);

    return 0;
}
