# My-Type -- Démo ECS minimale (SFML)

Une petite démo autonome d'Entity-Component-System (ECS) écrite en C++17.  
Ce dépôt présente un registre compact, un stockage hybride conservateur à base de sparse, un proxy optional-ref et un petit "zipper" indexé pour itérer sur des emplacements de composants alignés. La démo utilise SFML pour afficher une entité contrôlable et des objets statiques.

## Démarrage rapide (CMake)

Prérequis :
- Chaîne d'outils C++17 (g++, clang ou MSVC)
- CMake (version 3.15 ou supérieure recommandée)
- SFML (graphique, fenêtre, système) -- paquet de développement
- Sous Linux : outils de compilation (make, ninja, etc.)
- Sous Windows : Visual Studio, MSYS2/MinGW ou vcpkg

Linux (exemple avec compilation hors du répertoire racine)
```sh
# Installation des dépendances (exemple Ubuntu)
sudo apt update
sudo apt install -y build-essential cmake libsfml-dev

# Configuration et compilation
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

# exécuter
./build/src/bs-rtype
```

Windows (Exemple Visual Studio / vcpkg)
- Avec vcpkg :
  1. Installer SFML via vcpkg :
    - vcpkg install sfml:x64-windows
  2. Configurer avec la chaîne d'outils vcpkg :
    - cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE=C:/chemin/vers/vcpkg/scripts/buildsystems/vcpkg.cmake
    - cmake --build build --config Release
    - .\\build\\Release\\bs-rtype.exe

- Sans vcpkg :
  - Téléchargez le SDK SFML pour votre compilateur (MSVC / MinGW) et définissez SFML_DIR sur le dossier cmake du package, ou définissez CMAKE_PREFIX_PATH pour pointer vers SFML avant d'exécuter cmake.

Remarques :
- Si SFML est installé dans un emplacement personnalisé, définissez -DSFML_DIR=/chemin/vers/SFML pour aider find_package(SFML ...) à le localiser.
- Si vous souhaitez utiliser SFML statique sous Windows, ajoutez -DBUILD_SHARED_SFML=OFF. Il vous faudra peut-être également lier manuellement les dépendances SFML (voir les commentaires dans src/CMakeLists.txt).

Contrôles
- Flèches / ZQSD -- déplacer le joueur
- Échap -- quitter

## Architecture générale

- `registry` (dans include/Registry.hpp) orchestre les stockages de composants et les systèmes.
- Les composants sont des structs POD dans include/Components.hpp.
- Le stockage par défaut est `HybridArray` (include/HybridArray.hpp) -- un conteneur sparse construit sur `std::vector<std::optional<T>>`.
- Les systèmes sont des callables simples enregistrés auprès du registry ; des exemples se trouvent dans include/Systems.hpp.
- Le helper zipper indexé (include/Zipper.hpp) permet d'itérer de façon alignée sur différents stockages par indice.

## Diagramme ASCII du flux d'exécution (runtime)

Ce diagramme ASCII montre le flux à l'exécution : entités créées → stockages → zipper qui aligne les indices → systèmes consommant les vues de composants.

Légende :
- [E#] = Identifiant d'entité
- (S) = Emplacement de stockage (peut contenir std::optional<Component>)
- Z = Zipper produisant des tuples (index, résultats de get(...))
- -> = flux de données / contrôle
- [opt] = valeur de type optional (copie) retournée par get()
- [&] = proxy-référence retourné par get_ref() / optional_ref

```
 Entities              Storages (sparse/packed)           Indexed Zipper               Systems
+-------+            +---------------------------+    +-----------------+        +-----------------------------+
| E0    |            | Position (HybridArray)    |    |                 |        | position_system             |
| E1    |            | 0:(S) -> Position{x,y}    |    |  Ziter(index)   |        | control_system              |
| E2    |            | 1:std::nullopt            |    |  yields tuples  |        | draw_system (sf::RenderWin) |
| E3    |            | 2:(S) -> Position{x,y}    |    | (idx, pos_opt,  |        +-----------------------------+
+-------+            +---------------------------+    |  vel_opt, ...)  |
     \              /  +------------------------+     +-----------------+                ^
      \            /   | Velocity (HybridArray) |            |  ^  ^                     |
       \          /    | 0:(S) -> Velocity{dx}  |            |  |  |                     |
        \        /     | 1:(S) -> Velocity{dx}  |  get(i) -> |  |  | zipper aligne index |
         +------+      | 2:std::nullopt         |  returns   |  |  |  et produit tuples  |
        |Registry|     +------------------------+   [opt]    v  v  v                     |
         +------+     +-------------------------+                                   mutates / reads
        /    \        | Drawable (HybridArray)  |    tuple -> (idx, pos_opt, vel_opt, draw_opt)
       /      \       | 0:std::nullopt          |             le système dépacte et agit
      /        \      | 2:(S) -> Drawable{w,h}  |
  spawn()   add_component(...)  ...             |
                                                  Exemple de sortie du zipper :
                                                  (2, Position{...}, std::nullopt, Drawable{...})
```

Remarques sur le flux :
- Les entités sont des identifiants numériques gérés par registry.spawn_entity().
- Les stockages exposent deux modes d'accès :
  - get(id) -> retourne une copie optional-like ([opt]) adaptée au zipper et aux vérifications en lecture seule.
  - get_ref(id) -> retourne un proxy-référence ([&]) vers std::optional<T> pour mutation en place ou insertion.
- Le zipper indexé itère sur une plage déterministe d'indices (généralement jusqu'à la plus grande taille de stockage) et appelle get(index) sur chaque stockage. Il renvoie des tuples contenant l'indice et le résultat get de chaque stockage.
- Les systèmes peuvent :
  - Lire les copies optionnelles fournies par le zipper et agir lorsque les valeurs sont présentes.
  - Ou obtenir des références de stockage depuis le registry et appeler get_ref(id) pour muter les emplacements composants (ex. control_system écrivant dans Velocity).
- Le système de dessin lit typiquement Position + Drawable et émet des appels SFML ; le système de position lit Velocity + Position et modifie Position via get_ref.

## Résumé fichier par fichier

Cette section documente chaque header public et le fichier source principal pour localiser rapidement responsabilités et API importantes.

- src/main.cpp
  - Point d'entrée et configuration de la démo.
  - Crée une `sf::RenderWindow`, une instance de `registry`, enregistre des stockages de composants, crée des entités et enregistre des systèmes.
  - Systèmes enregistrés :
    - Control system -- lit l'entrée et met à jour `Velocity`.
    - Position system -- intègre `Velocity` dans `Position`.
    - Draw system -- rend `Drawable` à la position `Position`.

- include/Registry.hpp
  - Type d'orchestration central : `registry`.
  - Responsabilités :
    - Enregistrer et stocker les stockages de composants indexés par `std::type_index`.
    - Fournir des accesseurs de composants :
      - `register_component<T>()` -- créer et stocker un storage si absent.
      - `get_components<T>()` / `get_components_if<T>()` -- obtenir une référence au stockage ou nullptr.
    - Cycle de vie des entités :
      - `spawn_entity()` -- retourne un wrapper `Entity` contenant un id.
      - `kill_entity(Entity)` -- efface les composants de cette entité et recycle l'id.
    - Opérations sur composants :
      - `add_component(Entity, T)` / `emplace_component(...)` -- ajouter ou remplacer des composants.
      - `remove_component<Entity, T>()`.
    - Systèmes :
      - `add_system<Comps...>(fn)` -- enregistrer des callables à exécuter chaque frame.
      - `run_systems()` -- invoquer les systèmes dans l'ordre d'insertion.
  - Notes :
    - La map d'effacement interne et les callbacks assurent que kill_entity retire les emplacements correspondants dans tous les stockages enregistrés.
    - Le registry est minimal et conçu pour accepter différents backends de stockage qui implémentent les sémantiques attendues `get(id)` / `get_ref(id)`.

- include/HybridArray.hpp
  - Stockage sparse implémenté principalement via `std::vector<std::optional<T>>`.
  - API importante :
    - `insert_at(id, value)` / `emplace_at(id, ...)` -- insérer un composant à l'indice d'entité ; assure la capacité.
    - `get(id) const` -- retourne une copie `std::optional<T>` ; compatible zipper.
    - `get_ref(id)` -- retourne `std::optional<T>&` pour mutation in-place ; agrandit si nécessaire.
    - `erase(id)` -- met la case à `std::nullopt` (ne réduit pas la capacité).
    - `size()` -- renvoie la capacité sparse courante (max id + 1), utilisée pour aligner le zipper.
  - Justification :
    - Sémantiques simples et sûres : les lectures hors-plage renvoient `std::nullopt`, et get_ref() explicite la croissance pour mutation.
    - Conçu pour pouvoir être remplacé par une alternative packée sans modifier l'interface du registry.

- include/SparseArray.hpp
  - Wrapper utilitaire sparse léger (noms et sémantiques plus simples que HybridArray).
  - Basé sur `std::vector<std::optional<T>>`.
  - Fournit : `get`, `get_ref`, `insert_at`, `emplace_at`, `erase`.
  - À utiliser si vous voulez un conteneur sparse autonome et simple.

- include/PackedArray.hpp
  - Variante dense/packée pour les itérations sensibles aux performances.
  - Disposition :
    - `components_` -- vecteur dense de composants.
    - `entities_` -- vecteur dense d'identifiants d'entités alignés avec components_.
    - `index_map_` -- mapping sparse id d'entité -> index dans les tableaux denses.
  - API : `insert(entity, component)` / `emplace(entity, ...)`, `erase(entity)` (swap-remove), `contains(entity)`, `index_of(entity)`, `size()` (nombre d'actifs).
  - Usage :
    - Favorisez PackedArray lorsque l'itération sur les composants actifs et la localité cache sont prioritaires.

- include/OptionalRef.hpp
  - `optional_ref<T>` -- wrapper léger de type optional pour références (conceptuellement comme `optional<T&>`).
  - Implémentation :
    - Contient un `T*` en interne.
    - Méthodes : `has_value()`, `operator bool()`, `value()`, `reset()`, `assign(T&)`, `value_or(...)`.
  - Usage :
    - Évite de copier de gros objets dans des tuples d'itérateurs ; retourne un petit proxy-référence.

- include/Zipper.hpp
  - Implémente un zipper indexé : itérer des indices à travers plusieurs stockages et obtenir `get(index)` de chaque stockage.
  - Comportement clé :
    - Utilise la méthode `get(index)` de chaque container ; `get` doit retourner un type optional-like ou un proxy.
    - L'itérateur renvoie `std::tuple<std::size_t, get_result_t<Containers>...>` -- le premier élément est l'indice.
    - `make_indexed_zipper(containers...)` construit le zipper.
  - Usage :
    - Aligne l'itération sur des stockages sparse sans construire de listes d'intersection explicites. Fonctionne bien avec `HybridArray::get()`.

- include/Systems.hpp
  - Implémentations de systèmes d'exemple :
    - `position_system` -- intègre la vélocité dans la position par frame.
    - `control_system` -- lit le clavier et met à jour `Velocity` pour les entités avec `Controllable`.
    - `make_draw_system(window)` -- fabrique un système qui dessine `Drawable` à `Position` via `sf::RenderWindow&`.
  - Notes :
    - Les systèmes obtiennent les stockages via `registry::get_components_if<T>()` et itèrent par indice en utilisant `get()`/`get_ref()` selon le besoin.

- include/Components.hpp
  - Types de composants de la démo :
    - `Position { float x, y; }`
    - `Velocity { float dx, dy; }`
    - `Drawable { float w, h; Color color; }`
    - `Controllable { float speed; }`
    - `Color { uint8_t r,g,b,a; }`
  - Ces types sont POD-like pour être peu coûteux à déplacer et stocker.

- include/Entity.hpp
  - Wrapper léger `Entity` contenant un id numérique.
  - Aides : `getId()` et opérateur de conversion vers `size_t`.
  - Construit par le `registry` et utilisé comme indice dans les stockages de composants.

## Notes de conception et justification

- Sécurité et simplicité : utiliser `std::vector<std::optional<T>>` évite les comportements indéfinis et donne des sémantiques claires pour les composants manquants.
- Double mode d'accès :
  - `get(id)` retourne une copie optionnelle pour l'itération en lecture seule (compatibilité zipper).
  - `get_ref(id)` retourne une référence vers `std::optional<T>` pour la mutation en place et l'insertion.
- Packé vs sparse :
  - `HybridArray` / `SparseArray` sont simples à raisonner et adaptés aux accès aléatoires par id d'entité.
  - `PackedArray` est fourni quand la performance d'itération sur actifs est requise.
- `optional_ref` réduit les copies dans les tuples d'itérateurs et les callbacks systèmes.

## Évolution de la démo

- Ajouter des composants :
  - Ajouter la struct dans include/Components.hpp (ou un nouveau header).
  - Appeler `reg.register_component<NewComp>()` dans main avant de créer des entités qui l'utilisent.
- Ajouter des systèmes :
  - Utiliser `registry::add_system<CompA, CompB>(fn)` où `fn` reçoit les références aux stockages du registry ou le registry lui-même selon la variante d'API.
  - Les systèmes peuvent itérer via le zipper ou interroger directement les stockages par id d'entité.
- Remplacer les stockages :
  - Implémentez les mêmes sémantiques `get(id)` / `get_ref(id)` pour un nouveau backend de stockage et enregistrez-le dans le registry à la place de `HybridArray`.

## Contribution

- Préférez des commits petits et bien documentés pour les changements de sémantique de stockage ou de comportement du registry.
- Ajoutez des tests unitaires ou de petits exemples dans src/ lors de l'introduction de nouvelles fonctionnalités.
- Si vous ajoutez une nouvelle politique de stockage, assurez-vous de la compatibilité avec `make_indexed_zipper` ou mettez à jour le zipper en conséquence.

## Dépannage

- Erreurs d'édition SFML : assurez-vous que les bibliothèques de développement SFML sont installées et que pkg-config est disponible ; vérifiez le Makefile pour les flags de linkage.
- Composants manquants inattendus : appelez `register_component<T>()` avant d'ajouter des composants dans main ou vérifiez que `get_components_if<T>()` n'est pas nullptr avant usage.
- Mouvement dépendant du framerate : la démo utilise une limite de frame fixe ; multipliez les vitesses par le delta time pour supporter un pas de temps variable.

## Licence

Aucun fichier LICENSE inclus.