#include "DuoRogueConfig.h"
#include "Config.h"
#include "Log.h"
#include "ScriptMgr.h"
#include "WorldScript.h"
#include <sstream>

namespace DuoRogue
{
namespace
{
Config g_Config;

std::unordered_set<uint32> ParseZoneList(std::string const& value)
{
    std::unordered_set<uint32> zones;
    std::stringstream stream(value);
    std::string token;
    while (std::getline(stream, token, ','))
    {
        try
        {
            if (!token.empty())
                zones.insert(static_cast<uint32>(std::stoul(token)));
        }
        catch (...)
        {
            LOG_ERROR("module.duorogue", "Invalid zone id '{}' in DuoRogue.NoCities.ZoneIds", token);
        }
    }
    return zones;
}
}

Config const& GetConfig() { return g_Config; }
bool IsEnabled() { return g_Config.Enable; }
bool IsForbiddenCity(uint32 zoneId) { return g_Config.NoCitiesZoneIds.find(zoneId) != g_Config.NoCitiesZoneIds.end(); }

void LoadConfig()
{
    g_Config.Enable = sConfigMgr->GetOption<bool>("DuoRogue.Enable", true);
    g_Config.XPMultiplier = sConfigMgr->GetOption<float>("DuoRogue.XPMultiplier", 3.0f);
    g_Config.HardcoreEnable = sConfigMgr->GetOption<bool>("DuoRogue.Hardcore.Enable", true);
    g_Config.HardcoreSafeInDungeons = sConfigMgr->GetOption<bool>("DuoRogue.Hardcore.SafeInDungeons", true);
    g_Config.HardcoreDeleteCharacter = sConfigMgr->GetOption<bool>("DuoRogue.Hardcore.DeleteCharacter", false);
    g_Config.HardcoreLockDeadCharacter = sConfigMgr->GetOption<bool>("DuoRogue.Hardcore.LockDeadCharacter", true);
    g_Config.HardcoreTeleportDeadCharacterToGraveyard = sConfigMgr->GetOption<bool>("DuoRogue.Hardcore.TeleportDeadCharacterToGraveyard", true);
    g_Config.RandomOriginEnable = sConfigMgr->GetOption<bool>("DuoRogue.RandomOrigin.Enable", true);
    g_Config.RandomOriginForceOnFirstLogin = sConfigMgr->GetOption<bool>("DuoRogue.RandomOrigin.ForceOnFirstLogin", true);
    g_Config.RandomOriginAllowFactionMismatch = sConfigMgr->GetOption<bool>("DuoRogue.RandomOrigin.AllowFactionMismatch", false);
    g_Config.NoCitiesEnable = sConfigMgr->GetOption<bool>("DuoRogue.NoCities.Enable", true);
    g_Config.NoCitiesTeleportOut = sConfigMgr->GetOption<bool>("DuoRogue.NoCities.TeleportOut", true);
    g_Config.NoCitiesWarningMessage = sConfigMgr->GetOption<bool>("DuoRogue.NoCities.WarningMessage", true);
    g_Config.NoCitiesZoneIds = ParseZoneList(sConfigMgr->GetOption<std::string>("DuoRogue.NoCities.ZoneIds", "1519,1537,1657,3557,1637,1638,1497,3487,3703,4395"));
    g_Config.DungeonSafeZonesEnable = sConfigMgr->GetOption<bool>("DuoRogue.DungeonSafeZones.Enable", true);
    g_Config.ProgressionEnable = sConfigMgr->GetOption<bool>("DuoRogue.Progression.Enable", true);
    g_Config.CurrencyName = sConfigMgr->GetOption<std::string>("DuoRogue.Progression.CurrencyName", "Essence");
    g_Config.AwardOnLevelMilestone = sConfigMgr->GetOption<bool>("DuoRogue.Progression.AwardOnLevelMilestone", true);
    g_Config.AwardOnDungeonBossKill = sConfigMgr->GetOption<bool>("DuoRogue.Progression.AwardOnDungeonBossKill", true);
    g_Config.AwardOnRunDeath = sConfigMgr->GetOption<bool>("DuoRogue.Progression.AwardOnRunDeath", true);
    g_Config.NpcEntry = sConfigMgr->GetOption<uint32>("DuoRogue.NpcEntry", 900001);
    g_Config.NpcName = sConfigMgr->GetOption<std::string>("DuoRogue.NpcName", "The Run Keeper");
    g_Config.DuoBuffEnable = sConfigMgr->GetOption<bool>("DuoRogue.DuoBuff.Enable", true);
    g_Config.DuoBuffMaxPlayers = sConfigMgr->GetOption<uint32>("DuoRogue.DuoBuff.MaxPlayers", 2);
    g_Config.DuoBuffOnlyInDungeonsAndRaids = sConfigMgr->GetOption<bool>("DuoRogue.DuoBuff.OnlyInDungeonsAndRaids", true);
    g_Config.DuoBuffDamagePercent = sConfigMgr->GetOption<uint32>("DuoRogue.DuoBuff.DamagePercent", 25);
    g_Config.DuoBuffHealingPercent = sConfigMgr->GetOption<uint32>("DuoRogue.DuoBuff.HealingPercent", 25);
    g_Config.DuoBuffStaminaPercent = sConfigMgr->GetOption<uint32>("DuoRogue.DuoBuff.StaminaPercent", 35);
    g_Config.PeaceModeEnable = sConfigMgr->GetOption<bool>("DuoRogue.PeaceMode.Enable", true);
    g_Config.PeaceModeDisableOpenWorldPvP = sConfigMgr->GetOption<bool>("DuoRogue.PeaceMode.DisableOpenWorldPvP", true);
    g_Config.PeaceModeRemovePvPFlag = sConfigMgr->GetOption<bool>("DuoRogue.PeaceMode.RemovePvPFlag", true);
    g_Config.PeaceModeDisableFFA = sConfigMgr->GetOption<bool>("DuoRogue.PeaceMode.DisableFFA", true);
    g_Config.PeaceModeAllowDuels = sConfigMgr->GetOption<bool>("DuoRogue.PeaceMode.AllowDuels", true);
    g_Config.PeaceModeAllowDeathDuels = sConfigMgr->GetOption<bool>("DuoRogue.PeaceMode.AllowDeathDuels", true);
    g_Config.PeaceModeMakeFactionGuardsNeutral = sConfigMgr->GetOption<bool>("DuoRogue.PeaceMode.MakeFactionGuardsNeutral", true);
    g_Config.PeaceModeMakeCityFactionNPCsNeutral = sConfigMgr->GetOption<bool>("DuoRogue.PeaceMode.MakeCityFactionNPCsNeutral", true);
    g_Config.DeathDuelEnable = sConfigMgr->GetOption<bool>("DuoRogue.DeathDuel.Enable", true);
    g_Config.DeathDuelRequireBothPlayersConfirm = sConfigMgr->GetOption<bool>("DuoRogue.DeathDuel.RequireBothPlayersConfirm", true);
    g_Config.DeathDuelMarkHardcoreDeath = sConfigMgr->GetOption<bool>("DuoRogue.DeathDuel.MarkHardcoreDeath", true);
    g_Config.DeathDuelAllowInsideDungeons = sConfigMgr->GetOption<bool>("DuoRogue.DeathDuel.AllowInsideDungeons", false);
    g_Config.DeathDuelAnnounceStart = sConfigMgr->GetOption<bool>("DuoRogue.DeathDuel.AnnounceStart", true);
    g_Config.DeathDuelAnnounceWinner = sConfigMgr->GetOption<bool>("DuoRogue.DeathDuel.AnnounceWinner", true);
    g_Config.Debug = sConfigMgr->GetOption<bool>("DuoRogue.Debug", false);

    LOG_INFO("module.duorogue", "Duo Roguelike module {}", g_Config.Enable ? "enabled" : "disabled");
}
}

class DuoRogueWorldScript : public WorldScript
{
public:
    DuoRogueWorldScript() : WorldScript("DuoRogueWorldScript") { }
    void OnAfterConfigLoad(bool /*reload*/) override { DuoRogue::LoadConfig(); }
};

void AddDuoRogueConfigScripts()
{
    new DuoRogueWorldScript();
}
