#pragma once
#include <boost/asio.hpp>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include "sqlite/sqlite3.h"

class Session;

class Server {
public:
    Server(boost::asio::io_context& io_context, short port);
    ~Server();

    void join(std::shared_ptr<Session> session);
    void leave(int id);
    std::string getUserStatus(const std::string& username);
    void broadcast(const std::string& msg, int sender_id);
    bool sendMessage(int receiver_id, const std::string& msg);
    std::string getUsernameById(int id);
    std::string listUsers();

    void createGroup(int group_id);

    void joinGroup(int group_id, int user_id);

    void leaveGroup(int group_id, int user_id);

    void sendToGroup(int group_id, int sender_id, const std::string& text);

    bool registerUser(const std::string& username, const std::string& password);

    void logGroupMessage(int sender, int group_id, const std::string& text);

    void logPrivateMessage(int sender, int receiver, const std::string& msg);

    bool authenticateUser(const std::string& username, const std::string& password);
    std::string listGroupUsers(int gid);
private:
    void initDatabase();
    
    void do_accept();

    boost::asio::ip::tcp::acceptor acceptor_;
    std::map<int, std::vector<int>> groups;  
    std::map<int, std::shared_ptr<Session>> sessions_;
    std::mutex mutex_;
    int next_id_;
    sqlite3* db;
};
