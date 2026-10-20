# LoadoutSwitchCombatProto

Unreal Engine 5.8 C++ prototype for the contextual combo + loadout switch combat design.

## First-time setup

1. Run `setup-repo.ps1` (git init, LFS, first commit).
2. Right-click `LoadoutSwitchCombatProto.uproject` → **Generate Visual Studio project files**.
3. Open the `.sln`, build **Development Editor | Win64**, then launch the editor.

## What's in the box

- One runtime module, `LoadoutSwitchCombatProto`, with dependencies on Gameplay Abilities, Gameplay Tags, Enhanced Input and Motion Warping.
- Gameplay tags: the context tags (`Context.*`) and enemy poise tiers (`Enemy.Poise.*`) are native, in `Source/LoadoutSwitchCombatProto/Combat/CombatTags.cpp`. `Config/DefaultGameplayTags.ini` holds the tags only data refers to, such as the resolver slots (`Slot.*`).
- `.gitattributes`: `.uasset` / `.umap` and source art go through Git LFS, and Unreal assets are marked lockable.

## How an attack press resolves

Three pieces of data decide what an attack press does:

- **The context** (`UCombatContextComponent`): a set of `Context.*` tags describing the moment, e.g. `Context.Stance.Ground`, `Context.Range.Far`, `Context.Target.Crowd`.
- **The resolver rules** (a `CombatResolverRules` data asset, e.g. `Content/Data/DA_CombatResolverRules`): an ordered list of rows, highest priority first.
- **The loadout** (a `CombatLoadout` data asset): which ability sits in which slot, plus a basic attack.

Each rules row has two fields that do different jobs:

| Field | Role | Example |
|---|---|---|
| **Slot** | The row's output: a name the loadout looks the ability up by. | `Slot.Grid.Ground.Far` |
| **Requirement** | The row's test: the `Context.*` tags that must be present for the row to match. | `Context.Stance.Ground`, `Context.Range.Far` |

The Slot is only a label; it never affects whether a row matches. The Slot on a rule and the key in the loadout must be exactly the same tag (no parent/child matching).

### Fall-through

The resolver walks the rows top to bottom and picks the first row that **matches the context and has an ability in the active loadout**. A row that matches but whose slot is empty in the loadout is skipped, and the search carries on down the list. If nothing qualifies, it uses the rules asset's Fallback Slot with the loadout's basic attack.

This makes fallbacks a matter of row order, with no special cases in code:

- Put the more specific row above the more general one. For each grid cell, the Crowd row (requires stance, range and `Context.Target.Crowd`) sits directly above a general row (requires only stance and range).
- A general row must not require `Context.Target.Single`; otherwise it can't catch a crowd whose Crowd slot is empty.
- Fall-through only skips rows. It never relaxes a requirement, so an empty Ground Far Crowd slot falls to Ground Far, never to Ground Close.
- The same applies to the situational slots: with nothing in the Riser slot, a launched target falls through to the grid cell instead of the basic attack.

## Third-party content

Some assets come from Fab and are not stored in Git; a fresh clone needs them re-added before the project opens cleanly.

| Pack | Folder (git-ignored) | How to get it |
|---|---|---|
| Paragon: Greystone (Epic, free) | `Content/ParagonGreystone/` | Add it to your Fab library, then in the Epic Games Launcher: Unreal Engine > Library > Fab > Paragon: Greystone > **Add to Project** > `LoadoutSwitchCombatProto`. If 5.8 isn't offered, add it to a throwaway project on the newest version listed and use **Asset Actions > Migrate** to copy it into this project's `Content`. |
| Paragon: Kallari (Epic, free) | `Content/ParagonKallari/` | Same as Greystone. |
| Paragon: Kwang (Epic, free) | `Content/ParagonKwang/` | Same as Greystone. |

Rules for working with these packs:

- Never save our own work inside a pack folder; it would be ignored along with the pack. Montages, retargeted animations and Blueprints made from pack assets go in our folders, e.g. `Content/Combat/Animations`.
- Set the Epic Games Launcher's vault cache (Settings > Edit Vault Cache Location) to a drive with room; packs are several GB each.
- When adding a new pack, add its folder to `.gitignore` and a row to the table above.

## Version control notes

- Never commit `Binaries/`, `Intermediate/`, `Saved/` or `DerivedDataCache/`. They're ignored.
- When you add levels, use World Partition or turn on One File Per Actor so map edits stay small.
- Once there's a remote that supports LFS locking, set `git config lfs.locksverify true` and lock `.umap` files before editing them.
