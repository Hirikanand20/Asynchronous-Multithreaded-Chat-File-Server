#include "Session.hpp"
#include "Server.hpp"
#include <iostream>
#include <algorithm>


using boost::asio::ip::tcp;

Session::Session(tcp::socket socket, Server& server, int id)
    : socket_(std::move(socket)), server_(server), id_(id), authenticated(false){
}

int Session::getId() const { return id_; }

void Session::start() {
    do_read();
}

void Session::deliver(const std::string& msg) {
    auto self(shared_from_this());
    bool write_in_progress = !write_msgs_.empty();

    write_msgs_.push_back(msg);  // COPY & OWN the data

    if (!write_in_progress) {
        do_write();
    }
}

void Session::do_write() {
    auto self(shared_from_this());
    boost::asio::async_write(
        socket_,
        boost::asio::buffer(write_msgs_.front()),
        [this, self](boost::system::error_code ec, std::size_t) {
            if (!ec) {
                write_msgs_.pop_front();
                if (!write_msgs_.empty()) {
                    do_write();
                }
            }
            else {
                server_.leave(id_);
            }
 
         });
}

void Session::do_read() {
    auto self(shared_from_this());
    socket_.async_read_some(boost::asio::buffer(data_, max_length),
        [this, self](boost::system::error_code ec, std::size_t length) {


            if (!ec) {
                std::string msg(data_, length);
                if (!msg.empty() && msg.back() == '\n')
                    msg.pop_back();
                std::cout << "Client " << id_ << ": " << msg << std::endl;
                msg.erase(std::remove(msg.begin(), msg.end(), '\r'), msg.end());
                msg.erase(std::remove(msg.begin(), msg.end(), '\n'), msg.end());
                if (!authenticated) {

                    // ---- LOGIN ----
                    if (msg.rfind("login:", 0) == 0) {

                        size_t colon = msg.find(":") + 1;
                        size_t space = msg.find(" ", colon);

                        if (space == std::string::npos) {
                            deliver("INVALID_LOGIN_FORMAT\n");
                            do_read();
                            return;
                        }

                        std::string username = msg.substr(colon, space - colon);
                        std::string password = msg.substr(space + 1);

                        if (!password.empty() && password.back() == '\n')
                            password.pop_back();

                        if (server_.authenticateUser(username, password)) {
                            authenticated = true;
                            deliver("Login successful. Your ID = " + std::to_string(id_) + "\n");
                            server_.join(self);
                        }
                        else {
                            deliver("Login failed.\n");
                        }
                    }

                    // ---- REGISTER ----
                    else if (msg.rfind("register:", 0) == 0) {

                        std::string rest = msg.substr(9);
                        while (!rest.empty() && rest[0] == ' ')
                            rest.erase(0, 1);

                        size_t spacePos = rest.find(' ');
                        if (spacePos == std::string::npos) {
                            deliver("INVALID_REGISTER_FORMAT\n");
                            do_read();
                            return;
                        }

                        std::string user = rest.substr(0, spacePos);
                        std::string pass = rest.substr(spacePos + 1);

                        if (!pass.empty() && pass.back() == '\n')
                            pass.pop_back();

                        if (server_.registerUser(user, pass)) {
                            deliver("REGISTERED\n");
                        }
                        else {
                            deliver("USERNAME_TAKEN\n");
                        }
                    }

                    // ---- BLOCK EVERYTHING ELSE ----
                    else {
                        deliver("Please login or register first.\n");
                    }

                    do_read();
                    return;
                }


                if (msg.rfind("group:create", 0) == 0) {
                    int gid = std::stoi(msg.substr(13));
                    server_.createGroup(gid);
                    deliver("Group " + std::to_string(gid) + " created.\n");
                }

                // group:join 10
                else if (msg.rfind("group:join", 0) == 0) {
                    int gid = std::stoi(msg.substr(11));
                    server_.joinGroup(gid, id_);
                    deliver("Joined group " + std::to_string(gid) + "\n");
                }

                // group:leave 10
                else if (msg.rfind("group:leave", 0) == 0) {
                    try {
                        int gid = std::stoi(msg.substr(11));
                        server_.leaveGroup(gid, id_);
                        deliver("Left group " + std::to_string(gid) + "\n");
                    }
                    catch (...) {
                        deliver("Invalid group ID\n");
                    }
                }

                // group:list 12
                else if (msg.rfind("group:list", 0) == 0) {

                    std::istringstream iss(msg);
                    std::string cmd;
                    int gid;

                    iss >> cmd >> gid;

                    if (!iss || gid <= 0) {
                        deliver("Usage: group:list <group_id>\n");
                        return;
                    }

                    std::string users = server_.listGroupUsers(gid);
                    deliver(users);
                    return;
                }

                // group:10 Hello guys
                else if (msg.rfind("group:", 0) == 0) {

                    size_t colon = msg.find(":");
                    size_t space = msg.find(" ", colon + 1);

                    int gid = std::stoi(msg.substr(colon + 1, space - (colon + 1)));
                    std::string text = msg.substr(space + 1);

                    server_.sendToGroup(gid, id_, text);

                    server_.logGroupMessage(id_, gid, text);
                }
                else if (msg == "whoami\n" || msg == "whoami") {

                    std::string username = server_.getUsernameById(id_);

                    deliver("You are " + username +
                        " (ID: " + std::to_string(id_) + ")\n");

                    do_read();
                    return;
                }

                else if (msg == "logout\n" || msg == "logout") {

                    authenticated = false;

                    server_.leave(id_);   // remove from active sessions
                    // updates last_seen

                    deliver("Logged out successfully.\n"
                        "Please login again using: login:<username> <password>\n");

                    do_read();
                    return;
                }
                else if (msg.rfind("public:", 0) == 0) {
                    std::string text = msg.substr(7); // after "public:"
                    std::string out =
                        "Public | User " + std::to_string(id_) + ": " + text + "\n";

                    server_.broadcast(out, id_);
                }


                else if (msg.rfind("private:", 0) == 0) {

                    size_t id_pos = msg.find(":") + 1;
                    size_t space_pos = msg.find(" ", id_pos);

                    if (space_pos == std::string::npos) {
                        deliver("INVALID_PRIVATE_FORMAT\n");
                        do_read();
                        return;
                    }

                    int receiver_id = std::stoi(msg.substr(id_pos, space_pos - id_pos));
                    std::string text = msg.substr(space_pos + 1);

                    std::string formatted =
                        "Private message from " + std::to_string(id_) + ": " + text + "\n";

                    if (!server_.sendMessage(receiver_id, formatted)) {
                        deliver("User " + std::to_string(receiver_id) + " is OFFLINE\n");
                    }
                    else {
                        server_.logPrivateMessage(id_, receiver_id, text);
                    }

                    do_read();
                }
                else if (msg == "leave server\n" || msg == "leave server") {

                    deliver("Goodbye!\n");

                    // Remove user from server session list
                    server_.leave(id_);

                    // Close socket gracefully
                    boost::system::error_code ec;
                    socket_.shutdown(tcp::socket::shutdown_both, ec);
                    socket_.close(ec);

                    return; // STOP do_read()
                }

                else if (msg == "list users") {
                    deliver(server_.listUsers());
                    do_read();
                    return;
                }

                else if (msg.rfind("STATUS:", 0) == 0) {

                    std::string username = msg.substr(7);

                    // Trim leading spaces
                    while (!username.empty() && username.front() == ' ')
                        username.erase(username.begin());

                    // Trim trailing newline
                    if (!username.empty() && username.back() == '\n')
                        username.pop_back();

                    std::string status = server_.getUserStatus(username);
                    deliver(status + "\n");

                    do_read();
                    return;
                }



                else {
                    server_.broadcast(msg, id_);
                }

                do_read();
            }
            else {
                server_.leave(id_);
            }
        });
}
