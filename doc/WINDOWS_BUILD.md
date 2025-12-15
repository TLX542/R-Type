# Guide de compilation et test sur Windows

Ce guide explique comment compiler et tester le serveur R-Type sur Windows.

## Prérequis

### 1. Installer les outils de développement

- **Visual Studio 2019 ou plus récent** (Community Edition suffit)
  - Télécharger depuis: https://visualstudio.microsoft.com/
  - Lors de l'installation, sélectionner "Développement Desktop en C++"

- **CMake 3.17 ou plus récent**
  - Télécharger depuis: https://cmake.org/download/
  - Cocher "Add CMake to system PATH" lors de l'installation

- **Git pour Windows** (si pas déjà installé)
  - Télécharger depuis: https://git-scm.com/download/win

### 2. Installer ASIO

Vous avez deux options pour installer ASIO sur Windows:

#### Option A: Utiliser vcpkg (Recommandé)

vcpkg est un gestionnaire de paquets C++ pour Windows, Linux et MacOS.

```powershell
# 1. Installer vcpkg
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat

# 2. Installer ASIO
.\vcpkg install asio:x64-windows

# 3. Intégrer vcpkg avec CMake (une seule fois)
.\vcpkg integrate install
```

Après l'intégration, CMake trouvera automatiquement ASIO.

#### Option B: Installation manuelle

```powershell
# 1. Télécharger ASIO
# Aller sur: https://think-async.com/Asio/Download.html
# Télécharger la dernière version (asio-1.XX.X.zip)

# 2. Extraire dans C:\asio (ou autre dossier)

# 3. Lors de la compilation, spécifier le chemin:
cmake -DASIO_INCLUDE_DIR="C:\asio\include" ..
```

## Compilation avec CMake

### Méthode 1: Avec Visual Studio (Interface graphique)

1. Ouvrir Visual Studio
2. Fichier > Ouvrir > Dossier
3. Sélectionner le dossier `Serveur/`
4. Visual Studio détecte automatiquement CMakeLists.txt
5. Sélectionner la configuration (Debug ou Release)
6. Build > Générer tout

Les exécutables seront dans `Serveur/out/build/x64-Debug/` ou `Serveur/out/build/x64-Release/`

### Méthode 2: Ligne de commande (PowerShell ou CMD)

```powershell
# 1. Naviguer vers le dossier Serveur
cd Serveur

# 2. Créer un dossier de build
mkdir build
cd build

# 3. Générer les fichiers de projet
cmake ..

# Ou si vous avez installé ASIO manuellement:
cmake -DASIO_INCLUDE_DIR="C:\asio\include" ..

# 4. Compiler
cmake --build . --config Release

# Les exécutables seront dans: Serveur/build/Release/
```

### Méthode 3: Avec MinGW (Alternative GCC sur Windows)

Si vous préférez utiliser GCC au lieu de MSVC:

```bash
# 1. Installer MinGW-w64
# Télécharger depuis: https://www.mingw-w64.org/downloads/

# 2. Dans le dossier Serveur
mkdir build && cd build

# 3. Générer avec MinGW
cmake -G "MinGW Makefiles" ..

# 4. Compiler
mingw32-make

# Les exécutables seront dans: Serveur/build/
```

## Vérification de la compilation

Après la compilation, vous devriez avoir ces exécutables:

- `r-type_server.exe` - Le serveur de jeu
- `client_test.exe` - Client de test simple
- `game_client.exe` - Client interactif en terminal
- `test_headless.exe` - Client de test automatisé

## Tester le serveur

### Test 1: Lancer le serveur

```powershell
# Dans le dossier où se trouve r-type_server.exe
.\r-type_server.exe 4242 4243

# Vous devriez voir:
# R-Type Game Server is running...
# Waiting for clients to connect...
# [UDP] Server listening on port 4243
```

**Paramètres:**
- Premier argument: Port TCP (pour les connexions)
- Deuxième argument: Port UDP (pour le jeu)

### Test 2: Utiliser le client de test

Dans un autre terminal PowerShell:

```powershell
.\game_client.exe localhost 4242

# Vous devriez voir:
# [TCP] Connected to server
# [TCP] Server says: CONNECT_OK ...
# Enter your name: [tapez votre nom]
```

### Test 3: Test automatisé avec client headless

```powershell
.\test_headless.exe localhost 4242

# Ce client va:
# - Se connecter au serveur
# - Envoyer des inputs
# - Afficher les mises à jour reçues
```

### Test 4: Tester avec telnet (Windows)

```powershell
# Activer le client Telnet si nécessaire
dism /online /Enable-Feature /FeatureName:TelnetClient

# Se connecter au serveur
telnet localhost 4242

# Envoyer une commande de connexion manuelle
CONNECT username=test version=1.0
```

## Tests de connectivité réseau

### Vérifier que les ports sont ouverts

```powershell
# Afficher les ports en écoute
netstat -an | findstr "4242 4243"

# Vous devriez voir:
# TCP    0.0.0.0:4242    0.0.0.0:0    LISTENING
# UDP    0.0.0.0:4243    *:*
```

### Test avec plusieurs clients

Ouvrir plusieurs terminaux PowerShell et lancer plusieurs clients:

```powershell
# Terminal 1
.\game_client.exe localhost 4242

# Terminal 2
.\game_client.exe localhost 4242

# Terminal 3
.\game_client.exe localhost 4242
```

Le serveur devrait accepter jusqu'à 4 clients simultanément.

## Configuration du pare-feu Windows

Si les clients ne peuvent pas se connecter, vérifier le pare-feu:

```powershell
# Autoriser le serveur dans le pare-feu
New-NetFirewallRule -DisplayName "R-Type Server TCP" -Direction Inbound -LocalPort 4242 -Protocol TCP -Action Allow
New-NetFirewallRule -DisplayName "R-Type Server UDP" -Direction Inbound -LocalPort 4243 -Protocol UDP -Action Allow
```

Ou manuellement:
1. Panneau de configuration > Système et sécurité > Pare-feu Windows Defender
2. Paramètres avancés
3. Règles de trafic entrant > Nouvelle règle
4. Port > TCP > 4242 (et créer une autre pour UDP 4243)

## Problèmes courants

### Erreur: "ASIO not found"

**Solution:** Installer ASIO via vcpkg ou spécifier le chemin manuellement:
```powershell
cmake -DASIO_INCLUDE_DIR="C:\chemin\vers\asio\include" ..
```

### Erreur: "Cannot open include file asio.hpp"

**Solution:** Vérifier que ASIO est bien installé et que le chemin est correct.

Si vous utilisez vcpkg:
```powershell
# Vérifier l'installation
.\vcpkg list | findstr asio

# Réintégrer vcpkg
.\vcpkg integrate install
```

### Erreur de link: "ws2_32.lib not found"

**Solution:** Assurez-vous d'avoir installé "Développement Desktop en C++" avec Visual Studio.

### Le serveur se lance mais les clients ne peuvent pas se connecter

**Solutions:**
1. Vérifier que le pare-feu Windows ne bloque pas les ports
2. Vérifier que les ports ne sont pas déjà utilisés: `netstat -an | findstr 4242`
3. Essayer avec `127.0.0.1` au lieu de `localhost`

### Erreur: "Address already in use"

**Solution:** Un autre processus utilise le port. Changer de port ou tuer le processus:
```powershell
# Trouver le processus utilisant le port
netstat -ano | findstr 4242

# Tuer le processus (remplacer PID par le numéro)
taskkill /PID <PID> /F
```

## Tests avancés

### Test de performance

Lancer plusieurs clients headless pour tester la charge:

```powershell
# Script PowerShell pour lancer 10 clients
1..10 | ForEach-Object {
    Start-Process -FilePath ".\test_headless.exe" -ArgumentList "localhost", "4242"
}
```

### Test réseau local (LAN)

Pour tester avec d'autres machines sur le réseau:

```powershell
# Sur la machine serveur, obtenir l'IP
ipconfig

# Chercher "Adresse IPv4" (ex: 192.168.1.100)

# Sur une autre machine (client)
.\game_client.exe 192.168.1.100 4242
```

## Structure des fichiers après compilation

```
Serveur/
├── build/
│   ├── Release/
│   │   ├── r-type_server.exe
│   │   ├── client_test.exe
│   │   ├── game_client.exe
│   │   └── test_headless.exe
│   └── Debug/
│       └── [mêmes fichiers en version debug]
└── CMakeLists.txt
```

## Compilation en mode Debug vs Release

```powershell
# Mode Debug (avec symboles de débogage)
cmake --build . --config Debug

# Mode Release (optimisé)
cmake --build . --config Release
```

Le mode Release est plus rapide et recommandé pour les tests de performance.
Le mode Debug est utile pour déboguer avec Visual Studio.

## Debugging avec Visual Studio

1. Ouvrir Visual Studio
2. Fichier > Ouvrir > Projet/Solution
3. Sélectionner `Serveur/build/RTypeServer.sln`
4. Définir `r-type_server` comme projet de démarrage
5. Projet > Propriétés > Débogage > Arguments de commande: `4242 4243`
6. F5 pour lancer en mode debug

## Résumé des commandes

```powershell
# Installation vcpkg + ASIO (une seule fois)
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg install asio:x64-windows
.\vcpkg integrate install

# Compilation (dans le dossier Serveur)
mkdir build
cd build
cmake ..
cmake --build . --config Release

# Test (dans le dossier Serveur/build/Release)
.\r-type_server.exe 4242 4243        # Terminal 1
.\game_client.exe localhost 4242     # Terminal 2+
```

## Prochaines étapes

Une fois que le serveur fonctionne:
1. Consulter [PROTOCOL.md](PROTOCOL.md) pour comprendre le protocole réseau
2. Voir [ARCHITECTURE.md](ARCHITECTURE.md) pour l'architecture ECS
3. Lire [TROUBLESHOOTING.md](TROUBLESHOOTING.md) pour le débogage avancé

## Support

En cas de problème:
1. Vérifier les logs du serveur et du client
2. Consulter [TROUBLESHOOTING.md](TROUBLESHOOTING.md)
3. Vérifier que toutes les dépendances sont installées
4. S'assurer que le pare-feu Windows ne bloque pas les connexions
