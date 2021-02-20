#include <iostream>

export module server;

export class Server {
public:
    Server() {
        std::cout << "server created\n";
    }
};
