#include "DuoRogueDeathDuel.h"
#include "DuoRogueConfig.h"
#include "DuoRogueDatabase.h"
#include "CellImpl.h"
#include "Chat.h"
#include "Creature.h"
#include "GridNotifiers.h"
#include "Map.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "PlayerScript.h"
#include "ScriptedGossip.h"
#include "Unit.h"
#include "UnitScript.h"
#include "WorldSession.h"
#include "WorldSessionMgr.h"
#include <algorithm>
#include <string>
#include <unordered_map>

namespace DuoRogue::DeathDuel
{
namespace
{
float constexpr ChallengeRange = 30.0f;
float constexpr ActiveRange = 120.0f;
uint32 constexpr DeathDuelWinnerEssence = 5;
uint32 constexpr ValidationIntervalMs = 1000;

struct PendingChallenge
{
    uint32 ChallengerGuid = 0;
    uint32 TargetGuid = 0;
};

struct ActiveDuel
{
    uint32 FirstGuid = 0;
    uint32 SecondGuid = 0;
};

std::unordered_map<uint32, PendingChallenge> g_PendingByTarget;
std::unordered_map<uint32, ActiveDuel> g_ActiveByPlayer;
uint32 Guid(Player const* player)
{
    return player ? DuoRogue::GetGuid(player) : 0;
}

Player* FindPlayer(uint32 guid)
{
    return guid ? ObjectAccessor::FindPlayer(ObjectGuid::Create<HighGuid::Player>(guid)) : nullptr;
}

std::string PlayerName(uint32 guid)
{
    if (Player* player = FindPlayer(guid))
        return player->GetName();
    return "Unknown";
}

bool IsDungeonOrRaid(Player const* player)
{
    return player && player->GetMap() && (player->GetMap()->IsDungeon() || player->GetMap()->IsRaid());
}

bool IsHardcoreDead(Player* player)
{
    DuoRogue::EnsureCharacterState(player);
    return DuoRogue::GetCharacterState(Guid(player)).HardcoreDead;
}

void Send(Player* player, char const* message)
{
    if (player && player->GetSession())
        ChatHandler(player->GetSession()).PSendSysMessage("%s", message);
}

void Announce(std::string const& message)
{
    sWorldSessionMgr->SendServerMessage(SERVER_MSG_STRING, message);
}

Player* FindChallengeTarget(Player* challenger)
{
    if (!challenger)
        return nullptr;

    if (Unit* selected = challenger->GetSelectedUnit())
        if (Player* target = selected->ToPlayer())
            if (target != challenger && target->IsInWorld() && target->IsAlive() && challenger->GetMapId() == target->GetMapId() && challenger->GetDistance(target) <= ChallengeRange)
                return target;

    std::list<Player*> players;
    Acore::AnyPlayerInObjectRangeCheck checker(challenger, ChallengeRange, true, true);
    Acore::PlayerListSearcher<Acore::AnyPlayerInObjectRangeCheck> searcher(challenger, players, checker);
    Cell::VisitObjects(challenger, searcher, ChallengeRange);

    Player* nearest = nullptr;
    float nearestDistance = ChallengeRange + 1.0f;
    for (Player* candidate : players)
    {
        if (!candidate || candidate == challenger)
            continue;

        float const distance = challenger->GetDistance(candidate);
        if (distance < nearestDistance)
        {
            nearest = candidate;
            nearestDistance = distance;
        }
    }

    return nearest;
}

bool CanUseDeathDuel(Player* player, bool notify)
{
    if (!DuoRogue::IsEnabled() || !IsFeatureAvailable() || !player)
        return false;

    if (IsHardcoreDead(player))
    {
        if (notify)
            Send(player, "Dead hardcore runs cannot start or accept Death Duels.");
        return false;
    }

    if (!DuoRogue::GetConfig().DeathDuelAllowInsideDungeons && IsDungeonOrRaid(player))
    {
        if (notify)
            Send(player, "Death Duels are disabled inside dungeons and raids.");
        return false;
    }

    return true;
}

void ErasePendingInvolving(uint32 guid)
{
    for (auto itr = g_PendingByTarget.begin(); itr != g_PendingByTarget.end();)
    {
        if (itr->second.ChallengerGuid == guid || itr->second.TargetGuid == guid)
            itr = g_PendingByTarget.erase(itr);
        else
            ++itr;
    }
}

void CancelActive(uint32 guid, char const* reason)
{
    auto itr = g_ActiveByPlayer.find(guid);
    if (itr == g_ActiveByPlayer.end())
        return;

    ActiveDuel duel = itr->second;
    g_ActiveByPlayer.erase(duel.FirstGuid);
    g_ActiveByPlayer.erase(duel.SecondGuid);

    if (Player* first = FindPlayer(duel.FirstGuid))
    {
        first->SetPvP(false);
        Send(first, reason);
    }
    if (Player* second = FindPlayer(duel.SecondGuid))
    {
        second->SetPvP(false);
        Send(second, reason);
    }
}

void CancelAllFor(uint32 guid, char const* reason)
{
    ErasePendingInvolving(guid);
    CancelActive(guid, reason);
}

void StartDuel(Player* challenger, Player* target)
{
    uint32 const challengerGuid = Guid(challenger);
    uint32 const targetGuid = Guid(target);

    g_PendingByTarget.erase(targetGuid);
    ActiveDuel duel{ challengerGuid, targetGuid };
    g_ActiveByPlayer[challengerGuid] = duel;
    g_ActiveByPlayer[targetGuid] = duel;

    challenger->SetPvP(true);
    target->SetPvP(true);

    Send(challenger, "Death Duel accepted. Fight until one player dies.");
    Send(target, "Death Duel accepted. Fight until one player dies.");

    if (DuoRogue::GetConfig().DeathDuelAnnounceStart)
        Announce("Death Duel started: " + challenger->GetName() + " vs " + target->GetName() + ".");
}
}

bool IsFeatureAvailable()
{
    DuoRogue::Config const& config = DuoRogue::GetConfig();
    return config.DeathDuelEnable && config.PeaceModeAllowDeathDuels && config.DeathDuelRequireBothPlayersConfirm;
}

bool HasPendingFor(Player const* player)
{
    return g_PendingByTarget.find(Guid(player)) != g_PendingByTarget.end();
}

bool HasPendingFrom(Player const* player)
{
    uint32 const guid = Guid(player);
    return std::any_of(g_PendingByTarget.begin(), g_PendingByTarget.end(), [guid](auto const& value) { return value.second.ChallengerGuid == guid; });
}

bool IsActive(Player const* player)
{
    return g_ActiveByPlayer.find(Guid(player)) != g_ActiveByPlayer.end();
}

bool IsActiveBetween(Player const* first, Player const* second)
{
    uint32 const firstGuid = Guid(first);
    uint32 const secondGuid = Guid(second);
    auto itr = g_ActiveByPlayer.find(firstGuid);
    if (itr == g_ActiveByPlayer.end())
        return false;

    return (itr->second.FirstGuid == firstGuid && itr->second.SecondGuid == secondGuid) || (itr->second.FirstGuid == secondGuid && itr->second.SecondGuid == firstGuid);
}

bool IsAllowedPlayerDamage(Unit* attacker, Unit* victim)
{
    Player* attackerPlayer = attacker ? attacker->GetAffectingPlayer() : nullptr;
    Player* victimPlayer = victim ? victim->GetAffectingPlayer() : nullptr;
    if (!attackerPlayer || !victimPlayer || attackerPlayer == victimPlayer)
        return true;

    if (IsActiveBetween(attackerPlayer, victimPlayer))
        return true;

    if (DuoRogue::GetConfig().PeaceModeAllowDuels && attackerPlayer->duel && attackerPlayer->duel->Opponent == victimPlayer && attackerPlayer->duel->State == DUEL_STATE_IN_PROGRESS)
        return true;

    return false;
}

void AddGossipOptions(Player* player)
{
    if (!DuoRogue::IsEnabled() || !IsFeatureAvailable())
        return;

    AddGossipItemFor(player, GOSSIP_ICON_BATTLE, "Challenge nearby player to a Death Duel", GOSSIP_SENDER_MAIN, ActionChallenge);
    if (HasPendingFor(player))
        AddGossipItemFor(player, GOSSIP_ICON_BATTLE, "Accept pending Death Duel", GOSSIP_SENDER_MAIN, ActionAccept);
    if (HasPendingFor(player) || HasPendingFrom(player) || IsActive(player))
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Cancel pending Death Duel", GOSSIP_SENDER_MAIN, ActionCancel);
    AddGossipItemFor(player, GOSSIP_ICON_CHAT, "What is a Death Duel?", GOSSIP_SENDER_MAIN, ActionInfo);
}

bool HandleGossipAction(Player* player, Creature* /*creature*/, uint32 action)
{
    if (action != ActionChallenge && action != ActionAccept && action != ActionCancel && action != ActionInfo)
        return false;

    if (!DuoRogue::IsEnabled() || !IsFeatureAvailable())
    {
        Send(player, "Death Duels are disabled.");
        return true;
    }

    switch (action)
    {
        case ActionInfo:
            Send(player, "Death Duels are opt-in fights between two living players. Unlike normal duels, they end only when one player actually dies.");
            Send(player, "If configured, the loser is marked hardcore dead and the winner gains a small Essence reward.");
            return true;
        case ActionCancel:
            CancelAllFor(Guid(player), "Death Duel cancelled.");
            Send(player, "Death Duel cancelled.");
            return true;
        case ActionChallenge:
        {
            if (!CanUseDeathDuel(player, true))
                return true;

            if (IsActive(player) || HasPendingFor(player) || HasPendingFrom(player))
            {
                Send(player, "You already have a pending or active Death Duel.");
                return true;
            }

            Player* target = FindChallengeTarget(player);
            if (!target)
            {
                Send(player, "Select a nearby player or stand within 30 yards of the player you want to challenge.");
                return true;
            }

            if (!CanUseDeathDuel(target, true))
                return true;

            if (IsActive(target) || HasPendingFor(target) || HasPendingFrom(target))
            {
                Send(player, "That player already has a pending or active Death Duel.");
                return true;
            }

            g_PendingByTarget[Guid(target)] = PendingChallenge{ Guid(player), Guid(target) };
            ChatHandler(player->GetSession()).PSendSysMessage("Death Duel challenge sent to %s.", target->GetName().c_str());
            ChatHandler(target->GetSession()).PSendSysMessage("%s challenged you to a Death Duel. Speak with The Run Keeper to accept or cancel.", player->GetName().c_str());
            return true;
        }
        case ActionAccept:
        {
            auto itr = g_PendingByTarget.find(Guid(player));
            if (itr == g_PendingByTarget.end())
            {
                Send(player, "You have no pending Death Duel challenge.");
                return true;
            }

            PendingChallenge challenge = itr->second;
            Player* challenger = FindPlayer(challenge.ChallengerGuid);
            if (!challenger)
            {
                g_PendingByTarget.erase(itr);
                Send(player, "The challenger is no longer online.");
                return true;
            }

            if (!CanUseDeathDuel(player, true) || !CanUseDeathDuel(challenger, true))
                return true;

            if (player->GetMapId() != challenger->GetMapId() || player->GetDistance(challenger) > ChallengeRange)
            {
                g_PendingByTarget.erase(itr);
                Send(player, "Death Duel cancelled because the challenger is too far away.");
                Send(challenger, "Death Duel cancelled because the target is too far away.");
                return true;
            }

            StartDuel(challenger, player);
            return true;
        }
        default:
            return true;
    }
}

void HandlePlayerDeath(Player* deadPlayer)
{
    uint32 const deadGuid = Guid(deadPlayer);
    auto itr = g_ActiveByPlayer.find(deadGuid);
    if (itr == g_ActiveByPlayer.end())
        return;

    ActiveDuel duel = itr->second;
    uint32 const winnerGuid = duel.FirstGuid == deadGuid ? duel.SecondGuid : duel.FirstGuid;
    Player* winner = FindPlayer(winnerGuid);

    g_ActiveByPlayer.erase(duel.FirstGuid);
    g_ActiveByPlayer.erase(duel.SecondGuid);

    if (deadPlayer)
        deadPlayer->SetPvP(false);
    if (winner)
        winner->SetPvP(false);

    if (deadPlayer && DuoRogue::GetConfig().DeathDuelMarkHardcoreDeath && DuoRogue::GetConfig().HardcoreEnable && !IsHardcoreDead(deadPlayer))
        DuoRogue::MarkHardcoreDead(deadPlayer);

    if (winner && DuoRogue::GetConfig().ProgressionEnable)
        DuoRogue::AddEssence(DuoRogue::GetAccountId(winner), DeathDuelWinnerEssence, "Death Duel victory", winner);

    if (DuoRogue::GetConfig().DeathDuelAnnounceWinner)
        Announce("Death Duel won by " + PlayerName(winnerGuid) + " against " + PlayerName(deadGuid) + ".");
}

void HandlePlayerLogout(Player* player)
{
    CancelAllFor(Guid(player), "Death Duel cancelled because a player logged out.");
}

void ValidatePlayerState(Player* player)
{
    uint32 const guid = Guid(player);
    auto active = g_ActiveByPlayer.find(guid);
    if (active != g_ActiveByPlayer.end())
    {
        uint32 const otherGuid = active->second.FirstGuid == guid ? active->second.SecondGuid : active->second.FirstGuid;
        Player* other = FindPlayer(otherGuid);
        if (!player->IsAlive() || !other || !other->IsAlive())
            return;

        if (player->GetMapId() != other->GetMapId() || player->GetDistance(other) > ActiveRange)
        {
            CancelActive(guid, "Death Duel cancelled because the players moved too far apart.");
            return;
        }

        if (!DuoRogue::GetConfig().DeathDuelAllowInsideDungeons && (IsDungeonOrRaid(player) || IsDungeonOrRaid(other)))
        {
            CancelActive(guid, "Death Duel cancelled because a player entered a dungeon or raid.");
            return;
        }

        player->SetPvP(true);
        other->SetPvP(true);
    }
}
}

class DuoRogueDeathDuelPlayerScript : public PlayerScript
{
public:
    DuoRogueDeathDuelPlayerScript() : PlayerScript("DuoRogueDeathDuelPlayerScript") { }

    void OnPlayerJustDied(Player* player) override
    {
        DuoRogue::DeathDuel::HandlePlayerDeath(player);
    }

    void OnPlayerLogout(Player* player) override
    {
        DuoRogue::DeathDuel::HandlePlayerLogout(player);
    }

    void OnPlayerMapChanged(Player* player) override
    {
        DuoRogue::DeathDuel::ValidatePlayerState(player);
    }

    void OnPlayerUpdateZone(Player* player, uint32 /*newZone*/, uint32 /*newArea*/) override
    {
        DuoRogue::DeathDuel::ValidatePlayerState(player);
    }

    void OnPlayerUpdate(Player* player, uint32 diff) override
    {
        if (!DuoRogue::IsEnabled() || !DuoRogue::DeathDuel::IsActive(player))
            return;

        uint32& timer = _timers[DuoRogue::GetGuid(player)];
        if (timer > diff)
        {
            timer -= diff;
            return;
        }

        timer = 1000;
        DuoRogue::DeathDuel::ValidatePlayerState(player);
    }

private:
    std::unordered_map<uint32, uint32> _timers;
};

class DuoRogueDeathDuelUnitScript : public UnitScript
{
public:
    DuoRogueDeathDuelUnitScript() : UnitScript("DuoRogueDeathDuelUnitScript") { }

    void OnDamage(Unit* attacker, Unit* victim, uint32& damage) override
    {
        PreventBlockedPlayerDamage(attacker, victim, damage);
    }

    void ModifyMeleeDamage(Unit* target, Unit* attacker, uint32& damage) override
    {
        PreventBlockedPlayerDamage(attacker, target, damage);
    }

    void ModifyPeriodicDamageAurasTick(Unit* target, Unit* attacker, uint32& damage, SpellInfo const* /*spellInfo*/) override
    {
        PreventBlockedPlayerDamage(attacker, target, damage);
    }

    void ModifySpellDamageTaken(Unit* target, Unit* attacker, int32& damage, SpellInfo const* /*spellInfo*/) override
    {
        if (damage <= 0)
            return;

        uint32 unsignedDamage = uint32(damage);
        PreventBlockedPlayerDamage(attacker, target, unsignedDamage);
        damage = int32(unsignedDamage);
    }

    uint32 DealDamage(Unit* attacker, Unit* victim, uint32 damage, DamageEffectType /*damagetype*/) override
    {
        PreventBlockedPlayerDamage(attacker, victim, damage);
        return damage;
    }

private:
    void PreventBlockedPlayerDamage(Unit* attacker, Unit* victim, uint32& damage)
    {
        if (!DuoRogue::IsEnabled() || !DuoRogue::GetConfig().PeaceModeEnable || !DuoRogue::GetConfig().PeaceModeDisableOpenWorldPvP)
            return;

        Player* attackerPlayer = attacker ? attacker->GetAffectingPlayer() : nullptr;
        Player* victimPlayer = victim ? victim->GetAffectingPlayer() : nullptr;
        if (!attackerPlayer || !victimPlayer || attackerPlayer == victimPlayer)
            return;

        if (!DuoRogue::DeathDuel::IsAllowedPlayerDamage(attacker, victim))
            damage = 0;
    }
};

void AddDuoRogueDeathDuelScripts()
{
    new DuoRogueDeathDuelPlayerScript();
    new DuoRogueDeathDuelUnitScript();
}
