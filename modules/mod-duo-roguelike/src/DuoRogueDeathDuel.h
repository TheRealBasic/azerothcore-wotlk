#ifndef DUO_ROGUE_DEATH_DUEL_H
#define DUO_ROGUE_DEATH_DUEL_H

#include "Define.h"

class Creature;
class Player;
class Unit;

namespace DuoRogue::DeathDuel
{
uint32 constexpr ActionChallenge = 100500;
uint32 constexpr ActionAccept = 100501;
uint32 constexpr ActionCancel = 100502;
uint32 constexpr ActionInfo = 100503;

bool IsFeatureAvailable();
bool HasPendingFor(Player const* player);
bool HasPendingFrom(Player const* player);
bool IsActive(Player const* player);
bool IsActiveBetween(Player const* first, Player const* second);
bool IsAllowedPlayerDamage(Unit* attacker, Unit* victim);
void AddGossipOptions(Player* player);
bool HandleGossipAction(Player* player, Creature* creature, uint32 action);
void HandlePlayerDeath(Player* deadPlayer);
void HandlePlayerLogout(Player* player);
void ValidatePlayerState(Player* player);
}

#endif
