#include "DuoRogueDatabase.h"
#include "DuoRogueConfig.h"
#include "Chat.h"
#include "CharacterDatabase.h"
#include "Log.h"
#include "Player.h"
#include "WorldSession.h"

namespace DuoRogue
{
uint32 GetGuid(Player const* player) { return player->GetGUID().GetCounter(); }
uint32 GetAccountId(Player const* player) { return player->GetSession()->GetAccountId(); }

void EnsureAccountProgress(uint32 accountId)
{
    CharacterDatabase.Execute("INSERT IGNORE INTO duo_rl_account_progress (account_id, total_runs) VALUES ({}, 1)", accountId);
}

void EnsureCharacterState(Player* player)
{
    uint32 const accountId = GetAccountId(player);
    uint32 const guid = GetGuid(player);
    EnsureAccountProgress(accountId);
    CharacterDatabase.Execute("INSERT IGNORE INTO duo_rl_character_state (guid, account_id) VALUES ({}, {})", guid, accountId);
}

AccountProgress GetAccountProgress(uint32 accountId)
{
    EnsureAccountProgress(accountId);
    AccountProgress progress;
    progress.AccountId = accountId;
    if (QueryResult result = CharacterDatabase.Query("SELECT essence, total_runs, best_level, total_boss_kills, total_deaths FROM duo_rl_account_progress WHERE account_id = {}", accountId))
    {
        Field* fields = result->Fetch();
        progress.Essence = fields[0].Get<uint32>();
        progress.TotalRuns = fields[1].Get<uint32>();
        progress.BestLevel = fields[2].Get<uint8>();
        progress.TotalBossKills = fields[3].Get<uint32>();
        progress.TotalDeaths = fields[4].Get<uint32>();
    }
    return progress;
}

CharacterState GetCharacterState(uint32 guid)
{
    CharacterState state;
    state.Guid = guid;
    if (QueryResult result = CharacterDatabase.Query("SELECT account_id, run_id, origin_race, COALESCE(origin_name, ''), hardcore_dead, COALESCE(death_level, 0), COALESCE(death_map, 0), COALESCE(death_zone, 0), COALESCE(death_area, 0) FROM duo_rl_character_state WHERE guid = {}", guid))
    {
        Field* fields = result->Fetch();
        state.AccountId = fields[0].Get<uint32>();
        state.RunId = fields[1].Get<uint32>();
        state.OriginRace = fields[2].Get<uint8>();
        state.OriginName = fields[3].Get<std::string>();
        state.HardcoreDead = fields[4].Get<uint8>() != 0;
        state.DeathLevel = fields[5].Get<uint8>();
        state.DeathMap = fields[6].Get<uint32>();
        state.DeathZone = fields[7].Get<uint32>();
        state.DeathArea = fields[8].Get<uint32>();
    }
    return state;
}

void SetOrigin(uint32 guid, uint8 race, std::string const& originName)
{
    CharacterDatabase.Execute("UPDATE duo_rl_character_state SET origin_race = {}, origin_name = '{}' WHERE guid = {}", uint32(race), originName, guid);
}

void AddEssence(uint32 accountId, uint32 amount, std::string const& reason, Player* notify)
{
    if (!amount)
        return;

    EnsureAccountProgress(accountId);
    CharacterDatabase.Execute("UPDATE duo_rl_account_progress SET essence = essence + {} WHERE account_id = {}", amount, accountId);
    if (notify)
        ChatHandler(notify->GetSession()).PSendSysMessage("You gained %u %s (%s).", amount, GetConfig().CurrencyName.c_str(), reason.c_str());
}

bool SpendEssence(uint32 accountId, uint32 amount)
{
    EnsureAccountProgress(accountId);
    AccountProgress progress = GetAccountProgress(accountId);
    if (progress.Essence < amount)
        return false;
    CharacterDatabase.Execute("UPDATE duo_rl_account_progress SET essence = essence - {} WHERE account_id = {}", amount, accountId);
    return true;
}

void MarkHardcoreDead(Player* player)
{
    uint32 const guid = GetGuid(player);
    uint32 const accountId = GetAccountId(player);
    EnsureCharacterState(player);
    CharacterDatabase.Execute("UPDATE duo_rl_character_state SET hardcore_dead = 1, death_level = {}, death_map = {}, death_zone = {}, death_area = {} WHERE guid = {}",
        uint32(player->GetLevel()), player->GetMapId(), player->GetZoneId(), player->GetAreaId(), guid);
    CharacterDatabase.Execute("UPDATE duo_rl_account_progress SET total_deaths = total_deaths + 1, best_level = GREATEST(best_level, {}) WHERE account_id = {}", uint32(player->GetLevel()), accountId);
}

void ClearDeadStateAndIncrementRun(Player* player)
{
    uint32 const guid = GetGuid(player);
    uint32 const accountId = GetAccountId(player);
    EnsureCharacterState(player);
    CharacterDatabase.Execute("UPDATE duo_rl_character_state SET run_id = run_id + 1, hardcore_dead = 0, death_level = NULL, death_map = NULL, death_zone = NULL, death_area = NULL, origin_race = 0, origin_name = NULL WHERE guid = {}", guid);
    CharacterDatabase.Execute("UPDATE duo_rl_account_progress SET total_runs = total_runs + 1 WHERE account_id = {}", accountId);
}

bool HasUnlock(uint32 accountId, std::string const& key)
{
    return CharacterDatabase.Query("SELECT 1 FROM duo_rl_unlocks WHERE account_id = {} AND unlock_key = '{}'", accountId, key) != nullptr;
}

void AddUnlock(uint32 accountId, std::string const& key)
{
    CharacterDatabase.Execute("INSERT IGNORE INTO duo_rl_unlocks (account_id, unlock_key) VALUES ({}, '{}')", accountId, key);
}

std::vector<std::string> GetUnlocks(uint32 accountId, std::string const& prefix)
{
    std::vector<std::string> values;
    QueryResult result = prefix.empty()
        ? CharacterDatabase.Query("SELECT unlock_key FROM duo_rl_unlocks WHERE account_id = {} ORDER BY unlock_key", accountId)
        : CharacterDatabase.Query("SELECT unlock_key FROM duo_rl_unlocks WHERE account_id = {} AND unlock_key LIKE '{}%' ORDER BY unlock_key", accountId, prefix);
    if (!result)
        return values;
    do
    {
        values.push_back(result->Fetch()[0].Get<std::string>());
    } while (result->NextRow());
    return values;
}

bool HasCharacterPerk(uint32 guid, std::string const& key)
{
    return CharacterDatabase.Query("SELECT 1 FROM duo_rl_character_perks WHERE guid = {} AND perk_key = '{}'", guid, key) != nullptr;
}

void AddCharacterPerk(uint32 guid, std::string const& key, uint8 rank)
{
    CharacterDatabase.Execute("INSERT INTO duo_rl_character_perks (guid, perk_key, rank) VALUES ({}, '{}', {}) ON DUPLICATE KEY UPDATE rank = GREATEST(rank, VALUES(rank))", guid, key, uint32(rank));
}

std::vector<std::string> GetCharacterPerks(uint32 guid)
{
    std::vector<std::string> values;
    if (QueryResult result = CharacterDatabase.Query("SELECT perk_key FROM duo_rl_character_perks WHERE guid = {} ORDER BY perk_key", guid))
    {
        do
        {
            values.push_back(result->Fetch()[0].Get<std::string>());
        } while (result->NextRow());
    }
    return values;
}
}
