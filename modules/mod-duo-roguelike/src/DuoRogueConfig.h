#ifndef DUO_ROGUE_CONFIG_H
#define DUO_ROGUE_CONFIG_H

#include "Define.h"
#include <string>
#include <unordered_set>

namespace DuoRogue
{
struct Config
{
    bool Enable = true;
    float XPMultiplier = 3.0f;
    bool HardcoreEnable = true;
    bool HardcoreSafeInDungeons = true;
    bool HardcoreDeleteCharacter = false;
    bool HardcoreLockDeadCharacter = true;
    bool HardcoreTeleportDeadCharacterToGraveyard = true;
    bool RandomOriginEnable = true;
    bool RandomOriginForceOnFirstLogin = true;
    bool RandomOriginAllowFactionMismatch = false;
    bool NoCitiesEnable = true;
    bool NoCitiesTeleportOut = true;
    bool NoCitiesWarningMessage = true;
    std::unordered_set<uint32> NoCitiesZoneIds;
    bool DungeonSafeZonesEnable = true;
    bool ProgressionEnable = true;
    std::string CurrencyName = "Essence";
    bool AwardOnLevelMilestone = true;
    bool AwardOnDungeonBossKill = true;
    bool AwardOnRunDeath = true;
    uint32 NpcEntry = 900001;
    std::string NpcName = "The Run Keeper";
    bool DuoBuffEnable = true;
    uint32 DuoBuffMaxPlayers = 2;
    bool DuoBuffOnlyInDungeonsAndRaids = true;
    uint32 DuoBuffDamagePercent = 25;
    uint32 DuoBuffHealingPercent = 25;
    uint32 DuoBuffStaminaPercent = 35;
    bool PeaceModeEnable = true;
    bool PeaceModeDisableOpenWorldPvP = true;
    bool PeaceModeRemovePvPFlag = true;
    bool PeaceModeDisableFFA = true;
    bool PeaceModeAllowDuels = true;
    bool PeaceModeAllowDeathDuels = true;
    bool PeaceModeMakeFactionGuardsNeutral = true;
    bool PeaceModeMakeCityFactionNPCsNeutral = true;
    bool DeathDuelEnable = true;
    bool DeathDuelRequireBothPlayersConfirm = true;
    bool DeathDuelMarkHardcoreDeath = true;
    bool DeathDuelAllowInsideDungeons = false;
    bool DeathDuelAnnounceStart = true;
    bool DeathDuelAnnounceWinner = true;
    bool Debug = false;
};

Config const& GetConfig();
void LoadConfig();
bool IsEnabled();
bool IsForbiddenCity(uint32 zoneId);
}

#endif
