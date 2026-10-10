# LoadoutSwitchCombatProto

Unreal Engine 5.8 C++ prototype for the contextual combo + loadout switch combat design.

## First-time setup

1. Run `setup-repo.ps1` (git init, LFS, first commit).
2. Right-click `LoadoutSwitchCombatProto.uproject` → **Generate Visual Studio project files**.
3. Open the `.sln`, build **Development Editor | Win64**, then launch the editor.

## What's in the box

- One runtime module, `LoadoutSwitchCombatProto`, with dependencies on Gameplay Abilities, Gameplay Tags, Enhanced Input and Motion Warping.
- `Config/DefaultGameplayTags.ini`: the context tags (`Context.*`) and enemy poise tiers (`Enemy.Poise.*`) from the design doc.
- `.gitattributes`: `.uasset` / `.umap` and source art go through Git LFS, and Unreal assets are marked lockable.

## Version control notes

- Never commit `Binaries/`, `Intermediate/`, `Saved/` or `DerivedDataCache/`. They're ignored.
- When you add levels, use World Partition or turn on One File Per Actor so map edits stay small.
- Once there's a remote that supports LFS locking, set `git config lfs.locksverify true` and lock `.umap` files before editing them.
