#include "Server.hpp"
#include <boost/asio.hpp>
#include <iostream>
#include "Color2.hpp"

int main() {
    try {

        boost::asio::io_context io;
        Server server(io, 12345);
        ServerColor::set(ServerColor::BRIGHT_BLUE);
        std::cout << "Chat server running on port 12345\n";
        ServerColor::reset();

        ServerColor::set(ServerColor::BRIGHT_MAGENTA);

        io.run();
        ServerColor::reset();

    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }
}
