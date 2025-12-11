# R-Type Server and Client

Server-authoritative multiplayer game using ECS (Entity-Component-System) and binary network protocol.

## Features

- **Authoritative Server**: Game simulation runs on server using ECS
- **Thin Clients**: Clients only send input and render state from server
- **Binary Protocol**: Efficient UDP networking for real-time gameplay
- **Multiplayer**: Support for up to 4 simultaneous players
- **ECS Architecture**: Clean separation of data and logic

## Quick Start

See **[doc/BUILD.md](doc/BUILD.md)** for detailed build and run instructions.

**TL;DR:**
```bash
# Build
make

# Terminal 1: Start server
./r-type_server 4242 4243

# Terminal 2+: Start client(s)
./render_client localhost 4242
```

## Documentation

- **[doc/BUILD.md](doc/BUILD.md)** - Building and running
- **[doc/ARCHITECTURE.md](doc/ARCHITECTURE.md)** - System architecture
- **[doc/PROTOCOL.md](doc/PROTOCOL.md)** - Network protocol
- **[doc/TCP_UDP_ARCHITECTURE.md](doc/TCP_UDP_ARCHITECTURE.md)** - TCP/UDP design
- **[doc/CONNEXION_PROTOCOL.md](doc/CONNEXION_PROTOCOL.md)** - Connection flow
- **[doc/TROUBLESHOOTING.md](doc/TROUBLESHOOTING.md)** - Troubleshooting guide

## Components

### Server (`r-type_server`)

Authoritative game server that:
- Accepts TCP connections and authenticates players
- Spawns player entities in ECS world
- Runs game loop at 60 Hz
- Processes player inputs via UDP
- Broadcasts world state to all clients
- Handles player disconnection and cleanup

### Clients

**`render_client`** (SFML-based, recommended):
- Graphical client with visual rendering
- Arrow keys to move, Space to shoot
- Shows all players in real-time

**`game_client`** (Terminal-based):
- Text-based interactive client
- Manual input controls
- Good for testing

**`protocol_test`** (Testing):
- Protocol testing tool
- Sends test packets
- Validates responses

**`test_headless`** (Automated Testing):
- Headless test client (no GUI)
- Verifies server/client communication
- Useful for CI/CD and debugging

## Network Architecture

## Prérequis

- C++17
- Make
- Boost.Asio ou ASIO standalone

### Installation d'ASIO standalone (recommandé)

```bash
sudo apt-get update
sudo apt-get install libasio-dev
```

### Ou installation de Boost

```bash
sudo apt-get install libboost-all-dev
```

## Compilation

### Méthode 1 : Avec Makefile (recommandé)

```bash
cd Serveur
make
```

Cela générera deux exécutables :
- `r-type_server` : Le serveur
- `client_test` : Un client de test

**Autres commandes Makefile :**
```bash
make clean      # Nettoyer les fichiers objets
make fclean     # Nettoyer tout (objets + binaires)
make re         # Recompiler tout
make test_server # Compiler et lancer le serveur sur le port 4242
make test_client # Compiler et lancer le client
make help       # Afficher l'aide
```

### Méthode 2 : Avec CMake

```bash
cd Serveur
mkdir build
cd build
cmake ..
make
```

## Utilisation

### Lancer le serveur

```bash
./r-type_server <port>
```

Exemple :
```bash
./r-type_server 4242
```

### Tester avec le client inclus

Dans un autre terminal :

```bash
./client_test localhost 4242
```

Tapez ensuite vos messages et appuyez sur Entrée. Le serveur renverra un écho de vos messages.

### Lancer plusieurs clients simultanément

Pour tester la capacité multi-clients du serveur, utilisez le script fourni :

```bash
./launch_clients.sh
```

**Options disponibles :**
```bash
./launch_clients.sh -n 5              # Lance 5 clients
./launch_clients.sh -n 10 -p 8080     # Lance 10 clients sur le port 8080
./launch_clients.sh --help            # Affiche l'aide
```

Le script :
- Lance X clients simultanément (3 par défaut)
- Chaque client envoie des messages automatiquement
- Messages colorés par client pour faciliter la lecture
- Arrêt propre de tous les clients avec Ctrl+C

### Test de connexion avec telnet ou nc

Vous pouvez aussi tester la connexion avec `telnet` ou `nc` :

```bash
telnet localhost 4242
```

Ou avec `nc` :
```bash
nc localhost 4242
```

## Architecture

- **Server.hpp/cpp** : Classe serveur principale qui gère l'acceptation des connexions
- **Session** : Classe qui gère une connexion client individuelle
- **main.cpp** : Point d'entrée du serveur

### Pourquoi ASIO ?

**ASIO (Asynchronous I/O)** est une bibliothèque qui permet de gérer le réseau de manière **asynchrone et non-bloquante**.

**Avantages :**
- **Multi-clients sans threads** : Un seul thread peut gérer des centaines de clients simultanément
- **Pas de blocage** : Le serveur ne se bloque jamais en attendant des données
- **Performance optimale** : Utilise `epoll` sur Linux pour une efficacité maximale
- **Code portable** : Fonctionne sur Linux, Windows et MacOS
- **Parfait pour les jeux** : Latence minimale, idéal pour un jeu d'action comme R-Type

### Fonctionnement

```
┌─────────────────────────────────┐
│   1. Client se connecte         │
│           ↓                     │
│   2. async_accept détecte       │
│           ↓                     │
│   3. Session créée              │
│           ↓                     │
│   4. async_read_some lancé      │
│      (ne bloque pas!)           │
│           ↓                     │
│   5. Serveur continue à         │
│      accepter d'autres clients  │
│           ↓                     │
│   6. Données arrivent           │
│           ↓                     │
│   7. Callback appelé            │
│      automatiquement            │
│           ↓                     │
│   8. Traitement + réponse       │
│           ↓                     │
│   9. Nouvelle lecture...        │
└─────────────────────────────────┘
```

### Cycle de vie d'une requête

1. **io_context** : Le "réacteur" qui surveille tous les sockets
2. **async_accept** : Accepte les connexions sans bloquer
3. **Session** : Gère un client avec lecture/écriture asynchrone
4. **Callbacks** : Appelés automatiquement quand des événements arrivent

**Exemple simplifié :**
```cpp
// Le code ne bloque JAMAIS !
acceptor.async_accept([](socket) {
    // Appelé automatiquement à la connexion
    socket.async_read([](data) {
        // Appelé automatiquement à la réception
        process(data);
    });
});

io_context.run(); // Boucle d'événements
```

### Comparaison : Avec vs Sans ASIO

**Sans ASIO (sockets classiques) :**
- Besoin d'un thread par client (lourd)
- Le code se bloque en attendant les données
- Gestion complexe de la synchronisation
- Code différent selon l'OS

**Avec ASIO (notre serveur) :**
- Un seul thread pour tous les clients
- Code non-bloquant et réactif
- Pas de synchronisation compliquée
- Code portable automatiquement



## Current Status

✅ **Completed:**
- Server-authoritative ECS architecture
- Binary UDP protocol for gameplay
- Player movement and input handling
- Entity synchronization between clients
- Multi-player support (up to 4 players)
- Fixed black screen bug (UDP timing issue)
- Automated testing with headless client

🚧 **In Progress / TODO:**
- Shooting mechanics
- Enemy AI and spawning
- Collision detection
- Health/damage system
- Power-ups
- Game rooms/lobbies
- Score tracking

## Known Issues (Fixed)

### Black Screen Bug - FIXED ✅
**Issue**: Client showed black screen despite successful connection.

**Cause**: ENTITY_SPAWN packets were sent during TCP connection before the client's UDP endpoint was initialized, causing packets to never reach the client.

**Fix**: Implemented two-stage connection:
1. TCP phase: Player authenticated, entity created
2. UDP phase: When first UDP packet received, ENTITY_SPAWN broadcast sent

See [doc/TROUBLESHOOTING.md](doc/TROUBLESHOOTING.md) for details and debugging steps.
