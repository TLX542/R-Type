#include <asio.hpp>
#include <iostream>
#include <string>
#include <thread>

class Client {
public:
    Client(asio::io_context& io_context, const std::string& host, const std::string& port)
        : _socket(io_context) {
        asio::ip::tcp::resolver resolver(io_context);
        auto endpoints = resolver.resolve(host, port);
        asio::connect(_socket, endpoints);
        std::cout << "Connected to server!" << std::endl;
    }

    void send(const std::string& message) {
        asio::write(_socket, asio::buffer(message));
    }

    std::string receive() {
        char reply[1024];
        size_t reply_length = _socket.read_some(asio::buffer(reply));
        return std::string(reply, reply_length);
    }

private:
    asio::ip::tcp::socket _socket;
};

int main(int argc, char* argv[]) {
    try {
        if (argc != 3) {
            std::cerr << "Usage: " << argv[0] << " <host> <port>" << std::endl;
            return 1;
        }

        asio::io_context io_context;
        Client client(io_context, argv[1], argv[2]);

        std::cout << "Type messages to send to the server (Ctrl+C to quit):" << std::endl;

        std::string message;
        while (std::getline(std::cin, message)) {
            if (message.empty()) continue;

            client.send(message);
            std::cout << "Sent: " << message << std::endl;

            std::string response = client.receive();
            std::cout << "Received: " << response << std::endl;
        }

    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
