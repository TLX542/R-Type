#pragma once

#include "Protocol.hpp"
#include <cstdint>

/**
 * @file NetworkECS.hpp
 * @brief Fichier pour l'intégration réseau avec votre ECS
 *
 * TODO: Incluez ici votre header ECS
 * #include "votre_ecs.hpp"
 */

namespace NetworkECS {

/**
 * @brief Traiter un input joueur et l'appliquer à l'ECS
 *
 * TODO: Implémentez cette fonction pour appliquer les inputs à votre ECS
 *
 * @param registry Votre registry ECS
 * @param playerId ID du joueur (1-4)
 * @param moveX Direction horizontale (-1, 0, 1)
 * @param moveY Direction verticale (-1, 0, 1)
 * @param buttons Boutons pressés (BTN_SHOOT, BTN_SPECIAL)
 * @param timestamp Timestamp du client
 *
 * Exemple d'implémentation :
 *
 *   // Trouver l'entité du joueur
 *   auto playerEntity = findPlayerByID(registry, playerId);
 *
 *   // Appliquer la vélocité
 *   auto& velocity = registry.get_component<Velocity>(playerEntity);
 *   velocity.vx = moveX * PLAYER_SPEED;
 *   velocity.vy = moveY * PLAYER_SPEED;
 *
 *   // Gérer les boutons
 *   if (buttons & BTN_SHOOT) {
 *       spawnBullet(registry, playerEntity);
 *   }
 */
void handlePlayerInput(
    /* YourRegistry& registry, */
    uint8_t playerId,
    int8_t moveX,
    int8_t moveY,
    uint8_t buttons,
    uint32_t timestamp
);

/**
 * @brief Créer un paquet ENTITY_SPAWN à partir d'une entité ECS
 *
 * TODO: Implémentez pour sérialiser une entité ECS en paquet réseau
 *
 * @param registry Votre registry ECS
 * @param entity L'entité à sérialiser
 * @param networkId ID réseau unique de l'entité
 * @return Le payload prêt à être envoyé
 *
 * Exemple d'implémentation :
 *
 *   auto& pos = registry.get_component<Position>(entity);
 *   auto& vel = registry.get_component<Velocity>(entity);
 *   auto& health = registry.get_component<Health>(entity);
 *   auto& type = registry.get_component<EntityType>(entity);
 *
 *   EntitySpawnPayload payload;
 *   payload.networkId = networkId;
 *   payload.entityType = type.value;
 *   payload.posX = pos.x;
 *   payload.posY = pos.y;
 *   payload.velocityX = vel.vx;
 *   payload.velocityY = vel.vy;
 *   payload.health = health.current;
 *   return payload;
 */
EntitySpawnPayload serializeEntitySpawn(
    /* YourRegistry& registry, */
    /* YourEntity entity, */
    uint32_t networkId
);

/**
 * @brief Créer un paquet ENTITY_UPDATE à partir d'une entité ECS
 *
 * TODO: Implémentez pour sérialiser les updates d'entité
 */
EntityUpdatePayload serializeEntityUpdate(
    /* YourRegistry& registry, */
    /* YourEntity entity, */
    uint32_t networkId
);

/**
 * @brief Récupérer toutes les entités à synchroniser
 *
 * TODO: Implémentez pour récupérer les entités qui doivent être envoyées au réseau
 *
 * @param registry Votre registry ECS
 * @return Liste des entités à synchroniser
 *
 * Exemple :
 *   // Récupérer toutes les entités avec un component Networked
 *   return registry.view<Networked, Position>();
 */
/* std::vector<YourEntity> getEntitiesToSync(YourRegistry& registry); */

} // namespace NetworkECS
