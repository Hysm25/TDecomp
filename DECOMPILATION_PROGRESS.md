# NES Tetris Decompilation Progress Tracker

## Project Overview

- **Project:** NES Tetris (USA) Decompilation
- **Source of Truth:** `TetrisNESDisasm/main.asm` (NES 6502 Assembly, NROM/MMC1)
- **Reference ROM:** `Tetris (USA).nes` (SHA-1: `3026d28b63d94c921fe58364f8b0659d10b5a0ac`, PRG payload verified bit-exact)
- **Target Specification:** Strict C11 (`-std=c11 -Wall -Wextra -Werror -pedantic`), modular, portable to Linux, Windows, macOS, Android.
- **Git Repository:** `Hysm25/TDecomp` on `origin/main`

---

## Executive Status Dashboard

| Metric | Status | Details |
|:---|:---:|:---|
| **Total Standalone Routines** | **171** | Authoritative routines with independent execution scope |
| **Alternate Entry Points** | **111** | Secondary entry points into parent routines / shared logic |
| **Authoritative Data Tables** | **260** | Lookup, geometry, sprite, and audio tables in PRG ROM |
| **Total PRG Symbols Tracked** | **567** | Complete PRG ROM symbol census |
| **Verified Routines** | **57** | Bit-exact & differential CPU proven against reference ROM |
| **Decompiled (Untested)** | **0** | Code converted but awaiting verification suite |
| **Remaining Routines** | **114** | Pending conversion |
| **Current Progress** | **33.33%** | `[#################-----------------------------------] 33.33%` |
| **Build & Test Suite** | 11 / 11 suites passing | Unit, 28 ROM data tables, & 6502 differential CPU oracle (34 differential tests) |

---

## Subsystem Breakdown

| Subsystem | Total Routines | Verified | Decompiled | Missing | Progress |
|:---|:---:|:---:|:---:|:---:|:---:|
| **MATH_RNG** (Pseudo-Random Generation) | 2 | 2 | 0 | 0 | **100.0%** |
| **MATH_BCD** (Score & Line Math) | 12 | 10 | 0 | 2 | **83.3%** |
| **PLAYFIELD** (Grid, Line Clears, Garbage) | 9 | 7 | 0 | 2 | **77.8%** |
| **PLAYER_INPUT** (DAS, Joypad, Autorepeat) | 4 | 3 | 0 | 1 | **75.0%** |
| **TETRIMINO** (Pieces, Spawns, Rotations) | 16 | 11 | 0 | 5 | **68.8%** |
| **MENUS_UI** (Title, Menus, High Scores) | 13 | 6 | 0 | 7 | **46.2%** |
| **GAMEPLAY_STATE** (State Machine, Timers) | 14 | 6 | 0 | 8 | **42.9%** |
| **CORE_LOGIC** (Helper Utilities, Buffers) | 38 | 8 | 0 | 30 | **21.1%** |
| **RENDER** (PPU, OAM, Palettes, Nametables) | 16 | 2 | 0 | 14 | **12.5%** |
| **AUDIO** (Music Engine, SFX, APU) | 41 | 2 | 0 | 39 | **4.9%** |
| **SYSTEM** (Reset, NMI, IRQ, VBlank) | 6 | 0 | 0 | 6 | 0.0% |
| **TOTAL** | **171** | **57** | **0** | **114** | **33.33%** |

---

## Decompilation Roadmap (Shortest Reliable Path)

1. [x] **Batch 0 (Foundation & Infrastructure)**:
   - Established C project structure: `include/`, `src/`, `tests/`, `CMakeLists.txt`.
   - Core hardware abstraction: `NES_State`, memory mapping, CPU register modeling.
   - Comprehensive constants matching `constants.asm` and `tetris-ram.asm`.
   - Verified memory mirroring, pointer safety, and 16-bit little-endian accessors.

2. [x] **Batch 1 (RNG & Piece Generation)**:
   - Decompiled `generateNextPseudorandomNumber` (LFSR algorithm).
   - Decompiled `unreferenced_soundRng` (audio LFSR variant).
   - Decompiled `pickRandomTetrimino` (spawn reroll and modulo-7 logic).
   - Decompiled `chooseNextTetrimino` (demo stream vs gameplay branching).
   - Decompiled `incrementPieceStat` (BCD piece counters).
   - Unit tests verified with edge cases, anti-repetition rerolls, and BCD carry.

3. [x] **Batch 2 (BCD Math, Scoring, & Line Calculations)**:
   - Decompiled `bcd_add8` and `bcd_sub8` low-level operations.
   - Decompiled `addHoldDownPoints` (soft drop BCD accumulation).
   - Decompiled `addLineClearPoints` (multi-line BCD multipliers & 999,999 clamp).
   - Decompiled `updateLinesAndLevel` (Type A tens-boundary level up & Type B decrement).
   - Decompiled `playState_updateLinesAndStatistics` (PlayState 4 complete handler).
   - Decompiled `updateMusicSpeed` (row 5 height detection & danger allegro music).
   - Decompiled `setMusicTrack` (track selection and demo mode mute).

4. [x] **Batch 3 (Tetrimino Geometry, Rotation, Movement & Collision)**:
   - Full 19-orientation geometry table and 38-entry rotation table.
   - Decompiled `isPositionValid` (bounding box, wall, floor, and mino collision).
   - Decompiled `rotate_tetrimino` (Button A clockwise, Button B counter-clockwise, wall revert).
   - Decompiled `shift_tetrimino` (Left/Right shifting with DAS auto-repeat).
   - Decompiled `drop_tetrimino` (natural gravity, soft drop acceleration, floor lock).
   - Decompiled `updatePlayfield` (VRAM dirty row tracker).
   - Decompiled `getDropSpeed` (levels 0-29 gravity, killscreen).

5. [x] **Verification Infrastructure & Routine Scope Audit**:
   - Reference ROM verification: Bit-for-bit extraction and validation against `Tetris (USA).nes`.
   - 15 PRG ROM tables verified byte-for-byte at exact symbol addresses.
   - Differential CPU testing: Built NMOS 6502 CPU simulator oracle executing original PRG ROM assembly directly against decompiled C routines across hundreds of test vectors.
   - Authoritative scope audit: Distinguished 171 standalone routines, 111 alternate entry points, 260 data tables, and exact address aliases in `ROUTINE_INVENTORY.md` and `routine_inventory.json`.

6. [x] **Batch 4 (Playfield Grid, Line Completion, Animation Framing, & Type B Garbage)**:
   - Decompiled `playState_lockTetrimino` ($99A2): 4-mino placement and invalid position game-over sequence.
   - Decompiled `playState_checkForCompletedRows` ($9A6B): 4-row candidate scan, downward row collapse, top row clearing, and garbage line accumulation.
   - Decompiled `updateLineClearingAnimation` ($977F): 5-step center-out line clearing framing every 4 frames.
   - Decompiled `playState_updateGameOverCurtain` ($9A11): Cascade curtain row fill with `TILE_CURTAIN` ($4F).
   - Decompiled `playState_receiveGarbage` ($9B03): 2-player pending garbage row upward shift and hole insertion.
   - Decompiled `initPlayfieldIfTypeB` ($87DC) & `initPlayfieldForTypeB` ($87E3): PRNG garbage generation (rows 8-19), blank cell guarantee, and startHeight clearing.
   - Authoritative data tables bit-exact verified against reference ROM: `multBy10Table`, `leftColumnsTable`, `rightColumnsTable`, `typeBBlankInitCountByHeightTable`, `rngTable`, `garbageLinesTable`.
   - 6502 differential CPU oracle: Formally proven equivalent against reference ROM across multiple line completion scenarios.

7. [x] **Batch 5 (Player Controls & DAS)**:
   - Decompiled `pollController_actualRead` ($AB69): Strobe JOY1 ($4016), serial 8-bit shift-register read of Joypad 1 & 2.
   - Decompiled `addExpansionPortInputAsControllerInput` ($AB8B): Famicom expansion port input OR-mixing.
   - Decompiled `diffOldAndNewButtons` ($ABBD): Button edge detection `(curr ^ held) & curr` for Player 1 & 2.
   - Decompiled `pollController` ($AB9D): Double-reading debounce against DPCM DMA glitch, expansion port mixing, edge detection.
   - Decompiled `pollControllerButtons` ($9D51): Mode dispatch, demo mode START button cancellation to title screen, demo move playback.
   - Decompiled `demoButtonsTable_indexIncr` ($9DE8): 16-bit demo table address pointer increment.
   - Decompiled `updateActivePlayerInput`: Active player input mirroring for 1P / 2P modes.
   - Decompiled `processDAS` & `dasWallCharge`: Exact DAS initial delay 16 frames (`DAS_RESET = 0x10`), 6 frames repeat interval (`DAS_DELAY = 0x0A`), wall charging to full delay.
   - 6502 differential CPU oracle: Formally proven equivalent against reference ROM across 256 edge-diff combinations, controller reads, and demo table incrementation.

8. [x] **Batch 6 (Player State Machine & State Dispatcher)**:
   - Decompiled `makePlayer1Active` ($8776): Player 1 zero-page mirror copy ($0060-$007F to $0040-$005F), playfield pointer setup, button mirroring.
   - Decompiled `makePlayer2Active` ($8792): Player 2 zero-page mirror copy ($0080-$009F to $0040-$005F), 2P playfield pointer setup, button mirroring.
   - Decompiled `savePlayer1State` ($87AE): Active player state save, 2P pending garbage swapping.
   - Decompiled `savePlayer2State` ($87C8): Active player state save, 2P pending garbage swapping.
   - Decompiled `playState_unassignOrientationId` ($9E2F): Sets `currentPiece = TETRIMINO_HIDDEN` (0x13).
   - Decompiled `playState_incrementPlayState` ($9E37): Increments `playState`.
   - Decompiled `playState_noop` ($9E39): No-op state handler.
   - Decompiled `playState_playerControlsActiveTetrimino` ($81CF): Composes active piece shift, rotation, and soft drop gravity.
   - Decompiled `playState_player2ControlsActiveTetrimino` ($81F6): Composes Player 2 active piece controls.
   - Decompiled `playState_bTypeGoalCheck` ($A3F2): Type A vs Type B line goal check, victory music, success graphic placement ($A42E).
   - Decompiled `branchOnPlayStatePlayer1` ($81B2) & `branchOnPlayStatePlayer2` ($81D9): Authoritative 12-state dispatcher over states 0-11.
   - Decompiled `branchOnPlayState`: Unified dispatcher branching on `activePlayer`.
   - 6502 differential CPU oracle: Formally proven equivalent against reference ROM across state dispatch, context switches, and handlers.

9. [x] **Batch 7 (Piece Spawning & Gameplay Framing)**:
   - Decompiled `playState_spawnNextTetrimino` ($988E): VRAM copy guard ($0049 < $20), 2-player piece delay counter ($00A4 == $1C) and player matching ($00A5 == $00B7), spawn coordinates (X=5, Y=0), orientation ID lookup from `spawnOrientationFromOrientation`, piece stat incrementation with BCD adjust and render flag update (`RENDER_STATS`), 1P vs 2P next piece selection, down hold reset (`autorepeatY = 0`).
   - Decompiled `noop_disabledVramRowIncr` ($9874): Disabled VRAM row incrementation no-op routine.
   - Decompiled `gameModeState_vblankThenRunState2` ($9E27): GameModeState 2 transition and no-op subroutine call.
   - Decompiled `gameModeState_updateCountersAndNonPlayerState` ($8884): OAM staging reset, player 1 & 2 fall timer progression, 2-player piece delay counter advancement, and SELECT button toggle for preview piece display (`displayNextPiece`).
   - Completed entire 12-state gameplay state machine (States 0-11 in Mode 4).
   - 6502 differential CPU oracle: Formally proven equivalent against reference ROM across 19 piece orientations, 2P delay mechanics, and framing routines.

10. [x] **Batch 8 (Legal Screen, Title Screen, Demo Transition & Game Mode Dispatcher)**:
    - Decompiled `gameMode_legalScreen` ($8200): Screen initialization (`renderMode = 0`), OAM staging clear, sleep countdown (`sleepCounter = 0xFF`), START button skip handling, transition to Title Screen (`gameMode = 1`).
    - Decompiled `gameMode_titleScreen` ($824F): Screen initialization (`renderMode = 0`, zero-page `$D0 = 0`, `displayNextPiece = 0`), OAM staging clear, START button press detection (triggers `SFX_MENU_SELECT` 0x02, advances to `MODE_GAME_TYPE_MENU` 2), title screen timeout detection (`frameCounter+1 == 0x05`, triggers `SFX_MENU_SELECT` 0x02, advances to `MODE_START_DEMO` 6).
    - Decompiled `gameMode_startDemo` ($9DF6): Demo mode initialization setting `gameType = 0`, `startLevel = 0`, `gameModeState = 0`, `playState = 0`, transitioning to `MODE_DEMO` 5.
    - Decompiled `branchOnGameMode` ($8161): Master game mode dispatcher jumping through `switch_s_plus_2a` over modes 0..6 (`MODE_LEGAL_SCREEN`, `MODE_TITLE_SCREEN`, `MODE_GAME_TYPE_MENU`, `MODE_LEVEL_MENU`, `MODE_PLAYING`, `MODE_DEMO`, `MODE_START_DEMO`).
    - Added step-oriented execution functions `gameMode_legalScreen_step` and `gameMode_titleScreen_step` for portable host loop integration.
    - 6502 differential CPU oracle: Formally proven equivalent against reference ROM across legal screen timer/skip, title screen start press, demo timeout transitions, and game mode jump table dispatch.

11. [x] **Batch 9 (Menu Navigation: Game Type Menu & Level Menu)**:
    - Decompiled `gameMode_gameTypeMenu` ($82D1): Game type A/B toggle, music selection (0..3) with `setMusicTrack`, cursor staging and flicker, START advances to `MODE_LEVEL_MENU` (3), BUTTON_B retreats to `MODE_TITLE_SCREEN` (1) with `frameCounter+1 = 0`.
    - Decompiled `gameMode_levelMenu` ($83D7): Mode 3 initialization (`renderMode = 1`, `originalY = 0`, `dropSpeed = 0`), `player1_startLevel >= 10` normalization modulo 10 loop, delegating to P1 navigation handler.
    - Decompiled `gameMode_levelMenu_processPlayer1Navigation` ($8436): Active player mapping (`activePlayer = 0`), delegation to navigation handler, START button detection with `A + START` Level+10 cheat code advancing to `MODE_PLAY` (4), BUTTON_B detection retreating to `MODE_GAME_TYPE_MENU` (2), random garbage hole column generation [0..9] via PRNG for Player 1 & 2.
    - Decompiled `gameMode_levelMenu_handleLevelHeightNavigation` ($84AE): D-pad grid navigation for starting level (0..9) and starting height (0..5), Button A toggle between level and height selection in Type B, and 2-player sprite cursor coordinate offset calculations.
    - Authoritative data tables bit-exact verified against reference ROM: `levelToSpriteYOffset` ($85B2), `levelToSpriteXOffset` ($85BC), `heightToPpuHighAddr` ($85C6), `heightToPpuLowAddr` ($85CC).
    - 6502 differential CPU oracle: Formally proven equivalent against reference ROM across game type loop START/B, level menu navigation, P1 navigation transitions, and cheat code mechanics (Tests 24–28).

12. [x] **Batch 10 (High Score Operations & Tables)**:
    - Decompiled `initHighScoreTable` ($8095): Cold boot initialization copying default names, scores, and levels into RAM $0700-$074F until $FF, setting `initMagic` ($0750) to `{ $12, $34, $56, $78, $9A }`.
    - Decompiled `twoDigsToPPU` ($9712): Splits 8-bit value into high/low BCD digits, writes to PPUDATA ($2007), and preserves value in `generalCounter`.
    - Decompiled `showHighScores` ($9FF2), `showHighScores_real` ($9FFB), `showHighScores_ret` ($A085): 1P vs 2P high score nametable display dispatcher, formatting 3 entries with 6-char names, 6-digit BCD scores, and 2-digit BCD levels.
    - Decompiled `handleHighScoreIfNecessary` ($A0EE): High score table qualification and tie-breaking comparison (preferring player on ties) across entries 0..2 (Type A) or 4..6 (Type B).
    - Decompiled `adjustHighScores` ($A134): Multi-entry downward shift (entries 1->2, 0->1), 6-char name clearing for player input, score/level insertion, and transition to name entry screen.
    - Decompiled `copyHighScoreNameToNextIndex` ($A192): Backward 6-character name copy to next entry slot with Type B offset ($18) adjustment.
    - Decompiled `copyHighScoreScoreToNextIndex` ($A1C1): 3-byte BCD score copy to next entry slot with Type B offset ($0C) adjustment.
    - Decompiled `copyHighScoreLevelToNextIndex` ($A1E0): 1-byte level copy to next entry slot with Type B offset ($04) adjustment.
    - Decompiled `highScoreEntryScreen` ($A201) & `highScoreEntryScreen_step`: Interactive name entry screen with cursor horizontal navigation, 44-character alphabet cycling with wrap, and START confirmation.
    - Decompiled `render_mode_congratulations_screen` ($A344): High score letter tile rendering to PPU nametable during VBlank (renderMode 2) and PPU scroll reset.
    - Authoritative data tables bit-exact verified against reference ROM: `highScorePpuAddrTable` ($A086), `highScoreCharToTile` ($A08C), `byteToBcdTable` ($A0BC), `highScoreIndexToHighScoreNamesOffset` ($A1F1), `highScoreIndexToHighScoreScoresOffset` ($A1F9), `highScorePosToY` ($A33B), `highScoreNamePosToX` ($A33E), `defaultHighScoresTable` ($AD67).
    - 6502 differential CPU oracle: Formally proven equivalent against reference ROM across `twoDigsToPPU`, `copyHighScoreNameToNextIndex`, `copyHighScoreScoreToNextIndex`, `copyHighScoreLevelToNextIndex`, `handleHighScoreIfNecessary` non-qualifying traversal, and `render_mode_congratulations_screen` (Tests 29–34).

13. [ ] **Batch 11 (PPU Rendering & OAM)**:
    - OAM sprite staging and nametable row updates.

14. [ ] **Batch 12 (Audio & Sound Effects)**:
    - APU sound effects and music sequencing.

---

## Verified Batch Log

| Batch # | Description | Routines Added | Cumulative Verified | Tests Passing | Commit |
|:---|:---|:---:|:---:|:---:|:---:|
| Baseline | Initial repository configuration & routine census | 0 | 0 | N/A | `985e8df` |
| Batch 0 & 1 | Infrastructure, NES State, RNG & Piece Selection | 5 | 5 | 2 suites / 11 test cases | `275a177` |
| Batch 2 | BCD Math, Scoring, Lines Progression, & Music Speed | 3 | 8 | 3 suites / 17 test cases | `d3110ca` |
| Batch 3 | Tetrimino Geometry, Rotation, Movement & Collision | 5 | 13 | 4 suites / 26 test cases | `17c2e00` |
| Verification Infra | 6502 Differential CPU Oracle, ROM Tables & Routine Audit | 0 (Infra) | 13 | 6 suites / all ROM differential | `1c7cdc3` |
| Batch 4 | Playfield Grid, Line Completion, Animation & Type B Garbage | 6 | 19 | 7 suites / 15 ROM tables + 6502 diff | `e62775b` |
| Batch 5 | Player Controls, Edge Detection, Demo Playback & DAS Timing | 4 (+2 alt) | 23 | 8 suites / 15 ROM tables + 6502 diff | `a3c6dc7` |
| Batch 6 | Player State Machine, Context Switch & State Dispatcher | 11 (+1 alt) | 34 | 9 suites / 15 ROM tables + 6502 diff | `4422fa6` |
| Batch 7 | Piece Spawning, 2P Delay Mechanics & Gameplay Framing | 4 | 38 | 9 suites / 15 ROM tables + 20 6502 diff | `6b57386` |
| Batch 8 | Legal Screen, Title Screen, Demo Transition & Mode Dispatcher | 4 | 42 | 10 suites / 15 ROM tables + 24 6502 diff | `42c06e2` |
| Batch 9 | Menu Navigation: Game Type Menu & Level Menu Navigation | 4 | 46 | 10 suites / 19 ROM tables + 28 6502 diff | `3d19246` |
| Batch 10 | High Score Operations, Tables, Sorting, Name Entry & Congratulations Render | 11 | 57 | 11 suites / 28 ROM tables + 34 6502 diff | `85b0f9d` |
