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

## Setting up an attack montage

Checklist for turning an animation into an attack. Several of these fail silently, so go through all of them.

### The animation

1. Retarget it onto the mannequin (`SKM_Manny_Simple`) and save it in `Content/Combat/Animations` with a `Manny_` prefix, never inside a pack folder.
2. **Root motion:** on the sequence (not the montage), in Asset Details > Root Motion:
   - Attacks that move with a **Combat Lunge** window: **Enable Root Motion off**. The lunge moves the character; root motion left on fights it.
   - Attacks that should move the character the way the animation does, with no lunge: Enable Root Motion on.
3. Retargeted animations can carry the original pack's notifies (sounds, particles). Delete any you didn't add from the Notifies track.

### The montage

1. Right-click the sequence > Create > Create AnimMontage, saved next to it with a `_Montage` suffix.
2. **Blend Options > Blend Out Trigger Time = 0.** The default (-1) stops the montage a blend-out time (0.25 s) before it ends so the fade finishes on the last frame. From that point the code sees no montage playing: the input buffer opens early, and in a combo the chain restarts the first hit instead of moving on. With 0 the montage stays active to its last frame and fades afterwards.
3. Put notifies on the **montage's** Notifies track, not the sequence's. The code only accepts windows on the montage of the ability it's playing.
4. Set the ability asset's **Montage** to it.

### Lunge (closing the gap in code)

- Add a **Combat Lunge** notify state over the frames where the attack steps in. The window's length is the dash's duration.
- On the ability asset, set **Lunge Reach** (how far it can travel, 0 = no lunge) and **Lunge Stop Distance** (how far short of the target it stops). A target beyond Reach + Stop Distance is a miss.

### Hits

- Add a **Hit Window** notify state over the frames where the weapon should connect. While it's open, the weapon's hit segment is swept from where it was last frame to where it is now, so a fast swing can't skip past an enemy.
- Each enemy is hit at most once per window. A multi-hit attack gets one window per hit; windows mustn't overlap.
- **Weapons** on the window lists which Combat Weapon components it sweeps, by their name in the character's Components panel (e.g. `Weapon`). Leave it empty to sweep all of them. A misspelt name logs a warning when the window opens.
- How thick the weapon is to hits is its own **Hit Radius** (on the Combat Weapon component, default 20 cm). **Hit Radius Scale** on the window (default 1) multiplies it; raise it to make one attack more forgiving than the weapon really is. Spheres are swept along the segment no more than one radius apart, so the count follows from Hit Length and radius; there's nothing to set.
- On the ability asset, **Hit Launch Speed** above 0 launches the target upward (launchers, juggle hits); 0 leaves it where it is. **Hit Knockback Speed** above 0 pushes it away along the attacker's facing (flattened). Either, both or neither can be set; a launch with knockback sends it up and away.
- The dummy reacts with **Flinch Montage** on the ground and **Air Flinch Montage** when launched or hit in the air (empty = none). The air flinch fades out on landing. While launched, the dummy's **Launched** flag is true until it lands; its anim blueprint can read it to use a tumble/launched state instead of the plain fall.
- With the lunge's **Stop Distance**, check the swing actually reaches: if the weapon ends short of the target, every hit misses.

### The weapon

- The character's **Weapon** component is attached to `weapon_r_socket`, a socket on `hand_r` in the `SK_Mannequin` skeleton. Not on the `weapon_r` bone: that bone is in the skeleton but not in the Manny mesh, so a socket on it stays at the character's feet.
- Position the weapon by editing the socket (Relative Location / Rotation / Scale), with the weapon mesh added to it as a preview asset.
- The **Weapon** component (a Combat Weapon component) holds the weapon mesh and its hit segment. For a second hitbox (an off-hand dagger, a kick), add another Combat Weapon component in the Blueprint, attach it to its own socket, and name it in the windows that should use it. Select it to see the segment and its hit spheres drawn in the viewport. On it, **Weapon > Hit Segment > Hit Direction / Hit Start / Hit Length / Hit Radius** describe the part of the weapon that hits (a blade's edge, a hammer's head), in the weapon's own space: which way it runs from the mesh's pivot, where it starts and how long it is. The hit sweep uses these, not the mesh, so check them with the debug lines when swapping meshes.

### Combos (one montage, one section per hit)

1. Build one montage holding every hit in order: create it from the first hit, then drag the other sequences onto the slot track after it.
2. Add a **section** at the first frame of each hit (right-click the Sections track > New Montage Section). Names don't matter; the code goes by timeline order.
3. In the **Montage Sections** panel press **Clear**, so each section stops on its own. Linked sections (A→B→C→D, the default) play the whole combo from one press.
4. Add a **Combo Window** notify state to every hit except the last; the last hit having none is what ends the combo.
   - Window **start** = the earliest a press is kept for the next hit.
   - Window **end** = when a kept press chains. A press after the end, during the recovery, chains straight away.
   - Keep the window inside its own section.
5. A press that resolves to a different ability (e.g. the target is now Far) cancels into that ability instead of the next hit. A pause past the end of a hit drops the combo, and the next press starts from the first hit.

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
