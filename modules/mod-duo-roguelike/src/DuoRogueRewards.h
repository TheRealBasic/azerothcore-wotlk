#ifndef DUO_ROGUE_REWARDS_H
#define DUO_ROGUE_REWARDS_H

#include "Define.h"
#include <string>
#include <vector>

class Player;
class Creature;
class Unit;

namespace DuoRogue
{
struct SpellPackage
{
    char const* Key;
    char const* Name;
    char const* Description;
    uint32 Cost;
    std::vector<uint32> Spells;
};

struct PerkDef
{
    char const* Key;
    char const* Name;
    char const* Description;
    uint32 Cost;
    uint8 Tier;
};

std::vector<SpellPackage> const& GetClassSouls();
std::vector<PerkDef> const& GetPerks();
SpellPackage const* FindSoul(std::string const& key);
PerkDef const* FindPerk(std::string const& key);
void TeachSoul(Player* player, SpellPackage const& soul);
void ApplyPermanentPerks(Player* player);
void ApplyStartingPerks(Player* player);
void AwardLevelMilestone(Player* player, uint8 oldLevel);
void AwardBossKill(Player* player, Creature* killed);
uint32 GetDeathPayout(uint8 level);
bool HasDuoValor(Player const* player);
void RerollOrigin(Player* player);
}

#endif
