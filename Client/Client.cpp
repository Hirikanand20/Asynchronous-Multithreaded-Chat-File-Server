#include <boost/asio.hpp>
#include <iostream>
#include <thread>
#include <mutex>
#include "Color1.hpp"

using boost::asio::ip::tcp;

// ---------- Global mutex ----------
std::mutex cout_mutex;

// ---------- Prompt printer ----------
void print_prompt() {
    Color::set(Color::BRIGHT_CYAN);
    std::cout << "Enter command (or 'exit'): ";
    Color::reset();
    std::cout << std::flush;
}

// ---------- Reader thread ----------
void read_loop(tcp::socket& socket) {
    try {
        char data[1024];

        while (true) {
            size_t len = socket.read_some(boost::asio::buffer(data));

            std::lock_guard<std::mutex> lock(cout_mutex);

            // Clear current input line
            std::cout << "\r";
            std::cout << "                                    \r";

            // Print server message
            Color::set(Color::BRIGHT_MAGENTA);
            std::cout << "Server: " << std::string(data, len) << "\n";
            Color::reset();

            // Reprint prompt
            print_prompt();
        }
    }
    catch (...) {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << "\nDisconnected from server\n";
    }
}

// ---------- Main ----------
int main() {
    try {
        boost::asio::io_context io;
        tcp::resolver resolver(io);
        auto endpoints = resolver.resolve("127.0.0.1", "12345");

        tcp::socket socket(io);
        boost::asio::connect(socket, endpoints);

        {
            std::lock_guard<std::mutex> lock(cout_mutex);
            Color::set(Color::BRIGHT_GREEN);
            std::cout << "Connected to server.\n";
            Color::reset();
        }

        // Start reader thread
        std::thread reader(read_loop, std::ref(socket));

        // Writer loop
        while (true) {
            std::string input;

            {
                std::lock_guard<std::mutex> lock(cout_mutex);
                print_prompt();
            }

            std::getline(std::cin, input);

            if (input == "exit")
                break;

            input += "\n";
            boost::asio::write(socket, boost::asio::buffer(input));
        }

        socket.close();
        reader.join();
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
