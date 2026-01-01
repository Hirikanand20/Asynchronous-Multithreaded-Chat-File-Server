
#include <boost/asio.hpp>
#include <iostream>
#include <thread>

using boost::asio::ip::tcp;

class Client {
public:
    Client(boost::asio::io_context& io_context,
        const std::string& host, short port)
        : socket_(io_context)
    {
        tcp::resolver resolver(io_context);
        auto endpoints = resolver.resolve(host, std::to_string(port));
        boost::asio::connect(socket_, endpoints);

        std::cout << "Connected to server.\n";
        do_read();
    }

    void send(const std::string& msg) {
        boost::asio::async_write(
            socket_,
            boost::asio::buffer(msg),
            [this](boost::system::error_code ec, std::size_t /*length*/) {
                if (ec) {
                    std::cout << "Send failed: " << ec.message() << "\n";
                }
            });
    }

private:
    tcp::socket socket_;
    enum { max_length = 1024 };
    char data_[max_length];

    void do_read() {
        socket_.async_read_some(
            boost::asio::buffer(data_, max_length),
            [this](boost::system::error_code ec, std::size_t length) {
                if (!ec) {
                    std::string msg(data_, length);
                    std::cout << "\n[Server] " << msg << "\n> ";
                    std::cout.flush();
                    do_read();
                }
                else {
                    std::cout << "Disconnected from server.\n";
                }
            });
    }
};

int main() {
    try {
        boost::asio::io_context io_context;
        Client client(io_context, "127.0.0.1", 12345);

        std::thread t([&io_context]() { io_context.run(); });

        std::cout << "Type messages below.\n";
        std::cout << "Private message format: private:<id> <text>\n";
        std::cout << "> ";

        std::string line;
        while (std::getline(std::cin, line)) {
            client.send(line);
            std::cout << "> ";
        }

        t.join();
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
