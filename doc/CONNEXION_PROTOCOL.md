# Protocole de Connexion R-Type

## Vue d'ensemble

Le protocole R-Type utilise une **architecture hybride TCP + UDP** :
- **TCP** : Pour la connexion initiale et les messages critiques (connexion, déconnexion)
- **UDP** : Pour le gameplay en temps réel (inputs, positions, collisions)

### Pourquoi 2 ports différents ?

**Oui, il faut 1 port TCP et 1 port UDP** (par exemple TCP: 4242, UDP: 4243).

Bien que TCP et UDP puissent techniquement utiliser le même numéro de port (ce sont des protocoles différents), on utilise 2 ports différents pour :
- **Clarté** : Éviter toute confusion
- **Architecture** : Le serveur a deux sockets séparés (TCP acceptor + UDP socket)
- **Convention** : C'est une pratique standard (ex: DNS utilise port 53 pour TCP et UDP)

## Flow de Connexion Complet

```
CLIENT                              SERVEUR
  |                                    |
  |  1. TCP Connect (port 4242)        |
  |--------------------------------->  |
  |                                    |
  |  2. CONNECT username=Player1       |
  |     version=1.0                    |
  |--------------------------------->  |
  |                                    |
  |  3. Génération token + Player ID   |
  |     <------------------------      |
  |                                    |
  |  4. CONNECT_OK id=1                |
  |     token=abc123 udp_port=4243     |
  |  <-------------------------------- |
  |                                    |
  |  5. Configuration socket UDP       |
  |     (port local aléatoire)         |
  |                                    |
  |  6. PLAYER_INPUT (UDP port 4243)   |
  |     + token dans header            |
  |--------------------------------->  |
  |                                    |
  |  7. Validation token               |
  |     Stockage endpoint UDP          |
  |     <------------------------      |
  |                                    |
  |  8. ENTITY_UPDATE (UDP)            |
  |  <-------------------------------- |
  |                                    |
  |  ... Gameplay continue ...         |
  |                                    |
  |  9. DISCONNECT (TCP)               |
  |--------------------------------->  |
  |                                    |
  |  10. DISCONNECT_OK                 |
  |  <-------------------------------- |
  |                                    |
  |  11. Fermeture connexions          |
  |                                    |
```

## Détails par Étape

### Étape 1-2 : Connexion TCP

**Client → Serveur (TCP)**
```
CONNECT username=Player1 version=1.0\n
```

**Format** : `TYPE param1=value1 param2=value2\n`
- Type : `CONNECT`
- Paramètres :
  - `username` : Nom du joueur (max 16 caractères)
  - `version` : Version du client (ex: "1.0")

**Validation serveur** :
- Vérifier que le serveur n'est pas plein (max 4 joueurs)
- Valider le nom d'utilisateur (non vide, ≤ 16 caractères)
- Vérifier la compatibilité de version (optionnel)

### Étape 3-4 : Authentification

**Serveur → Client (TCP)**
```
CONNECT_OK id=1 token=c5b320db udp_port=4243\n
```

**Contenu** :
- `id` : Player ID unique (1-4)
- `token` : Token de session en hexadécimal (32 bits)
- `udp_port` : Port UDP à utiliser pour le gameplay

**Génération du token** :
```cpp
std::mt19937 randomGenerator;
std::uniform_int_distribution<uint32_t> dist(1, 0xFFFFFFFF);
uint32_t token = dist(randomGenerator);
```

Le token est **crucial** pour la sécurité UDP car il empêche les clients non authentifiés d'envoyer des paquets.

### Étape 5 : Configuration UDP Client

Le client configure son socket UDP :
```cpp
asio::ip::udp::socket udpSocket(io_context);
udpSocket.open(asio::ip::udp::v4());
// Le port local est assigné automatiquement par l'OS
```

**Pas besoin de bind** : Le système d'exploitation assigne automatiquement un port UDP source aléatoire.

### Étape 6 : Premier Paquet UDP

**Structure du paquet UDP** :
```
[Header 9 bytes] [Payload variable]
```

**Header UDP** :
```cpp
struct PacketHeader {
    uint16_t magic;         // 0xABCD (identifie un paquet R-Type)
    uint8_t version;        // 0x01 (version du protocole)
    uint8_t payloadSize;    // Taille du payload (0-255 bytes)
    uint8_t type;           // Type de message (ex: PLAYER_INPUT = 0x10)
    uint32_t sessionToken;  // Token reçu lors du CONNECT_OK
};
```

**Exemple : PLAYER_INPUT**
```cpp
// Header (9 bytes)
PacketHeader header;
header.magic = 0xABCD;
header.version = 0x01;
header.type = 0x10;  // PLAYER_INPUT
header.payloadSize = 8;
header.sessionToken = 0xc5b320db;  // Token reçu

// Payload (8 bytes)
struct PlayerInputPayload {
    uint32_t timestamp;  // Horodatage (ms)
    uint8_t playerId;    // ID du joueur
    uint8_t buttons;     // Bitfield des boutons
    int8_t moveX;        // Direction X (-1, 0, +1)
    int8_t moveY;        // Direction Y (-1, 0, +1)
};
```

### Étape 7 : Validation et Découverte Endpoint

**Côté serveur** :
1. Recevoir le paquet UDP
2. Extraire le header
3. Valider :
   - Magic number = 0xABCD ✓
   - Version = 0x01 ✓
   - Taille cohérente ✓
4. **Chercher le client avec ce token**
5. Si c'est le premier paquet UDP de ce client :
   - Stocker son `endpoint` (adresse IP + port)
   - Marquer `udpInitialized = true`

**Découverte de l'endpoint** :
Le serveur ne connaît pas à l'avance le port UDP du client (il est aléatoire). Le premier paquet UDP permet de découvrir l'adresse complète du client.

```cpp
if (!clientInfo.udpInitialized) {
    clientInfo.udpEndpoint = senderEndpoint;
    clientInfo.udpInitialized = true;
    // Maintenant le serveur peut répondre au client
}
```

### Étape 8 : Communication Bidirectionnelle UDP

Une fois l'endpoint UDP connu, le serveur peut envoyer des données au client :

**Serveur → Client (UDP)**
```cpp
// Exemple : Mise à jour de position
EntityUpdatePayload update;
update.networkId = 42;
update.posX = 150.0f;
update.posY = 200.0f;
// ...

udpSocket.send_to(buffer, clientInfo.udpEndpoint);
```

### Étape 9-10 : Déconnexion

**Client → Serveur (TCP)**
```
DISCONNECT\n
```

**Serveur → Client (TCP)**
```
DISCONNECT_OK\n
```

**Nettoyage serveur** :
1. Envoyer `PLAYER_LEAVE` aux autres joueurs
2. Supprimer le client de la liste
3. Détruire l'entité du joueur dans l'ECS
4. Fermer la socket TCP

## Sécurité UDP avec Token

### Pourquoi le token est important ?

UDP est un protocole **sans connexion**. N'importe qui peut envoyer un paquet UDP au serveur. Sans token :
- Un attaquant pourrait envoyer de faux inputs
- Le serveur ne pourrait pas différencier un vrai client d'un attaquant

### Comment ça fonctionne ?

1. **Connexion TCP** : Le client prouve son identité (connexion établie)
2. **Token unique** : Le serveur génère un token aléatoire de 32 bits
3. **Chaque paquet UDP** : Le client doit inclure ce token dans le header
4. **Validation** : Le serveur vérifie le token avant d'accepter le paquet

**Probabilité de deviner un token** : 1/4,294,967,296 (1 chance sur 4 milliards)

## Gestion des Erreurs

### Erreurs TCP

**CONNECT_ERROR** :
```
CONNECT_ERROR reason=server_full\n
```

Raisons possibles :
- `server_full` : Serveur plein (4 joueurs max)
- `invalid_username` : Nom invalide
- `version_mismatch` : Version incompatible

### Erreurs UDP

Les paquets UDP invalides sont **silencieusement ignorés** :
- Magic number incorrect → Ignoré
- Token invalide → Ignoré + log d'avertissement
- Taille incorrecte → Ignoré

**Pourquoi ne pas répondre ?**
- Éviter les attaques DoS (Denial of Service)
- UDP ne garantit pas la livraison de toute façon

## Exemples de Code

### Client : Connexion Complète

```cpp
// 1. Connexion TCP
asio::ip::tcp::socket tcpSocket(io_context);
asio::connect(tcpSocket, resolver.resolve("localhost", "4242"));

// 2. Envoyer CONNECT
std::string connectMsg = "CONNECT username=Player1 version=1.0\n";
asio::write(tcpSocket, asio::buffer(connectMsg));

// 3. Recevoir CONNECT_OK
char buffer[1024];
size_t length = tcpSocket.read_some(asio::buffer(buffer));
std::string response(buffer, length);

// 4. Parser la réponse
TCPProtocol::Message msg = TCPProtocol::Message::parse(response);
uint8_t playerId = std::stoi(msg.params["id"]);
uint32_t token = std::stoul(msg.params["token"], nullptr, 16);
short udpPort = std::stoi(msg.params["udp_port"]);

// 5. Configurer UDP
asio::ip::udp::socket udpSocket(io_context);
udpSocket.open(asio::ip::udp::v4());
asio::ip::udp::endpoint serverUdpEndpoint(
    asio::ip::address::from_string("127.0.0.1"), udpPort);

// 6. Envoyer un input UDP
PacketHeader header;
header.magic = 0xABCD;
header.version = 0x01;
header.type = PLAYER_INPUT;
header.payloadSize = sizeof(PlayerInputPayload);
header.sessionToken = token;

PlayerInputPayload payload;
payload.playerId = playerId;
payload.buttons = BTN_SHOOT;
payload.moveX = 1;
payload.moveY = 0;

char packet[sizeof(PacketHeader) + sizeof(PlayerInputPayload)];
memcpy(packet, &header, sizeof(PacketHeader));
memcpy(packet + sizeof(PacketHeader), &payload, sizeof(PlayerInputPayload));

udpSocket.send_to(asio::buffer(packet, sizeof(packet)), serverUdpEndpoint);
```

### Serveur : Validation d'un Paquet UDP

```cpp
void Server::handleUDPPacket(const char* data, size_t length,
                              const asio::ip::udp::endpoint& senderEndpoint) {
    // 1. Vérifier taille minimale
    if (length < sizeof(PacketHeader)) {
        return; // Ignorer
    }

    // 2. Extraire header
    PacketHeader header;
    memcpy(&header, data, sizeof(PacketHeader));

    // 3. Valider
    if (!validatePacket(header, length)) {
        return; // Ignorer
    }

    // 4. Trouver le client avec ce token
    std::shared_ptr<Session> clientSession = nullptr;
    for (auto& session : _sessions) {
        if (session->getClientInfo().sessionToken == header.sessionToken) {
            clientSession = session;
            break;
        }
    }

    if (!clientSession) {
        std::cerr << "Invalid token: 0x" << std::hex
                  << header.sessionToken << std::dec << std::endl;
        return;
    }

    // 5. Premier paquet UDP ? Stocker l'endpoint
    if (!clientSession->getClientInfo().udpInitialized) {
        clientSession->getClientInfo().udpEndpoint = senderEndpoint;
        clientSession->getClientInfo().udpInitialized = true;
    }

    // 6. Traiter le paquet selon son type
    switch (header.type) {
        case PLAYER_INPUT:
            handlePlayerInput(data + sizeof(PacketHeader));
            break;
        // ... autres types
    }
}
```

## Résumé : Check-list d'Implémentation

### Côté Client
- [ ] Se connecter en TCP au port 4242
- [ ] Envoyer CONNECT avec username et version
- [ ] Recevoir CONNECT_OK et extraire id, token, udp_port
- [ ] Créer un socket UDP
- [ ] Envoyer tous les paquets UDP avec le token dans le header
- [ ] Envoyer DISCONNECT avant de quitter

### Côté Serveur
- [ ] Accepter les connexions TCP sur port 4242
- [ ] Parser les messages CONNECT
- [ ] Générer un token aléatoire unique
- [ ] Répondre avec CONNECT_OK contenant id, token, udp_port
- [ ] Écouter sur le port UDP 4243
- [ ] Valider tous les paquets UDP (magic, version, token)
- [ ] Stocker l'endpoint UDP du client au premier paquet
- [ ] Gérer les déconnexions proprement

## Commandes pour Tester

### Lancer le serveur
```bash
cd build
./r-type_server 4242 4243
```

### Tester avec le client de test
```bash
./test_client localhost 4242
```

### Résultat attendu
```
=== Étape 1 : Connexion TCP ===
Connecté au serveur sur localhost:4242

=== Étape 2 : Envoi du message CONNECT ===
Envoi : CONNECT username=TestPlayer version=1.0

=== Étape 3 : Réception de CONNECT_OK ===
Authentification réussie !
  Player ID : 1
  Token     : 0xc5b320db
  UDP Port  : 4243

=== Étape 5 : Envoi d'inputs UDP ===
Input envoyé : buttons=0x1 moveX=0

=== Étape 6 : Test PING/PONG ===
PONG reçu (latence: 10ms)

=== Test terminé avec succès ! ===
```

## Prochaines Étapes

Maintenant que le protocole de connexion fonctionne, tu peux :
1. Intégrer l'ECS pour gérer les entités
2. Implémenter les messages ENTITY_SPAWN, ENTITY_UPDATE, ENTITY_DESTROY
3. Créer un client graphique avec SFML
4. Ajouter la logique de jeu (mouvement, collisions, tirs)
5. Synchroniser l'état du jeu entre le serveur et les clients
