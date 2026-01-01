#include "Server.hpp"
#include "Session.hpp"
#include <iostream>
#include "Color2.hpp"
using boost::asio::ip::tcp;

Server::Server(boost::asio::io_context& io_context, short port)
    : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)), next_id_(1)
{
    initDatabase();
    do_accept();
}

Server::~Server() {
    sqlite3_close(db);
}

void Server::join(std::shared_ptr<Session> session) {
    std::lock_guard<std::mutex> lock(mutex_);
    sessions_[session->getId()] = session;
}

void Server::leave(int id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    sqlite3_stmt* stmt;
    const char* sql = "UPDATE users SET last_seen = CURRENT_TIMESTAMP WHERE id = ?";

    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, id);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    sessions_.erase(id);

}
std::string Server::getUserStatus(const std::string& username) {
    sqlite3_stmt* stmt;

    const char* sql = "SELECT id, last_seen FROM users WHERE username = ?";
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);

    int result = sqlite3_step(stmt);
    if (result != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return "USER_NOT_FOUND";
    }

    int uid = sqlite3_column_int(stmt, 0);
    std::string lastSeen = (const char*)sqlite3_column_text(stmt, 1);

    sqlite3_finalize(stmt);

    // check if online
    std::lock_guard<std::mutex> lock(mutex_);
    if (sessions_.count(uid))
        return "ONLINE"; 

    return "OFFLINE last seen: " + lastSeen;
}


void Server::broadcast(const std::string& msg, int sender_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& s : sessions_) {
        if (s.first != sender_id) {
            s.second->deliver(msg);
        }
    }
}

bool Server::sendMessage(int receiver_id, const std::string& msg) {
    auto it = sessions_.find(receiver_id);
    if (it != sessions_.end()) {
        it->second->deliver(msg);
        return true;
    }
    return false;
}


void Server::createGroup(int group_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    groups[group_id] = {};
}

void Server::joinGroup(int group_id, int user_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    groups[group_id].push_back(user_id);
}

void Server::leaveGroup(int group_id, int user_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = groups.find(group_id);
    if (it == groups.end())
        return;

    auto& members = it->second;
    members.erase(std::remove(members.begin(), members.end(), user_id),
        members.end());

    // Remove empty group
    if (members.empty()) {
        groups.erase(it);
    }
}


void Server::sendToGroup(int group_id, int sender_id, const std::string& text) {
   
    std::lock_guard<std::mutex> lock(mutex_);
    for (int uid : groups[group_id]) {
        if (uid != sender_id && sessions_.count(uid)) {
            ServerColor::set(ServerColor::BRIGHT_MAGENTA);
            sessions_[uid]->deliver(
                "Group " + std::to_string(group_id) + " | " +
                "User " + std::to_string(sender_id) + ": " + text
            );
            ServerColor::reset();

        }
    }
}
std::string Server::listUsers() {
    sqlite3_stmt* stmt;
    std::string result = "ONLINE:\n";

    const char* sql = "SELECT id, username FROM users";

    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    std::lock_guard<std::mutex> lock(mutex_);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        std::string username = (const char*)sqlite3_column_text(stmt, 1);

        if (sessions_.count(id)) {
            result += std::to_string(id) + ": " + username + "\n";
        }
    }

    sqlite3_finalize(stmt);

    if (result == "ONLINE:\n")
        result += "No users online\n";

    return result;
}

bool Server::registerUser(const std::string& username, const std::string& password) {
    sqlite3_stmt* stmt;
    const char* sql = "INSERT INTO users (username, password) VALUES (?, ?)";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, password.c_str(), -1, SQLITE_STATIC);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);

    return success;
}

void Server::logGroupMessage(int sender, int group_id, const std::string& text) {
    sqlite3_stmt* stmt;
    const char* sql =
        "INSERT INTO group_messages (group_id, sender_id, message) VALUES (?, ?, ?)";

    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, group_id);
    sqlite3_bind_int(stmt, 2, sender);
    sqlite3_bind_text(stmt, 3, text.c_str(), -1, SQLITE_STATIC);

    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}


void Server::logPrivateMessage(int sender, int receiver, const std::string& text) {
    sqlite3_stmt* stmt;
    const char* sql =
        "INSERT INTO private_messages (sender_id, receiver_id, message) VALUES (?, ?, ?)";

    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, sender);
    sqlite3_bind_int(stmt, 2, receiver);
    sqlite3_bind_text(stmt, 3, text.c_str(), -1, SQLITE_STATIC);

    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}
bool Server::authenticateUser(const std::string& username, const std::string& password) {
    sqlite3_stmt* stmt;

    const char* sql = "SELECT id FROM users WHERE username = ? AND password = ?";

    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, password.c_str(), -1, SQLITE_STATIC);
    
    bool success = (sqlite3_step(stmt) == SQLITE_ROW);

    sqlite3_finalize(stmt);
    return success;
}


void Server::initDatabase() {
    if (sqlite3_open("chat.db", &db)) {
        std::cerr << "Failed to open database.\n";
    }


    const char* createUserTable =
        "CREATE TABLE IF NOT EXISTS users ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "username TEXT UNIQUE NOT NULL,"
        "password TEXT NOT NULL,"
        "last_seen DATETIME DEFAULT CURRENT_TIMESTAMP);";
    

    char* errMsg2 = nullptr;
    sqlite3_exec(db, createUserTable, nullptr, nullptr, &errMsg2);
    
    if (errMsg2) {
        std::cerr << "DB Error: " << errMsg2 << std::endl;
        sqlite3_free(errMsg2);
    }

  
    const char* createGroupTable =
        "CREATE TABLE IF NOT EXISTS group_messages ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "group_id INTEGER NOT NULL,"
        "sender_id INTEGER NOT NULL,"
        "message TEXT NOT NULL,"
        "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP);";
    char* errMsg1 = nullptr;

    sqlite3_exec(db, createGroupTable, nullptr, nullptr, &errMsg1);
    if (errMsg1) {
        std::cerr << "DB Error: " << errMsg1 << std::endl;
        sqlite3_free(errMsg1);
    }

    const char* createTable =
        "CREATE TABLE IF NOT EXISTS private_messages ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "sender_id INTEGER NOT NULL,"
        "receiver_id INTEGER NOT NULL,"
        "message TEXT NOT NULL,"
        "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP);";

    char* errMsg = nullptr;
    sqlite3_exec(db, createTable, nullptr, nullptr, &errMsg);
    if (errMsg) {
        std::cerr << "DB Error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
    }
}
std::string Server::listGroupUsers(int gid) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!groups.count(gid))
        return "Group does not exist.\n";

    std::string result = "Group " + std::to_string(gid) + " users:\n";

    for (int uid : groups[gid]) {
        std::string username = getUsernameById(uid);

        if (sessions_.count(uid))
            result += username + " (online)\n";
        else
            result += username + " (offline)\n";
    }

    if (groups[gid].empty())
        result += "No users in this group.\n";

    return result;
}

std::string Server::getUsernameById(int id) {
    sqlite3_stmt* stmt;
    const char* sql = "SELECT username FROM users WHERE id = ?";

    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, id);

    std::string username = "UNKNOWN";
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        username = (const char*)sqlite3_column_text(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return username;
}


void Server::do_accept() {
    acceptor_.async_accept(
        [this](boost::system::error_code ec, tcp::socket socket) {
            if (!ec) {
                auto session = std::make_shared<Session>(std::move(socket), *this, next_id_++);
                

                std::cout << "Client connected: " << session->getId() << std::endl;
                session->start();
            }
            do_accept();
        });
}
