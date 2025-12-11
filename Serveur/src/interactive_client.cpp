#include "../include/Protocol.hpp"
#include <asio.hpp>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <atomic>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

class InteractiveClient {
public:
    InteractiveClient(const std::string& host, short tcpPort)
        : _socket(_ioContext),
          _udpSocket(_ioContext),
          _host(host),
          _tcpPort(tcpPort),
          _running(false) {}

    void run() {
        try {
            // Connexion TCP
            std::cout << "\n╔════════════════════════════════════════╗" << std::endl;
            std::cout << "║     R-Type Interactive Client          ║" << std::endl;
            std::cout << "╚════════════════════════════════════════╝\n" << std::endl;

            std::cout << "[TCP] Connexion au serveur " << _host << ":" << _tcpPort << "..." << std::endl;
            asio::ip::tcp::resolver resolver(_ioContext);
            auto endpoints = resolver.resolve(_host, std::to_string(_tcpPort));
            asio::connect(_socket, endpoints);

            auto localTcp = _socket.local_endpoint();
            auto remoteTcp = _socket.remote_endpoint();
            std::cout << "[TCP] Connecté !" << std::endl;
            std::cout << "      Local:  " << localTcp.address().to_string() << ":" << localTcp.port() << std::endl;
            std::cout << "      Remote: " << remoteTcp.address().to_string() << ":" << remoteTcp.port() << std::endl;

            // Demander le nom d'utilisateur
            std::string username;
            std::cout << "\nEntrez votre nom : ";
            std::getline(std::cin, username);
            if (username.empty()) username = "Player";

            // Envoyer CONNECT
            std::cout << "\n[TCP] Envoi de CONNECT..." << std::endl;
            TCPProtocol::Message connectMsg;
            connectMsg.type = TCPProtocol::CONNECT;
            connectMsg.params["username"] = username;
            connectMsg.params["version"] = "1.0";
            asio::write(_socket, asio::buffer(connectMsg.serialize()));

            // Recevoir CONNECT_OK
            char buffer[1024];
            size_t length = _socket.read_some(asio::buffer(buffer));
            std::string response(buffer, length);

            TCPProtocol::Message responseMsg = TCPProtocol::Message::parse(response);
            if (responseMsg.type != TCPProtocol::CONNECT_OK) {
                std::cerr << "[TCP] Erreur de connexion : " << responseMsg.type << std::endl;
                if (responseMsg.params.count("reason")) {
                    std::cerr << "      Raison : " << responseMsg.params["reason"] << std::endl;
                }
                return;
            }

            _playerId = std::stoi(responseMsg.params["id"]);
            std::stringstream ss;
            ss << std::hex << responseMsg.params["token"];
            ss >> _sessionToken;
            _udpPort = std::stoi(responseMsg.params["udp_port"]);

            std::cout << "[TCP] Authentification réussie !" << std::endl;
            std::cout << "      Player ID : " << (int)_playerId << std::endl;
            std::cout << "      Token     : 0x" << std::hex << _sessionToken << std::dec << std::endl;
            std::cout << "      UDP Port  : " << _udpPort << std::endl;

            // Configuration UDP
            std::cout << "\n[UDP] Configuration du socket..." << std::endl;
            _udpSocket.open(asio::ip::udp::v4());
            asio::ip::udp::resolver udpResolver(_ioContext);
            auto udpEndpoints = udpResolver.resolve(asio::ip::udp::v4(), _host, std::to_string(_udpPort));
            _udpEndpoint = *udpEndpoints.begin();

            auto localUdp = _udpSocket.local_endpoint();
            std::cout << "[UDP] Socket configuré !" << std::endl;
            std::cout << "      Local:  " << localUdp.address().to_string() << ":" << localUdp.port() << std::endl;
            std::cout << "      Remote: " << _udpEndpoint.address().to_string() << ":" << _udpEndpoint.port() << std::endl;

            // Envoyer un premier paquet UDP pour initialiser l'endpoint côté serveur
            sendPlayerInput(0, 0, 0);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            // Démarrer la boucle de jeu
            std::cout << "\n╔════════════════════════════════════════╗" << std::endl;
            std::cout << "║           CONTRÔLES                    ║" << std::endl;
            std::cout << "╠════════════════════════════════════════╣" << std::endl;
            std::cout << "║  Z : Haut                              ║" << std::endl;
            std::cout << "║  Q : Gauche                            ║" << std::endl;
            std::cout << "║  S : Bas                               ║" << std::endl;
            std::cout << "║  D : Droite                            ║" << std::endl;
            std::cout << "║  ESPACE : Tir                          ║" << std::endl;
            std::cout << "║  X : Spécial                           ║" << std::endl;
            std::cout << "║  ECHAP : Quitter                       ║" << std::endl;
            std::cout << "╚════════════════════════════════════════╝\n" << std::endl;

            gameLoop();

            // Déconnexion
            std::cout << "\n[TCP] Déconnexion..." << std::endl;
            TCPProtocol::Message disconnectMsg;
            disconnectMsg.type = TCPProtocol::DISCONNECT_MSG;
            asio::write(_socket, asio::buffer(disconnectMsg.serialize()));

            length = _socket.read_some(asio::buffer(buffer));
            std::cout << "[TCP] Déconnecté du serveur" << std::endl;

            _socket.close();
            _udpSocket.close();

        } catch (std::exception& e) {
            std::cerr << "Erreur : " << e.what() << std::endl;
        }

        // Restaurer le terminal
        restoreTerminal();
    }

private:
    void gameLoop() {
        _running = true;
        setupTerminal();

        std::cout << "Jeu démarré ! Utilisez ZQSD pour vous déplacer..." << std::endl;

        auto lastUpdate = std::chrono::steady_clock::now();
        const int updateRate = 30; // 60 Hz
        const auto updateInterval = std::chrono::milliseconds(1000 / updateRate);

        int8_t moveX = 0;
        int8_t moveY = 0;
        uint8_t buttons = 0;

        while (_running) {
            auto now = std::chrono::steady_clock::now();

            // Lire les touches
            char c = readKey();

            // Réinitialiser les inputs à chaque frame
            moveX = 0;
            moveY = 0;
            buttons = 0;

            if (c != 0) {
                switch (c) {
                    case 'z': case 'Z': moveY = -1; break;  // Haut
                    case 'q': case 'Q': moveX = -1; break;  // Gauche
                    case 's': case 'S': moveY = 1; break;   // Bas
                    case 'd': case 'D': moveX = 1; break;   // Droite
                    case ' ': buttons |= BTN_SHOOT; break;  // Tir
                    case 'x': case 'X': buttons |= BTN_SPECIAL; break; // Spécial
                    case 27: // ESC
                        std::cout << "\nESCHAP pressé - Arrêt du jeu..." << std::endl;
                        _running = false;
                        break;
                }
            }

            // Envoyer les inputs au serveur à 30 Hz
            if (now - lastUpdate >= updateInterval) {
                sendPlayerInput(moveX, moveY, buttons);
                lastUpdate = now;
            }

            // Petit sleep pour ne pas consommer 100% CPU
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        restoreTerminal();
    }

    void setupTerminal() {
        // Sauvegarder les paramètres actuels
        tcgetattr(STDIN_FILENO, &_oldTermios);

        // Nouveaux paramètres : mode non-canonique, pas d'écho
        struct termios newTermios = _oldTermios;
        newTermios.c_lflag &= ~(ICANON | ECHO);
        newTermios.c_cc[VMIN] = 0;  // Non-bloquant
        newTermios.c_cc[VTIME] = 0;

        tcsetattr(STDIN_FILENO, TCSANOW, &newTermios);

        // Mettre stdin en mode non-bloquant
        int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
        fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
    }

    void restoreTerminal() {
        tcsetattr(STDIN_FILENO, TCSANOW, &_oldTermios);

        // Restaurer le mode bloquant
        int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
        fcntl(STDIN_FILENO, F_SETFL, flags & ~O_NONBLOCK);
    }

    char readKey() {
        char c = 0;
        if (read(STDIN_FILENO, &c, 1) > 0) {
            return c;
        }
        return 0;
    }

    void sendPlayerInput(int8_t moveX, int8_t moveY, uint8_t buttons) {
        PacketHeader header;
        header.type = PLAYER_INPUT;
        header.payloadSize = sizeof(PlayerInputPayload);
        header.sessionToken = _sessionToken;

        PlayerInputPayload payload;
        payload.timestamp = static_cast<uint32_t>(
            std::chrono::system_clock::now().time_since_epoch().count() / 1000000);
        payload.playerId = _playerId;
        payload.buttons = buttons;
        payload.moveX = moveX;
        payload.moveY = moveY;

        char packet[sizeof(PacketHeader) + sizeof(PlayerInputPayload)];
        std::memcpy(packet, &header, sizeof(PacketHeader));
        std::memcpy(packet + sizeof(PacketHeader), &payload, sizeof(PlayerInputPayload));

        _udpSocket.send_to(asio::buffer(packet, sizeof(packet)), _udpEndpoint);

        // Afficher uniquement si il y a un mouvement ou un bouton
        if (moveX != 0 || moveY != 0 || buttons != 0) {
            std::string direction = "";
            if (moveY == -1) direction += "Z";
            if (moveX == -1) direction += "Q";
            if (moveY == 1) direction += "S";
            if (moveX == 1) direction += "D";
            if (direction.empty()) direction = "-";

            std::string buttonStr = "";
            if (buttons & BTN_SHOOT) buttonStr += "SHOOT ";
            if (buttons & BTN_SPECIAL) buttonStr += "SPECIAL ";
            if (buttonStr.empty()) buttonStr = "-";

            std::cout << "\r[UDP] → Direction: [" << direction << "] | Buttons: [" << buttonStr << "]     " << std::flush;
        }
    }

    asio::io_context _ioContext;
    asio::ip::tcp::socket _socket;
    asio::ip::udp::socket _udpSocket;
    asio::ip::udp::endpoint _udpEndpoint;
    std::string _host;
    short _tcpPort;
    short _udpPort;
    uint8_t _playerId;
    uint32_t _sessionToken;
    std::atomic<bool> _running;
    struct termios _oldTermios;
};

int main(int argc, char* argv[]) {
    try {
        if (argc != 3) {
            std::cerr << "Usage: " << argv[0] << " <host> <tcp_port>" << std::endl;
            std::cerr << "Example: " << argv[0] << " localhost 4242" << std::endl;
            return 1;
        }

        std::string host = argv[1];
        short tcpPort;
        try {
            int port = std::stoi(argv[2]);
            if (port <= 0 || port > 65535) {
                std::cerr << "Error: Port must be between 1 and 65535" << std::endl;
                return 1;
            }
            tcpPort = static_cast<short>(port);
        } catch (const std::exception& e) {
            std::cerr << "Error: Invalid port number: " << argv[2] << std::endl;
            return 1;
        }

        InteractiveClient client(host, tcpPort);
        client.run();

    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
