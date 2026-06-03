#ifndef DUO_ROGUE_DATABASE_H
#define DUO_ROGUE_DATABASE_H

#include "Define.h"
#include <string>
#include <vector>

class Player;
class Creature;

namespace DuoRogue
{
struct CharacterState
{
    uint32 Guid = 0;
    uint32 AccountId = 0;
    uint32 RunId = 1;
    uint8 OriginRace = 0;
    std::string OriginName;
    bool HardcoreDead = false;
    uint8 DeathLevel = 0;
    uint32 DeathMap = 0;
    uint32 DeathZone = 0;
    uint32 DeathArea = 0;
};

struct AccountProgress
{
    uint32 AccountId = 0;
    uint32 Essence = 0;
    uint32 TotalRuns = 0;
    uint8 BestLevel = 1;
    uint32 TotalBossKills = 0;
    uint32 TotalDeaths = 0;
};

void EnsureAccountProgress(uint32 accountId);
void EnsureCharacterState(Player* player);
AccountProgress GetAccountProgress(uint32 accountId);
CharacterState GetCharacterState(uint32 guid);
void SetOrigin(uint32 guid, uint8 race, std::string const& originName);
void MarkHardcoreDead(Player* player);
void ClearDeadStateAndIncrementRun(Player* player);
void AddEssence(uint32 accountId, uint32 amount, std::string const& reason, Player* notify = nullptr);
bool SpendEssence(uint32 accountId, uint32 amount);
bool HasUnlock(uint32 accountId, std::string const& key);
void AddUnlock(uint32 accountId, std::string const& key);
std::vector<std::string> GetUnlocks(uint32 accountId, std::string const& prefix = "");
bool HasCharacterPerk(uint32 guid, std::string const& key);
void AddCharacterPerk(uint32 guid, std::string const& key, uint8 rank = 1);
std::vector<std::string> GetCharacterPerks(uint32 guid);
uint32 GetGuid(Player const* player);
uint32 GetAccountId(Player const* player);
}

#endif
