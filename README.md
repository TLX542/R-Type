# My-Type -- Minimal ECS demo (SFML)

A small, self-contained Entity-Component-System (ECS) demo written in C++17.  
This repository demonstrates a compact registry, a conservative sparse-backed hybrid storage, an optional-reference proxy, and a tiny indexed zipper helper for iterating aligned component slots. The demo uses SFML to render a controllable entity and static drawables.

## Quick start (CMake)

Requirements:
- C++17 toolchain (g++, clang, or MSVC)
- CMake (>= 3.15 recommended)
- SFML (graphics, window, system) -- development package
- On Linux: build tools (make, ninja, etc.)
- On Windows: Visual Studio, or MSYS2/MinGW, or use vcpkg

Linux (example with out-of-tree build)
```sh
# install dependencies (Ubuntu example)
sudo apt update
sudo apt install -y build-essential cmake libsfml-dev

# configure and build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

# run
./build/src/bs-rtype
```

Windows (Visual Studio / vcpkg example)
- With vcpkg:
  1. Install SFML via vcpkg:
     - vcpkg install sfml:x64-windows
  2. Configure with vcpkg toolchain:
     - cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE=C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
     - cmake --build build --config Release
     - .\\build\\Release\\bs-rtype.exe

- Without vcpkg:
  - Download the SFML SDK for your compiler (MSVC / MinGW) and set SFML_DIR to the package's cmake folder, or set CMAKE_PREFIX_PATH to point to SFML before running cmake.

Notes:
- If SFML is installed in a custom location, set -DSFML_DIR=/path/to/SFML to help find_package(SFML ...) locate it.
- If you want static SFML on Windows, pass -DBUILD_SHARED_SFML=OFF; you may also need to link SFML dependencies manually (see comments in src/CMakeLists.txt).

## High-level architecture

- `registry` (in include/Registry.hpp) orchestrates component storages and systems.
- Components are plain POD structs in include/Components.hpp.
- Default storage is `HybridArray` (include/HybridArray.hpp) -- a sparse-backed container implemented over `std::vector<std::optional<T>>`.
- Systems are simple callables registered with the registry; example systems are in include/Systems.hpp.
- Indexed zipper helper (include/Zipper.hpp) allows aligned iteration across different storages by index.

## Runtime data-flow diagram

This ASCII diagram shows the runtime flow from created entities through storages, the zipper that aligns indices, and finally into systems that consume component views.

Legend:
- [E#] = Entity id
- (S) = Storage slot (may hold std::optional<Component>)
- Z = Zipper producing tuples (index, get(...) results)
- -> = data/control flow
- [opt] = optional-like value (copy) returned by get()
- [&] = reference-proxy returned by get_ref() / optional_ref

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
        \        /     | 1:(S) -> Velocity{dx}  |  get(i) -> |  |  | zipper aligns index |
         +------+      | 2:std::nullopt         |  returns   |  |  |  and yields tuples  |
        |Registry|     +------------------------+   [opt]    v  v  v                     |
         +------+     +-------------------------+                                   mutates / reads
        /    \        | Drawable (HybridArray)  |    tuple -> (idx, pos_opt, vel_opt, draw_opt)
       /      \       | 0:std::nullopt          |             system unpacks and acts
      /        \      | 2:(S) -> Drawable{w,h}  |
  spawn()   add_component(...)  ...             |
                                                  Example zipper output:
                                                  (2, Position{...}, std::nullopt, Drawable{...})
```

Notes on the flow:
- Entities are numeric ids managed by registry.spawn_entity().
- Storages expose two access patterns:
  - get(id) -> returns an optional-like copy ([opt]) suitable for zipper consumption and read-only checks.
  - get_ref(id) -> returns a reference-proxy ([&]) to std::optional<T> for in-place mutation or insertion.
- The indexed zipper iterates indices in a deterministic range (usually up to the largest storage size) and calls get(index) on each storage. It yields tuples containing the index and each storage's get result.
- Systems can:
  - Read optional copies produced by zipper and act when values are present.
  - Or obtain storage references from registry and call get_ref(id) to mutate component slots (e.g., control_system writing Velocity).
- The draw system typically reads Position + Drawable and issues SFML draw calls; position_system reads Velocity + Position and mutates Position via get_ref.



## File-by-file detailed summary

This section documents each public header and main source file so you can quickly locate responsibilities and important API surfaces.

- src/main.cpp
  - Program entrypoint and demo setup.
  - Creates an `sf::RenderWindow`, a `registry` instance, registers component storages, spawns entities, and registers systems.
  - Registers basic systems:
    - Control system -- reads input and updates `Velocity`.
    - Position system -- integrates `Velocity` into `Position`.
    - Draw system -- renders `Drawable` at `Position`.

- include/Registry.hpp
  - Central orchestration type: `registry`.
  - Responsibilities:
    - Register and store component storages keyed by `std::type_index`.
    - Provide component accessors:
      - `register_component<T>()` -- create and store a storage if missing.
      - `get_components<T>()` / `get_components_if<T>()` -- obtain storage references or nullptr.
    - Entity lifecycle:
      - `spawn_entity()` -- returns an `Entity` wrapper holding an id.
      - `kill_entity(Entity)` -- erases components for that entity and recycles id.
    - Component operations:
      - `add_component(Entity, T)` / `emplace_component(...)` -- add or replace components.
      - `remove_component<Entity, T>()`.
    - Systems:
      - `add_system<Comps...>(fn)` -- register callables to run each frame.
      - `run_systems()` -- invoke registered systems in insertion order.
  - Notes:
    - The internal erase map and callback mechanics make `kill_entity` remove matching slots across registered storages.
    - The registry is intentionally minimal and designed to accept different storage backends that implement the expected `get(id)` / `get_ref(id)` semantics.

- include/HybridArray.hpp
  - Sparse-backed storage implemented primarily as `std::vector<std::optional<T>>`.
  - API highlights:
    - `insert_at(id, value)` / `emplace_at(id, ...)` -- insert component at entity index; ensures capacity.
    - `get(id) const` -- returns a copy of `std::optional<T>`; zipper-friendly.
    - `get_ref(id)` -- returns `std::optional<T>&` to mutate in place; grows storage as needed.
    - `erase(id)` -- set slot to `std::nullopt` (does not shrink capacity).
    - `size()` -- returns the current sparse capacity (max entity id + 1), used for zipper alignment.
  - Rationale:
    - Keeps semantics simple and safe: out-of-range reads are handled via `get()` returning `std::nullopt`, and `get_ref()` explicitly grows for mutation.
    - Intended to be swapped with a packed/dense alternative without changing registry interface.

- include/SparseArray.hpp
  - Lightweight utility sparse array wrapper (simpler naming and semantics than HybridArray).
  - Backed by `std::vector<std::optional<T>>`.
  - Provides:
    - `get`, `get_ref`, `insert_at`, `emplace_at`, `erase`.
  - Use when you want a small standalone sparse container without hybrid/policy hooks.

- include/PackedArray.hpp
  - Dense/packed storage variant for performance-sensitive iteration.
  - Layout:
    - `components_` -- dense vector of components.
    - `entities_` -- dense vector of entity ids mapped one-to-one to components_.
    - `index_map_` -- sparse mapping entity id -> index into dense arrays.
  - API highlights:
    - `insert(entity, component)` / `emplace(entity, ...)` -- add or replace.
    - `erase(entity)` -- swap-remove to keep arrays dense.
    - `contains(entity)` / `index_of(entity)`.
    - `size()` returns number of active components (dense count).
  - Rationale:
    - Use PackedArray when iteration over active components and cache locality are priorities.

- include/OptionalRef.hpp
  - `optional_ref<T>` -- a lightweight optional-like wrapper for references (conceptually like `optional<T&>`).
  - Implementation:
    - Stores a `T*` internally.
    - Methods: `has_value()`, `operator bool()`, `value()`, `reset()`, `assign(T&)`, `value_or(...)`.
  - Use:
    - Avoid copying large objects in iterator tuples; return a small reference proxy instead.

- include/Zipper.hpp
  - Implements an indexed zipper: iterate indices across multiple storages and obtain each storage's `get(index)` result.
  - Key behavior:
    - Uses each container's `get(index)` method; `get` is expected to return an optional-like type or proxy.
    - Iterator yields `std::tuple<std::size_t, get_result_t<Containers>...>` -- the first element is the index.
    - `make_indexed_zipper(containers...)` constructs the zipper.
  - Use:
    - Aligns iteration across sparse storages without building explicit intersection lists. Works well with `HybridArray::get()`.

- include/Systems.hpp
  - Example system implementations used in the demo:
    - `position_system` -- integrates velocity into position per-frame.
    - `control_system` -- reads keyboard and updates `Velocity` for entities with `Controllable`.
    - `make_draw_system(window)` -- factory returning a system that draws `Drawable` at `Position` using an `sf::RenderWindow&`.
  - Notes:
    - Systems obtain storages via `registry::get_components_if<T>()` and iterate by index, using `get()`/`get_ref()` as needed.

- include/Components.hpp
  - Demo component types:
    - `Position { float x, y; }`
    - `Velocity { float dx, dy; }`
    - `Drawable { float w, h; Color color; }`
    - `Controllable { float speed; }`
    - `Color { uint8_t r,g,b,a; }`
  - These are POD-like types so they are cheap to move and store.

- include/Entity.hpp
  - `Entity` lightweight wrapper that holds a numeric id.
  - Simple helpers: `getId()` and conversion operator to `size_t`.
  - Constructed by `registry` and used as index into component storages.

## Design notes and rationale

- Safety and simplicity: sparse `std::vector<std::optional<T>>` avoids undefined behavior and provides clear semantics for missing components.
- Dual access pattern:
  - `get(id)` returns an optional copy for safe read-only iteration (zipper compatibility).
  - `get_ref(id)` returns a reference to `std::optional<T>` for in-place mutation and insertion.
- Packed vs sparse:
  - `HybridArray` / `SparseArray` are simple to reason about and good for random access by entity id.
  - `PackedArray` is included when iteration performance over active components is required.
- `optional_ref` reduces copying in iterator tuples and system callbacks.

## Extending the demo

- Add new components:
  - Add the struct in include/Components.hpp (or a new header).
  - Call `reg.register_component<NewComp>()` in main before spawning entities that use it.
- Add systems:
  - Use `registry::add_system<CompA, CompB>(fn)` where `fn` receives references to the registries' storage objects or the registry itself depending on your registration API variant.
  - Systems may iterate aligned indices via the zipper or directly query storages by entity id.
- Swap storages:
  - Implement the same `get(id)` / `get_ref(id)` semantics for a new storage backend and register it in the registry in place of `HybridArray`.

## Contributing

- Prefer small, well-documented commits for changes to storage semantics or registry behavior.
- Add unit tests or small example programs under src/ when introducing new features.
- If adding a new storage policy, ensure compatibility with `make_indexed_zipper` or update the zipper implementation accordingly.

## Troubleshooting

- SFML link errors: ensure SFML development libraries are installed and pkg-config is available; inspect the Makefile for link flags.
- Unexpected missing components: call `register_component<T>()` before adding components in main or ensure `get_components_if<T>()` is checked for nullptr before use.
- Frame-rate dependent movement: the demo uses a fixed frame limit; multiply velocities by delta time when adapting to variable time steps.

## License

No LICENSE file included.
