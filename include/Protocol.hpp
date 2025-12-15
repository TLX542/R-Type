#pragma once

#include <cstdint>
#include <string>
#include <cstring>
#include <map>

// Magic number pour identifier les paquets R-Type
constexpr uint16_t PROTOCOL_MAGIC = 0xABCD;
constexpr uint8_t PROTOCOL_VERSION = 0x01;

// Types de messages UDP
enum MessageType : uint8_t {
    // Connection (TCP)
    CONNECT_REQUEST = 0x01,
    CONNECT_RESPONSE = 0x02,
    DISCONNECT = 0x03,

    // Input (UDP - Client → Server)
    PLAYER_INPUT = 0x10,

    // Entities (UDP - Server → Client)
    ENTITY_SPAWN = 0x20,
    ENTITY_UPDATE = 0x21,
    ENTITY_DESTROY = 0x22,
    ENTITY_BATCH_UPDATE = 0x23,

    // Game Events (UDP)
    PLAYER_SHOOT = 0x30,
    COLLISION = 0x31,
    SCORE_UPDATE = 0x32,

    // Reliability
    PING = 0xF1,
    PONG = 0xF2
};

// Types d'entités
enum EntityType : uint8_t {
    PLAYER = 0,
    ENEMY = 1,
    BULLET_PLAYER = 2,
    BULLET_ENEMY = 3,
    POWERUP = 4,
    OBSTACLE = 5
};

// Force l'alignement sur 1 byte (pas de padding)
#pragma pack(push, 1)

// Header UDP (9 bytes)
struct PacketHeader {
    uint16_t magic;         // 0xABCD
    uint8_t version;        // 0x01
    uint8_t payloadSize;    // Taille du payload (0-255)
    uint8_t type;           // Type de message (MessageType)
    uint32_t sessionToken;  // Token d'authentification

    PacketHeader() : magic(PROTOCOL_MAGIC), version(PROTOCOL_VERSION),
                     payloadSize(0), type(0), sessionToken(0) {}
};

// PLAYER_INPUT Payload (8 bytes)
struct PlayerInputPayload {
    uint32_t timestamp;     // Timestamp client (ms)
    uint8_t playerId;       // ID du joueur (1-4)
    uint8_t buttons;        // Bitfield des boutons
    int8_t moveX;           // Direction horizontale (-1, 0, +1)
    int8_t moveY;           // Direction verticale (-1, 0, +1)
};

// Button flags
constexpr uint8_t BTN_SHOOT = 0x01;      // Bit 0
constexpr uint8_t BTN_SPECIAL = 0x02;    // Bit 1
constexpr uint8_t BTN_UP = 0x04;         // Bit 2
constexpr uint8_t BTN_DOWN = 0x08;       // Bit 3
constexpr uint8_t BTN_LEFT = 0x10;       // Bit 4
constexpr uint8_t BTN_RIGHT = 0x20;      // Bit 5

// ENTITY_SPAWN Payload (41 bytes)
struct EntitySpawnPayload {
    uint32_t networkId;     // ID réseau unique
    uint8_t entityType;     // Type d'entité
    uint8_t ownerPlayer;    // Propriétaire (0 = serveur)
    float posX;             // Position X
    float posY;             // Position Y
    float velocityX;        // Vitesse X
    float velocityY;        // Vitesse Y
    uint8_t health;         // Points de vie
    char username[16];      // Nom du joueur (seulement pour les entités PLAYER)
};

// ENTITY_UPDATE Payload (21 bytes)
struct EntityUpdatePayload {
    uint32_t networkId;     // ID réseau
    float posX;             // Position X
    float posY;             // Position Y
    float velocityX;        // Vitesse X
    float velocityY;        // Vitesse Y
    uint8_t health;         // Points de vie
};

// ENTITY_DESTROY Payload (4 bytes)
struct EntityDestroyPayload {
    uint32_t networkId;     // ID réseau de l'entité à détruire
};

// Entry pour ENTITY_BATCH_UPDATE (13 bytes par entité)
struct EntityBatchEntry {
    uint32_t networkId;     // ID réseau
    float posX;             // Position X
    float posY;             // Position Y
    uint8_t health;         // Points de vie
};

// ENTITY_BATCH_UPDATE Payload (variable, max 10 entités)
constexpr uint8_t MAX_BATCH_ENTITIES = 10;
struct EntityBatchUpdatePayload {
    uint8_t count;          // Nombre d'entités (1-10)
    EntityBatchEntry entities[MAX_BATCH_ENTITIES];
};

#pragma pack(pop)

// Utilitaires pour le protocole TCP (texte)
namespace TCPProtocol {
    // Parse une ligne TCP au format "TYPE param1=value1 param2=value2"
    struct Message {
        std::string type;
        std::map<std::string, std::string> params;

        static Message parse(const std::string& line);
        std::string serialize() const;
    };

    // Types de messages TCP
    constexpr const char* CONNECT = "CONNECT";
    constexpr const char* CONNECT_OK = "CONNECT_OK";
    constexpr const char* CONNECT_ERROR = "CONNECT_ERROR";
    constexpr const char* DISCONNECT_MSG = "DISCONNECT";
    constexpr const char* DISCONNECT_OK = "DISCONNECT_OK";
    constexpr const char* GAME_START = "GAME_START";
    constexpr const char* GAME_OVER = "GAME_OVER";
    constexpr const char* PLAYER_JOIN = "PLAYER_JOIN";
    constexpr const char* PLAYER_LEAVE = "PLAYER_LEAVE";
}

// Validation de paquet
inline bool validatePacket(const PacketHeader& header, size_t receivedSize) {
    // Vérifier la taille minimale
    if (receivedSize < sizeof(PacketHeader)) {
        return false;
    }

    // Vérifier le magic number
    if (header.magic != PROTOCOL_MAGIC) {
        return false;
    }

    // Vérifier la version
    if (header.version != PROTOCOL_VERSION) {
        return false;
    }

    // Vérifier que la taille totale correspond
    if (receivedSize != sizeof(PacketHeader) + header.payloadSize) {
        return false;
    }

    return true;
}
