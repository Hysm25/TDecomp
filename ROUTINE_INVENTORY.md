# NES Tetris Authoritative Routine Inventory & Scope Audit

- **Disassembly Source of Truth:** `TetrisNESDisasm/main.asm` (NES 6502 Assembly, NROM/MMC1)
- **Reference ROM:** `Tetris (USA).nes` (PRG SHA-1 verified)
- **C Target:** Strict C11 (`-std=c11 -Wall -Wextra -Werror -pedantic`)

---

## 1. Executive Summary & Decompilation Scope

| Metric | Count | Description |
|:---|:---:|:---|
| **Total Standalone Routines** | **171** | Independent callable routines with distinct scope |
| **Alternate Entry Points** | **111** | Secondary entries into existing routines / shared fall-throughs |
| **Authoritative Data Tables** | **260** | Lookup, geometry, sprite, and audio tables in PRG ROM |
| **Exact Address Aliases** | **0** | Identical address alias labels |
| **Total PRG Symbols Tracked** | **567** | Complete symbol census of PRG ROM |
| **Verified (Tested)** | **46** | Bit-exact & differential CPU proven against ROM |
| **Decompiled (Untested)** | **0** | C converted awaiting differential verification |
| **Remaining Routines** | **125** | Pending conversion |
| **Completion Percentage** | **26.90%** | Standalone routine completion rate |

---

## 2. Standalone Routine Category Breakdown

| Category | Total Routines | Verified | Decompiled | Missing | % Complete |
|:---|:---:|:---:|:---:|:---:|:---:|
| **AUDIO** | 41 | 2 | 0 | 39 | 4.9% |
| **CORE_LOGIC** | 38 | 8 | 0 | 30 | 21.1% |
| **GAMEPLAY_STATE** | 14 | 6 | 0 | 8 | 42.9% |
| **MATH_BCD** | 12 | 1 | 0 | 11 | 8.3% |
| **MATH_RNG** | 2 | 2 | 0 | 0 | 100.0% |
| **MENUS_UI** | 13 | 6 | 0 | 7 | 46.2% |
| **PLAYER_INPUT** | 4 | 3 | 0 | 1 | 75.0% |
| **PLAYFIELD** | 9 | 7 | 0 | 2 | 77.8% |
| **RENDER** | 16 | 0 | 0 | 16 | 0.0% |
| **SYSTEM** | 6 | 0 | 0 | 6 | 0.0% |
| **TETRIMINO** | 16 | 11 | 0 | 5 | 68.8% |

---

## 3. Standalone Routines Directory by Category

### AUDIO (41 Routines)

| Routine Name | Address | Lines | Call Mechanism | Status | Notes |
|:---|:---:|:---:|:---:|:---:|:---|
| `updateMusicSpeed` | `0x9D17` | L3596–L3629 | JSR | `VERIFIED` | Verified: Playfield row 5 height check and danger allegro switching differential proven |
| `setMusicTrack` | `0x9E07` | L3746–L3755 | JSR | `VERIFIED` | Verified: Track selection and demo mute override differential proven |
| `updateAudioWaitForNmiAndResetOamStaging` | `0xAA2F` | L5295–L5308 | JSR | `MISSING` |  |
| `updateAudioAndWaitForNmi` | `0xAA45` | L5309–L5318 | JSR | `MISSING` |  |
| `updateAudioWaitForNmiAndDisablePpuRendering` | `0xAA52` | L5319–L5322 | JSR | `MISSING` |  |
| `updateAudioWaitForNmiAndEnablePpuRendering` | `0xAA5F` | L5328–L5335 | JSR | `MISSING` |  |
| `updateAudio_jmp` | `0xE000` | L5851–L5854 | JSR | `MISSING` |  |
| `updateAudio2` | `0xE003` | L5855–L5857 | JSR | `MISSING` |  |
| `computeSoundEffMethod` | `0xE072` | L5933–L5962 | JSR | `MISSING` |  |
| `advanceAudioSlotFrame` | `0xE0B5` | L5979–L5988 | JSR | `MISSING` |  |
| `updateSoundEffectSlot2` | `0xE194` | L6063–L6067 | JSR | `MISSING` |  |
| `initAudioAndMarkInited` | `0xE1E5` | L6114–L6119 | Entry / Direct | `MISSING` |  |
| `updateAudio_pause` | `0xE1EF` | L6120–L6141 | Entry / Direct | `MISSING` |  |
| `updateAudio` | `0xE216` | L6142–L6163 | Entry / Direct | `MISSING` |  |
| `soundEffectSlot2_makesNoSound` | `0xE244` | L6164–L6165 | JSR | `MISSING` |  |
| `muteAudio` | `0xE271` | L6188–L6199 | JSR | `MISSING` |  |
| `initSoundEffectShared` | `0xE287` | L6200–L6234 | JSR | `MISSING` |  |
| `soundEffectSlot0_endingRocketInit` | `0xE2CC` | L6235–L6239 | Jump Table | `MISSING` |  |
| `soundEffectSlot0_makesNoSound` | `0xE2DD` | L6248–L6252 | Jump Table | `MISSING` |  |
| `advanceSoundEffectSlot0WithoutUpdate` | `0xE2E4` | L6253–L6255 | Jump Table | `MISSING` |  |
| `updateSoundEffectSlot0_apu` | `0xE2FF` | L6272–L6276 | Jump Table | `MISSING` |  |
| `updateSoundEffectNoiseAudio` | `0xE307` | L6277–L6287 | Entry / Direct | `MISSING` |  |
| `getSoundEffectNoiseNibble` | `0xE31A` | L6288–L6311 | JSR | `MISSING` |  |
| `soundEffectSlot1_chirpChirpPlaying` | `0xE34F` | L6323–L6345 | Jump Table | `MISSING` |  |
| `soundEffectSlot1_chirpChirpInit` | `0xE37F` | L6346–L6349 | Jump Table | `MISSING` |  |
| `soundEffectSlot1Playing_advance` | `0xE39C` | L6364–L6366 | Jump Table | `MISSING` |  |
| `soundEffectSlot1_tetrisAchievedInit` | `0xE403` | L6426–L6431 | Jump Table | `MISSING` |  |
| `soundEffectSlot1_lineCompletedInit` | `0xE41A` | L6439–L6444 | Jump Table | `MISSING` |  |
| `soundEffectSlot1_levelUpPlaying` | `0xE4D1` | L6524–L6534 | Jump Table | `MISSING` |  |
| `soundEffectSlot1_levelUpInit` | `0xE4EC` | L6537–L6541 | Jump Table | `MISSING` |  |
| `soundEffectSlot2_mediumBuzz` | `0xE50B` | L6547–L6550 | Jump Table | `MISSING` |  |
| `soundEffectSlot3_donk` | `0xE534` | L6568–L6572 | Jump Table | `MISSING` |  |
| `soundEffectSlot3Playing_advance` | `0xE53B` | L6573–L6575 | Jump Table | `MISSING` |  |
| `updateSoundEffectSlot3_apu` | `0xE551` | L6586–L6599 | Jump Table | `MISSING` |  |
| `soundEffectSlot3_fallingAlien` | `0xE56E` | L6600–L6607 | Jump Table | `MISSING` |  |
| `updateMusic_noSoundJmp` | `0xE580` | L6610–L6612 | Entry / Direct | `MISSING` |  |
| `updateMusic` | `0xE583` | L6613–L6635 | JSR | `MISSING` |  |
| `updateMusicFrame_setChanLo` | `0xE5D0` | L6643–L6664 | JSR | `MISSING` |  |
| `initSq12IfTrashedBySoundEffect` | `0xE735` | L6831–L6849 | JSR | `MISSING` |  |
| `updateMusicFrame_setChanVol` | `0xE766` | L6854–L6921 | JSR | `MISSING` |  |
| `musicGetNextInstructionByte` | `0xEA41` | L7293–L7299 | JSR | `MISSING` |  |

### CORE_LOGIC (38 Routines)

| Routine Name | Address | Lines | Call Mechanism | Status | Notes |
|:---|:---:|:---:|:---:|:---:|:---|
| `branchOnGameMode` | `0x8161` | L432–L441 | JSR | `VERIFIED` | Verified: Master game mode 0..6 dispatcher differential proven against 6502 ROM switch_s_plus_2a jump table |
| `makePlayer1Active` | `0x8776` | L1269–L1287 | JSR | `VERIFIED` | Verified: Player 1 context mirroring, playfield pointer setup, and button copy differential proven against 6502 ROM |
| `makePlayer2Active` | `0x8792` | L1288–L1306 | JSR | `VERIFIED` | Verified: Player 2 context mirroring, second playfield pointer, and button copy differential proven against 6502 ROM |
| `savePlayer1State` | `0x87AE` | L1307–L1324 | JSR | `VERIFIED` | Verified: Active player state saving and 2-player pending garbage swapping differential proven against 6502 ROM |
| `savePlayer2State` | `0x87C8` | L1325–L1338 | JSR | `VERIFIED` | Verified: Active player state saving and pending garbage swapping differential proven against 6502 ROM |
| `isPositionValid` | `0x948B` | L2392–L2456 | JSR | `VERIFIED` | Verified: Boundary box, floor, and mino collision detection verified |
| `gameMode_startDemo` | `0x9DF6` | L3735–L3745 | Jump Table | `VERIFIED` | Verified: Demo mode initialization clearing mode/play states and level/gameType differential proven against 6502 ROM |
| `endingAnimation` | `0x9E3A` | L3790–L3801 | JSR | `MISSING` |  |
| `endingAnimationB` | `0x9E49` | L3802–L3944 | Entry / Direct | `MISSING` |  |
| `add100Points` | `0x9F62` | L3945–L3956 | JSR | `MISSING` |  |
| `checkSelectHeldToAddPoints` | `N/A` | L4610–L4622 | Entry / Direct | `MISSING` |  |
| `ending_initTypeBVars` | `0xA463` | L4623–L4663 | JSR | `MISSING` |  |
| `ending_typeB` | `0xA52E` | L4739–L4748 | Entry / Direct | `MISSING` |  |
| `ending_typeBConcert` | `0xA53B` | L4749–L4906 | JSR | `MISSING` |  |
| `ending_typeBCathedral` | `0xA625` | L4907–L4960 | Entry / Direct | `MISSING` |  |
| `ending_computeTypeBCathedralYTableIndex` | `0xA6AE` | L4980–L4991 | JSR | `MISSING` |  |
| `LA6BC` | `0xA6BC` | L4992–L4992 | JSR | `MISSING` |  |
| `endingAnimationA` | `0xA926` | L5165–L5196 | Entry / Direct | `MISSING` |  |
| `selectEndingScreen` | `0xA96E` | L5197–L5225 | JSR | `MISSING` |  |
| `LAA82` | `0xAA82` | L5351–L5355 | JSR | `MISSING` |  |
| `addExpansionPortInputAsControllerInput` | `0xAB8B` | L5515–L5525 | JSR | `VERIFIED` | Verified: Famicom expansion port input OR-mixing verified with unit vectors |
| `memset_page` | `0xAC6A` | L5635–L5653 | JSR | `MISSING` |  |
| `switch_s_plus_2a` | `0xAC82` | L5654–L5683 | JSR | `MISSING` |  |
| `setMMC1Control` | `0xACA3` | L5684–L5695 | JSR | `MISSING` |  |
| `changeCHRBank0` | `0xACB7` | L5696–L5710 | JSR | `MISSING` |  |
| `changeCHRBank1` | `0xACCB` | L5711–L5725 | JSR | `MISSING` |  |
| `changePRGBank` | `0xACDF` | L5726–L5740 | JSR | `MISSING` |  |
| `LE006` | `0xE006` | L5858–L5860 | JSR | `MISSING` |  |
| `copyToSq1Channel` | `0xE04B` | L5903–L5905 | JSR | `MISSING` |  |
| `LE1D8` | `0xE1D8` | L6107–L6113 | Entry / Direct | `MISSING` |  |
| `LE253` | `0xE253` | L6173–L6179 | JSR | `MISSING` |  |
| `setNoiseLo` | `0xE2D3` | L6240–L6243 | Entry / Direct | `MISSING` |  |
| `loadNoiseLo` | `0xE2D7` | L6244–L6247 | JSR | `MISSING` |  |
| `LE33B` | `0xE33B` | L6312–L6320 | JSR | `MISSING` |  |
| `LE46C` | `0xE46C` | L6480–L6481 | Entry / Direct | `MISSING` |  |
| `LE4E9` | `0xE4E9` | L6535–L6536 | Entry / Direct | `MISSING` |  |
| `playNoise` | `0xE9EB` | L7244–L7254 | JSR | `MISSING` |  |
| `playDmc` | `0xEA03` | L7255–L7292 | JSR | `MISSING` |  |

### GAMEPLAY_STATE (14 Routines)

| Routine Name | Address | Lines | Call Mechanism | Status | Notes |
|:---|:---:|:---:|:---:|:---:|:---|
| `gameModeState_updatePlayer1` | `0x8174` | L442–L450 | Jump Table | `MISSING` |  |
| `gameModeState_updatePlayer2` | `0x8186` | L451–L461 | Jump Table | `MISSING` |  |
| `branchOnPlayStatePlayer1` | `0x81B2` | L474–L488 | JSR | `VERIFIED` | Verified: 12-state dispatcher differential proven against 6502 ROM switch_s_plus_2a jump table |
| `branchOnPlayStatePlayer2` | `0x81D9` | L495–L509 | JSR | `VERIFIED` | Verified: Player 2 state dispatcher verified with 6502 simulator |
| `gameModeState_initGameBackground` | `0x85F0` | L1092–L1167 | Jump Table | `MISSING` |  |
| `gameModeState_initGameBackground_finish` | `0x8693` | L1168–L1182 | Entry / Direct | `MISSING` |  |
| `gameModeState_initGameState` | `0x86DC` | L1190–L1268 | Jump Table | `MISSING` |  |
| `gameModeState_updateCountersAndNonPlayerState` | `0x8884` | L1441–L1464 | Jump Table | `VERIFIED` | Verified: Fall timer progression, 2P piece delay advancement, and SELECT preview toggle differential proven against 6502 ROM |
| `gameModeState_handleGameOver` | `0x9CBF` | L3545–L3595 | Jump Table | `MISSING` |  |
| `gameModeState_checkForResetKeyCombo` | `0x9E16` | L3756–L3770 | Jump Table | `MISSING` |  |
| `gameModeState_vblankThenRunState2` | `0x9E27` | L3771–L3776 | Jump Table | `VERIFIED` | Verified: GameMode state 2 transition and no-op hook differential proven against 6502 ROM |
| `playState_incrementPlayState` | `0x9E37` | L3785–L3786 | Jump Table | `VERIFIED` | Verified: PlayState incrementation differential proven against 6502 ROM |
| `playState_bTypeGoalCheck` | `0xA3F2` | L4545–L4584 | Jump Table | `VERIFIED` | Verified: Type A vs Type B line goal check and victory tile placement verified |
| `playState_bTypeGoalCheck_ret` | `0xA42B` | L4585–L4588 | Entry / Direct | `MISSING` |  |

### MATH_BCD (12 Routines)

| Routine Name | Address | Lines | Call Mechanism | Status | Notes |
|:---|:---:|:---:|:---:|:---:|:---|
| `gameMode_playAndEndingHighScore_jmp` | `0x815D` | L428–L431 | Jump Table | `MISSING` |  |
| `gameMode_playAndEndingHighScore` | `0x819B` | L462–L473 | JSR | `MISSING` |  |
| `playState_updateLinesAndStatistics` | `0x9B58` | L3329–L3377 | Jump Table | `VERIFIED` | Verified: PlayState 5 scoring, line counts, and stat update proven |
| `showHighScores` | `0x9FF2` | L4015–L4020 | JSR | `VERIFIED` | Verified: 1P vs 2P high score nametable display dispatcher differential proven |
| `showHighScores_real` | `0x9FFB` | L4021–L4092 | Entry / Direct | `VERIFIED` | Verified: 3-entry names, 6-digit BCD scores, and 2-digit BCD levels PPU display verified |
| `showHighScores_ret` | `0xA085` | L4093–L4094 | Entry / Direct | `VERIFIED` | Verified: High score return stub verified |
| `handleHighScoreIfNecessary` | `0xA0EE` | L4116–L4159 | JSR | `VERIFIED` | Verified: Score comparison, tie breaking, and slot selection differential proven against 6502 ROM |
| `adjustHighScores` | `0xA134` | L4160–L4207 | Entry / Direct | `VERIFIED` | Verified: Multi-entry shifting, name clearing, score/level insertion, and entry transition verified |
| `copyHighScoreNameToNextIndex` | `0xA192` | L4208–L4238 | JSR | `VERIFIED` | Verified: 6-character name shift for Type A/B differential proven against 6502 ROM |
| `copyHighScoreScoreToNextIndex` | `0xA1C1` | L4239–L4258 | JSR | `VERIFIED` | Verified: 3-byte BCD score shift for Type A/B differential proven against 6502 ROM |
| `copyHighScoreLevelToNextIndex` | `0xA1E0` | L4259–L4271 | JSR | `VERIFIED` | Verified: 1-byte level shift for Type A/B differential proven against 6502 ROM |
| `highScoreEntryScreen` | `0xA201` | L4276–L4438 | Entry / Direct | `VERIFIED` | Verified: Interactive name entry, cursor navigation, alphabet modulo 44 cycling, and START confirmation verified |

### MATH_RNG (2 Routines)

| Routine Name | Address | Lines | Call Mechanism | Status | Notes |
|:---|:---:|:---:|:---:|:---:|:---|
| `generateNextPseudorandomNumber` | `0xAB47` | L5468–L5485 | JSR | `VERIFIED` | Verified: 6502 LFSR algorithm proven equivalent via CPU simulator & unit vectors |
| `unreferenced_soundRng` | `0xE09E` | L5963–L5978 | Entry / Direct | `VERIFIED` | Verified: Audio LFSR variant proven equivalent via differential testing |

### MENUS_UI (13 Routines)

| Routine Name | Address | Lines | Call Mechanism | Status | Notes |
|:---|:---:|:---:|:---:|:---:|:---|
| `gameMode_legalScreen` | `0x8200` | L516–L558 | Jump Table | `VERIFIED` | Verified: Copyright screen timing, START bypass, OAM/render init differential proven against 6502 ROM |
| `gameMode_titleScreen` | `0x824F` | L559–L635 | Jump Table | `VERIFIED` | Verified: Title menu transition on START and demo countdown timeout differential proven against 6502 ROM |
| `render_mode_legal_and_title_screens` | `0x82B1` | L636–L656 | Jump Table | `MISSING` |  |
| `gameMode_gameTypeMenu` | `0x82D1` | L657–L682 | Jump Table | `VERIFIED` | Verified: Game type A/B toggle, music 0..3 selection, START/B transitions differential proven against 6502 ROM loop |
| `gameMode_levelMenu` | `0x83D7` | L799–L844 | Jump Table | `VERIFIED` | Verified: Level menu setup, startLevel >= 10 normalization modulo 10 verified |
| `gameMode_levelMenu_processPlayer1Navigation` | `0x8436` | L845–L919 | Entry / Direct | `VERIFIED` | Verified: P1 state sync, navigation handling, START/B transitions, A+START cheat (+10 level), garbage hole PRNG differential proven against 6502 ROM |
| `gameMode_levelMenu_handleLevelHeightNavigation` | `0x84AE` | L920–L1065 | JSR | `VERIFIED` | Verified: Level (0..9) & height (0..5) D-pad grid navigation, Button A Type B toggling, and cursor staging differential proven against 6502 ROM |
| `render_mode_menu_screens` | `0x85DA` | L1078–L1091 | Jump Table | `MISSING` |  |
| `soundEffectSlot1_menuOptionSelectPlaying_ret` | `0xE3B2` | L6377–L6379 | Entry / Direct | `MISSING` |  |
| `soundEffectSlot1_menuOptionSelectPlaying` | `0xE3B3` | L6380–L6391 | Jump Table | `MISSING` |  |
| `soundEffectSlot1_menuOptionSelectInit` | `0xE3CA` | L6392–L6395 | Jump Table | `MISSING` |  |
| `soundEffectSlot1_menuScreenSelectInit` | `0xE473` | L6485–L6492 | Jump Table | `MISSING` |  |
| `soundEffectSlot1_menuScreenSelectPlaying` | `0xE481` | L6493–L6501 | Jump Table | `MISSING` |  |

### PLAYER_INPUT (4 Routines)

| Routine Name | Address | Lines | Call Mechanism | Status | Notes |
|:---|:---:|:---:|:---:|:---:|:---|
| `pollControllerButtons` | `0x9D51` | L3630–L3724 | JSR | `VERIFIED` | Verified: Mode dispatch, Start button demo cancellation, and demo move playback verified |
| `demoButtonsTable_indexIncr` | `0x9DE8` | L3725–L3734 | JSR | `VERIFIED` | Verified: 16-bit demo table address pointer increment verified with 6502 simulator |
| `gameModeState_startButtonHandling` | `0xA37F` | L4475–L4544 | Jump Table | `MISSING` |  |
| `pollController_actualRead` | `0xAB69` | L5493–L5514 | JSR | `VERIFIED` | Verified: Serial joypad 4016/4017 strobe and 8-bit read verified with unit vectors |

### PLAYFIELD (9 Routines)

| Routine Name | Address | Lines | Call Mechanism | Status | Notes |
|:---|:---:|:---:|:---:|:---:|:---|
| `initPlayfieldIfTypeB` | `0x87DC` | L1339–L1343 | JSR | `VERIFIED` | Verified: Type B mode check and garbage playfield initialization verified |
| `copyPlayfieldRowToVRAM` | `0x9725` | L2704–L2756 | JSR | `MISSING` |  |
| `updateLineClearingAnimation` | `0x977F` | L2757–L2827 | JSR | `VERIFIED` | Verified: 5-step center-out line clearing animation framing verified |
| `noop_disabledVramRowIncr` | `0x9874` | L2878–L2895 | JSR | `VERIFIED` | Verified: Unused/disabled VRAM row increment routine differential proven against 6502 ROM |
| `playState_updateGameOverCurtain` | `0x9A11` | L3130–L3187 | Jump Table | `VERIFIED` | Verified: Game over curtain cascade filling playfield with curtain tiles verified |
| `playState_checkForCompletedRows` | `0x9A6B` | L3188–L3279 | Jump Table | `VERIFIED` | Verified: Row completion detection, collapsing, and garbage calculation differential proven against 6502 ROM |
| `playState_receiveGarbage` | `0x9B03` | L3280–L3326 | Jump Table | `VERIFIED` | Verified: 2-player pending garbage shifting and bottom row filling verified |
| `updatePlayfield` | `0x9CAF` | L3532–L3544 | JSR | `VERIFIED` | Verified: VRAM row dirty flag tracking verified |
| `soundEffectSlot1_lineClearingPlaying` | `0xE43B` | L6458–L6461 | Jump Table | `MISSING` |  |

### RENDER (16 Routines)

| Routine Name | Address | Lines | Call Mechanism | Status | Notes |
|:---|:---:|:---:|:---:|:---:|:---|
| `render` | `0x804B` | L290–L296 | JSR | `MISSING` |  |
| `loadSpriteIntoOamStaging` | `0x8C27` | L1861–L1903 | JSR | `MISSING` |  |
| `render_mode_play_and_demo` | `0x94EE` | L2457–L2671 | Jump Table | `MISSING` |  |
| `twoDigsToPPU` | `0x9712` | L2691–L2703 | JSR | `VERIFIED` | Verified: 2-digit BCD nibble separation to PPUDATA differential proven against 6502 ROM |
| `updatePaletteForLevel` | `0x9808` | L2833–L2870 | JSR | `MISSING` |  |
| `render_mode_ending_animation` | `0x9F95` | L3975–L4009 | Jump Table | `MISSING` |  |
| `render_mode_congratulations_screen` | `0xA344` | L4443–L4474 | Jump Table | `VERIFIED` | Verified: High score name entry letter tile render to nametable and scroll reset differential proven against 6502 ROM |
| `ending_typeBConcertPatchToPpuForHeight` | `0xA4B1` | L4664–L4712 | JSR | `MISSING` |  |
| `patchToPpu` | `0xA507` | L4713–L4733 | JSR | `MISSING` |  |
| `render_ending` | `0xA527` | L4734–L4738 | JSR | `MISSING` |  |
| `ending_typeBCathedralSetSprite` | `0xA690` | L4961–L4979 | JSR | `MISSING` |  |
| `render_endingUnskippable` | `0xA7FD` | L5098–L5106 | JSR | `MISSING` |  |
| `copyCurrentScrollAndCtrlToPPU` | `0xAA8A` | L5356–L5363 | JSR | `MISSING` |  |
| `bulkCopyToPpu` | `0xAA98` | L5364–L5367 | JSR | `MISSING` |  |
| `copyOamStagingToOam` | `0xAB5E` | L5486–L5492 | JSR | `MISSING` |  |
| `memset_ppu_page_and_more` | `0xAC1C` | L5595–L5611 | JSR | `MISSING` |  |

### SYSTEM (6 Routines)

| Routine Name | Address | Lines | Call Mechanism | Status | Notes |
|:---|:---:|:---:|:---:|:---:|:---|
| `initRam` | `0x8000` | L245–L247 | Entry / Direct | `MISSING` |  |
| `nmi` | `0x8005` | L248–L287 | Jump Table | `MISSING` |  |
| `initRamContinued` | `0x805A` | L297–L427 | Entry / Direct | `MISSING` |  |
| `sleep_for_14_vblanks` | `0xA44D` | L4594–L4601 | JSR | `MISSING` |  |
| `sleep_for_a_vblanks` | `0xA459` | L4602–L4609 | JSR | `MISSING` |  |
| `reset` | `0xFF00` | L7692–L7769 | Jump Table | `MISSING` |  |

### TETRIMINO (16 Routines)

| Routine Name | Address | Lines | Call Mechanism | Status | Notes |
|:---|:---:|:---:|:---:|:---:|:---|
| `playState_playerControlsActiveTetrimino` | `0x81CF` | L489–L494 | Jump Table | `VERIFIED` | Verified: Shift, rotate, and drop composition verified |
| `playState_player2ControlsActiveTetrimino` | `0x81F6` | L510–L515 | Jump Table | `VERIFIED` | Verified: Shift, rotate, and drop composition verified |
| `rotate_tetrimino` | `0x88AB` | L1465–L1502 | JSR | `VERIFIED` | Verified: A/B button rotation and wall reverting behavior verified |
| `drop_tetrimino` | `0x8914` | L1530–L1600 | JSR | `VERIFIED` | Verified: Natural gravity, soft drop acceleration, and floor lock verified |
| `shift_tetrimino` | `0x89AE` | L1616–L1667 | JSR | `VERIFIED` | Verified: Horizontal shifting with DAS auto-repeat verified |
| `stageSpriteForCurrentPiece` | `0x8A0A` | L1668–L1758 | JSR | `MISSING` |  |
| `stageSpriteForNextPiece` | `0x8BCE` | L1832–L1846 | JSR | `MISSING` |  |
| `playState_spawnNextTetrimino` | `0x988E` | L2896–L2947 | Jump Table | `VERIFIED` | Verified: Piece spawning, orientation table mapping, 2P delay counter, and stats update differential proven against 6502 ROM |
| `chooseNextTetrimino` | `0x98EB` | L2948–L2963 | JSR | `VERIFIED` | Verified: Gameplay vs demo mode piece selection differential proven |
| `pickRandomTetrimino` | `0x9903` | L2964–L2992 | Entry / Direct | `VERIFIED` | Verified: 7-bag modulo & reroll logic verified with 6502 CPU simulator |
| `incrementPieceStat` | `0x9969` | L3030–L3054 | JSR | `VERIFIED` | Verified: BCD piece statistic increments verified with CPU simulator |
| `playState_lockTetrimino` | `0x99A2` | L3062–L3129 | Jump Table | `VERIFIED` | Verified: Locking 4 minos into playfield and invalid-position game over sequence verified |
| `playState_unassignOrientationId` | `0x9E2F` | L3777–L3784 | Jump Table | `VERIFIED` | Verified: Hidden orientation unassign differential proven against 6502 ROM |
| `soundEffectSlot1_lockTetriminoInit` | `0xE384` | L6350–L6356 | Jump Table | `MISSING` |  |
| `soundEffectSlot1_shiftTetriminoInit` | `0xE390` | L6357–L6363 | Jump Table | `MISSING` |  |
| `soundEffectSlot1_rotateTetriminoInit` | `0xE3D1` | L6399–L6404 | Jump Table | `MISSING` |  |

---

## 4. Alternate Entry Points Directory

Total alternate entry points identified: **111**

| Label | Address | Lines | Parent Routine Context | JSR / Table | Status |
|:---|:---:|:---:|:---|:---:|:---:|
| `irq` | `0x804A` | L288–L289 | `nmi` | Yes | `MISSING` |
| `L830B` | `0x830B` | L683–L798 | `gameMode_gameTypeMenu` | Internal | `MISSING` |
| `typeBRows` | `0x87E7` | L1348–L1360 | `initPlayfieldIfTypeB` | Internal | `MISSING` |
| `typeBGarbageInRow` | `0x87FC` | L1361–L1381 | `initPlayfieldIfTypeB` | Internal | `MISSING` |
| `initCopyPlayfieldToPlayer2` | `0x884A` | L1403–L1404 | `initPlayfieldIfTypeB` | Internal | `MISSING` |
| `copyPlayfieldToPlayer2` | `0x884C` | L1405–L1416 | `initPlayfieldIfTypeB` | Internal | `MISSING` |
| `typeBBlankInitPlayer1` | `0x885D` | L1417–L1427 | `initPlayfieldIfTypeB` | Internal | `MISSING` |
| `typeBBlankInitPlayer2` | `0x886D` | L1428–L1432 | `initPlayfieldIfTypeB` | Internal | `MISSING` |
| `endTypeBInit` | `0x8875` | L1433–L1435 | `initPlayfieldIfTypeB` | Internal | `MISSING` |
| `L8B9D` | `0x8B9D` | L1802–L1831 | `stageSpriteForCurrentPiece` | Internal | `MISSING` |
| `L992A` | `0x992A` | L2993–L2998 | `pickRandomTetrimino` | Internal | `MISSING` |
| `useNewSpawnID` | `0x9938` | L3001–L3004 | `pickRandomTetrimino` | Internal | `MISSING` |
| `L9996` | `0x9996` | L3055–L3061 | `incrementPieceStat` | Internal | `MISSING` |
| `incrementLines` | `0x9BA8` | L3378–L3394 | `playState_updateLinesAndStatistics` | Internal | `MISSING` |
| `L9BC7` | `0x9BC7` | L3395–L3405 | `playState_updateLinesAndStatistics` | Internal | `MISSING` |
| `L9BFB` | `0x9BFB` | L3429–L3430 | `playState_updateLinesAndStatistics` | Internal | `MISSING` |
| `addHoldDownPoints` | `0x9BFE` | L3431–L3448 | `playState_updateLinesAndStatistics` | Internal | `MISSING` |
| `L9C18` | `0x9C18` | L3449–L3456 | `playState_updateLinesAndStatistics` | Internal | `MISSING` |
| `L9C27` | `0x9C27` | L3457–L3459 | `playState_updateLinesAndStatistics` | Internal | `MISSING` |
| `addLineClearPoints` | `0x9C2D` | L3460–L3465 | `playState_updateLinesAndStatistics` | Internal | `MISSING` |
| `L9C37` | `0x9C37` | L3466–L3478 | `playState_updateLinesAndStatistics` | Internal | `MISSING` |
| `L9C4E` | `0x9C4E` | L3479–L3490 | `playState_updateLinesAndStatistics` | Internal | `MISSING` |
| `L9C64` | `0x9C64` | L3491–L3499 | `playState_updateLinesAndStatistics` | Internal | `MISSING` |
| `L9C75` | `0x9C75` | L3500–L3507 | `playState_updateLinesAndStatistics` | Internal | `MISSING` |
| `L9C84` | `0x9C84` | L3508–L3515 | `playState_updateLinesAndStatistics` | Internal | `MISSING` |
| `L9C94` | `0x9C94` | L3516–L3528 | `playState_updateLinesAndStatistics` | Internal | `MISSING` |
| `playState_noop` | `0x9E39` | L3787–L3789 | `playState_incrementPlayState` | Yes | `VERIFIED` |
| `L9F76` | `0x9F76` | L3957–L3964 | `add100Points` | Internal | `MISSING` |
| `L9F85` | `0x9F85` | L3965–L3972 | `add100Points` | Internal | `MISSING` |
| `L9F94` | `0x9F94` | L3973–L3974 | `add100Points` | Internal | `MISSING` |
| `L9FE9` | `0x9FE9` | L4010–L4014 | `render_mode_ending_animation` | Internal | `MISSING` |
| `LA6BE` | `0xA6BE` | L4993–L5001 | `LA6BC` | Internal | `MISSING` |
| `LA6D0` | `0xA6D0` | L5002–L5006 | `LA6BC` | Internal | `MISSING` |
| `LA6D9` | `0xA6D9` | L5007–L5020 | `LA6BC` | Internal | `MISSING` |
| `LA6F7` | `0xA6F7` | L5021–L5045 | `LA6BC` | Internal | `MISSING` |
| `LA72C` | `0xA72C` | L5046–L5051 | `LA6BC` | Internal | `MISSING` |
| `ending_selected` | `0xA9A5` | L5226–L5233 | `selectEndingScreen` | Internal | `MISSING` |
| `LA9FC` | `0xA9FC` | L5272–L5278 | `selectEndingScreen` | Internal | `MISSING` |
| `LAA0B` | `0xAA0B` | L5279–L5280 | `selectEndingScreen` | Internal | `MISSING` |
| `LAA10` | `0xAA10` | L5282–L5283 | `selectEndingScreen` | Internal | `MISSING` |
| `_updatePpuMask` | `0xAA59` | L5323–L5327 | `updateAudioWaitForNmiAndDisablePpuRendering` | Internal | `MISSING` |
| `waitForVBlankAndEnableNmi` | `0xAA6B` | L5336–L5342 | `updateAudioWaitForNmiAndEnablePpuRendering` | Yes | `MISSING` |
| `disableNmi` | `0xAA78` | L5343–L5345 | `updateAudioWaitForNmiAndEnablePpuRendering` | Yes | `MISSING` |
| `_updatePpuCtrl` | `0xAA7C` | L5346–L5350 | `updateAudioWaitForNmiAndEnablePpuRendering` | Internal | `MISSING` |
| `LAAB5` | `0xAAB5` | L5381–L5388 | `bulkCopyToPpu` | Internal | `MISSING` |
| `LAAC2` | `0xAAC2` | L5389–L5392 | `bulkCopyToPpu` | Internal | `MISSING` |
| `LAAC7` | `0xAAC7` | L5393–L5395 | `bulkCopyToPpu` | Internal | `MISSING` |
| `LAACA` | `0xAACA` | L5396–L5397 | `bulkCopyToPpu` | Internal | `MISSING` |
| `LAACD` | `0xAACD` | L5398–L5408 | `bulkCopyToPpu` | Internal | `MISSING` |
| `LAAE6` | `0xAAE6` | L5409–L5416 | `bulkCopyToPpu` | Internal | `MISSING` |
| `copyToPpu` | `0xAAF2` | L5417–L5423 | `bulkCopyToPpu` | Internal | `MISSING` |
| `LAB0A` | `0xAB0A` | L5432–L5445 | `bulkCopyToPpu` | Internal | `MISSING` |
| `copyAddrAtReturnAddressToTmp_incrReturnAddrBy2` | `0xAB21` | L5446–L5467 | `bulkCopyToPpu` | Yes | `MISSING` |
| `pollController` | `0xAB9D` | L5526–L5540 | `addExpansionPortInputAsControllerInput` | Yes | `VERIFIED` |
| `diffOldAndNewButtons` | `0xABBD` | L5541–L5553 | `addExpansionPortInputAsControllerInput` | Internal | `VERIFIED` |
| `LABD1` | `0xABD1` | L5556–L5567 | `addExpansionPortInputAsControllerInput` | Internal | `MISSING` |
| `LABEA` | `0xABEA` | L5568–L5584 | `addExpansionPortInputAsControllerInput` | Internal | `MISSING` |
| `LAC0D` | `0xAC0D` | L5585–L5594 | `addExpansionPortInputAsControllerInput` | Internal | `MISSING` |
| `LAC40` | `0xAC40` | L5612–L5613 | `memset_ppu_page_and_more` | Internal | `MISSING` |
| `LAC44` | `0xAC44` | L5614–L5627 | `memset_ppu_page_and_more` | Internal | `MISSING` |
| `LAC61` | `0xAC61` | L5628–L5630 | `memset_ppu_page_and_more` | Internal | `MISSING` |
| `LAC67` | `0xAC67` | L5631–L5634 | `memset_ppu_page_and_more` | Internal | `MISSING` |
| `copyToTriChannel` | `0xE04F` | L5906–L5908 | `copyToSq1Channel` | Yes | `MISSING` |
| `copyToNoiseChannel` | `0xE053` | L5909–L5911 | `copyToSq1Channel` | Yes | `MISSING` |
| `copyToSq2Channel` | `0xE057` | L5912–L5914 | `copyToSq1Channel` | Yes | `MISSING` |
| `copyToApuChannel` | `0xE059` | L5915–L5932 | `copyToSq1Channel` | Internal | `MISSING` |
| `updateSoundEffectSlot3` | `0xE19C` | L6068–L6072 | `updateSoundEffectSlot2` | Yes | `MISSING` |
| `updateSoundEffectSlot1` | `0xE1AC` | L6078–L6084 | `updateSoundEffectSlot2` | Yes | `MISSING` |
| `updateSoundEffectSlot0` | `0xE1B9` | L6085–L6089 | `updateSoundEffectSlot2` | Yes | `MISSING` |
| `updateSoundEffectSlotShared` | `0xE1BF` | L6090–L6103 | `updateSoundEffectSlot2` | Internal | `MISSING` |
| `updateSoundEffectSlotShared_rts` | `0xE1D7` | L6104–L6106 | `updateSoundEffectSlot2` | Internal | `MISSING` |
| `muteAudioAndClearTriControl` | `0xE247` | L6166–L6172 | `soundEffectSlot2_makesNoSound` | Yes | `MISSING` |
| `LE265` | `0xE265` | L6180–L6187 | `LE253` | Internal | `MISSING` |
| `stopSoundEffectSlot0` | `0xE2E9` | L6256–L6260 | `advanceSoundEffectSlot0WithoutUpdate` | Internal | `MISSING` |
| `updateSoundEffectSlot0WithoutUpdate_ret` | `0xE2F3` | L6261–L6263 | `advanceSoundEffectSlot0WithoutUpdate` | Internal | `MISSING` |
| `soundEffectSlot0_gameOverCurtainInit` | `0xE2F8` | L6267–L6271 | `advanceSoundEffectSlot0WithoutUpdate` | Yes | `MISSING` |
| `LE34E` | `0xE34E` | L6321–L6322 | `LE33B` | Internal | `MISSING` |
| `soundEffectSlot1Playing_stop` | `0xE3A1` | L6367–L6373 | `soundEffectSlot1Playing_advance` | Internal | `MISSING` |
| `soundEffectSlot1Playing_ret` | `0xE3B1` | L6374–L6376 | `soundEffectSlot1Playing_advance` | Internal | `MISSING` |
| `soundEffectSlot1_rotateTetrimino_ret` | `0xE3D0` | L6396–L6398 | `soundEffectSlot1_menuOptionSelectInit` | Internal | `MISSING` |
| `soundEffectSlot1_rotateTetriminoPlaying` | `0xE3DD` | L6405–L6425 | `soundEffectSlot1_rotateTetriminoInit` | Yes | `MISSING` |
| `soundEffectSlot1_tetrisAchievedPlaying` | `0xE40E` | L6432–L6436 | `soundEffectSlot1_tetrisAchievedInit` | Yes | `MISSING` |
| `LE417` | `0xE417` | L6437–L6438 | `soundEffectSlot1_tetrisAchievedInit` | Yes | `MISSING` |
| `soundEffectSlot1_lineCompletedPlaying` | `0xE425` | L6445–L6449 | `soundEffectSlot1_lineCompletedInit` | Yes | `MISSING` |
| `soundEffectSlot1_lineClearingInit` | `0xE42E` | L6450–L6454 | `soundEffectSlot1_lineCompletedInit` | Yes | `MISSING` |
| `LE437` | `0xE437` | L6455–L6455 | `soundEffectSlot1_lineCompletedInit` | Internal | `MISSING` |
| `LE43A` | `0xE43A` | L6456–L6457 | `soundEffectSlot1_lineCompletedInit` | Internal | `MISSING` |
| `LE442` | `0xE442` | L6462–L6479 | `soundEffectSlot1_lineClearingPlaying` | Internal | `MISSING` |
| `LE472` | `0xE472` | L6483–L6484 | `LE46C` | Internal | `MISSING` |
| `LE4AC` | `0xE4AC` | L6514–L6514 | `soundEffectSlot1_menuScreenSelectPlaying` | Internal | `MISSING` |
| `LE4AF` | `0xE4AF` | L6515–L6516 | `soundEffectSlot1_menuScreenSelectPlaying` | Internal | `MISSING` |
| `soundEffectSlot2_lowBuzz` | `0xE513` | L6551–L6555 | `soundEffectSlot2_mediumBuzz` | Yes | `MISSING` |
| `LE51B` | `0xE51B` | L6556–L6567 | `soundEffectSlot2_mediumBuzz` | Internal | `MISSING` |
| `soundEffectSlot3Playing_stop` | `0xE540` | L6576–L6582 | `soundEffectSlot3Playing_advance` | Internal | `MISSING` |
| `soundEffectSlot3Playing_ret` | `0xE550` | L6583–L6585 | `soundEffectSlot3Playing_advance` | Internal | `MISSING` |
| `LE613` | `0xE613` | L6680–L6683 | `updateMusicFrame_setChanLo` | Internal | `MISSING` |
| `musicLoOffset_minus2_6` | `0xE628` | L6696–L6705 | `updateMusicFrame_setChanLo` | Internal | `MISSING` |
| `updateMusicFrame_setChanLoOffset` | `0xE637` | L6706–L6739 | `updateMusicFrame_setChanLo` | Yes | `MISSING` |
| `musicLoOffset_setLoAndSaveFrameCounter` | `0xE671` | L6740–L6744 | `updateMusicFrame_setChanLo` | Internal | `MISSING` |
| `musicLoOffset_setLo` | `0xE677` | L6745–L6759 | `updateMusicFrame_setChanLo` | Internal | `MISSING` |
| `loadMusicTrack` | `0xE6D7` | L6780–L6809 | Shared Context | Yes | `MISSING` |
| `derefNextAddr` | `0xE711` | L6810–L6821 | Shared Context | Internal | `MISSING` |
| `storeDeref1AndContinue` | `0xE72A` | L6822–L6830 | Shared Context | Internal | `MISSING` |
| `initSq12IfTrashedBySoundEffect_ret` | `0xE765` | L6850–L6853 | `initSq12IfTrashedBySoundEffect` | Internal | `MISSING` |
| `updateMusicFrame_progLoadNextScript` | `0xE7D9` | L6922–L6940 | `updateMusicFrame_setChanVol` | Internal | `MISSING` |
| `updateMusicFrame_ret` | `0xE7FE` | L6943–L6945 | `updateMusicFrame_setChanVol` | Internal | `MISSING` |
| `updateMusicFrame_progLoadRoutine` | `0xE818` | L6961–L6982 | `updateMusicFrame_setChanVol` | Internal | `MISSING` |
| `updateMusicFrame_progNextRoutine_jmp` | `0xE83D` | L6983–L6985 | `updateMusicFrame_setChanVol` | Internal | `MISSING` |
| `updateMusicFrame_incSlotFromOffset` | `0xE84A` | L6993–L6997 | `updateMusicFrame_setChanVol` | Internal | `MISSING` |
| `updateMusicFrame_incSlot` | `0xE84D` | L6998–L7007 | `updateMusicFrame_setChanVol` | Internal | `MISSING` |
| `updateMusicFrame_updateChannel` | `0xE85A` | L7008–L7243 | `updateMusicFrame_setChanVol` | Internal | `MISSING` |
