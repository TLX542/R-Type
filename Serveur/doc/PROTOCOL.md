# Protocole Binaire - Guide Complet

## Qu'est-ce qu'un protocole binaire ?

### Différence : Protocole Texte vs Binaire

#### Protocole Texte (comme HTTP, JSON)
Un protocole texte envoie les données sous forme de chaînes de caractères lisibles. Par exemple, pour envoyer la position d'un joueur, on écrirait quelque chose comme "MOVE PLAYER 1 X=123.45 Y=678.90 ANGLE=45.5\n", ce qui prend environ 45 bytes.

**Caractéristiques :**
- Lisible par un humain
- Facile à débugger
- Nécessite du parsing (conversion de texte en données)
- Taille importante
- Plus lent à traiter

#### Protocole Binaire (optimisé)
Un protocole binaire envoie les mêmes informations sous forme de bytes bruts directement interprétables par la machine. La même information de position prend seulement 17 bytes (2.6x plus petit).

**Caractéristiques :**
- Non lisible par un humain
- Plus difficile à débugger
- Pas de parsing nécessaire (copie directe en mémoire)
- Taille réduite
- Très rapide à traiter

### Avantages du Protocole Binaire

**Pour un jeu comme R-Type :**
- ✅ **Performance** : 2-5x plus rapide à encoder/décoder
- ✅ **Bande passante** : 2-10x moins de données transmises
- ✅ **Latence** : Critique pour un jeu d'action en temps réel
- ✅ **CPU** : Moins de traitement (pas de parsing de texte)

**Exemple concret :**
Si vous envoyez 60 mises à jour par seconde pour 4 joueurs et 10 entités (2400 messages/seconde) :
- Protocole texte : ~108 KB/s
- Protocole binaire : ~40 KB/s (62% d'économie)

## Pourquoi UDP pour le gameplay ?

### TCP vs UDP

#### TCP (Transmission Control Protocol)
TCP est un protocole **fiable** qui garantit que tous les paquets arrivent dans l'ordre.

**Fonctionnement :**
1. Établissement de connexion (handshake en 3 étapes)
2. Envoi de données
3. Accusé de réception (ACK) pour chaque paquet
4. Si un paquet est perdu, retransmission automatique

**Caractéristiques TCP :**
- ✅ Fiable : Tous les paquets arrivent dans l'ordre
- ✅ Pas de perte de données
- ❌ Latence élevée (attente des ACK et retransmissions)
- ❌ Head-of-line blocking : si un paquet est perdu, tout s'arrête en attendant sa retransmission

#### UDP (User Datagram Protocol)
UDP est un protocole **non fiable** qui envoie les paquets sans garantie de livraison.

**Fonctionnement :**
1. Envoi direct des données (pas de handshake)
2. Pas d'accusé de réception
3. Si un paquet est perdu, on continue avec le suivant

**Caractéristiques UDP :**
- ✅ Très rapide (pas d'overhead)
- ✅ Pas de latence de retransmission
- ✅ Parfait pour les données temps-réel
- ❌ Pas de garantie de livraison
- ❌ Pas d'ordre garanti

### Pourquoi UDP pour R-Type ?

**Dans un jeu d'action en temps réel :**
Imaginez que vous envoyez la position d'un vaisseau à chaque frame :
- Frame N : Position X=100
- Frame N+1 : Position X=105
- Frame N+2 : Position X=110

Si le paquet de la frame N+1 est perdu :
- **Avec TCP** : Tout se bloque en attendant la retransmission → LAG visible
- **Avec UDP** : On passe directement à la frame N+2 → Jeu fluide

**Règle d'or** : Les données anciennes sont inutiles dans un jeu temps-réel ! Mieux vaut recevoir la position la plus récente que d'attendre une position obsolète.

## Structure d'un Paquet Binaire

### Anatomie d'un Paquet

Un paquet binaire est composé de plusieurs sections :

**HEADER (en-tête, 4 bytes)** :
- Magic number (2 bytes) : Séquence spéciale (ex: 0xABCD) pour vérifier qu'il s'agit bien d'un paquet valide
- Version (1 byte) : Version du protocole (permet la compatibilité)
- Payload size (1 byte) : Taille des données (0-255 bytes)

**TYPE (1 byte)** :
- Type de message (ex: 0x10 = input joueur, 0x20 = spawn entité, etc.)

**PAYLOAD (taille variable)** :
- Les données réelles du message (position, vie, ID joueur, etc.)

**CHECKSUM (optionnel, 2 bytes)** :
- Somme de contrôle pour détecter la corruption des données

### Concept de #pragma pack

En C++, les structures sont automatiquement "alignées" par le compilateur (padding) pour optimiser l'accès mémoire. Cela peut ajouter des bytes inutiles.

**#pragma pack(push, 1)** force l'alignement sur 1 byte, garantissant que votre structure fait exactement la taille attendue, sans bytes supplémentaires.

## Types de Messages pour R-Type

### Énumération des Types

Chaque type de message a un identifiant unique (1 byte) :

**Connection (TCP acceptable)** :
- CONNECT_REQUEST (0x01) : Demande de connexion d'un client
- CONNECT_RESPONSE (0x02) : Réponse du serveur
- DISCONNECT (0x03) : Déconnexion

**Game State (UDP obligatoire)** :
- PLAYER_INPUT (0x10) : Entrées clavier du joueur
- PLAYER_MOVE (0x11) : Position du joueur
- PLAYER_SHOOT (0x12) : Tir du joueur

**Entities (UDP obligatoire)** :
- ENTITY_SPAWN (0x20) : Création d'une nouvelle entité
- ENTITY_UPDATE (0x21) : Mise à jour de position
- ENTITY_DESTROY (0x22) : Destruction d'entité

**Game Events (UDP obligatoire)** :
- COLLISION (0x30) : Collision détectée
- SCORE_UPDATE (0x31) : Score mis à jour
- GAME_OVER (0x32) : Fin de partie

**Reliability (UDP avec gestion manuelle)** :
- ACK (0xF0) : Accusé de réception manuel
- PING (0xF1) : Heartbeat pour vérifier la connexion
- PONG (0xF2) : Réponse au ping

## Exemples de Messages Complets

### 1. PLAYER_INPUT (Entrées du joueur)

**Contenu du message (8 bytes de payload) :**
- Timestamp (4 bytes) : Horodatage du client en millisecondes
- Player ID (1 byte) : Identifiant du joueur (0-255)
- Buttons (1 byte) : Boutons pressés sous forme de bitfield (chaque bit = 1 bouton)
- Move X (1 byte) : Direction horizontale (-1, 0, ou +1)
- Move Y (1 byte) : Direction verticale (-1, 0, ou +1)

**Bitfield des boutons :**
Chaque bit représente un bouton :
- Bit 0 : Tir
- Bit 1 : Spécial
- Bit 2 : Saut
- ... jusqu'à 8 boutons possibles

**Paquet complet (13 bytes)** :
- Header (5 bytes) + Payload (8 bytes)

### 2. ENTITY_UPDATE (Mise à jour d'entité)

**Contenu du message (22 bytes de payload) :**
- Entity ID (4 bytes) : Identifiant unique de l'entité
- Position X (4 bytes) : Coordonnée X (float 32 bits)
- Position Y (4 bytes) : Coordonnée Y (float 32 bits)
- Velocity X (4 bytes) : Vitesse horizontale
- Velocity Y (4 bytes) : Vitesse verticale
- Health (1 byte) : Points de vie (0-255)
- State (1 byte) : État (normal, invincible, etc.)

**Format IEEE 754 :**
Les nombres à virgule flottante (float) sont encodés selon le standard IEEE 754, qui permet de représenter des nombres décimaux en 4 bytes.

### 3. Batch Update (Optimisation)

Pour réduire le nombre de paquets, on peut regrouper plusieurs entités dans un seul message :

**Contenu :**
- Entity count (1 byte) : Nombre d'entités dans le batch (max 10)
- Pour chaque entité (17 bytes) :
  - Entity ID (4 bytes)
  - Position X (4 bytes)
  - Position Y (4 bytes)
  - Velocity X (4 bytes)
  - Velocity Y (4 bytes)
  - Health (1 byte)

**Avantage :** Envoyer 10 entités en un seul paquet plutôt que 10 paquets séparés.

## Gestion des Erreurs et Sécurité

### 1. Validation du Paquet

**Étapes de validation :**
1. Vérifier que la taille minimale est respectée (au moins la taille du header)
2. Vérifier le magic number (0xABCD)
3. Vérifier la version du protocole
4. Vérifier que la taille totale correspond (header + payload size)
5. Vérifier que le payload size ne dépasse pas la limite

**Si la validation échoue :** Ignorer le paquet et continuer.

### 2. Protection contre les Buffer Overflows

**Buffer Overflow :** Écriture au-delà de la taille d'un buffer, causant des crashs ou des failles de sécurité.

**Protection :**
- Toujours vérifier la taille avant de lire les données
- Ne jamais faire confiance à la valeur de "payload size" sans vérification
- Utiliser memcpy avec des tailles explicites

### 3. Rate Limiting (Protection DoS)

**DoS (Denial of Service) :** Attaque visant à surcharger le serveur avec trop de messages.

**Rate Limiting :**
- Limiter le nombre de messages par seconde par client (ex: max 100 messages/sec)
- Garder un historique des timestamps des derniers messages
- Nettoyer les timestamps de plus d'1 seconde
- Refuser les messages si la limite est dépassée

### 4. Checksum (Détection de Corruption)

**Checksum :** Valeur calculée à partir des données pour détecter les erreurs de transmission.

**Méthodes courantes :**
- **CRC16** : Algorithme robuste de détection d'erreur
- **XOR simple** : XOR de tous les bytes (plus rapide mais moins fiable)

**Utilisation :**
1. Calculer le checksum avant l'envoi
2. Ajouter le checksum à la fin du paquet
3. Recalculer le checksum à la réception
4. Comparer avec la valeur reçue
5. Si différent, rejeter le paquet

## Gestion de la Fiabilité avec UDP

### Problème : UDP n'est pas fiable

Certains messages DOIVENT arriver (ex: connexion, déconnexion, scores finaux).

### Solution : ACK/NACK Manuel

**Concept :**
1. Attribuer un numéro de séquence unique à chaque message important
2. Marquer le message comme "nécessitant un ACK"
3. Stocker une copie du message
4. Attendre un ACK du destinataire
5. Si pas d'ACK après un délai (ex: 100ms), retransmettre
6. Quand ACK reçu, supprimer la copie

**Composants :**
- **Sequence Number** : Numéro unique incrémental pour chaque message
- **Needs Ack** : Flag indiquant si le message nécessite un accusé de réception
- **Pending Acks** : Map des messages en attente d'ACK avec timestamp
- **Retransmission Timer** : Vérification périodique des messages non acquittés

## Compression (Optionnel mais Recommandé)

### Delta Encoding pour les Positions

**Concept :**
Au lieu d'envoyer la position absolue (8 bytes pour X et Y en float), on envoie la différence par rapport à la position précédente.

**Exemple :**
- Position absolue : X=123.456 (4 bytes float)
- Delta : deltaX=+5 (1 byte int8)

**Avantage :**
- 8 bytes → 2 bytes (75% de réduction)
- Fonctionne bien pour les petits mouvements (±127 pixels)

**Limitation :**
Si le mouvement dépasse ±127, il faut envoyer une position absolue.

## Résumé : Best Practices

### ✅ À FAIRE :

1. **Utiliser #pragma pack(1)** : Éviter le padding des structures
2. **Valider TOUS les paquets** : Vérifier magic number, version, tailles
3. **Tailles fixes** : Utiliser des types de taille fixe (uint32_t, float, etc.)
4. **Magic number** : Identifier facilement vos paquets
5. **Versionner** : Permettre l'évolution du protocole
6. **Limiter la taille** : < 1400 bytes pour UDP (éviter la fragmentation)
7. **Rate limiting** : Protéger contre les attaques DoS
8. **Tester avec paquets malformés** : Vérifier la robustesse
9. **Documenter** : Décrire chaque message précisément

### ❌ À ÉVITER :

1. **Parser du texte** : En temps réel, privilégier le binaire
2. **Strings** : Éviter les chaînes de caractères de longueur variable
3. **Faire confiance au client** : Toujours valider côté serveur
4. **TCP pour gameplay** : Trop de latence pour les actions temps-réel
5. **Oublier l'endianness** : Le réseau utilise big-endian, les PC utilisent souvent little-endian
6. **Paquets > 1400 bytes** : Risque de fragmentation UDP (perte de performance)

## Documentation du Protocole

### Format recommandé pour documenter vos messages

Pour chaque message, spécifier :
- **ID du message** : Ex: 0x10
- **Nom** : Ex: PLAYER_INPUT
- **Direction** : Client → Server ou Server → Client
- **Transport** : TCP ou UDP
- **Fréquence** : Ex: 60/sec, 1/session
- **Taille** : Taille totale en bytes
- **Structure** : Tableau décrivant chaque champ (offset, type, nom, description)

**Exemple de documentation :**

### 0x10 - PLAYER_INPUT
- **Direction** : Client → Server
- **Transport** : UDP
- **Fréquence** : Jusqu'à 60/sec
- **Taille** : 8 bytes

| Offset | Type    | Nom       | Description              |
|--------|---------|-----------|--------------------------|
| 0x00   | uint32  | timestamp | Timestamp client (ms)    |
| 0x04   | uint8   | buttons   | Bitfield des boutons     |
| 0x05   | int8    | moveX     | Mouvement horizontal     |
| 0x06   | int8    | moveY     | Mouvement vertical       |

**Button Bitfield :**
- Bit 0 : Tir
- Bit 1 : Spécial
- Bits 2-7 : Réservés

---

**Cette documentation explique les concepts d'un protocole binaire robuste pour R-Type, sans inclure de code d'implémentation !**
