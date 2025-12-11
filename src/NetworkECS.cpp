#include "../include/NetworkECS.hpp"

namespace NetworkECS {

void handlePlayerInput(
    uint8_t playerId,
    int8_t moveX,
    int8_t moveY,
    uint8_t buttons,
    uint32_t timestamp
) {
    // TODO: Implémentez ici
    // 1. Trouver l'entité du joueur avec playerId
    // 2. Appliquer moveX/moveY à la vélocité
    // 3. Gérer les boutons (SHOOT, SPECIAL)
}

EntitySpawnPayload serializeEntitySpawn(
    uint32_t networkId
) {
    // TODO: Implémentez ici
    EntitySpawnPayload payload;
    // Remplissez le payload avec les données de votre entité ECS
    return payload;
}

EntityUpdatePayload serializeEntityUpdate(
    uint32_t networkId
) {
    // TODO: Implémentez ici
    EntityUpdatePayload payload;
    // Remplissez le payload avec les données mises à jour
    return payload;
}

} // namespace NetworkECS
