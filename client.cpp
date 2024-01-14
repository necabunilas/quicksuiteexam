#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>

const char *SERVER_IP = "127.0.0.1";
const int PORT = 8080;
const int BUFFER_SIZE = 1024;

struct data
{
    uint32_t initialKey;
    uint16_t ciphers[64];
};

bool login(int clientSocket)
{
    std::string username;
    std::string password;
    std::string creds;
    int bytes = 0;

    std::cout << "Enter username: ";
    std::cin >> username;
    std::cout << "Enter password: ";
    std::cin >> password;

    creds = username + ' ' + password;

    // Send login information to the server
    send(clientSocket, creds.c_str(), strlen(creds.c_str()), 0);

    // Wait for the server's response
    int response;
    bytes = recv(clientSocket, &response, sizeof(response), 0);

    std::cout << "response: " << response << std::endl;

    // Check if login was successful
    if (response == 1)
    {
        std::cout << "Login successful!" << std::endl;
        return true;
    }
    else
    {
        std::cout << "Login failed. Server says: " << response << std::endl;
        return false;
    }
}

int main()
{
    int clientSocket;
    struct sockaddr_in serverAddr;
    char buffer[BUFFER_SIZE];

    // Create socket
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1)
    {
        std::cerr << "Error creating socket." << std::endl;
        return -1;
    }

    // Set up server address structure
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);
    serverAddr.sin_port = htons(PORT);

    // Connect to the server
    if (connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1)
    {
        std::cerr << "Error connecting to server." << std::endl;
        close(clientSocket);
        return -1;
    }

    // Perform login
    if (!login(clientSocket))
    {
        // Login failed, close the connection
        close(clientSocket);
        return -1;
    }

    std::cout << "Press Enter to continue...";
    std::cin.ignore(); // Ignore the newline character in the input buffer
    std::cin.get();

    // Echo back data from the server
    std::cout << "Enter message (or type 'exit' to quit): ";
    std::cin.getline(buffer, BUFFER_SIZE);

    std::cout << "You entered: " << buffer << std::endl;

    send(clientSocket, buffer, strlen(buffer), 0);

    // Receive and display the echoed message
    data serverData;
    recv(clientSocket, &serverData, sizeof(serverData), 0);
    std::cout << "initial key: 0x" << std::hex << serverData.initialKey << std::endl;
    std::cout << "cipher keys (first 64 bytes): " << std::endl;

    // Print using a loop with the known size
    for (int i = 0; i < 64; ++i)
    {
        std::cout << std::hex << serverData.ciphers[i] << " ";
    }

    // Close the client socket
    close(clientSocket);

    return 0;
}
