#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <boost/asio.hpp>
#include <string>

using boost::asio::ip::tcp;

class Client {
public:
    Client(boost::asio::io_context& io_context,
        const std::string& host,
        short port);

    void send(const std::string& msg);

private:
    void do_read();

    tcp::socket socket_;
    enum { max_length = 1024 };
    char data_[max_length];
};

#endif // CLIENT_HPP
