# R-Type Server

Serveur de base pour le projet R-Type utilisant ASIO pour la communication réseau.

## Fonctionnalités

- Accepte plusieurs connexions clients simultanément
- Gère les sessions client de manière asynchrone
- Echo des messages reçus
- Support TCP pour la communication réseau

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



## Prochaines étapes

- Intégration avec l'ECS du projet R-Type
- Ajout du protocole UDP pour le gameplay
- Implémentation de la logique du jeu
- Synchronisation des entités entre clients
- Gestion des salles de jeu
