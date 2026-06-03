#include "DuoRogueRewards.h"
#include "DuoRogueConfig.h"
#include "DuoRogueDatabase.h"
#include "CharacterDatabase.h"
#include "Creature.h"
#include "Group.h"
#include "Map.h"
#include "Player.h"
#include "Unit.h"
#include "UnitScript.h"

namespace DuoRogue
{
std::vector<SpellPackage> const& GetClassSouls()
{
    // Conservative low-rank spell IDs, kept in one section for testing/replacement.
    static std::vector<SpellPackage> const souls =
    {
        { "soul_warrior", "Warrior Soul", "Charge, Battle Shout, and Heroic Strike.", 5, { 100, 6673, 78 } },
        { "soul_paladin", "Paladin Soul", "Devotion Aura, Flash of Light, and Blessing of Might.", 5, { 465, 19750, 19740 } },
        { "soul_hunter", "Hunter Soul", "Arcane Shot, Aspect of the Monkey, and Hunter's Mark.", 5, { 3044, 13163, 1130 } },
        { "soul_rogue", "Rogue Soul", "Sinister Strike, Evasion, and Sprint.", 5, { 1752, 5277, 2983 } },
        { "soul_priest", "Priest Soul", "Lesser Heal, Power Word: Fortitude, and Smite.", 5, { 2050, 1243, 585 } },
        { "soul_death_knight", "Death Knight Soul", "Icy Touch, Death Grip, and Horn of Winter.", 8, { 45477, 49576, 57330 } },
        { "soul_shaman", "Shaman Soul", "Lightning Bolt, Healing Wave, and Strength of Earth Totem.", 5, { 403, 331, 8075 } },
        { "soul_mage", "Mage Soul", "Frostbolt, Fireball, and Blink.", 5, { 116, 133, 1953 } },
        { "soul_warlock", "Warlock Soul", "Shadow Bolt, Demon Skin, and Life Tap.", 5, { 686, 687, 1454 } },
        { "soul_druid", "Druid Soul", "Rejuvenation, Wrath, and Mark of the Wild.", 5, { 774, 5176, 1126 } }
    };
    return souls;
}

std::vector<PerkDef> const& GetPerks()
{
    static std::vector<PerkDef> const perks =
    {
        { "perk_health_5", "+5% max health", "Applied through max-health stat hook.", 3, 1 },
        { "perk_damage_5", "+5% damage", "Applied through damage hooks.", 3, 1 },
        { "perk_healing_5", "+5% healing", "Applied through healing hooks.", 3, 1 },
        { "perk_speed_5", "+5% movement speed", "Applied on login and periodic refresh.", 3, 1 },
        { "perk_start_bags", "Start with 4 bags", "Grants four 16-slot bags when learned.", 4, 1 },
        { "perk_start_gold", "Start with 1 gold", "Grants 1 gold when learned.", 2, 1 },
        { "perk_health_10", "+10% max health", "Higher-rank max-health bonus.", 8, 2 },
        { "perk_dungeon_damage_10", "+10% dungeon damage", "Damage bonus only in dungeons/raids.", 8, 2 },
        { "perk_dungeon_healing_10", "+10% dungeon healing", "Healing bonus only in dungeons/raids.", 8, 2 },
        { "perk_start_level_10", "Start at level 10", "Raises the character to level 10 on a new run.", 15, 3 },
        { "perk_extra_boss_essence", "Extra boss Essence", "Adds +1 Essence to dungeon boss rewards.", 15, 3 }
    };
    return perks;
}

SpellPackage const* FindSoul(std::string const& key)
{
    for (SpellPackage const& soul : GetClassSouls())
        if (key == soul.Key)
            return &soul;
    return nullptr;
}

PerkDef const* FindPerk(std::string const& key)
{
    for (PerkDef const& perk : GetPerks())
        if (key == perk.Key)
            return &perk;
    return nullptr;
}

void TeachSoul(Player* player, SpellPackage const& soul)
{
    for (uint32 spellId : soul.Spells)
        if (!player->HasSpell(spellId))
            player->learnSpell(spellId, false);
}

void ApplyPermanentPerks(Player* player)
{
    uint32 const guid = GetGuid(player);
    if (HasCharacterPerk(guid, "perk_speed_5"))
        player->SetSpeed(MOVE_RUN, 1.05f, true);
}

void ApplyStartingPerks(Player* player)
{
    uint32 const guid = GetGuid(player);
    if (HasCharacterPerk(guid, "perk_start_bags"))
        player->AddItem(21841, 4);
    if (HasCharacterPerk(guid, "perk_start_gold"))
        player->ModifyMoney(10000);
    if (HasCharacterPerk(guid, "perk_start_level_10") && player->GetLevel() < 10)
        player->GiveLevel(10);
}

void AwardLevelMilestone(Player* player, uint8 oldLevel)
{
    if (!IsEnabled() || !GetConfig().ProgressionEnable || !GetConfig().AwardOnLevelMilestone)
        return;

    static std::vector<std::pair<uint8, uint32>> const milestones = { {10, 1}, {20, 2}, {30, 3}, {40, 4}, {50, 5}, {60, 8}, {70, 10}, {80, 15} };
    uint32 const guid = GetGuid(player);
    for (auto const& milestone : milestones)
    {
        if (oldLevel < milestone.first && player->GetLevel() >= milestone.first)
        {
            std::string key = "milestone_" + std::to_string(milestone.first);
            if (!HasCharacterPerk(guid, key))
            {
                AddCharacterPerk(guid, key);
                AddEssence(GetAccountId(player), milestone.second, "level milestone", player);
            }
        }
    }
}

uint32 GetDeathPayout(uint8 level)
{
    return std::max<uint32>(1, uint32(level) / 10);
}

void AwardBossKill(Player* player, Creature* killed)
{
    if (!IsEnabled() || !GetConfig().ProgressionEnable || !GetConfig().AwardOnDungeonBossKill || !player || !killed || !killed->GetMap())
        return;

    if (!killed->GetMap()->IsDungeon() && !killed->GetMap()->IsRaid())
        return;

    if (!killed->IsDungeonBoss() && !killed->isWorldBoss())
        return;

    uint32 reward = killed->GetMap()->IsRaid() ? 3 : 1;
    if (HasCharacterPerk(GetGuid(player), "perk_extra_boss_essence"))
        ++reward;

    CharacterDatabase.Execute("UPDATE duo_rl_account_progress SET total_boss_kills = total_boss_kills + 1 WHERE account_id = {}", GetAccountId(player));
    AddEssence(GetAccountId(player), reward, "boss kill", player);
}

bool HasDuoValor(Player const* player)
{
    if (!player || !IsEnabled() || !GetConfig().DuoBuffEnable)
        return false;

    Map const* map = player->GetMap();
    if (!map)
        return false;

    if (GetConfig().DuoBuffOnlyInDungeonsAndRaids && !map->IsDungeon() && !map->IsRaid())
        return false;

    Group const* group = player->GetGroup();
    uint32 groupSize = group ? group->GetMembersCount() : 1;
    return groupSize >= 1 && groupSize <= GetConfig().DuoBuffMaxPlayers;
}
}

class DuoRogueUnitScript : public UnitScript
{
public:
    DuoRogueUnitScript() : UnitScript("DuoRogueUnitScript") { }

    void OnDamage(Unit* attacker, Unit* /*victim*/, uint32& damage) override
    {
        Player* player = attacker ? attacker->ToPlayer() : nullptr;
        if (!player)
            return;

        uint32 bonus = 0;
        if (DuoRogue::HasCharacterPerk(DuoRogue::GetGuid(player), "perk_damage_5"))
            bonus += 5;
        if (DuoRogue::HasCharacterPerk(DuoRogue::GetGuid(player), "perk_dungeon_damage_10") && player->GetMap() && (player->GetMap()->IsDungeon() || player->GetMap()->IsRaid()))
            bonus += 10;
        if (DuoRogue::HasDuoValor(player))
            bonus += DuoRogue::GetConfig().DuoBuffDamagePercent;

        if (bonus)
            ApplyPct(damage, bonus);
    }

    void OnHeal(Unit* healer, Unit* /*receiver*/, uint32& gain) override
    {
        Player* player = healer ? healer->ToPlayer() : nullptr;
        if (!player)
            return;

        uint32 bonus = 0;
        if (DuoRogue::HasCharacterPerk(DuoRogue::GetGuid(player), "perk_healing_5"))
            bonus += 5;
        if (DuoRogue::HasCharacterPerk(DuoRogue::GetGuid(player), "perk_dungeon_healing_10") && player->GetMap() && (player->GetMap()->IsDungeon() || player->GetMap()->IsRaid()))
            bonus += 10;
        if (DuoRogue::HasDuoValor(player))
            bonus += DuoRogue::GetConfig().DuoBuffHealingPercent;

        if (bonus)
            ApplyPct(gain, bonus);
    }
};

void AddDuoRogueRewardScripts()
{
    new DuoRogueUnitScript();
}
