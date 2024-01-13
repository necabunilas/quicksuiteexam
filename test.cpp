#include <iostream>
#include <cstring>

const int MAX_LENGTH = 100;

int main() {
    char buffer[MAX_LENGTH];

    std::cout << "Enter a line of text: ";
    std::cin.getline(buffer, MAX_LENGTH);

    std::cout << "You entered: " << buffer << std::endl;

    return 0;
}
