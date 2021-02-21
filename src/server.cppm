#include <boost/asio/dispatch.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <iostream>
#include <thread>

export module server;

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace asio = boost::asio;

using tcp = boost::asio::ip::tcp;

class Session {
    websocket::stream<beast::tcp_stream> _websocket;
    beast::flat_buffer _buffer;

    Session(tcp::socket &&socket)
        : _websocket(std::move(socket)) {
        std::cout << "server created\n";
    }
};

class Listener : std::enable_shared_from_this<Listener> {
public:
    Listener(asio::io_context &context, tcp::endpoint endpoint)
        : _context{context}
        , _acceptor{context} {

        beast::error_code ec;

        _acceptor.open(endpoint.protocol(), ec);

        if (ec) {
            throw std::runtime_error{"could not open acceptor"};
        }

        _acceptor.set_option(asio::socket_base::reuse_address{true}, ec);

        if (ec) {
            throw std::runtime_error{"error: acceptor reuse address"};
        }

        _acceptor.bind(endpoint, ec);

        if (ec) {
            throw std::runtime_error{"error: acceptor bind"};
        }

        _acceptor.listen(asio::socket_base::max_listen_connections, ec);

        if (ec) {
            throw std::runtime_error{"error: acceptor listen"};
        }
    }

    void onAccept(tcp::socket socket) {
        run();
    }

    void run() {
        _acceptor.async_accept(asio::make_strand(_context),
                               [self = shared_from_this()](beast::error_code ec,
                                                           tcp::socket socket) {
                                   self->onAccept(std::move(socket));
                               });
    }

    asio::io_context &_context;
    tcp::acceptor _acceptor;
};

export void runServer(unsigned short port, int numThreads = 1) {
    asio::io_context context{numThreads};
    auto threads = std::vector<std::thread>{};
    threads.reserve(numThreads);

    std::make_shared<Listener>(context, tcp::endpoint{tcp::v4(), port})->run();

    for (size_t i = 0; i < numThreads; ++i) {
        threads.emplace_back([&context] { context.run(); });
    }

    for (auto &t : threads) {
        t.join();
    }
}
