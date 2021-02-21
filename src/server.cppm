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

class Session : std::enable_shared_from_this<Session> {
public:
    websocket::stream<beast::tcp_stream> _stream;
    beast::flat_buffer _buffer;

    Session(tcp::socket &&socket)
        : _stream(std::move(socket)) {
        std::cout << "server created\n";
    }

    void run() {
        asio::dispatch(_stream.get_executor(), [self = shared_from_this()] {
            // To run on right context/strand
            self->onRun();
        });
    }

    void onRun() {
        _stream.set_option(websocket::stream_base::timeout::suggested(
            beast::role_type::server));

        _stream.set_option(websocket::stream_base::decorator(
            [](websocket::response_type &res) {
                res.set(beast::http::field::server,
                        std::string{BOOST_BEAST_VERSION_STRING} +
                            " websocket-server-async");
            }));

        _stream.async_accept([self = shared_from_this()](beast::error_code ec) {

        });
    }

    void onAccept(beast::error_code ec) {
        if (ec) {
            std::cerr << "failed session accept\n";
            return;
        }

        read();
    }

    void read() {
        _stream.async_read(_buffer,
                           [self = shared_from_this()](beast::error_code ec,
                                                       size_t bytesTransfered) {
                               self->onRead(ec, bytesTransfered);
                           });
    }

    void onRead(beast::error_code ec, size_t bytesTransfered) {
        boost::ignore_unused(bytesTransfered);

        if (ec == websocket::error::closed) {
            return;
        }
        else if (ec) {
            std::cerr << "error onRead: " << ec << std::endl;
        }

        _stream.text(_stream.got_text());
        _stream.async_write(
            _buffer.data(),
            [self = shared_from_this()](auto ec, auto bytesTransfered) {

            });
    }

    void onWrite(beast::error_code ec, size_t bytesTransfered) {
        boost::ignore_unused(bytesTransfered);

        if (ec) {
            std::cerr << "failed onWrite " << ec << "\n";
            return;
        }

        _buffer.consume(_buffer.size());

        read();
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

    void onAccept(beast::error_code ec, tcp::socket socket) {
        if (ec) {
            std::cerr << "failed to accept\n";
            return;
        }
        else {
            std::cout << "accept client" << std::endl;

            std::make_shared<Session>(std::move(socket))->run();
        }

        run();
    }

    void run() {
        _acceptor.async_accept(asio::make_strand(_context),
                               [self = shared_from_this()](beast::error_code ec,
                                                           tcp::socket socket) {
                                   self->onAccept(ec, std::move(socket));
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
