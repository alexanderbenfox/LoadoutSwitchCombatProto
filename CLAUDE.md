# LoadoutSwitchCombatProto

UE 5.8 C++ prototype for a contextual-combo + loadout-switch action combat system (KH-style position-driven combos, DMC-style mid-combo style switching).

Design doc (source of truth): https://claude.ai/code/artifact/b7553e58-edd9-40bb-acd3-2f00315a7b5d

## Key design decisions
- One attack button resolves to a move from context. Grid = stance (Ground/Air) x range (Close/Far, no Mid) x target count (Single/Crowd) = 8 cells, plus situational slots: Riser, Post-dodge, Swap-in, Launcher, Finisher (ground/air).
- Resolver priority: Swap-in > Post-dodge > Riser > Finisher > grid cell > weapon basic attack. The priority order must be designer-editable data (a data asset), never hard-coded.
- Fall-through: the resolver takes the first rule that matches the context AND whose slot the active loadout has filled; empty slots are skipped, else the fallback slot + basic attack. Fallbacks (e.g. Crowd cell -> general cell) come from row order in the rules asset, never from code. Rule Slot is an output label only; Requirement is the test. See README "How an attack press resolves".
- Loadout = weapon + slotted abilities. Each loadout has its OWN AP budget (start 10 AP); players start with 2 loadout slots, unlock a 3rd later.
- Swap: one button, ~0.1 s, cancels recovery of connected hits only, combo counter / air state / target carry over. Off-hand charge powers the Swap-in ability.
- Enemies have poise tiers (Unarmored / Armored / Juggernaut); abilities have stagger power. Both designer-editable.
- Hard lock: R3 toggles, right stick switches target while locked (Dark Souls style). All bindings remappable via Enhanced Input user settings.

## Architecture (planned)
- Combat Context component: builds a Gameplay Tag container (native tags in Source/.../Combat/CombatTags.h; the .ini is for content-only tags).
- Loadout component: equipped loadouts, active index, combo counter, off-hand charge.
- Resolver: buffered input -> context -> priority order -> slot lookup -> activate GAS ability.
- Data assets: Loadout, Ability Definition (ability class, montage, AP cost, allowed weapon classes, warp settings, damage, stagger).
- Base context-attack Gameplay Ability: plays montage, sets motion-warp target, applies damage via Gameplay Effect.
- Animation notify windows: hit, combo, dodge-cancel, swap-cancel, motion warp.
- Animation-driven directives: frame-specific gameplay (spawn projectile, magnetize) is placed as Anim Notifies on the montage. Motion warping uses the built-in Motion Warping notify state. Custom notifies send a Gameplay Event (`Event.*` tag) to the owner's ASC; the active ability listens with WaitGameplayEvent, so notifies stay dumb and the logic lives in the ability. Example: the throw ability spawns its projectile on `Event.Throw.Release` and grants a `State.WeaponThrown` tag that attack abilities list as Activation Blocked; the returning projectile removes the tag and triggers a catch montage.

## Planned follow-ups
- Target-airborne detection, quick fix: the dummy enemy is an `ACharacter`, so the context component's existing `IsFalling` check sees it in the air. Not a physics-body query; characters are moved kinematically by Character Movement.
- Target-airborne detection, later: once enemies have an Ability System component, the enemy owns its combat state as tags and the context component reads those instead of movement state. Working names: `State.Floating` (launched, juggled or flying; a valid Riser / air-juggle target) and `State.Vulnerable` (staggered). The two can be held together, e.g. launched and staggered. The player's own stance stays on `IsFalling`.

- Soft-target selection biased by camera facing: the placeholder picks the nearest enemy, so a lunge can go to one behind the player. Score candidates by distance and by angle to the camera's forward vector (hard lock overrides this).
- Lock-on indicator shows what the press will do: the lock-on symbol itself changes to show whether the resolved attack is a Close move, a Far move, or will miss (target out of the resolved move's reach). Replaces the debug reticle; reads from the same `Resolve()` prediction so indicator and press never disagree.

## Milestones
1. One loadout, eight-cell grid, resolver + debug overlay (live context tags + resolved ability). Exit: each cell visibly different, reticle predicts it.
2+. See design doc. Second loadout + swap is milestone 4; stop and rework if swap isn't fun against a dummy.

## Conventions
- Prototype animations: Paragon (Greystone/Kallari/Sparrow), Game Animation Sample, Mixamo; retarget to UE5 mannequin.
- Git + LFS. Never commit Binaries/, Intermediate/, Saved/, DerivedDataCache/.
- Build: Development Editor | Win64. Engine is at `D:\UE_5.8`; from a shell:
  `& "D:\UE_5.8\Engine\Build\BatchFiles\Build.bat" LoadoutSwitchCombatProtoEditor Win64 Development -project="<repo>\LoadoutSwitchCombatProto.uproject" -waitmutex`
  (always go through `Build.bat`, which uses the engine's bundled .NET; calling `UnrealBuildTool.exe` directly fails on this machine).
- Toolchain: needs MSVC 14.44 (VS 2022 17.14). This machine's global `%APPDATA%\Unreal Engine\UnrealBuildTool\BuildConfiguration.xml` pins 14.34 for older engines, so `Saved/UnrealBuildTool/BuildConfiguration.xml` (git-ignored) sets `CompilerVersion` to `Latest` for this project. Recreate it if `Saved/` is deleted.
