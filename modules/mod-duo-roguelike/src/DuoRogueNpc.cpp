#include "DuoRogueConfig.h"
#include "DuoRogueDatabase.h"
#include "DuoRogueDeathDuel.h"
#include "DuoRogueRewards.h"
#include "Chat.h"
#include "Creature.h"
#include "CreatureScript.h"
#include "GossipDef.h"
#include "Player.h"
#include "ScriptedGossip.h"
#include "WorldSession.h"
#include <sstream>

namespace
{
enum DuoActions : uint32
{
    ACTION_RULES = GOSSIP_ACTION_INFO_DEF + 1,
    ACTION_RUN = GOSSIP_ACTION_INFO_DEF + 2,
    ACTION_SPEND = GOSSIP_ACTION_INFO_DEF + 3,
    ACTION_SOULS = GOSSIP_ACTION_INFO_DEF + 4,
    ACTION_PERKS = GOSSIP_ACTION_INFO_DEF + 5,
    ACTION_RESET = GOSSIP_ACTION_INFO_DEF + 6,
    ACTION_ADMIN = GOSSIP_ACTION_INFO_DEF + 7,
    ACTION_BACK = GOSSIP_ACTION_INFO_DEF + 8,
    ACTION_ADMIN_ADD_ESSENCE = GOSSIP_ACTION_INFO_DEF + 9,
    ACTION_ADMIN_CLEAR_DEAD = GOSSIP_ACTION_INFO_DEF + 10,
    ACTION_ADMIN_REROLL_ORIGIN = GOSSIP_ACTION_INFO_DEF + 11,
    ACTION_ADMIN_PRINT = GOSSIP_ACTION_INFO_DEF + 12,
    ACTION_SOUL_BASE = GOSSIP_ACTION_INFO_DEF + 100,
    ACTION_PERK_BASE = GOSSIP_ACTION_INFO_DEF + 200
};

void SendMain(Player* player, Creature* creature)
{
    ClearGossipMenuFor(player);
    AddGossipItemFor(player, GOSSIP_ICON_CHAT, "What are the rules?", GOSSIP_SENDER_MAIN, ACTION_RULES);
    AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Show my current run", GOSSIP_SENDER_MAIN, ACTION_RUN);
    AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Spend Essence", GOSSIP_SENDER_MAIN, ACTION_SPEND);
    AddGossipItemFor(player, GOSSIP_ICON_TRAINER, "Unlock Class Souls", GOSSIP_SENDER_MAIN, ACTION_SOULS);
    AddGossipItemFor(player, GOSSIP_ICON_TRAINER, "Buy Run Perks", GOSSIP_SENDER_MAIN, ACTION_PERKS);
    AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Reset my dead run", GOSSIP_SENDER_MAIN, ACTION_RESET);
    DuoRogue::DeathDuel::AddGossipOptions(player);
    if (player->IsGameMaster())
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Admin/debug options", GOSSIP_SENDER_MAIN, ACTION_ADMIN);
    SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
}

void SendSpend(Player* player, Creature* creature)
{
    ClearGossipMenuFor(player);
    DuoRogue::AccountProgress progress = DuoRogue::GetAccountProgress(DuoRogue::GetAccountId(player));
    AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Essence: " + std::to_string(progress.Essence), GOSSIP_SENDER_MAIN, ACTION_RUN);
    AddGossipItemFor(player, GOSSIP_ICON_TRAINER, "Unlock Class Souls", GOSSIP_SENDER_MAIN, ACTION_SOULS);
    AddGossipItemFor(player, GOSSIP_ICON_TRAINER, "Buy Permanent/Starting Perks", GOSSIP_SENDER_MAIN, ACTION_PERKS);
    AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Back", GOSSIP_SENDER_MAIN, ACTION_BACK);
    SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
}

void SendSouls(Player* player, Creature* creature)
{
    ClearGossipMenuFor(player);
    uint32 const accountId = DuoRogue::GetAccountId(player);
    uint32 index = 0;
    for (DuoRogue::SpellPackage const& soul : DuoRogue::GetClassSouls())
    {
        if (!DuoRogue::HasUnlock(accountId, soul.Key))
            AddGossipItemFor(player, GOSSIP_ICON_TRAINER, std::string(soul.Name) + " - " + std::to_string(soul.Cost) + " Essence", GOSSIP_SENDER_MAIN, ACTION_SOUL_BASE + index);
        ++index;
    }
    AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Back", GOSSIP_SENDER_MAIN, ACTION_BACK);
    SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
}

void SendPerks(Player* player, Creature* creature)
{
    ClearGossipMenuFor(player);
    uint32 const guid = DuoRogue::GetGuid(player);
    uint32 index = 0;
    for (DuoRogue::PerkDef const& perk : DuoRogue::GetPerks())
    {
        if (!DuoRogue::HasCharacterPerk(guid, perk.Key))
            AddGossipItemFor(player, GOSSIP_ICON_TRAINER, std::string("T") + std::to_string(perk.Tier) + " " + perk.Name + " - " + std::to_string(perk.Cost) + " Essence", GOSSIP_SENDER_MAIN, ACTION_PERK_BASE + index);
        ++index;
    }
    AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Back", GOSSIP_SENDER_MAIN, ACTION_BACK);
    SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
}

void SendAdmin(Player* player, Creature* creature)
{
    ClearGossipMenuFor(player);
    AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Add 10 Essence", GOSSIP_SENDER_MAIN, ACTION_ADMIN_ADD_ESSENCE);
    AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Clear dead state", GOSSIP_SENDER_MAIN, ACTION_ADMIN_CLEAR_DEAD);
    AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Reroll origin", GOSSIP_SENDER_MAIN, ACTION_ADMIN_REROLL_ORIGIN);
    AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Print debug character state", GOSSIP_SENDER_MAIN, ACTION_ADMIN_PRINT);
    AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Back", GOSSIP_SENDER_MAIN, ACTION_BACK);
    SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
}

void ShowRun(Player* player)
{
    DuoRogue::EnsureCharacterState(player);
    DuoRogue::AccountProgress progress = DuoRogue::GetAccountProgress(DuoRogue::GetAccountId(player));
    DuoRogue::CharacterState state = DuoRogue::GetCharacterState(DuoRogue::GetGuid(player));
    std::vector<std::string> souls = DuoRogue::GetUnlocks(DuoRogue::GetAccountId(player), "soul_");
    std::vector<std::string> perks = DuoRogue::GetCharacterPerks(DuoRogue::GetGuid(player));
    ChatHandler(player->GetSession()).PSendSysMessage("Run %u | Origin: %s | Hardcore: %s | Level: %u | Essence: %u", state.RunId, state.OriginName.empty() ? "Unassigned" : state.OriginName.c_str(), state.HardcoreDead ? "DEAD" : "Alive", uint32(player->GetLevel()), progress.Essence);
    ChatHandler(player->GetSession()).PSendSysMessage("Unlocked souls: %u | Character perks/milestones: %u", uint32(souls.size()), uint32(perks.size()));
}
}

class npc_duo_roguelike_run_keeper : public CreatureScript
{
public:
    npc_duo_roguelike_run_keeper() : CreatureScript("npc_duo_roguelike_run_keeper") { }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        if (!DuoRogue::IsEnabled())
            return false;
        DuoRogue::EnsureCharacterState(player);
        SendMain(player, creature);
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
    {
        ClearGossipMenuFor(player);
        if (!DuoRogue::IsEnabled())
            return false;

        switch (action)
        {
            case ACTION_RULES:
                ChatHandler(player->GetSession()).PSendSysMessage("Rules: 3x XP, hardcore outside dungeons, dungeons are safe zones, cities are forbidden, origins are random, Essence unlocks permanent progression.");
                SendMain(player, creature);
                return true;
            case ACTION_RUN:
                ShowRun(player);
                SendMain(player, creature);
                return true;
            case ACTION_SPEND:
                SendSpend(player, creature);
                return true;
            case ACTION_SOULS:
                SendSouls(player, creature);
                return true;
            case ACTION_PERKS:
                SendPerks(player, creature);
                return true;
            case ACTION_RESET:
            {
                DuoRogue::CharacterState state = DuoRogue::GetCharacterState(DuoRogue::GetGuid(player));
                if (!state.HardcoreDead)
                    ChatHandler(player->GetSession()).PSendSysMessage("Your run is still alive.");
                else
                {
                    DuoRogue::ClearDeadStateAndIncrementRun(player);
                    DuoRogue::RerollOrigin(player);
                    ChatHandler(player->GetSession()).PSendSysMessage("Dead run reset. A new origin has been rolled.");
                }
                SendMain(player, creature);
                return true;
            }
            case ACTION_ADMIN:
                if (player->IsGameMaster())
                    SendAdmin(player, creature);
                return true;
            case ACTION_BACK:
                SendMain(player, creature);
                return true;
            case ACTION_ADMIN_ADD_ESSENCE:
                if (player->IsGameMaster())
                    DuoRogue::AddEssence(DuoRogue::GetAccountId(player), 10, "GM debug", player);
                SendAdmin(player, creature);
                return true;
            case ACTION_ADMIN_CLEAR_DEAD:
                if (player->IsGameMaster())
                {
                    DuoRogue::ClearDeadStateAndIncrementRun(player);
                    ChatHandler(player->GetSession()).PSendSysMessage("GM debug: dead state cleared.");
                }
                SendAdmin(player, creature);
                return true;
            case ACTION_ADMIN_REROLL_ORIGIN:
                if (player->IsGameMaster())
                    DuoRogue::RerollOrigin(player);
                SendAdmin(player, creature);
                return true;
            case ACTION_ADMIN_PRINT:
                if (player->IsGameMaster())
                    ShowRun(player);
                SendAdmin(player, creature);
                return true;
            default:
                break;
        }

        if (DuoRogue::DeathDuel::HandleGossipAction(player, creature, action))
        {
            SendMain(player, creature);
            return true;
        }

        if (action >= ACTION_SOUL_BASE && action < ACTION_SOUL_BASE + DuoRogue::GetClassSouls().size())
        {
            DuoRogue::SpellPackage const& soul = DuoRogue::GetClassSouls()[action - ACTION_SOUL_BASE];
            uint32 const accountId = DuoRogue::GetAccountId(player);
            if (DuoRogue::HasUnlock(accountId, soul.Key))
                ChatHandler(player->GetSession()).PSendSysMessage("You already unlocked %s.", soul.Name);
            else if (!DuoRogue::SpendEssence(accountId, soul.Cost))
                ChatHandler(player->GetSession()).PSendSysMessage("Not enough Essence.");
            else
            {
                DuoRogue::AddUnlock(accountId, soul.Key);
                DuoRogue::TeachSoul(player, soul);
                ChatHandler(player->GetSession()).PSendSysMessage("Unlocked %s and learned its spells.", soul.Name);
            }
            SendSouls(player, creature);
            return true;
        }

        if (action >= ACTION_PERK_BASE && action < ACTION_PERK_BASE + DuoRogue::GetPerks().size())
        {
            DuoRogue::PerkDef const& perk = DuoRogue::GetPerks()[action - ACTION_PERK_BASE];
            uint32 const guid = DuoRogue::GetGuid(player);
            uint32 const accountId = DuoRogue::GetAccountId(player);
            if (DuoRogue::HasCharacterPerk(guid, perk.Key))
                ChatHandler(player->GetSession()).PSendSysMessage("You already bought %s.", perk.Name);
            else if (!DuoRogue::SpendEssence(accountId, perk.Cost))
                ChatHandler(player->GetSession()).PSendSysMessage("Not enough Essence.");
            else
            {
                DuoRogue::AddCharacterPerk(guid, perk.Key);
                DuoRogue::ApplyPermanentPerks(player);
                DuoRogue::ApplyStartingPerks(player);
                ChatHandler(player->GetSession()).PSendSysMessage("Bought perk: %s.", perk.Name);
            }
            SendPerks(player, creature);
            return true;
        }

        SendMain(player, creature);
        return true;
    }
};

void AddDuoRogueNpcScripts()
{
    new npc_duo_roguelike_run_keeper();
}
