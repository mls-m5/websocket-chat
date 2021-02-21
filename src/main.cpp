#include <iostream>

import server;

//! To test this, run in browser javascript console:
//!
//! var exampleSocket = new WebSocket("ws://localhost:8080")
//! exampleSocket.onmessage = x => console.log(x)
//! exampleSocket.send("hello");
//!
int main(int argc, char *argv[]) {
    runServer(8080, 1);

    std::cout << "hello\n";
    return 0;
}
