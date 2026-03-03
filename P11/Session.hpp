#ifndef SESSION_HPP
#define SESSION_HPP

#include <boost/asio.hpp>
#include <memory>
#include <string>
#include <deque>
#include <fstream>
#include <array>
#include <filesystem>

namespace fs = std::filesystem;
// Forward declaration
class Server;

using boost::asio::ip::tcp;

class Session : public std::enable_shared_from_this<Session> {
public:
    Session(tcp::socket socket, Server& server, int id);

    void start();
    void deliver(const std::string& msg);
    int getUserId() const { return user_id_; }
    int getSessionId() const { return session_id_; };
    // ===== File transfer state =====
    std::string current_filename_;
    // File transfer progress
    std::size_t bytes_sent_ = 0;
    std::size_t bytes_received_ = 0;
    std::size_t expected_filesize_ = 0;

    // ===== Pending file send =====
    std::string pending_send_file_;
    int pending_target_id_ = -1;

private:
    // ===== Internal helpers =====


    fs::path user_root_dir_;
    fs::path send_dir_;
    fs::path recv_dir_;

    void do_read();
    void do_write();

    void handle_data_port(int port, const std::string& filename, std::uintmax_t filesize, const boost::asio::ip::address& addr);

    void prepare_file_receive(const std::string& filename,
        std::uintmax_t filesize,
        std::shared_ptr<Session> sender);

    void connect_and_send_file(const std::string& filename,
        const tcp::endpoint& ep);

    void do_file_send();
    void do_file_receive(std::shared_ptr<Session> sender);

    /* ===== Member variables (ORDER IS CRITICAL) ===== */

    // Executor (must be first)
    boost::asio::any_io_executor executor_;

    // Control connection
    tcp::socket socket_;

    // Data connections
    tcp::socket   data_socket_;
    tcp::acceptor data_acceptor_;
    tcp::socket   send_data_socket_;
    tcp::socket   recv_data_socket_;

    // Server/session state
    Server& server_;
    int session_id_;     
    int user_id_;
    std::string username_;
    bool authenticated;

    // File I/O
    std::ifstream infile_;
    std::ofstream outfile_;
    std::array<char, 8192> file_buf_{};

    // Message queue (chat)
    std::deque<std::string> write_msgs_;

    // Read buffer
    static constexpr std::size_t max_length = 1024;
    char data_[max_length]{};
};

#endif  // SESSION_HPP