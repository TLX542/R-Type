# Architecture TCP + UDP pour R-Type

## Vue d'ensemble

L'architecture réseau de R-Type utilise **deux protocoles complémentaires** :
- **TCP** pour les opérations critiques et peu fréquentes (connexion, configuration)
- **UDP** pour le gameplay en temps réel (positions, actions)

### Schéma de communication

**Phase 1 : Connexion (TCP)**
1. Client se connecte au serveur via TCP sur le port 4242
2. Client envoie "CONNECT" avec son nom d'utilisateur
3. Serveur répond avec l'ID du joueur et un token de session
4. Serveur communique le port UDP à utiliser (ex: 4243)

**Phase 2 : Gameplay (UDP)**
1. Client envoie ses inputs (clavier) via UDP au port 4243 à 60 Hz
2. Serveur envoie les mises à jour d'état (positions, entités) via UDP à 30 Hz
3. Communication continue tant que la partie est en cours

**Phase 3 : Déconnexion (TCP)**
1. Client envoie "DISCONNECT" via TCP
2. Serveur confirme et ferme la connexion proprement

## 1. Protocole TCP (Texte) - Connexion/Setup

### Pourquoi TCP pour la connexion ?

**Avantages :**
- ✅ **Fiabilité** : La connexion DOIT réussir, pas de tolérance à la perte
- ✅ **Simplicité** : Format texte facile à débugger
- ✅ **Fréquence faible** : Une seule connexion par session

**Inconvénient acceptable :**
- ❌ Latence légèrement plus élevée (mais peu important car rare)

### Format des Messages TCP

Format simple basé sur des lignes de texte :
```
TYPE [param1=value1] [param2=value2]\n
```

**Caractéristiques :**
- Séparation par espaces
- Paramètres sous forme clé=valeur
- Terminaison par retour à la ligne \n
- Facile à lire et débugger

### Messages TCP Disponibles

#### CONNECT (Client → Serveur)
**Objectif** : Demander une connexion au serveur

**Paramètres :**
- username : Nom du joueur (max 16 caractères)
- version : Version du client

**Réponse Succès :**
Le serveur accepte la connexion et envoie :
- id : ID du joueur assigné (1-4)
- token : Token de session pour authentifier les paquets UDP
- udp_port : Port UDP à utiliser pour le gameplay

**Réponse Échec :**
Le serveur refuse la connexion avec une raison :
- server_full : Serveur plein (4 joueurs max)
- version_mismatch : Version incompatible
- invalid_username : Nom invalide

#### DISCONNECT (Client → Serveur)
**Objectif** : Déconnexion propre du client

**Réponse :**
Le serveur confirme la déconnexion.

#### GAME_START (Serveur → Tous les clients)
**Objectif** : Notifier le début de la partie

**Paramètres :**
- level : Numéro du niveau
- players : Nombre de joueurs dans la partie

#### GAME_OVER (Serveur → Tous les clients)
**Objectif** : Notifier la fin de la partie

**Paramètres :**
- winner : ID du joueur gagnant
- score : Score final

#### PLAYER_JOIN (Serveur → Tous les clients)
**Objectif** : Notifier l'arrivée d'un nouveau joueur

**Paramètres :**
- id : ID du nouveau joueur
- username : Nom du joueur

#### PLAYER_LEAVE (Serveur → Tous les clients)
**Objectif** : Notifier le départ d'un joueur

**Paramètres :**
- id : ID du joueur qui part

#### CHAT (Optionnel)
**Objectif** : Échanger des messages texte entre joueurs

**Paramètres :**
- from : ID de l'expéditeur
- message : Contenu du message

### Parsing des Messages TCP

**Parsing** : Processus de conversion du texte en données exploitables.

**Étapes :**
1. Recevoir la ligne de texte jusqu'au \n
2. Extraire le type (premier mot)
3. Parcourir les paramètres (format clé=valeur)
4. Stocker dans une structure (map, dictionnaire)

## 2. Protocole UDP (Binaire) - Gameplay

### Pourquoi UDP pour le gameplay ?

**Avantages :**
- ✅ **Latence minimale** : Pas d'overhead de TCP
- ✅ **Pas de blocage** : Les paquets perdus n'affectent pas les suivants
- ✅ **Performance** : Idéal pour 60 messages/sec

**Inconvénients gérés :**
- ❌ Perte de paquets : Acceptable car les données sont rapidement périmées
- ❌ Ordre non garanti : Non important car chaque paquet contient un timestamp

### Format des Paquets UDP

**Structure binaire stricte** (voir PROTOCOL.md pour les détails)

**Header (5 bytes)** :
- Magic number (2 bytes) : 0xABCD
- Version (1 byte) : 0x01
- Payload size (1 byte)
- Type (1 byte)

**Note importante :** Le header inclut également un **session token** (4 bytes supplémentaires) pour l'authentification.

### Sécurité : Token d'Authentification

**Problème :**
Avec UDP, n'importe qui peut envoyer des paquets au serveur (UDP est sans connexion).

**Solution :**
1. Le client reçoit un token unique via TCP lors de la connexion
2. Chaque paquet UDP inclut ce token dans le header
3. Le serveur vérifie le token à chaque paquet reçu
4. Si le token est invalide, le paquet est rejeté

**Avantage :**
- Empêche les paquets non autorisés
- Lie la session TCP à la session UDP

## 3. Intégration Client Complète

### Architecture du Client

**Composants :**
1. **Socket TCP** : Connexion persistante pour les commandes
2. **Socket UDP** : Communication rapide pour le gameplay
3. **Session Token** : Authentification des paquets UDP
4. **Player ID** : Identifiant unique attribué par le serveur
5. **ECS Registry** : Gestion des entités du jeu

### Flow de Connexion Client

**Étape 1 : Connexion TCP**
1. Résoudre l'adresse du serveur (hostname → IP)
2. Se connecter au port TCP (4242)
3. Envoyer le message CONNECT avec username
4. Recevoir la réponse CONNECT_OK
5. Extraire l'ID joueur, le token et le port UDP

**Étape 2 : Configuration UDP**
1. Créer un socket UDP local (port aléatoire)
2. Configurer l'endpoint du serveur (IP + port UDP)
3. Le socket UDP est prêt à envoyer/recevoir

**Étape 3 : Gameplay Loop**
1. Lire les inputs clavier/souris
2. Sérialiser en paquet binaire PLAYER_INPUT
3. Ajouter le token d'authentification au header
4. Envoyer via UDP
5. Recevoir les paquets UDP du serveur (ENTITY_UPDATE, etc.)
6. Désérialiser et mettre à jour l'ECS
7. Rendre à l'écran

**Étape 4 : Déconnexion**
1. Envoyer DISCONNECT via TCP
2. Attendre la confirmation
3. Fermer les sockets TCP et UDP

## 4. Intégration Serveur Complète

### Architecture du Serveur

**Composants :**
1. **TCP Acceptor** : Écoute les nouvelles connexions TCP
2. **Socket UDP** : Reçoit les inputs et envoie les updates
3. **Liste des Clients** : Stocke les infos de chaque joueur connecté
4. **ECS Registry** : État du jeu (entités, positions, vies, etc.)

### Structure Client Info

Pour chaque client connecté, le serveur stocke :
- Player ID : Identifiant unique
- Session Token : Pour valider les paquets UDP
- TCP Socket : Pour les commandes
- UDP Endpoint : Adresse et port UDP du client (découvert au premier paquet UDP reçu)
- Username : Nom du joueur
- UDP Initialized : Flag indiquant si le premier paquet UDP a été reçu

### Flow de Gestion Serveur

**Phase 1 : Acceptation de Connexion**
1. Attendre les connexions TCP (mode asynchrone)
2. Quand un client se connecte, créer un handler TCP
3. Recevoir le message CONNECT
4. Vérifier si le serveur est plein (max 4 joueurs)
5. Générer un Player ID unique et un token aléatoire
6. Créer une structure ClientInfo
7. Répondre avec CONNECT_OK (ID, token, port UDP)
8. Créer l'entité du joueur dans l'ECS
9. Notifier les autres joueurs (PLAYER_JOIN)

**Phase 2 : Réception Inputs UDP**
1. Lire tous les paquets UDP disponibles (non-bloquant)
2. Valider chaque paquet (magic number, token)
3. Associer le paquet à un client via le token
4. Si c'est le premier paquet UDP du client, stocker son endpoint
5. Extraire les inputs (direction, boutons)
6. Mettre à jour les composants ECS (Velocity, actions)

**Phase 3 : Broadcast État du Jeu**
1. Parcourir les entités de l'ECS
2. Sérialiser les positions, vies, vitesses
3. Créer des paquets ENTITY_BATCH_UPDATE
4. Envoyer à tous les clients UDP initialisés

**Phase 4 : Déconnexion**
1. Recevoir DISCONNECT via TCP
2. Répondre DISCONNECT_OK
3. Supprimer le client de la liste
4. Détruire l'entité du joueur dans l'ECS
5. Notifier les autres joueurs (PLAYER_LEAVE)

## 5. Résumé : Quand utiliser quoi

| Action | Protocole | Format | Fréquence | Raison |
|--------|-----------|--------|-----------|--------|
| Connexion initiale | **TCP** | **Texte** | 1×/session | Critique, debug facile |
| Attribution ID/token | **TCP** | **Texte** | 1×/session | Doit arriver |
| Déconnexion | **TCP** | **Texte** | 1×/session | Propre |
| Player join/leave | **TCP** | **Texte** | Rare | Important |
| Game start/end | **TCP** | **Texte** | Rare | Critique |
| **Input joueur** | **UDP** | **Binaire** | 60×/sec | Temps-réel |
| **Entity updates** | **UDP** | **Binaire** | 30×/sec | Volume énorme |
| **Collisions** | **UDP** | **Binaire** | Variable | Temps-réel |
| **Tirs** | **UDP** | **Binaire** | Variable | Latence critique |

## 6. Flow Complet d'une Session

**Séquence chronologique complète :**

1. **CLIENT → TCP** : "CONNECT username=Player1 version=1.0"
2. **SERVER → TCP** : "CONNECT_OK id=1 token=abc123 udp_port=4243"
3. Client configure son socket UDP avec le token
4. **CLIENT → UDP** : [binaire] PLAYER_INPUT (60 fois par seconde)
5. **SERVER → UDP** : [binaire] ENTITY_UPDATE (30 fois par seconde)
6. ... Gameplay continue tant que la partie est active ...
7. **CLIENT → TCP** : "DISCONNECT"
8. **SERVER → TCP** : "DISCONNECT_OK"

## 7. Avantages de cette Architecture Hybride

**✅ Séparation des Responsabilités**
- TCP : Gestion de session (connexion, configuration)
- UDP : Données temps-réel (gameplay)

**✅ Performance Optimale**
- Pas de latence TCP pour le gameplay
- Fiabilité TCP pour les opérations critiques

**✅ Sécurité**
- Token d'authentification lie TCP et UDP
- Validation de tous les paquets UDP

**✅ Debugging Facile**
- Messages TCP en texte clair
- Facile de voir les connexions/déconnexions

**✅ Conformité Sujet R-Type**
- UDP pour le gameplay (requis)
- TCP pour la gestion de session (permis)

## 8. Considérations Réseau

### MTU et Fragmentation

**MTU (Maximum Transmission Unit)** : Taille maximale d'un paquet sur le réseau (généralement 1500 bytes).

**Problème de fragmentation UDP :**
Si votre paquet UDP > MTU, il sera fragmenté en plusieurs paquets IP. Si un fragment est perdu, le paquet entier est perdu.

**Recommandation :**
- Limiter les paquets UDP à 1400 bytes maximum
- Utiliser des batch updates pour regrouper plusieurs entités

### Gestion de la Latence

**Latence** : Temps pour qu'un paquet voyage du client au serveur.

**Techniques pour masquer la latence :**
1. **Client Prediction** : Le client prédit son propre mouvement instantanément
2. **Server Reconciliation** : Le serveur corrige la position si nécessaire
3. **Interpolation** : Lissage du mouvement des autres joueurs

### NAT Traversal

**NAT (Network Address Translation)** : Mécanisme utilisé par les routeurs pour partager une IP publique entre plusieurs appareils.

**Problème :**
Le serveur ne connaît pas l'endpoint UDP du client avant de recevoir le premier paquet.

**Solution :**
- Le client envoie d'abord un paquet UDP au serveur
- Le serveur découvre l'endpoint UDP via l'adresse source du paquet
- Le serveur peut ensuite répondre à cet endpoint

C'est pourquoi la structure ClientInfo contient :
- udpEndpoint : Découvert à la réception du premier paquet
- udpInitialized : Flag pour savoir si l'endpoint est connu

---

**Cette architecture hybride TCP/UDP offre le meilleur des deux mondes : simplicité et fiabilité du TCP pour la connexion, et performance du binaire UDP pour le gameplay !**
