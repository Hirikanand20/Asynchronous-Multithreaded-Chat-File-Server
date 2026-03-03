#include "Session.hpp"
#include "Server.hpp"
#include <iostream>
#include <algorithm>



using boost::asio::ip::tcp;
Session::Session(tcp::socket socket, Server& server, int session_id)
    : executor_(socket.get_executor()),
    socket_(std::move(socket)),
    data_socket_(executor_),
    data_acceptor_(executor_),
    send_data_socket_(executor_),
    recv_data_socket_(executor_),
    server_(server),
    session_id_(session_id),
    user_id_(-1),
    username_(""),
    authenticated(false)
{
}
//"C:\Users\hirik\source\repos\P11\P11\files\send\inter prep.txt"
//C:\Users\hirik\source\repos\P11\x64\Debug\client.exe

void Session::start() { 

    do_read();
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
                server_.leave(session_id_);
            }

        });
}

void Session::deliver(const std::string& msg) {
    auto self(shared_from_this());
    bool write_in_progress = !write_msgs_.empty();

    write_msgs_.push_back(msg);  // COPY & OWN the data 

    if (!write_in_progress) {
        do_write();
    }
}


void Session::handle_data_port(
    int port,
    const std::string& filename,
    std::uintmax_t filesize,
    const boost::asio::ip::address& addr)
{
    pending_send_file_ = filename;
    expected_filesize_ = filesize;
    bytes_sent_ = 0;

    tcp::endpoint ep(addr, port);

    deliver("SENDING_FILE " + filename +
        " SIZE " + std::to_string(filesize) + "\n");

    connect_and_send_file(filename, ep);
}

void Session::prepare_file_receive(
    const std::string& filename,
    std::uintmax_t filesize,
    std::shared_ptr<Session> sender)
{
    recv_data_socket_ = tcp::socket(socket_.get_executor());

    data_acceptor_.open(tcp::v4());
    data_acceptor_.bind({ tcp::v4(), 0 });
    data_acceptor_.listen();

    fs::path recv_path = recv_dir_ / filename;
    outfile_.open(recv_path, std::ios::binary);

    if (!outfile_) {
        deliver("CANNOT_CREATE_FILE\n");
        return;
    }


    int port = data_acceptor_.local_endpoint().port();

    sender->handle_data_port(
        port,
        filename,
        filesize,
        socket_.remote_endpoint().address()
    );

    auto self = shared_from_this();
    data_acceptor_.async_accept(
        recv_data_socket_,
        [this, self, sender](boost::system::error_code ec) {
            if (!ec) {
                deliver("RECEIVER_DATA_CONNECTED\n");
                do_file_receive(sender);
            }
        });
}


void Session::connect_and_send_file(
    const std::string& filename,
    const tcp::endpoint& ep)
{
    send_data_socket_ = tcp::socket(socket_.get_executor());

    infile_.open(send_dir_ / filename, std::ios::binary);
    if (!infile_) {
        deliver("FILE_NOT_FOUND\n");
        return;
    }

    auto self = shared_from_this();
    send_data_socket_.async_connect(
        ep,
        [this, self](boost::system::error_code ec) {
            if (!ec) {
                deliver("SENDER_DATA_CONNECTED\n");
                do_file_send();
            }
        });
}



void Session::do_file_send()
{
    infile_.read(file_buf_.data(), file_buf_.size());
    std::size_t n = infile_.gcount();

    if (n == 0) {
        infile_.close();
        send_data_socket_.close();

        deliver("FILE_SENT " + current_filename_ +
            " BYTES " + std::to_string(bytes_sent_) + "\n");
        return;
    }

    bytes_sent_ += n;

    auto self = shared_from_this();
    boost::asio::async_write(
        send_data_socket_,
        boost::asio::buffer(file_buf_.data(), n),
        [this, self](boost::system::error_code ec, std::size_t) {
            if (!ec) do_file_send();
        });
}

void Session::do_file_receive(std::shared_ptr<Session> sender)
{
    auto self = shared_from_this();
    recv_data_socket_.async_read_some(
        boost::asio::buffer(file_buf_),
        [this, self, sender](boost::system::error_code ec, std::size_t len) {

            if (!ec) {
                outfile_.write(file_buf_.data(), len);
                bytes_received_ += len;
                do_file_receive(sender);
            }
            else if (ec == boost::asio::error::eof) {
                outfile_.close();
                recv_data_socket_.close();

                sender->deliver(
                    "FILE_DELIVERED " + current_filename_ +
                    " BYTES " + std::to_string(bytes_received_) + "\n"
                );
            }
        });
}

void Session::do_read() {
    auto self(shared_from_this());
    socket_.async_read_some(boost::asio::buffer(data_, max_length),
        [this, self](boost::system::error_code ec, std::size_t length) {
            if (ec) {
                server_.leave(session_id_);
                return;
            }


            if (!ec) {
                std::string msg(data_, length);
                if (!msg.empty() && msg.back() == '\n')
                    msg.pop_back();
                std::cout << "Client " << session_id_ << ": " << msg << std::endl;
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

                        int db_id = server_.authenticateUser(username, password);

                        if (db_id != -1) {
                            authenticated = true;
                            user_id_ = db_id;
                            username_ = username;

                            user_root_dir_ = fs::path("files") / username_;
                            send_dir_ = user_root_dir_ / "send";
                            recv_dir_ = user_root_dir_ / "recv";

                            fs::create_directories(send_dir_);
                            fs::create_directories(recv_dir_);

                            deliver("Login successful. Your User ID = "
                                + std::to_string(user_id_) + "\n");

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
                    server_.createGroup(gid, user_id_); 
                    deliver("Group " + std::to_string(gid) + " created.\n");
                    do_read();
                    return;

                }

                // group:join 10
                else if (msg.rfind("group:join", 0) == 0) {
                    int gid = std::stoi(msg.substr(11));
                    if (!server_.joinGroup(gid, user_id_)) {
                        deliver("Group does not exist.\n");
                    }
                    else {
                        deliver("Joined group " + std::to_string(gid) + "\n");
                    }
                    do_read();
                    return;

                }

                // group:leave 10
                else if (msg.rfind("group:leave", 0) == 0) {
                    try {
                        int gid = std::stoi(msg.substr(11));
                        server_.leaveGroup(gid, user_id_);
                        deliver("Left group " + std::to_string(gid) + "\n");
                        do_read();
                        return;
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
                    do_read();

                    return;
                }

                // group:10 Hello guys
                else if (msg.rfind("group:", 0) == 0) {

                    size_t colon = msg.find(":");
                    size_t space = msg.find(" ", colon + 1);

                    int gid = std::stoi(msg.substr(colon + 1, space - (colon + 1)));
                    std::string text = msg.substr(space + 1);

                    server_.sendToGroup(gid, user_id_, text);

                    server_.logGroupMessage(user_id_, gid, text);
                    do_read();
                    return;

                }
                else if (msg == "whoami\n" || msg == "whoami") {

                    std::string username = server_.getUsernameById(user_id_);

                    deliver("You are " + username +
                        " (ID: " + std::to_string(user_id_) + ")\n");

                    do_read();
                    return;
                }

                else if (msg == "logout\n" || msg == "logout") {

                    authenticated = false;

                    server_.removeUserFromAllGroups(user_id_);

                    deliver("Logged out successfully.\n"
                        "Please login again using: login:<username> <password>\n");

                    do_read();
                    return;
                }

                else if (msg.rfind("public:", 0) == 0) {
                    std::string text = msg.substr(7); // after "public:"
                    std::string out =
                        "Public | User " + std::to_string(user_id_) + ": " + text + "\n";

                    server_.broadcast(out, user_id_);
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
                        "Private message from " + std::to_string(user_id_) + ": " + text + "\n";

                    if (!server_.sendMessage(receiver_id, formatted)) {
                        deliver("User " + std::to_string(receiver_id) + " is OFFLINE\n");
                    }
                    else {
                        server_.logPrivateMessage(user_id_, receiver_id, text);
                    }

                    do_read();
                }
                else if (msg == "leave server\n" || msg == "leave server") {

                    deliver("Goodbye!\n");

                    // Remove user from server session list
                    server_.leave(user_id_);

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
                else if (msg.rfind("file:send", 0) == 0) {

                    std::istringstream iss(msg);
                    std::string cmd, filename;
                    int target_id;

                    iss >> cmd >> target_id >> filename;

                    if (filename.empty()) {
                        deliver("INVALID_FILE_COMMAND\n");
                        do_read();
                        return;
                    }

                    auto receiver = server_.getSession(target_id);
                    if (!receiver) {
                        deliver("TARGET_OFFLINE\n");
                        do_read();
                        return;
                    }

                    // 🔹 Build full send path
                    fs::path send_path = send_dir_ / filename;

                    if (!fs::exists(send_path)) {
                        deliver("FILE_NOT_FOUND\n");
                        do_read();
                        return;
                    }

                    // 🔹 Get file size
                    std::uintmax_t filesize = fs::file_size(send_path);

                    // 🔹 Store pending sender-side info
                    pending_send_file_ = filename;
                    pending_target_id_ = target_id;
                    expected_filesize_ = filesize;
                    bytes_sent_ = 0;

                    // 🔹 Ask receiver to open its data socket
                    receiver->prepare_file_receive(
                        filename,
                        filesize,
                        self
                    );

                    deliver(
                        "FILE_REQUEST_SENT " + filename +
                        " SIZE " + std::to_string(filesize) + "\n"
                    );

                    do_read();
                    return;
                

                }
             
             }

       });
}

 
