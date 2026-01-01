#pragma once
#include <boost/asio.hpp>
#include <memory>
#include <string>
#include <deque>


class Server;

class Session : public std::enable_shared_from_this<Session> {
public:
    Session(boost::asio::ip::tcp::socket socket, Server& server, int id);

    void start();
    void deliver(const std::string& msg);
    int getId() const;

 private:
    void do_read();

    void do_write();
    bool authenticated;
    boost::asio::ip::tcp::socket socket_;
    enum { max_length = 1024 };
    char data_[max_length];
    Server& server_;
    int id_;

    std::deque<std::string> write_msgs_;
};
