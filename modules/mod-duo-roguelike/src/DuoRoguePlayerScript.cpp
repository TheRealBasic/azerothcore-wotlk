#include "DuoRogueConfig.h"
#include "DuoRogueDatabase.h"
#include "DuoRogueRewards.h"
#include "Chat.h"
#include "Creature.h"
#include "Map.h"
#include "Player.h"
#include "PlayerScript.h"
#include "Random.h"
#include "SharedDefines.h"
#include "WorldSession.h"
#include <vector>

namespace
{
struct OriginDef
{
    uint8 Race;
    char const* Name;
    TeamId Team;
    uint32 Map;
    float X;
    float Y;
    float Z;
    float O;
    uint32 Spell;
};

std::vector<OriginDef> const& Origins()
{
    static std::vector<OriginDef> const origins =
    {
        { RACE_HUMAN, "Human - Northshire", TEAM_ALLIANCE, 0, -8949.95f, -132.493f, 83.5312f, 0.0f, 20599 },
        { RACE_DWARF, "Dwarf/Gnome - Dun Morogh", TEAM_ALLIANCE, 0, -6240.32f, 331.033f, 382.758f, 0.0f, 20596 },
        { RACE_NIGHTELF, "Night Elf - Teldrassil", TEAM_ALLIANCE, 1, 10311.3f, 832.463f, 1326.41f, 0.0f, 20582 },
        { RACE_DRAENEI, "Draenei - Azuremyst", TEAM_ALLIANCE, 530, -3961.64f, -13931.2f, 100.615f, 0.0f, 28875 },
        { RACE_ORC, "Orc/Troll - Durotar", TEAM_HORDE, 1, -618.518f, -4251.67f, 38.718f, 0.0f, 20572 },
        { RACE_TAUREN, "Tauren - Mulgore", TEAM_HORDE, 1, -2917.58f, -257.98f, 52.9968f, 0.0f, 20550 },
        { RACE_UNDEAD_PLAYER, "Undead - Tirisfal", TEAM_HORDE, 0, 1676.35f, 1677.45f, 121.67f, 0.0f, 20577 },
        { RACE_BLOODELF, "Blood Elf - Eversong", TEAM_HORDE, 530, 10349.6f, -6357.29f, 33.4026f, 0.0f, 28730 }
    };
    return origins;
}

bool IsDungeonOrRaid(Player* player)
{
    return player && player->GetMap() && (player->GetMap()->IsDungeon() || player->GetMap()->IsRaid());
}

void TeleportSafeFallback(Player* player)
{
    if (!player)
        return;

    if (player->GetTeamId() == TEAM_HORDE)
        player->TeleportTo(1, -618.518f, -4251.67f, 38.718f, 0.0f);
    else
        player->TeleportTo(0, -8949.95f, -132.493f, 83.5312f, 0.0f);
}

void AssignRandomOrigin(Player* player, bool force)
{
    if (!DuoRogue::IsEnabled() || !DuoRogue::GetConfig().RandomOriginEnable || !player)
        return;

    DuoRogue::CharacterState state = DuoRogue::GetCharacterState(DuoRogue::GetGuid(player));
    if (state.OriginRace && !force)
        return;

    std::vector<OriginDef const*> available;
    for (OriginDef const& origin : Origins())
        if (DuoRogue::GetConfig().RandomOriginAllowFactionMismatch || origin.Team == player->GetTeamId())
            available.push_back(&origin);

    if (available.empty())
        return;

    OriginDef const* origin = available[urand(0, available.size() - 1)];
    DuoRogue::SetOrigin(DuoRogue::GetGuid(player), origin->Race, origin->Name);
    if (origin->Spell && !player->HasSpell(origin->Spell))
        player->learnSpell(origin->Spell, false);

    ChatHandler(player->GetSession()).PSendSysMessage("Random Origin assigned: %s. This is origin flavour, not a client-side race change.", origin->Name);
    player->TeleportTo(origin->Map, origin->X, origin->Y, origin->Z, origin->O);
    DuoRogue::ApplyStartingPerks(player);
}

void CheckForbiddenCity(Player* player)
{
    if (!DuoRogue::IsEnabled() || !DuoRogue::GetConfig().NoCitiesEnable || !player)
        return;

    if (!DuoRogue::IsForbiddenCity(player->GetZoneId()))
        return;

    if (DuoRogue::GetConfig().NoCitiesWarningMessage)
        ChatHandler(player->GetSession()).PSendSysMessage("Cities are forbidden in Duo Roguelike. You are being moved to safety.");

    if (DuoRogue::GetConfig().NoCitiesTeleportOut)
        TeleportSafeFallback(player);
}
}

class DuoRoguePlayerScript : public PlayerScript
{
public:
    DuoRoguePlayerScript() : PlayerScript("DuoRoguePlayerScript") { }

    void OnPlayerLogin(Player* player) override
    {
        if (!DuoRogue::IsEnabled())
            return;

        DuoRogue::EnsureCharacterState(player);
        DuoRogue::CharacterState state = DuoRogue::GetCharacterState(DuoRogue::GetGuid(player));
        if (state.HardcoreDead)
        {
            ChatHandler(player->GetSession()).PSendSysMessage("This run is dead. Visit The Run Keeper on another character or use a GM reset to begin again.");
            if (DuoRogue::GetConfig().HardcoreTeleportDeadCharacterToGraveyard)
                TeleportSafeFallback(player);
            return;
        }

        if (DuoRogue::GetConfig().RandomOriginForceOnFirstLogin && !state.OriginRace)
            AssignRandomOrigin(player, false);

        DuoRogue::ApplyPermanentPerks(player);
        CheckForbiddenCity(player);
    }

    void OnPlayerUpdateZone(Player* player, uint32 /*newZone*/, uint32 /*newArea*/) override
    {
        CheckForbiddenCity(player);
        if (DuoRogue::IsEnabled() && DuoRogue::GetConfig().DungeonSafeZonesEnable && IsDungeonOrRaid(player))
            ChatHandler(player->GetSession()).PSendSysMessage("Dungeon Safe Zone active: deaths here do not end your hardcore run.");
    }

    void OnPlayerGiveXP(Player* player, uint32& amount, Unit* /*victim*/, uint8 /*xpSource*/) override
    {
        if (!DuoRogue::IsEnabled() || !player)
            return;

        DuoRogue::CharacterState state = DuoRogue::GetCharacterState(DuoRogue::GetGuid(player));
        if (state.HardcoreDead && DuoRogue::GetConfig().HardcoreLockDeadCharacter)
        {
            amount = 0;
            return;
        }

        if (DuoRogue::GetConfig().XPMultiplier > 0.0f)
            amount = uint32(float(amount) * DuoRogue::GetConfig().XPMultiplier);
    }

    void OnPlayerJustDied(Player* player) override
    {
        if (!DuoRogue::IsEnabled() || !DuoRogue::GetConfig().HardcoreEnable || !player)
            return;

        if (DuoRogue::GetConfig().HardcoreSafeInDungeons && IsDungeonOrRaid(player))
        {
            ChatHandler(player->GetSession()).PSendSysMessage("Dungeon Safe Zone protected this death. Your hardcore run continues.");
            return;
        }

        DuoRogue::CharacterState state = DuoRogue::GetCharacterState(DuoRogue::GetGuid(player));
        if (state.HardcoreDead)
            return;

        DuoRogue::MarkHardcoreDead(player);
        if (DuoRogue::GetConfig().ProgressionEnable && DuoRogue::GetConfig().AwardOnRunDeath)
            DuoRogue::AddEssence(DuoRogue::GetAccountId(player), DuoRogue::GetDeathPayout(player->GetLevel()), "run death", player);
        ChatHandler(player->GetSession()).PSendSysMessage("Hardcore run ended. This character is now marked dead.");
    }

    void OnPlayerLevelChanged(Player* player, uint8 oldLevel) override
    {
        DuoRogue::AwardLevelMilestone(player, oldLevel);
    }

    void OnPlayerCreatureKill(Player* killer, Creature* killed) override
    {
        DuoRogue::AwardBossKill(killer, killed);
    }

    void OnPlayerAfterUpdateMaxHealth(Player* player, float& value) override
    {
        if (!DuoRogue::IsEnabled() || !player)
            return;

        uint32 bonus = 0;
        uint32 const guid = DuoRogue::GetGuid(player);
        if (DuoRogue::HasCharacterPerk(guid, "perk_health_5"))
            bonus += 5;
        if (DuoRogue::HasCharacterPerk(guid, "perk_health_10"))
            bonus += 10;
        if (DuoRogue::HasDuoValor(player))
            bonus += DuoRogue::GetConfig().DuoBuffStaminaPercent;
        if (bonus)
            value += value * float(bonus) / 100.0f;
    }
};

void AddDuoRoguePlayerScripts()
{
    new DuoRoguePlayerScript();
}

namespace DuoRogue
{
void RerollOrigin(Player* player)
{
    AssignRandomOrigin(player, true);
}
}
