#include "DuoRogueConfig.h"
#include "DuoRogueDeathDuel.h"
#include "DuoRogueDatabase.h"
#include "Chat.h"
#include "Player.h"
#include "PlayerScript.h"
#include "SharedDefines.h"
#include "UnitDefines.h"
#include "WorldSession.h"
#include <unordered_map>

namespace
{
uint32 constexpr PeaceModeUpdateIntervalMs = 3000;
char constexpr PeaceModeMessage[] = "Peace Mode is active. Open-world PvP is disabled.";

bool ShouldApplyPeaceMode(Player* player)
{
    return DuoRogue::IsEnabled() && player && DuoRogue::GetConfig().PeaceModeEnable;
}

void RemovePeaceModeFlags(Player* player, bool notify)
{
    if (!ShouldApplyPeaceMode(player))
        return;

    if (DuoRogue::DeathDuel::IsActive(player))
        return;

    DuoRogue::Config const& config = DuoRogue::GetConfig();
    bool changed = false;

    if (config.PeaceModeRemovePvPFlag && player->IsPvP())
    {
        player->SetPvP(false);
        player->RemovePlayerFlag(PLAYER_FLAGS_PVP_TIMER);
        player->RemovePlayerFlag(PLAYER_FLAGS_CONTESTED_PVP);
        changed = true;
    }

    if (config.PeaceModeDisableFFA && player->IsFFAPvP())
    {
        player->RemoveByteFlag(UNIT_FIELD_BYTES_2, 1, UNIT_BYTE2_FLAG_FFA_PVP);
        changed = true;
    }

    if (changed && notify && player->GetSession())
        ChatHandler(player->GetSession()).PSendSysMessage("%s", PeaceModeMessage);
}
}

class DuoRoguePeaceModePlayerScript : public PlayerScript
{
public:
    DuoRoguePeaceModePlayerScript() : PlayerScript("DuoRoguePeaceModePlayerScript") { }

    void OnPlayerLogin(Player* player) override
    {
        RemovePeaceModeFlags(player, false);
    }

    void OnPlayerUpdateZone(Player* player, uint32 /*newZone*/, uint32 /*newArea*/) override
    {
        RemovePeaceModeFlags(player, true);
    }

    void OnPlayerMapChanged(Player* player) override
    {
        RemovePeaceModeFlags(player, true);
    }

    void OnPlayerPVPFlagChange(Player* player, bool state) override
    {
        if (state)
            RemovePeaceModeFlags(player, true);
    }

    void OnPlayerFfaPvpStateUpdate(Player* player, bool state) override
    {
        if (state)
            RemovePeaceModeFlags(player, true);
    }

    void OnPlayerIsPvP(Player* player, bool& result) override
    {
        if (!ShouldApplyPeaceMode(player) || !DuoRogue::GetConfig().PeaceModeDisableOpenWorldPvP)
            return;

        if (DuoRogue::DeathDuel::IsActive(player))
        {
            result = true;
            return;
        }

        result = false;
    }

    void OnPlayerIsFFAPvP(Player* player, bool& result) override
    {
        if (ShouldApplyPeaceMode(player) && DuoRogue::GetConfig().PeaceModeDisableFFA)
            result = false;
    }

    void OnPlayerUpdate(Player* player, uint32 diff) override
    {
        if (!ShouldApplyPeaceMode(player))
            return;

        uint32 const guid = DuoRogue::GetGuid(player);
        uint32& timer = _timers[guid];
        if (timer > diff)
        {
            timer -= diff;
            return;
        }

        timer = PeaceModeUpdateIntervalMs;
        RemovePeaceModeFlags(player, true);
    }

    void OnPlayerLogout(Player* player) override
    {
        _timers.erase(DuoRogue::GetGuid(player));
    }

private:
    std::unordered_map<uint32, uint32> _timers;
};

void AddDuoRoguePeaceModeScripts()
{
    new DuoRoguePeaceModePlayerScript();
}
