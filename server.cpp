#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sstream>
#include <cstdint>

const int PORT = 8080;
const int BUFFER_SIZE = 1024;

uint8_t calculateChecksum(const std::string& username) {
    uint8_t checksum = 0;

    for (char c : username) {
        checksum += static_cast<uint8_t>(c);
    }

    return ~checksum; // Take the one's complement to get the sum complement
}

bool checkUsername(uint8_t userChecksum){

    if(calculateChecksum("testuser") == userChecksum){
        return true;
    }

    return false;
}

bool checkPassword(uint8_t passChecksum){

    if(calculateChecksum("testpass") == passChecksum){
        return true;
    }

    return false;
}

std::vector<uint8_t> performLogin(int clientSocket) {
    char creds[50];
    int bytes = 0;
    char delimiter = ' ';
    std::vector<uint8_t> sums;

    // Receive username and password from the client
    bytes = recv(clientSocket, creds, sizeof(creds), 0);
    creds[bytes] = '\0';
    std::string str(creds);

    std::istringstream iss(str);
    std::string token;
    std::vector<std::string> tokens;

    while (std::getline(iss, token, delimiter)) {
        tokens.push_back(token);
    }

     // Calculate the checksum
    uint8_t uchecksum = calculateChecksum(tokens[0]);
    sums.push_back(uchecksum);

    // Calculate the checksum
    uint8_t pchecksum = calculateChecksum(tokens[1]);
    sums.push_back(pchecksum);

     //Perform a simple login check (replace this with your authentication logic)
    int valueToSend = 0;
    if (checkUsername(uchecksum) && checkPassword(pchecksum)) {
        valueToSend = 1;
        send(clientSocket, &valueToSend, sizeof(valueToSend), 0);
        return sums;
    } 

        send(clientSocket, &valueToSend, sizeof(valueToSend), 0);
        return std::vector<uint8_t>(); //send empty vector
}

int main() {
    int serverSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    char buffer[BUFFER_SIZE];
    std::vector<uint8_t> sums;

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

        std::vector<uint8_t> sums = performLogin(clientSocket);

        // Perform login
        if (!sums.empty()) {
            // Echo back data from authenticated clients
            while (true) {
                int bytesRead = recv(clientSocket, buffer, BUFFER_SIZE, 0);
                if (bytesRead <= 0) {
                    // Connection closed or error
                    break;
                }

                uint8_t msgSequence = static_cast<uint8_t>(std::stoi(buffer)); //cast sequence to uint8_t
                int initialKey = (msgSequence << 16) | (sums[0] << 8) | sums[1];
                std::cout << "initial key: 0x" << std::hex << initialKey << std::endl;

                send(clientSocket, buffer, bytesRead, 0);

                //clear buffer array
                std::memset(buffer, 0, BUFFER_SIZE);
                buffer[0] = '\0';
            }
        }

        std::cout << "Connection closed by client." << std::endl;
        close(clientSocket);
    }

    // Close the server socket
    close(serverSocket);

    return 0;
}
