# Contextual Combo + Style Switch Combat — Design Doc

Oct 9, 2026 · @Alex

## Vision and pillars

One attack button picks the right move from where you are standing, and one swap button changes what "the right move" means. The player builds several loadouts, each a weapon plus a set of contextual abilities bought with AP, and swaps between them mid-fight to change role: diver, juggler, zoner, bruiser.

Kingdom Hearts supplies the low-execution, position-driven combo. Devil May Cry supplies the expression of switching kits mid-string. The bet is that switching *whole context tables* gives DMC-style variety without DMC-style input complexity.

**Pillars**

1. **Position is the input.** Height, distance and recent action choose the move. The player steers with movement, not with motion inputs or button sequences.
2. **A loadout is a role, not a skin.** Each loadout answers a different question (how do I get in, how do I stay safe, how do I cash out). Two loadouts that play the same are a failed build.
3. **Swapping is a move.** A swap is fast, cancels into and out of attacks, and can itself trigger an ability. It is never a menu or a pause.
4. **The build is authored before the fight, expressed during it.** AP limits force trade-offs in the menu. In combat the player reads the situation and picks the role, not the individual move.

**Not in scope for the prototype:** magic and MP, party members, summons, progression and leveling, story, more than one enemy archetype beyond what the switch decision needs.

## Reference research

Kingdom Hearts II already is a context table: almost every combat ability is a rule of the form "in situation X, the attack button does Y", priced at 2 to 5 AP. Kingdom Hearts III already lets you swap between three of those kits mid-combo. The gap this design fills is that KH never lets the player author the kits per weapon, and DMC never lets position do the selecting.

### Kingdom Hearts II: abilities as context rules

The table sorts KH II and Final Mix action abilities by the situation that triggers them. Source: [KH Wiki, Kingdom Hearts II abilities](https://www.khwiki.com/Action_ability).

| Context | Ability | AP | What the attack becomes |
| --- | --- | --- | --- |
| Ground, target near | Slapshot | 2 | Rapid hit that is not pushed back |
| Ground, target near, several enemies | Dodge Slash | 2 | Area damage around Sora |
| Ground, target slightly distant | Flash Step (FM) | 2 | Guard-piercing lunge |
| Ground, target far | Slide Dash | 2 | Gap-closing dash attack |
| Ground, surrounded | Vicinity Break (FM) | 2 | Wide clearing attack |
| Ground, mid-combo, alternate button | Upper Slash | 4 | Launcher |
| Ground, combo end | Finishing Leap, Guard Break, Explosion | 5, 3, 3 | Alternative finishers |
| Air, target airborne | Aerial Sweep | 2 | Leaping attack up to the target |
| Air, target far | Aerial Dive (FM), Aerial Spiral | 2, 2 | Air gap-closers |
| Air, mid-combo, alternate button | Horizontal Slash | 2 | Wide air sweep |
| Air, combo end | Aerial Finish, Magnet Burst (FM) | 3, 3 | Air finishers |
| Knocked down | Retaliating Slash | 3 | Recovery counterattack |
| Guarding | Counterguard | 4 | Counter from block |

Three things to take from this:

- **Contexts are few and legible.** Ground or air, three distance bands, crowd, combo position, and a couple of defensive states cover the whole list.
- **Combo shape is also bought.** Combo Plus and Air Combo Plus (1 AP each) add a hit; Negative Combo (2 AP) removes one to reach the finisher sooner; Finishing Plus (5 AP) chains finishers. Length and finisher frequency are build choices, separate from which moves fill the slots.
- **Movement lives in the same budget.** Dodge Roll, Aerial Dodge and Glide cost 3 AP per level, so mobility competes with offense.

The known weakness: because rules stack and the game picks, players often cannot predict which move will come out. A common community practice, recalled from memory and not sourced here, is unequipping abilities to get a cleaner combo. This design has to keep the readability and fix the predictability.

### Kingdom Hearts III: the closest precedent

KH III lets Sora carry three keyblades and swap between them mid-combo. Each keyblade has one or two Formchanges that replace combo attacks, finishers, dodge and block. Formchanges on inactive keyblades stay active with their timers frozen, so the player can bank a form and return to it. Source: [KH Wiki, Formchange](https://www.khwiki.com/Formchange).

What it does not do: the ability list is global, not per keyblade. Swapping changes the weapon's animations and form, not the context rules. The swap is also rarely *necessary*, which is the main design risk for this project.

### Devil May Cry: switching as expression

- **DMC 3** has four base styles (Trickster for mobility, Swordmaster for melee, Gunslinger for guns, Royal Guard for parry and stored counter), picked at checkpoints and level starts. Dante carries two melee weapons and two guns and swaps all four on the fly. Source: [Wikipedia, Devil May Cry 3](https://en.wikipedia.org/wiki/Devil_May_Cry_3:_Dante%27s_Awakening).
- **DMC 4 and 5** move style selection to the d-pad so all four are live at once, and the style button's action depends on the equipped weapon. Source for the four live styles: [Wikipedia, Devil May Cry 5](https://en.wikipedia.org/wiki/Devil_May_Cry_5). The d-pad mapping and per-weapon style actions are from memory, not from a page opened for this doc.
- **The style rank** rewards move variety and combo length, which is what makes switching worth doing even when it is not required.

What to take: a style is a *verb modifier*, switching is instant and cancels freely, and a scoring system pays for variety. What to leave: four styles times many weapons times directional inputs is the execution wall this design wants to avoid.

### Nearby games worth a look

These are from memory and not yet verified; each is a candidate for a follow-up research pass.

- **Bayonetta**: two weapon sets swapped mid-combo, with Dodge Offset letting a combo string survive a dodge. A direct model for "combo progress survives a swap".
- **NieR: Automata**: two weapon sets where swapping mid-string produces a unique transition attack.
- **Final Fantasy XVI**: three Eikon kits cycled with one button, each with its own cooldown abilities. A model for kits with separate resources.
- **Wuthering Waves and Genshin Impact**: character swap with intro and outro skills, a model for swap-triggered moves.
- **Kingdom Hearts 358/2 Days**: the panel grid, where abilities take physical space and link to each other, and layouts save as decks ([KH Wiki, Panel System](https://www.khwiki.com/Panel)). An alternative to a flat AP number.

### Unreal Engine building blocks

- **Gameplay Ability System**: an Ability System Component on the character, Gameplay Abilities for each move, Attribute Sets for numbers such as AP and HP, Gameplay Effects to change them, and Ability Tasks for async steps such as waiting on a montage. Source: [Epic, Gameplay Ability System](https://dev.epicgames.com/documentation/en-us/unreal-engine/gameplay-ability-system-for-unreal-engine) (UE 5.8 docs).
- **Gameplay Tags**: hierarchical labels such as `Movement.Mode.Swimming`, held in containers and tested with tag queries (Any, All, None). This is the natural representation for combat contexts. Source: [Epic, Gameplay Tags](https://dev.epicgames.com/documentation/en-us/unreal-engine/using-gameplay-tags-in-unreal-engine).
- **Motion Warping**: a notify window on a montage that bends root motion toward a named warp target. This is what makes one gap-closer animation land at any distance. Source: [Epic, Motion Warping](https://dev.epicgames.com/documentation/en-us/unreal-engine/motion-warping-in-unreal-engine).

## Context system

Every attack press reads the fight as a small set of tags, then looks up one move in the active loadout. Each context is a **slot that holds exactly one ability**, so the player always knows what will come out. This is the deliberate departure from Kingdom Hearts, where rules stack and the game picks.

### The contexts

The core is a 2 by 2 by 2 grid: stance (ground, air) by range (close, far) by target count (single, crowd), plus a handful of situational slots that override it. There is no mid band: two ranges keep the reticle readable, and the crowd split gives every range both a single-target and a group answer. The same slot plays very differently per loadout: at Ground / Far / Single, an assassin loadout lunges or magnets to the target, while a ranger loadout fires a long shot.

| Slot | Trigger | Typical job |
| --- | --- | --- |
| Ground / Close / Single | On ground, one enemy within close band | Bread-and-butter string |
| Ground / Close / Crowd | On ground, crowd within close band | Sweeping string, knockback |
| Ground / Far / Single | On ground, target in far band | Lunge, magnet pull or long shot |
| Ground / Far / Crowd | On ground, crowd in far band | Area dash, spread shot |
| Air / Close / Single | Airborne, one enemy within close band | Air string, juggle |
| Air / Close / Crowd | Airborne, crowd within close band | Air spin, multi-target juggle |
| Air / Far / Single | Airborne, target in far band | Dive, teleport, air shot |
| Air / Far / Crowd | Airborne, crowd in far band | Slam from above, air volley |
| Riser | On ground, target airborne above you | Leap up to the target |
| Post-dodge | Attack within a short window after a dodge | Counter, reposition strike |
| Swap-in | Attack within a short window after a swap | The loadout's entrance move |
| Launcher | Alternate attack button mid-combo, on ground | Pop the target up |
| Finisher (ground) | Last hit of a ground combo | Knockback, area, stagger |
| Finisher (air) | Last hit of an air combo | Spike, slam, pull |

Candidates held back for later: Post-guard, Knocked-down recovery, Low-HP. They are proven in KH II but each adds a rule the player has to track. Surrounded is now covered by the Crowd cells of the grid.

### Resolution rules

&#91;embedded content: attack resolution · 5 checks, 1 fallback\]

The first check that passes names the slot; the active loadout then either has a move there or falls back to the weapon's basic attack.

1. **Situational beats spatial.** Swap-in, then Post-dodge, then Riser are checked before the stance and range grid. Finisher replaces whatever the grid would have chosen on the last hit. This order is a starting point: it lives in data, so designers can reorder it and test other priority configurations.
2. **An empty slot falls back to the weapon's basic attack** for that stance. A loadout with zero AP spent still fights; it just has no range coverage and no tricks.
3. **Context is sampled on the press, not continuously.** Once a move starts it finishes or is cancelled; it never morphs mid-swing.
4. **Range bands use hysteresis.** A target on the boundary does not flicker between bands from frame to frame.

### Starting numbers

These are first guesses to tune in the prototype, not targets.

| Parameter | Start value |
| --- | --- |
| Close band | 0 to 4 m |
| Far band | 4 to 15 m |
| Band hysteresis | 0.5 m |
| Crowd threshold | 3 or more enemies within 4 m of the target |
| Post-dodge window | 0.4 s |
| Swap-in window | 0.5 s |
| Input buffer | 0.25 s |

### Readability

The lock-on reticle shows the current band (shape or ring count), a crowd marker when the Crowd cell is live, and a small icon for the move the next press will produce. If the player cannot predict the move from the reticle alone, the system has failed pillar 1. A debug overlay listing the live context tags and the resolved ability is the first thing to build.

### Targeting

Soft lock picks the best target by distance and by stick or camera direction, and all context is measured against it. Hard lock works as in Dark Souls: pressing the right stick in (R3) locks on or releases, and moving the right stick while locked switches to the next target in that direction. With no target in range, the grid resolves as Close and the move plays in the facing direction.

## Loadouts and AP

A loadout is one weapon plus the abilities slotted into its context table. The player starts with two and unlocks a third as the game progresses, each with its own AP budget.

### What a loadout contains

- **Weapon.** Sets the free basic attacks for ground and air, base speed, reach and stagger, and which abilities are allowed.
- **Context abilities.** At most one per slot from the context table. Each costs AP.
- **Combo shapers.** Passive modifiers to the string itself: an extra hit, one fewer hit to reach the finisher sooner, a second finisher.
- **Mobility.** The dodge type (roll, step, short teleport) and air options (double jump, air dash, glide).
- **Swap-in ability.** The entrance move, covered under Style switching.

### Ability pools

Abilities are either **universal** (any weapon can slot them) or **weapon-class** (heavy, dual, ranged). Universal abilities keep builds flexible; class abilities give each weapon its identity. A first target is about 8 universal and 4 per weapon class, which is enough to force choices without a large animation bill.

### AP budget

Recommended: **a separate AP budget for each loadout.** Every kit gets the same small budget, about half of what it takes to fill its slots, so each one is strong in a few contexts and empty in the rest. No amount of menu work makes one loadout cover everything, so switching mid-fight is the expected way to play, not a niche option. A shared pool would let players pour everything into one main kit and ignore the others.

Starting costs, to be tuned:

| Ability type | AP |
| --- | --- |
| Grid ability (stance, range and crowd) | 2 |
| Riser, Post-dodge | 2 |
| Swap-in | 3 |
| Launcher | 3 |
| Finisher | 3 |
| Combo shaper: extra hit | 1 |
| Combo shaper: shorter combo | 2 |
| Mobility upgrade | 3 |

Each loadout starts with 10 AP, which fills roughly half its slots. That is the intended pressure: no single loadout can cover every range, so the gaps are what the other loadouts are for.

### How many loadouts

- **Two** makes swap a single toggle button with no selection step. Best for proving the core loop.
- **Three** allows the full diver, juggler, zoner triangle and needs a direct-select input (two shoulder buttons for next and previous, or d-pad). Matches KH III's three keyblades.
- **Four or more** starts to recreate DMC's memorization load. Not planned.

**Loadout slots unlock with progression.** The player starts with two, which keeps swap a simple toggle while the core loop is learned. A third slot unlocks once enemy variety starts asking for more strategy. By then, two-loadout swapping is second nature, so adding a third feels like an extension of a habit rather than a new system. Early encounters should be tuned to be fully solvable with two loadouts.

## Style switching

A swap is one button press that replaces the whole context table, takes about a tenth of a second, and keeps the combo going. The combo counter, air state and target all carry over, so a string can start in one loadout and finish in another.

### Rules

1. **Swap cancels recovery.** Any attack that has connected can be cancelled into a swap. Whiffed attacks cannot, so swapping is not a free escape from a bad commitment. This works like a fighting-game special cancel, and like those it will likely produce emergent combos nobody designed. Watch for them in playtests: keep the ones that are fun, and close the ones that break the swap rules.
2. **Combo position carries over.** Three hits in loadout A then a swap means the next press is hit four, in loadout B's table. The finisher that ends the string belongs to whichever loadout is active on the last hit.
3. **Swap-in is a context.** For a short window after the swap, the attack button fires the new loadout's Swap-in ability. Not pressing attack simply lets the window lapse.
4. **Swap works in the air and during a dodge.** Swapping mid-dodge keeps the dodge's invulnerability, and the Post-dodge slot that follows is the new loadout's.
5. **No swap while in hitstun** in the first prototype. A defensive burst-swap that costs a resource is a later experiment.

### Why the player swaps

KH III shows that a free swap with no reason behind it goes unused. Three reasons are built in, in order of how much they are trusted:

- **Coverage gaps.** Each loadout's own budget is too small to fill every slot. The heavy weapon has no Far answer; the daggers have no launcher. This is structural and costs nothing extra to build.
- **Off-hand charge.** Damage dealt with the active loadout charges the inactive ones. A loadout's Swap-in ability scales with its charge, from a plain strike at zero to its signature move at full. This turns swapping into a rhythm: build with one kit, cash out with the other.
- **Enemy pressure.** Enemies make specific ranges or heights unsafe for a while. This is the reason that makes or breaks the system: loadout archetypes and enemy types must be designed together, so each enemy pressures some archetypes and favors others. See the pressure matrix under Enemy and encounter implications.

### Cost and friction

The first prototype has no cooldown and no cost. Friction is added only if testing shows degenerate swap spam. The options, in the order to try them: a short lockout of about one second after a swap; Swap-in abilities that need charge to fire at all; a swap gauge. Starting free keeps the feel honest and shows what actually needs limiting.

### Presentation

The swap needs to read instantly: weapon mesh change, a distinct sound per loadout, a color flash on the character and the reticle, and the HUD's context icons flipping to the new table. The animation is a short additive flourish layered over whatever the body is doing, not a full-body montage that would steal control.

## Example loadouts and combat scenarios

Three sample loadouts, each spending its full 10 AP budget. Each is strong in a few slots and empty in the rest on purpose. Ability names are placeholders.

### Breaker (greatsword, heavy class)

Slow, armored, owns the ground. No air game and no answer at Far.

| Slot | Ability | AP | Effect |
| --- | --- | --- | --- |
| Ground / Far / Single | Shoulder Charge | 2 | Armored lunge that closes to Close |
| Launcher | Skyward Cleave | 3 | Pops the target and nearby enemies up |
| Finisher (ground) | Quake | 3 | Area knockdown around the player |
| Combo shaper | Short Fuse | 2 | One fewer hit before the finisher |

### Fang (dual daggers, dual class)

Fast, fragile, owns the air. No launcher and weak against groups on the ground.

| Slot | Ability | AP | Effect |
| --- | --- | --- | --- |
| Swap-in | Blink Strike | 3 | Teleports to the target at any height and strikes |
| Post-dodge | Backstab | 2 | Step behind the target, guard-piercing hit |
| Finisher (air) | Pinwheel Spike | 3 | Multi-hit spin, then slams the target down |
| Combo shaper | Flurry (x2) | 2 | Two extra hits in air strings |

### Longshot (bow, ranged class)

Safe and steady, owns Far and crowds. Low damage up close and no finisher.

| Slot | Ability | AP | Effect |
| --- | --- | --- | --- |
| Ground / Far / Single | Aimed Volley | 2 | Three charged shots, strong stagger |
| Ground / Close / Crowd | Scatter | 2 | Cone shot that pushes enemies back |
| Post-dodge | Backflip Shot | 2 | Fires while leaping away to Far |
| Swap-in | Recoil Shot | 3 | Point-blank blast that throws the player back |
| Combo shaper | Extra hit | 1 | One more shot per string |

### Scenario A: launch heavy, cash out light (Breaker and Fang)

1. Target at Far. Attack fires Shoulder Charge and arrives at Close with armor.
2. Two basic greatsword hits. Each hit charges Fang.
3. Alternate attack fires Skyward Cleave. The target and its neighbours are airborne.
4. Swap during Cleave's recovery. Attack inside the Swap-in window fires Blink Strike, which puts the player next to the airborne target.
5. Fang's air string runs with two extra hits, above anything still on the ground.
6. The last hit is Pinwheel Spike. The target slams down; the player lands.
7. Swap back. The hits in the air charged Breaker, so its entrance is ready for the group on the ground.

The player pressed attack, alternate attack and swap. Every move was chosen by position and loadout.

### Scenario B: dive in, back out when it turns (Fang and Longshot)

1. Fang closes and works a single target, using dodge then Backstab to stay behind its guard.
2. Health is low, or the enemy telegraphs an area attack. The player dodges away and swaps during the dodge.
3. Attack after the dodge now reads Longshot's table: Backflip Shot fires and carries the player out to Far.
4. From Far, attack is Aimed Volley. The shots stagger and charge Fang.
5. The enemy staggers. Swap, and Blink Strike returns the player to melee range at full charge.

The same dodge-then-attack input produced an aggressive flank in step 1 and a retreat in step 3. That is the loadout-as-role pillar working.

## Controls and input

The whole combat vocabulary is six buttons, and only three of them are used inside a combo: attack, alternate attack, swap. Gamepad first; keyboard and mouse mapping follows once the feel is right.

| Action | Xbox / PlayStation | Notes |
| --- | --- | --- |
| Attack | X / Square | Resolves through the context table |
| Alternate attack | Y / Triangle | Launcher mid-combo; other uses open |
| Jump | A / Cross | Second press for the air option |
| Dodge | B / Circle | Type set by the active loadout |
| Swap | RB / R1 | Toggle with two loadouts; next with three |
| Swap back | LB / L1 | Previous, only needed with three loadouts |
| Guard | LT / L2 | Held. Out of scope until Post-guard exists |
| Lock-on | R3 | Press in to toggle hard lock; move the right stick while locked to switch target (Dark Souls style) |

**Every binding is remappable.** The table above is the default. Full rebinding ships from the first playable build, through Unreal's Enhanced Input user settings. A second preset, **Shoulder attack**, keeps the right thumb on the camera while fighting: Attack on RB / R1, Alternate attack on RT / R2, Swap on LB / L1, and Swap back on Y / Triangle. Guard stays on LT / L2. Presets are a starting point; any action can still be moved after picking one.

### Buffering and cancels

- **Input buffer.** A press up to 0.25 s before a move becomes cancellable is held and fires on the first valid frame. Without this, the game feels like it drops inputs.
- **Cancel windows are authored per move** as notify windows on the animation: when the next attack can chain, when a dodge can cancel, when a swap can cancel. Dodge windows open earliest, then swap, then attack.
- **Context is sampled when the buffered input fires,** not when it was pressed. The player's position at the moment the move starts is what counts.
- **Alternate attack outside a combo** is unassigned for now. Candidates: a held charge version of the current slot, or a second independent context table. Leave it empty until the single table is proven.

## Enemy and encounter implications

Swapping only matters if enemies make one range, height or tempo temporarily wrong. Each enemy type below exists to close one context and open another, and the prototype needs only the first two to test the idea.

| Enemy | What it denies | What it rewards | Prototype |
| --- | --- | --- | --- |
| Brute | Staying at Close: periodic telegraphed area pulse, cannot be launched by light weapons | Heavy launcher, or backing out to Far during the pulse | Yes |
| Skirmisher | Ground play: hops to the air or keeps at Far, low health | Riser, air strings, gap-closers | Yes |
| Shieldbearer | Frontal attacks at any range | Post-dodge flank, guard-piercing moves | Later |
| Caster | Ignoring Far: slow, punishing projectiles from distance | Teleport and dive abilities, ranged trades | Later |

### Pressure by archetype

Enemy design makes or breaks this system, so archetypes and enemies are designed as a pair. Every enemy must pressure at least one archetype and favor a different one, so the right answer to a mixed room is a swap. The matrix below checks the three sample loadouts against the four enemy types; it gets a new row or column whenever an enemy or archetype is added.

| Enemy | Breaker (greatsword) | Fang (daggers) | Longshot (ranged) |
| --- | --- | --- | --- |
| Brute | Favored: armor and heavy launcher | Pressured: can't launch it, fragile at Close | Favored: stays at Far |
| Skirmisher | Pressured: no Far or air answer | Favored: Blink Strike reaches any height | Neutral |
| Shieldbearer | Neutral | Favored: Backstab flanks the shield | Pressured: frontal shots are blocked |
| Caster | Pressured: no Far answer | Favored: teleports through projectiles | Favored: wins ranged trades |
| Juggernaut (later) | Pressured: heavy hits can't stagger it | Pressured: light hits can't stagger it | Favored to open it, pressured to finish it |

The check for each new archetype: at least one prototype enemy pressures it. The check for each new enemy: it does not pressure or favor every archetype the same way. Ratings are design intent, to be confirmed in playtests.

### Stagger tiers

Every hit has a stagger power, and every enemy has a poise tier. A hit only causes hitstun when its power meets the enemy's tier. Armor becomes a hard reason to swap: if your current loadout can't interrupt the enemy, the fix is to change kits, not to keep mashing.

| Enemy poise | Staggered by | Not staggered by | Example |
| --- | --- | --- | --- |
| Unarmored | Everything | Nothing | Skirmisher, Caster |
| Armored | Heavy hits, charged shots | Light hits (Fang) | Brute |
| Juggernaut | Charged shots only | Light and heavy hits | Later enemy, after the prototype |

- **Unstaggerable is not invulnerable.** Light hits still deal damage and build off-hand charge. They just can't interrupt the enemy's attacks.
- **Don't let the bow become the universal answer.** The Juggernaut must pressure Longshot another way, for example by closing distance fast or reflecting shots from the front, so charged shots open it up but can't finish it alone.
- **Implementation.** Poise tier is a Gameplay Tag on the enemy and stagger power is a field on the ability definition, both editable by designers.

### Encounter rules of thumb

- **Mix at least two denial types per encounter.** A room of only Brutes is solved by one loadout.
- **Telegraphs are long and readable.** The player needs time to decide to swap, not just to dodge. A decision window of about a second is the starting point.
- **Airborne is a safe zone with a clock.** Juggling above a crowd is the reward for a good launch; enemies with anti-air or the combo's hit limit bring the player back down.
- **A training room comes first.** One dummy with toggles for airborne, guarding, armored and distance is enough to tune every context before any AI exists.

## Risks and open questions

The biggest risk is that swapping is optional: if one well-built loadout clears every fight, the design collapses into Kingdom Hearts with extra menus. Everything else is tuning.

| Question | Current lean | What the prototype has to show |
| --- | --- | --- |
| Is swapping necessary or just available? | Coverage gaps plus off-hand charge | In a mixed Brute and Skirmisher room, a single-loadout run is clearly slower or riskier than a two-loadout run |
| Can the player predict the move? | One ability per slot, reticle shows band and next move | A new player names the move before pressing, most of the time, after a few minutes |
| Shared AP pool or one budget per loadout? | Per loadout | Players use every loadout in a fight, and no single kit clears it alone |
| Does combo position carry across a swap? | Yes | Launch-swap-juggle works in one string without feeling like a reset or an exploit |
| Free swap or gated? | Free first | Whether swap spam appears, and whether it is fun or degenerate |
| How many contexts can a player hold in their head? | 12 slots, about half filled | Whether players use the situational slots or only the grid |
| Two loadouts or three? | Two for the prototype | Whether a toggle feels limiting once the loop works |
| What does alternate attack do outside a combo? | Unassigned | Whether one attack button feels thin after an hour |
| Do range bands feel fair? | 2.5 m, 7 m, 15 m with hysteresis | Players are not surprised by a band flip at the boundary |
| Is there a style rank? | Not in the prototype | Whether the loop is rewarding without external scoring |

### Smaller risks

- **Animation bill.** Fourteen slots times three weapons is 42 moves before variants. Mitigation: universal abilities, motion warping so one gap-closer serves many distances, and placeholder animations until the design holds.
- **Camera.** Launches and teleports move the player far and fast. Lock-on camera behavior needs attention as early as the first air combo.
- **Menu weight.** Building several loadouts across 12 slots is a lot of setup. Presets and a copy-loadout action should exist from the first menu.

### Prototype animation scope

The prototype only animates what the two sample loadouts actually slot. That means Breaker and Fang, about four abilities each, plus the basic strings, dodge and swap flourish. The total is roughly 15 to 20 moves instead of 42. Everything comes from free packs, retargeted to the UE5 mannequin with IK Retargeter. Paragon's free heroes happen to match the three sample archetypes almost exactly.

| Source | Cost | Covers | Notes |
| --- | --- | --- | --- |
| Paragon: [Greystone, Kallari, Sparrow](https://www.unrealengine.com/en-US/blog/final-round-of-free-paragon-assets-released) | Free | Greatsword, dual daggers and bow kits, with full attack and ability sets | Epic released all Paragon heroes free for commercial and personal Unreal projects. Built for UE4, so they need a UE5 import and retarget. |
| [Game Animation Sample](https://www.fab.com/listings/259f8545-f820-47b3-8fc1-e8ec5458214d) | Free | Locomotion, jumps, traversal, plus a few interactions | Epic's 500+ animation sample, already on the UE5 mannequin. No combat moves. This listing is a community retarget of it. |
| [Mixamo](https://app.cinevva.com/guides/free-character-animations-rigging) | Free with an Adobe ID | Generic sword, great sword and bow sets for gaps | Royalty-free for commercial use. The exact weapon packs are from memory: check the library before relying on it. |
| [GreatSword pack (AniBearlee)](https://www.fab.com/listings/58dbe678-f46e-41c3-86a1-4389ca77f94b) | Paid (price not shown on the listing) | 112 greatsword sequences, including a 5-hit combo, dash, slam, cleaves, aerial attack, hit reactions and lock-on movement | Only needed if Greystone falls short. UE4 skeleton; UE5 needs a retarget. |

Motion warping still does the distance work, so a single lunge or gap-closer clip covers every Far range.

## Unreal Engine prototype plan

The prototype is three small components on the character, a resolver between input and the Gameplay Ability System, and data assets for everything a designer tunes. Loadouts and abilities are data, so rebuilding a kit never needs a recompile.

&#91;embedded content: prototype architecture · runtime components over data assets\]

The resolver is the only new idea; everything to its right is standard Gameplay Ability System, and everything below it is state or data it reads.

### Pieces

- **Combat Context component.** Builds a Gameplay Tag container on demand: `Context.Stance.Air`, `Context.Range.Far`, `Context.Target.Airborne`, `Context.Recent.Dodge`, `Context.Recent.Swap`, `Context.Combo.Finisher`. Reads the movement component, the current target and short timers.
- **Targeting component.** Scores candidates by distance and by stick or camera direction, owns soft and hard lock, and supplies the warp target.
- **Loadout component.** Holds the equipped loadouts and the active index, the combo counter and the off-hand charge. Combo state lives here, not inside an ability, so it survives a swap.
- **Resolver.** On a buffered attack input: ask for the context, apply the priority order (read from a designer-editable Resolver Priority data asset, never hard-coded, so alternative orders can be swapped in and tested), look up the slot in the active loadout, and activate that ability. Falls back to the weapon's basic attack.
- **Loadout data asset.** Weapon definition, a map from slot tag to ability definition, combo shapers, dodge type.
- **Ability definition data asset.** Gameplay Ability class, montage, AP cost, allowed weapon classes, warp settings, damage and stagger values.
- **Base context-attack ability.** Plays the montage, sets the motion warp target, listens for hit events from the animation, applies damage through a Gameplay Effect. Individual moves are Blueprint children that mostly just change data.
- **Animation notify windows.** Hit window (weapon trace on), combo window, dodge-cancel window, swap-cancel window, motion warp window.
- **Enemy.** Its own Ability System Component with state tags (`State.Airborne`, `State.Staggered`, `State.Armored`), launch and hitstop reactions, and simple behavior. A StateTree or Behavior Tree is enough.
- **Debug overlay.** Live context tags, the resolved slot and ability, combo step, charge, range band rings drawn in the world.

### Engine choices

- **Version.** Epic's current documentation is for UE 5.8; use the latest stable 5.x release. Start from the Third Person template, not Lyra. Lyra is a useful reference for how Epic wires GAS and input, but too large to prototype inside.
- **C++ for the spine, Blueprint for content.** Components, resolver and the base ability in C++; individual abilities, loadouts and tuning in Blueprint and data assets. GAS needs a small amount of C++ for attribute sets regardless.
- **Enhanced Input** for actions, with a small custom buffer on top.
- **Motion Warping plugin** for every move that travels to a target.
- **Placeholder animation.** Free marketplace or Mixamo sword, dagger and bow sets, retargeted to the template mannequin. Timing and cancel windows matter far more than polish at this stage.

### Milestones

Each milestone ends in something playable and has one exit test. No dates are set yet.

1. **Graybox and context HUD.** Third Person template, a dummy, lock-on, the Context component and the debug overlay. *Exit:* walking and jumping around the dummy flips the tags correctly and the band rings match what the overlay says.
2. **One loadout, the eight-cell grid.** Resolver, base ability, eight placeholder moves with motion warping. *Exit:* each cell produces a visibly different move and the reticle predicts it.
3. **Combo, dodge, launch.** Combo counter, finishers, dodge with Post-dodge, launcher, air state on the dummy, hitstop. *Exit:* ground string, launch, air string, air finisher on the dummy, repeatably.
4. **Second loadout and swap.** Loadout component swap, cancel windows, combo carry-over, Swap-in, off-hand charge. *Exit:* Scenario A performed in one unbroken string.
5. **AP and a loadout editor.** Per-loadout AP budgets, a plain menu to assign abilities to slots, save and load presets. *Exit:* a new build goes from idea to playable in under a minute, with no editor open.
6. **Two enemies and the real test.** Brute and Skirmisher, a mixed arena. *Exit:* answers to the first three rows of the open questions table.

Milestones 1 to 4 are the feel prototype. If the swap in milestone 4 is not fun against a dummy, stop and rework before building the menu or the enemies.
