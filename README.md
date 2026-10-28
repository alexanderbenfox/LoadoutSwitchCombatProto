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
- A target only counts as airborne (`Context.Target.Airborne`, the Riser) when it's falling and at least **Target Airborne Min Height** (120 cm, on the Combat Context component) off the ground, measured down from the bottom of its capsule. A hit that pops it up a little, or the end of its fall, still reads as grounded and gets the ground attacks. The context debug line shows the target's height next to the threshold.

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
- **Lunge Step Without Target** (0 = none): with no target, the lunge steps this far straight ahead instead, so a combo swung at nothing still moves forward.
- Forward momentum in a ground combo comes from pairing the two: each hit's Knockback pushes the target back a little, and the next hit's lunge follows it in.

### Rise (going up with the attack)

- Add a **Combat Rise** notify state over the frames where the attack carries the character up, e.g. a riser jumping with the enemy it launches. The window's length is how long the climb takes; the window's end is the top, where the character starts to fall.
- The climb eases out like a jump: fast off the ground, slowing to a stop at the top, so gravity takes over without a jolt. A shorter window gives a snappier jump. For a different shape, set **Rise Curve** on the ability (a Float Curve asset, X = time through the window and Y = progress, both 0 to 1; end on a flat slope).
- On the ability asset, set **Rise Height** (0 = no rise). **Rise Reach** lets the rise also drift across towards the target (0 = straight up), stopping **Lunge Stop Distance** short of it, so the character stays in front of what it launched. The rise also turns the character to face the target.
- To meet a launched target at its peak: launched at speed v, it tops out at v² / 1960 cm after v / 980 s at default gravity (1000 cm/s: ~510 cm after ~1 s).
- Don't overlap it with a Combat Lunge window; only one of them can move the character at a time. Put the lunge first if the attack does both.

### Hits

- Add a **Hit Window** notify state over the frames where the weapon should connect. While it's open, the weapon's hit segment is swept from where it was last frame to where it is now, so a fast swing can't skip past an enemy.
- Each enemy is hit at most once per window. A multi-hit attack gets one window per hit; windows mustn't overlap.
- **Weapons** on the window lists which Combat Weapon components it sweeps, by their name in the character's Components panel (e.g. `Weapon`). Leave it empty to sweep all of them. A misspelt name logs a warning when the window opens.
- How thick the weapon is to hits is its own **Hit Radius** (on the Combat Weapon component, default 20 cm). **Hit Radius Scale** on the window (default 1) multiplies it; raise it to make one attack more forgiving than the weapon really is. Spheres are swept along the segment no more than one radius apart, so the count follows from Hit Length and radius; there's nothing to set.
- What a hit does is set on the ability asset, under **Hit**:
  - **Damage**.
  - **Hit Stun** (seconds; at 60 fps, 0.1 s is 6 frames): how long the target can't act. Each hit restarts it.
  - **Launch Speed** above 0 launches the target upward (launchers, juggle re-pops); below 0 slams it down (an air combo finisher). At 0, a hit on a target in the air holds it up for the hit stun instead: its fall stops and its gravity drops to the dummy's **Air Hit Gravity Scale** (0.15), so an air combo juggles it. A target launched upward floats at its peak the same way, for whatever is left of the launching hit's stun; give a launcher a long Hit Stun to leave time to follow it up.
  - **Knockback Speed** above 0 pushes it away along the attacker's facing (flattened). Either, both or neither can be set; both sends it up and away.
  - **Hit Stop** (seconds, default 0, off): the attacker and the target freeze for this long when the hit lands; the launch or knockback plays out after it. Off by default to keep combos fluid, Kingdom Hearts style; a single hit can still opt in (around 0.05–0.1). How frozen is the resolver's **Hit Stop Time Dilation** (0.02; 1 turns hit-stop off).
- These are the defaults for all the ability's hits. Each Hit Window can override any of them for its own hit: tick the value's checkbox on the window. Typical uses: a combo finisher with bigger knockback, or the last hit of an air combo with no launch.
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
   - Window **end** = when a kept press chains. A press after the end, during the recovery, chains straight away. This sets the combo's speed, so end it as soon as the swing is over (where the hand stops moving fast), not at the end of the hit's clip: the hold and return to stance after it only play when the player stops.
   - Keep the window inside its own section.
   - The window's end also starts the hit's **recovery**: from there, moving or jumping stops the attack too (see Recovery below).
5. A press that resolves to a different ability (e.g. the target is now Far) cancels into that ability instead of the next hit. A pause past the end of a hit drops the combo, and the next press starts from the first hit.

### Recovery

- An attack with no Combo Window (a finisher, a launcher, the Riser) gets a **Recovery** notify state from the point its action is over to the end of the montage.
- From the recovery's start, an attack press starts the next attack straight away, a jump cancels, and movement stops the attack (once it's been in recovery the resolver's **Movement Cancel Delay**, 0.1 s, so a press just after a combo hit still chains with the stick held). Moving or jumping out ends the combo string.
- Left alone, the recovery plays out, so the animation still settles when the player does nothing. This is the baseline for every attack, hit or whiff; earlier cancels on hit are a later, bought ability (design doc, Cancel abilities).

### Combo strings and finishers

- The **combo counter** (on the Combat Loadout component, so it will carry over a swap) counts every attack in a string, whichever ability it is and whether it hits or not. A string ends when an attack finishes with nothing chained into it, when an attack is cancelled (e.g. a jump), when the finisher has been used, or when the stance changes: air attacks after ground ones start an air string of their own, so launcher → Riser → air combo begins counting again in the air.
- **Ground Combo Length** and **Air Combo Length** on the loadout (4 and 3) say how many attacks a string has. The last one is the finisher: for it the context gets `Context.Combo.Finisher`, so the **Finisher** rule (`Slot.Finisher.Ground` / `Slot.Finisher.Air`, ranked just below Riser) replaces whatever the grid would have picked. 0 means no finisher.
- With the finisher slot empty, the press falls through to the grid as usual, e.g. the last section of the ground combo montage.
- A finisher montage has no Combo Window, so nothing chains out of it; the next press after it is the first of a new string.
- The on-screen debug shows the string as `Combo: 2/4 ground`, and each ATTACK message and log line has the count.
- The current finishers, `TestFinisher` (ground: knocks the target far away with a small pop) and `TestFinisherAir` (spikes it down), both use the `Manny_Ability_R` overhead slam as a placeholder. Their montages are built by a script like the air combo's.

**Building a combo montage from a script.** `UCombatMontageLibrary::BuildComboMontage` (editor module) does steps 1–3 and the Blend Out Trigger Time in one go: give it an asset path and the hit sequences in order. Rerunning it on an existing montage rebuilds its track and sections. `Manny_AirCombo_Montage` was made this way, with its notifies laid out per hit; to swap in real air attacks, change the sequence list and rerun (ask Claude, or call it from editor Python as `unreal.CombatMontageLibrary.build_combo_montage(path, [seq, ...])`).

### Air attacks

- **Air Hang Gravity Scale** on the ability (1 = off): while the attack plays in the air, the character's gravity is multiplied by it, and each attack or combo hit starts by stopping the fall. With the target held up by its hits (see Launch Speed above), the character and the target both stay in the air for the combo. Gravity goes back to normal when the attack's recovery starts (so the character drops out of a hit it doesn't follow up), or when it ends, is cancelled, or the character lands. Landing during an air attack's recovery ends the attack. **Air Hang Time** (0 = the whole attack) ends the hang that long into the attack instead, so the character drops out of a long recovery, as the air finisher does after its slam.
- In the air, a **Combat Lunge** moves across only and keeps the character's height, like on the ground. With **Lunge Matches Height** on the ability, it also moves to the target's height (homing), stopping **Lunge Stop Distance** short across it, up to **Lunge Reach** in all. The air combo leaves it off: its hits step in and push the target back the way the ground combo does, without pulling the character up or down to it; only the Riser homes in.
- A Hit Window that closes without hitting anything prints a red **WHIFF** message with where the target was (across, and above or below). The dummy shows its air state and its last few hits above its head (**Show Hit Debug**).
- A launched dummy's knockback dies off in the air (**Air Braking Deceleration** on the dummy), so it drifts a little and then hangs instead of sailing out of range. For a launcher to float the dummy at its peak, its Hit Stun has to outlast the climb: about Launch Speed / 980 seconds (0.9 s at 900).
- When an air hold runs out with the dummy still in the air, it floats down on the dummy's **Juggle Fall Gravity Scale** (0.5) until it lands or is hit again, instead of dropping. A launch, slam or knock-away falls on normal gravity.
- The current air combo (`TestCloseAir`, `Manny_AirCombo_Montage`): three wide, arcing swings retargeted from Paragon (Kallari's air slash, Greystone's spinning jump attack, the air slash again), with Hit Windows over the whole arc and a 1.75× hit radius, so they're forgiving the way KH's aerials are. Each air hit steps in across (Lunge Reach 120, no height matching) and pushes the dummy back (Knockback 350). In a full air string the third hit is replaced by the air finisher (`TestFinisherAir`, Kallari's in-air spinning ultimate), which knocks the dummy far away (KH Aerial Finish style) so the string can't be followed up. The montage's own third hit knocks away the same way and only plays with the Finisher.Air slot empty.
- **Retargeting from the Paragon packs:** `Content/Combat/Retarget` holds auto-generated IK Rigs (`IK_Manny_Auto`, `IK_Greystone_Auto`, `IK_Kallari_Auto`) and retargeters (`RTG_Greystone_To_Manny`, `RTG_Kallari_To_Manny`), set up the way the Retarget Animations window does it. To bring in another animation, right-click it > Retarget Animations and pick the RTG, or ask Claude, which uses the same setup from a script. Results go in `Content/Combat/Animations` as `Manny_GS_*` / `Manny_KA_*`, with the packs' notifies removed.
- **After the air string's last hit** (the finisher, or the montage's third hit with no finisher slotted), air attacks are spent until the character lands: attack presses in the air are ignored, so the finisher's recovery lasts to the ground instead of starting another air string. Movement can still cut the recovery; the character just falls. The debug text shows "air attacks spent until landing" meanwhile.
- **Juggle loop:** to keep a target up, skip the finisher. Jump, do two air hits, let yourself drop, land, and jump again; landing ends the air string, so the next jump starts a fresh one. Each air hit holds the target for its Hit Stun (0.9 s), and it floats down after that.

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
