#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sstream>
#include <cstdint>

const int PORT = 8080;
const int BUFFER_SIZE = 1024;

struct data
{
    uint32_t initialKey;
    uint ciphers[64];
};

uint8_t calculateChecksum(const std::string &strval)
{
    uint8_t checksum = 0;

    for (char c : strval)
    {
        checksum += static_cast<uint8_t>(c);
    }

    return ~checksum; // Take the one's complement to get the sum complement
}

bool checkUsername(uint8_t userChecksum)
{

    if (calculateChecksum("testuser") == userChecksum)
    {
        return true;
    }

    return false;
}

bool checkPassword(uint8_t passChecksum)
{

    if (calculateChecksum("testpass") == passChecksum)
    {
        return true;
    }

    return false;
}

uint32_t next_key(uint32_t key)
{
    return (key * 1103515245 + 12345) % 0x7FFFFFFF;
}

std::vector<uint8_t> performLogin(int clientSocket)
{
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

    while (std::getline(iss, token, delimiter))
    {
        tokens.push_back(token);
    }

    // Calculate the checksum
    uint8_t uchecksum = calculateChecksum(tokens[0]);
    sums.push_back(uchecksum);

    // Calculate the checksum
    uint8_t pchecksum = calculateChecksum(tokens[1]);
    sums.push_back(pchecksum);

    // Perform a simple login check (replace this with your authentication logic)
    int valueToSend = 0;
    if (checkUsername(uchecksum) && checkPassword(pchecksum))
    {
        valueToSend = 1;
        send(clientSocket, &valueToSend, sizeof(valueToSend), 0);
        return sums;
    }

    send(clientSocket, &valueToSend, sizeof(valueToSend), 0);
    return std::vector<uint8_t>(); // send empty vector
}

int main()
{
    int serverSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    char buffer[BUFFER_SIZE];
    std::vector<uint8_t> sums;

    // Create socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1)
    {
        std::cerr << "Error creating socket." << std::endl;
        return -1;
    }

    // Set up server address structure
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    // Bind socket
    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1)
    {
        std::cerr << "Error binding socket." << std::endl;
        close(serverSocket);
        return -1;
    }

    // Listen for incoming connections
    if (listen(serverSocket, 10) == -1)
    {
        std::cerr << "Error listening for connections." << std::endl;
        close(serverSocket);
        return -1;
    }

    std::cout << "Server listening on port " << PORT << "..." << std::endl;

    // Accept incoming connections and perform login
    while (true)
    {
        socklen_t clientAddrLen = sizeof(clientAddr);
        clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &clientAddrLen);

        if (clientSocket == -1)
        {
            std::cerr << "Error accepting connection." << std::endl;
            continue;
        }

        std::cout << "Connection accepted from " << inet_ntoa(clientAddr.sin_addr) << ":" << ntohs(clientAddr.sin_port) << std::endl;

        std::vector<uint8_t> sums = performLogin(clientSocket);

        // Perform login
        if (!sums.empty())
        {
            // Echo back data from authenticated clients
            while (true)
            {
                int bytesRead = recv(clientSocket, buffer, BUFFER_SIZE, 0);
                if (bytesRead <= 0)
                {
                    // Connection closed or error
                    break;
                }

                int msgSequence = std::stoi(buffer, nullptr, 16); // cast sequence to uint8_t
                uint8_t username = ~sums[0];
                uint8_t password = ~sums[1];
                std::cout << "sequence key: 0x" << std::hex << msgSequence << std::endl;
                std::cout << "username: 0x" << std::hex << static_cast<int>(username) << std::endl;
                std::cout << "password: 0x" << std::hex << static_cast<int>(password) << std::endl;

                data clientData;
                clientData.initialKey = (msgSequence << 16) | (static_cast<int>(username) << 8) | static_cast<int>(password);
                std::cout << "initial key: 0x" << std::hex << clientData.initialKey << std::endl;

                uint counter = 0;
                uint next = 0;

                do
                {
                    // get next key
                    if (counter == 0)
                    {
                        next = next_key(clientData.initialKey);
                    }
                    else
                    {
                        next = next_key(next);
                    }

                    clientData.ciphers[counter] = next % 256;

                    // loop back
                    counter++;
                } while (counter < 64);

                send(clientSocket, &clientData, sizeof(clientData), 0);
            }
        }

        std::cout << "Connection closed by client." << std::endl;
        close(clientSocket);
    }

    // Close the server socket
    close(serverSocket);

    return 0;
}
