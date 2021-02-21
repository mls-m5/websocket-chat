#include <iostream>

import server;

int main(int argc, char *argv[]) {
    //    auto server = Server{8080};

    //    server.run();

    runServer(8080, 1);

    std::cout << "hello\n";
    return 0;
}
