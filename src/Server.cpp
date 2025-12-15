#include "../include/Server.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>

// Session implementation
Session::Session(asio::ip::tcp::socket socket, int id, Server* server)
    : _socket(std::move(socket)), _clientId(id), _server(server) {}

void Session::start() {
    std::cout << "[TCP] Client #" << _clientId << " connected from "
              << _socket.remote_endpoint().address().to_string() << std::endl;
    doRead();
}

void Session::send(const std::string& message) {
    bool writeInProgress = !_writeQueue.empty();
    _writeQueue.push_back(message);
    if (!writeInProgress) {
        doWrite();
    }
}

void Session::handleMessage(const std::string& message) {
    // Parser le message TCP
    TCPProtocol::Message msg = TCPProtocol::Message::parse(message);

    std::cout << "[TCP] Client #" << _clientId << " → " << msg.type << std::endl;

    if (msg.type == TCPProtocol::CONNECT) {
        // Vérifier si le serveur est plein (max 4 joueurs)
        constexpr size_t MAX_PLAYERS = 4;
        if (_server->getClientCount() > MAX_PLAYERS) {
            TCPProtocol::Message response;
            response.type = TCPProtocol::CONNECT_ERROR;
            response.params["reason"] = "server_full";
            send(response.serialize());
            return;
        }

        // Extraire les paramètres
        std::string username = msg.params["username"];
        std::string version = msg.params["version"];

        // Valider le username
        if (username.empty() || username.length() > 16) {
            TCPProtocol::Message response;
            response.type = TCPProtocol::CONNECT_ERROR;
            response.params["reason"] = "invalid_username";
            send(response.serialize());
            return;
        }

        // Générer un token de session et assigner un player ID
        _clientInfo.playerId = static_cast<uint8_t>(_clientId);
        _clientInfo.sessionToken = _server->generateSessionToken();
        _clientInfo.username = username;
        _clientInfo.udpInitialized = false;

        // Envoyer la réponse CONNECT_OK
        TCPProtocol::Message response;
        response.type = TCPProtocol::CONNECT_OK;
        response.params["id"] = std::to_string(_clientInfo.playerId);

        // Convertir le token en hexadécimal
        std::stringstream ss;
        ss << std::hex << _clientInfo.sessionToken;
        response.params["token"] = ss.str();

        response.params["udp_port"] = std::to_string(_server->getUdpPort());

        send(response.serialize());

        std::cout << "[TCP] Client #" << _clientId << " (" << username << ") authenticated"
                  << " | Player ID: " << (int)_clientInfo.playerId
                  << " | Token: 0x" << ss.str() << std::endl;

        // Notify game logic about new player
        _server->onPlayerConnected(_clientInfo.playerId);

        // Notifier les autres joueurs
        TCPProtocol::Message joinMsg;
        joinMsg.type = TCPProtocol::PLAYER_JOIN;
        joinMsg.params["id"] = std::to_string(_clientInfo.playerId);
        joinMsg.params["username"] = username;
        _server->broadcastMessage(joinMsg.serialize(), _clientId);

    } else if (msg.type == TCPProtocol::DISCONNECT_MSG) {
        // Gérer la déconnexion propre
        TCPProtocol::Message response;
        response.type = TCPProtocol::DISCONNECT_OK;
        send(response.serialize());

        std::cout << "[TCP] Client #" << _clientId << " requested disconnect" << std::endl;

        // Notify game logic about player disconnection
        if (_clientInfo.playerId != 0) {
            _server->onPlayerDisconnected(_clientInfo.playerId);
        }

        // Notifier les autres joueurs
        if (_clientInfo.playerId != 0) {
            TCPProtocol::Message leaveMsg;
            leaveMsg.type = TCPProtocol::PLAYER_LEAVE;
            leaveMsg.params["id"] = std::to_string(_clientInfo.playerId);
            _server->broadcastMessage(leaveMsg.serialize(), _clientId);
        }

        // Remove this session from the server's session list
        _server->removeSession(_clientId);

        // Fermer la socket
        _socket.close();
    }
}

void Session::doRead() {
    auto self(shared_from_this());
    _socket.async_read_some(
        asio::buffer(_data, max_length),
        [this, self](std::error_code ec, std::size_t length) {
            if (!ec) {
                std::string receivedData(_data, length);

                // Gérer le message (peut contenir plusieurs lignes)
                size_t pos = 0;
                while ((pos = receivedData.find('\n')) != std::string::npos) {
                    std::string line = receivedData.substr(0, pos);
                    if (!line.empty()) {
                        handleMessage(line);
                    }
                    receivedData.erase(0, pos + 1);
                }

                doRead();
            } else {
                std::cout << "[TCP] Client #" << _clientId << " disconnected" << std::endl;

                // Notify game logic about player disconnection
                if (_clientInfo.playerId != 0) {
                    _server->onPlayerDisconnected(_clientInfo.playerId);
                }

                // Notifier les autres joueurs du départ
                if (_clientInfo.playerId != 0) {
                    TCPProtocol::Message leaveMsg;
                    leaveMsg.type = TCPProtocol::PLAYER_LEAVE;
                    leaveMsg.params["id"] = std::to_string(_clientInfo.playerId);
                    _server->broadcastMessage(leaveMsg.serialize(), _clientId);
                }

                // Remove this session from the server's session list
                _server->removeSession(_clientId);
            }
        });
}

void Session::doWrite() {
    auto self(shared_from_this());
    asio::async_write(
        _socket,
        asio::buffer(_writeQueue.front()),
        [this, self](std::error_code ec, std::size_t /*length*/) {
            if (!ec) {
                _writeQueue.erase(_writeQueue.begin());
                if (!_writeQueue.empty()) {
                    doWrite();
                }
            } else {
                std::cout << "Error writing to client #" << _clientId << std::endl;
            }
        });
}

// Server implementation
Server::Server(asio::io_context& io_context, short tcpPort, short udpPort)
    : _ioContext(io_context),
      _acceptor(io_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), tcpPort)),
      _udpSocket(io_context, asio::ip::udp::endpoint(asio::ip::udp::v4(), udpPort)),
      _udpPort(udpPort),
      _nextClientId(1) {

    // Initialiser le générateur aléatoire
    std::random_device rd;
    _randomGenerator.seed(rd());

    std::cout << "R-Type Server started" << std::endl;
    std::cout << "TCP port: " << tcpPort << std::endl;
    std::cout << "UDP port: " << udpPort << std::endl;

    doAccept();
    doReceiveUDP();
}

void Server::run() {
    _ioContext.run();
}

uint32_t Server::generateSessionToken() {
    std::uniform_int_distribution<uint32_t> dist(1, 0xFFFFFFFF);
    return dist(_randomGenerator);
}

void Server::doAccept() {
    _acceptor.async_accept(
        [this](std::error_code ec, asio::ip::tcp::socket socket) {
            if (!ec) {
                auto session = std::make_shared<Session>(std::move(socket), _nextClientId++, this);
                _sessions.push_back(session);
                session->start();
            } else {
                std::cerr << "Accept error: " << ec.message() << std::endl;
            }

            doAccept();
        });
}

void Server::doReceiveUDP() {
    _udpSocket.async_receive_from(
        asio::buffer(_udpBuffer, udp_buffer_size),
        _udpSenderEndpoint,
        [this](std::error_code ec, std::size_t length) {
            if (!ec && length > 0) {
                // std::cout << "[UDP] Received packet from "
                //           << _udpSenderEndpoint.address().to_string() << ":"
                //           << _udpSenderEndpoint.port()
                //           << " (" << length << " bytes)" << std::endl;
                handleUDPPacket(_udpBuffer, length, _udpSenderEndpoint);
            } else if (ec) {
                std::cerr << "[UDP] Receive error: " << ec.message() << std::endl;
            }

            // Continue listening
            doReceiveUDP();
        });
}

void Server::handleUDPPacket(const char* data, size_t length,
                              const asio::ip::udp::endpoint& senderEndpoint) {
    // Vérifier la taille minimale
    if (length < sizeof(PacketHeader)) {
        std::cerr << "[UDP] Packet too small: " << length << " bytes from "
                  << senderEndpoint.address().to_string() << ":" << senderEndpoint.port() << std::endl;
        return;
    }

    // Extraire le header
    PacketHeader header;
    std::memcpy(&header, data, sizeof(PacketHeader));

    // Valider le paquet
    if (!validatePacket(header, length)) {
        std::cerr << "[UDP] Invalid packet from "
                  << senderEndpoint.address().to_string() << ":" << senderEndpoint.port() << std::endl;
        return;
    }

    // Trouver le client avec ce token
    std::shared_ptr<Session> clientSession = nullptr;
    for (auto& session : _sessions) {
        if (session->getClientInfo().sessionToken == header.sessionToken) {
            clientSession = session;
            break;
        }
    }

    if (!clientSession) {
        std::cerr << "[UDP] Packet with invalid token: 0x" << std::hex
                  << header.sessionToken << std::dec << " from "
                  << senderEndpoint.address().to_string() << ":" << senderEndpoint.port() << std::endl;
        return;
    }

    // Si c'est le premier paquet UDP du client, stocker son endpoint
    if (!clientSession->getClientInfo().udpInitialized) {
        clientSession->getClientInfo().udpEndpoint = senderEndpoint;
        clientSession->getClientInfo().udpInitialized = true;
        std::cout << "[UDP] Client #" << clientSession->getId()
                  << " endpoint initialized: "
                  << senderEndpoint.address().to_string() << ":"
                  << senderEndpoint.port() << std::endl;
        
        // Notify game logic that UDP is ready
        onPlayerUdpReady(clientSession->getClientInfo().playerId);
    }

    // Traiter le paquet selon son type
    switch (header.type) {
        case PLAYER_INPUT: {
            if (header.payloadSize == sizeof(PlayerInputPayload)) {
                PlayerInputPayload payload;
                std::memcpy(&payload, data + sizeof(PacketHeader), sizeof(PlayerInputPayload));

                // SECURITY: Use the playerId from the authenticated session, NOT from the payload
                // This prevents clients from controlling other players by sending fake playerIds
                uint8_t authenticatedPlayerId = clientSession->getClientInfo().playerId;

                // Convertir moveX/moveY en directions ZQSD
                std::string direction = "";
                if (payload.moveY == -1) direction += "Z";
                if (payload.moveX == -1) direction += "Q";
                if (payload.moveY == 1) direction += "S";
                if (payload.moveX == 1) direction += "D";
                if (direction.empty()) direction = "-";

                // Afficher les boutons pressés
                std::string buttons = "";
                if (payload.buttons & BTN_SHOOT) buttons += "SHOOT ";
                if (payload.buttons & BTN_SPECIAL) buttons += "SPECIAL ";
                if (buttons.empty()) buttons = "-";

                // N'afficher que s'il y a un input réel (pas de mouvement vide)
                if (direction != "-" || buttons != "-") {
                    std::cout << "[UDP] Player " << (int)authenticatedPlayerId
                              << " → Direction: [" << direction << "]"
                              << " | Buttons: [" << buttons << "]"
                              << std::endl;
                }

                // Apply input to game logic using authenticated player ID
                handlePlayerInput(authenticatedPlayerId, payload.moveX, payload.moveY, payload.buttons);
            }
            break;
        }

        case PING: {
            // Répondre avec un PONG
            PacketHeader pongHeader;
            pongHeader.type = PONG;
            pongHeader.payloadSize = 0;
            pongHeader.sessionToken = header.sessionToken;

            _udpSocket.send_to(asio::buffer(&pongHeader, sizeof(PacketHeader)), senderEndpoint);
            std::cout << "[UDP] PING → PONG" << std::endl;
            break;
        }

        default:
            std::cerr << "[UDP] Unhandled message type: 0x" << std::hex
                      << (int)header.type << std::dec << std::endl;
            break;
    }
}

void Server::broadcastMessage(const std::string& message, int excludeClientId) {
    for (auto& session : _sessions) {
        if (session->getId() != excludeClientId) {
            session->send(message);
        }
    }
}

void Server::removeSession(int clientId) {
    auto it = std::remove_if(_sessions.begin(), _sessions.end(),
        [clientId](const std::shared_ptr<Session>& session) {
            return session->getId() == clientId;
        });
    
    if (it != _sessions.end()) {
        _sessions.erase(it, _sessions.end());
        std::cout << "[Server] Removed session for client #" << clientId 
                  << " (active sessions: " << _sessions.size() << ")" << std::endl;
    } else {
        std::cout << "[Server] Warning: Attempted to remove non-existent session for client #" 
                  << clientId << std::endl;
    }
}
