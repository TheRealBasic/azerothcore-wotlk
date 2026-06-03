# mod-duo-roguelike

`mod-duo-roguelike` is a private/friends-only AzerothCore 3.3.5a module for a two-player roguelike hardcore experience. It is implemented as a normal AzerothCore module with C++, SQL, and a module config file. It does **not** require client patches, DBC edits, custom maps, or client data.

## What this module does

- Adds module-side 3x XP through `OnPlayerGiveXP` when `DuoRogue.XPMultiplier = 3.0`.
- Adds hardcore run state outside dungeons/raids.
- Treats dungeon and raid maps as safe zones for hardcore deaths when configured.
- Assigns a Random Origin on first login and teleports the character to a race starter area.
- Blocks configured city zones by warning and teleporting players to a safe starter fallback.
- Tracks account-wide Essence in the characters database.
- Adds The Run Keeper gossip NPC for rules, run status, Class Souls, perks, dead-run resets, and GM debug actions.
- Adds conservative Class Soul spell packages using normal server-side spell learning.
- Adds a first-pass NPC gossip skill tree/perk system.
- Adds simple Duo Valor damage/healing/stamina hooks for one or two players in dungeon/raid maps.

## What this module does not do

- It does not create a custom WoW client.
- It does not patch DBC files.
- It does not add custom real WoW classes.
- It does not implement a custom talent UI.
- It does not include maps, vmaps, mmaps, DBC files, credentials, account panels, launchers, websites, payment systems, or public-server tooling.
- It does not fully rebalance every dungeon or raid encounter by itself.

## Installation steps

1. Keep this folder at `modules/mod-duo-roguelike`.
2. Reconfigure AzerothCore with modules enabled, for example:
   ```bash
   cmake -S . -B var/build -DMODULES=static -DSCRIPTS=static
   ```
3. Build normally:
   ```bash
   cmake --build var/build -j$(nproc)
   ```
4. Copy `modules/mod-duo-roguelike/conf/duo_roguelike.conf.dist` to your server config directory as `duo_roguelike.conf` and edit as desired.
5. Import the SQL listed below.
6. Restart `worldserver`.

## SQL import notes

Import the characters SQL into your characters database:

```bash
mysql -u <user> -p <characters_db> < modules/mod-duo-roguelike/data/sql/db-characters/base/duo_roguelike_characters.sql
```

Import the world SQL into your world database:

```bash
mysql -u <user> -p <world_db> < modules/mod-duo-roguelike/data/sql/db-world/base/duo_roguelike_world.sql
```

The character-table SQL uses `CREATE TABLE IF NOT EXISTS`. The world SQL deletes/reinserts only NPC template entry `900001` so it can be re-run for this module entry.

## Config options

Default config is in `conf/duo_roguelike.conf.dist`.

Key options:

- `DuoRogue.Enable`: master enable/disable switch.
- `DuoRogue.XPMultiplier`: module-side XP multiplier.
- `DuoRogue.Hardcore.*`: death marking and dead-run behavior.
- `DuoRogue.RandomOrigin.*`: first-login Random Origin behavior.
- `DuoRogue.NoCities.*`: forbidden city zones and teleport behavior.
- `DuoRogue.Progression.*`: Essence award toggles.
- `DuoRogue.NpcEntry`: The Run Keeper creature template entry.
- `DuoRogue.DuoBuff.*`: one/two-player dungeon/raid bonuses.
- `DuoRogue.Debug`: reserved debug toggle.

## How to spawn The Run Keeper

The world SQL creates creature template `900001` with script `npc_duo_roguelike_run_keeper`. Spawn it manually in game with a GM account near a starter area:

```text
.npc add 900001
```

Suggested places: Northshire, Valley of Trials, or a neutral custom staging spot outside city zones.

## How to set 3x XP

The module multiplies XP with `DuoRogue.XPMultiplier = 3.0` via the supported player XP hook.

For normal AzerothCore server-wide XP rates, you may also set these in `worldserver.conf` if you prefer or want matching server defaults:

```ini
Rate.XP.Kill = 3
Rate.XP.Quest = 3
Rate.XP.Explore = 3
```

Avoid stacking both methods unintentionally unless you want more than 3x XP.

## How hardcore death works

- Death outside a dungeon/raid marks `duo_rl_character_state.hardcore_dead = 1`.
- The death level, map, zone, and area are recorded.
- Account total deaths are incremented.
- A death payout of `max(1, floor(level / 10))` Essence is awarded when enabled.
- Dead runs receive no XP while `DuoRogue.Hardcore.LockDeadCharacter = 1`.
- Dead characters receive a clear login warning and may be teleported to a safe starter fallback.
- Characters are not deleted by default. `DuoRogue.Hardcore.DeleteCharacter` is present for future use but intentionally not implemented as destructive behavior in this safe base version.

## How Random Origin works

Random Origin is not a true race change. On first login, the module rolls an origin from starter-area definitions that match the player's faction unless `DuoRogue.RandomOrigin.AllowFactionMismatch = 1`.

Origins included:

- Human - Northshire
- Dwarf/Gnome - Dun Morogh
- Night Elf - Teldrassil
- Draenei - Azuremyst
- Orc/Troll - Durotar
- Tauren - Mulgore
- Undead - Tirisfal
- Blood Elf - Eversong

The origin is saved in `duo_rl_character_state`, the character is teleported to the origin starter area, and a small racial flavour spell is learned when safe.

## How Class Souls work

Class Souls are account unlocks saved in `duo_rl_unlocks`. A player keeps their real class but can learn a conservative spell package through The Run Keeper.

Spell IDs used in this first version:

| Soul | Key | Cost | Spell IDs |
| --- | --- | ---: | --- |
| Warrior Soul | `soul_warrior` | 5 | Charge `100`, Battle Shout `6673`, Heroic Strike `78` |
| Paladin Soul | `soul_paladin` | 5 | Devotion Aura `465`, Flash of Light `19750`, Blessing of Might `19740` |
| Hunter Soul | `soul_hunter` | 5 | Arcane Shot `3044`, Aspect of the Monkey `13163`, Hunter's Mark `1130` |
| Rogue Soul | `soul_rogue` | 5 | Sinister Strike `1752`, Evasion `5277`, Sprint `2983` |
| Priest Soul | `soul_priest` | 5 | Lesser Heal `2050`, Power Word: Fortitude `1243`, Smite `585` |
| Death Knight Soul | `soul_death_knight` | 8 | Icy Touch `45477`, Death Grip `49576`, Horn of Winter `57330` |
| Shaman Soul | `soul_shaman` | 5 | Lightning Bolt `403`, Healing Wave `331`, Strength of Earth Totem `8075` |
| Mage Soul | `soul_mage` | 5 | Frostbolt `116`, Fireball `133`, Blink `1953` |
| Warlock Soul | `soul_warlock` | 5 | Shadow Bolt `686`, Demon Skin `687`, Life Tap `1454` |
| Druid Soul | `soul_druid` | 5 | Rejuvenation `774`, Wrath `5176`, Mark of the Wild `1126` |

Some cross-class spells may have class, power, reagent, stance, form, rune, or client-action-bar quirks after real gameplay testing. Replace problematic spells with safer IDs as needed.

## How to reset a run

Talk to The Run Keeper and select `Reset my dead run`.

- If `hardcore_dead = 1`, the module increments `run_id`, clears death fields, clears origin, rerolls a new origin, and teleports the player.
- If the run is alive, the NPC says `Your run is still alive.`
- GM-only debug options can also add 10 Essence, clear dead state, reroll origin, and print state.

## Recommended setup for two players

- Keep the server private or friends-only.
- Keep `DuoRogue.DuoBuff.Enable = 1`.
- Keep `DuoRogue.DuoBuff.MaxPlayers = 2`.
- Start with low-level dungeons first; endgame and raid balance is intentionally rough in this base module.
- Spawn The Run Keeper near your chosen starting/staging area.
- Consider giving the pair GM access only on a test realm for debug/reset validation.

## Optional AutoBalance recommendation

`modules/mod-autobalance` was not present in this source tree when this module was created. This module therefore does not hard-depend on AutoBalance and only includes a lightweight Duo Valor hook.

For serious dungeon/raid two-player scaling, install and configure AzerothCore `mod-autobalance` separately and tune it for one to two players. Keep this module's Duo Valor as a simple baseline layer or reduce its percentages if AutoBalance makes content too easy.

## Known limitations

- Random Origin is not true client-side race changing.
- Unlockable classes are implemented as spell packages, not real new classes.
- Custom skill tree is NPC gossip-based, not a custom UI.
- Raid balance requires AutoBalance or later deeper tuning.
- Some cross-class spells may need replacing after testing.
- Duo Valor is implemented through stat/damage/healing hooks and chat notices, not a custom DBC aura icon.
- Dead characters are blocked from XP but not fully prevented from every possible combat interaction in this safe base version.
- Boss Essence rewards can currently be repeated; this is temporary base-game behavior.
- Starting perks are character-perk purchases in this first version; future work should split permanent account unlocks from per-run selections.

## Testing checklist

Test 1: Create a new character. Expected: Random Origin assigned and saved.

Test 2: Enter Stormwind/Orgrimmar/Dalaran. Expected: Warning and teleport out.

Test 3: Die outside a dungeon. Expected: Character marked hardcore dead.

Test 4: Log in as dead character. Expected: Dead-run message and progression blocked.

Test 5: Die inside Ragefire Chasm/Deadmines. Expected: Character is not marked hardcore dead.

Test 6: Use GM debug option to add Essence. Expected: Essence updates in DB and NPC menu.

Test 7: Unlock Mage Soul. Expected: Essence deducted and spells learned.

Test 8: Buy +5% health perk. Expected: Perk saved and applied through max-health hook.

Test 9: Enter dungeon with one or two players. Expected: Duo Valor damage/healing/stamina hooks apply.

Test 10: Leave dungeon. Expected: Duo Valor conditions stop applying outside dungeon/raid maps.

## Future TODO list

- Add prepared custom statements if the local module framework is extended for module-owned prepared IDs.
- Add richer account/per-run separation for permanent unlocks versus selected run perks.
- Add safer throttling/caching for frequently checked character perk data.
- Add first-clear dungeon rewards with lockouts to reduce repeated farming.
- Add a non-destructive GM chat command in addition to NPC debug options.
- Replace any Class Soul spells that fail in real cross-class gameplay testing.
- Add optional AutoBalance detection/config examples if `mod-autobalance` is installed.
- Add a visual Duo Valor aura only if implemented without client DBC patches.
