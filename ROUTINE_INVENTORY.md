# NES Tetris Authoritative Routine Inventory & Scope Audit

- **Disassembly Source of Truth:** `TetrisNESDisasm/main.asm` (NES 6502 Assembly, NROM/MMC1)
- **Reference ROM:** `Tetris (USA).nes` (PRG SHA-1 verified)
- **C Target:** Strict C11 (`-std=c11 -Wall -Wextra -Werror -pedantic`)

---

## 1. Executive Summary & Decompilation Scope

| Metric | Count |
|:---|:---:|
| **Total Standalone Routines** | **205** |
| **Verified (Tested)** | **5** |
| **Decompiled (Untested)** | **0** |
| **Remaining (Missing)** | **200** |
| **Completion Percentage** | **2.44%** |

---

## 2. Category Breakdown

| Category | Total Routines | Verified | Decompiled | Missing | % Complete |
|:---|:---:|:---:|:---:|:---:|:---:|
| **AUDIO** | 56 | 0 | 0 | 56 | 0.0% |
| **CORE_LOGIC** | 51 | 0 | 0 | 51 | 0.0% |
| **GAMEPLAY_STATE** | 15 | 0 | 0 | 15 | 0.0% |
| **MATH_BCD** | 9 | 0 | 0 | 9 | 0.0% |
| **MATH_RNG** | 2 | 2 | 0 | 0 | 100.0% |
| **MENUS_UI** | 13 | 0 | 0 | 13 | 0.0% |
| **PLAYER_INPUT** | 4 | 0 | 0 | 4 | 0.0% |
| **PLAYFIELD** | 11 | 0 | 0 | 11 | 0.0% |
| **RENDER** | 19 | 0 | 0 | 19 | 0.0% |
| **SYSTEM** | 6 | 0 | 0 | 6 | 0.0% |
| **TETRIMINO** | 19 | 3 | 0 | 16 | 15.8% |

---

## 3. Routine Directory by Category

### AUDIO (56 Routines)

| Routine Name | Address | Lines | JSR Called | Status | Notes |
|:---|:---:|:---:|:---:|:---:|:---|
| `updateMusicSpeed` | `0x9D17` | L3596–L3629 | Yes | `MISSING` |  |
| `setMusicTrack` | `0x9E07` | L3746–L3755 | Yes | `MISSING` |  |
| `updateAudioWaitForNmiAndResetOamStaging` | `0xAA2F` | L5295–L5308 | Yes | `MISSING` |  |
| `updateAudioAndWaitForNmi` | `0xAA45` | L5309–L5318 | Yes | `MISSING` |  |
| `updateAudioWaitForNmiAndDisablePpuRendering` | `0xAA52` | L5319–L5322 | Yes | `MISSING` |  |
| `updateAudioWaitForNmiAndEnablePpuRendering` | `0xAA5F` | L5328–L5335 | Yes | `MISSING` |  |
| `updateAudio_jmp` | `0xE000` | L5851–L5854 | Yes | `MISSING` |  |
| `updateAudio2` | `0xE003` | L5855–L5857 | Yes | `MISSING` |  |
| `copyToApuChannel` | `0xE059` | L5915–L5932 | Entry | `MISSING` |  |
| `computeSoundEffMethod` | `0xE072` | L5933–L5962 | Yes | `MISSING` |  |
| `advanceAudioSlotFrame` | `0xE0B5` | L5979–L5988 | Yes | `MISSING` |  |
| `updateSoundEffectSlot2` | `0xE194` | L6063–L6067 | Yes | `MISSING` |  |
| `updateSoundEffectSlot3` | `0xE19C` | L6068–L6072 | Yes | `MISSING` |  |
| `updateSoundEffectSlot1` | `0xE1AC` | L6078–L6084 | Yes | `MISSING` |  |
| `updateSoundEffectSlot0` | `0xE1B9` | L6085–L6089 | Yes | `MISSING` |  |
| `updateSoundEffectSlotShared_rts` | `0xE1D7` | L6104–L6106 | Entry | `MISSING` |  |
| `initAudioAndMarkInited` | `0xE1E5` | L6114–L6119 | Entry | `MISSING` |  |
| `updateAudio` | `0xE216` | L6142–L6163 | Entry | `MISSING` |  |
| `soundEffectSlot2_makesNoSound` | `0xE244` | L6164–L6165 | Yes | `MISSING` |  |
| `muteAudioAndClearTriControl` | `0xE247` | L6166–L6172 | Yes | `MISSING` |  |
| `muteAudio` | `0xE271` | L6188–L6199 | Yes | `MISSING` |  |
| `initSoundEffectShared` | `0xE287` | L6200–L6234 | Yes | `MISSING` |  |
| `soundEffectSlot0_endingRocketInit` | `0xE2CC` | L6235–L6239 | Table | `MISSING` |  |
| `soundEffectSlot0_makesNoSound` | `0xE2DD` | L6248–L6252 | Table | `MISSING` |  |
| `advanceSoundEffectSlot0WithoutUpdate` | `0xE2E4` | L6253–L6255 | Table | `MISSING` |  |
| `updateSoundEffectSlot0WithoutUpdate_ret` | `0xE2F3` | L6261–L6263 | Entry | `MISSING` |  |
| `updateSoundEffectSlot0_apu` | `0xE2FF` | L6272–L6276 | Table | `MISSING` |  |
| `updateSoundEffectNoiseAudio` | `0xE307` | L6277–L6287 | Entry | `MISSING` |  |
| `getSoundEffectNoiseNibble` | `0xE31A` | L6288–L6311 | Yes | `MISSING` |  |
| `soundEffectSlot1_chirpChirpPlaying` | `0xE34F` | L6323–L6345 | Table | `MISSING` |  |
| `soundEffectSlot1_chirpChirpInit` | `0xE37F` | L6346–L6349 | Table | `MISSING` |  |
| `soundEffectSlot1Playing_advance` | `0xE39C` | L6364–L6366 | Table | `MISSING` |  |
| `soundEffectSlot1Playing_ret` | `0xE3B1` | L6374–L6376 | Entry | `MISSING` |  |
| `soundEffectSlot1_tetrisAchievedInit` | `0xE403` | L6426–L6431 | Table | `MISSING` |  |
| `soundEffectSlot1_tetrisAchievedPlaying` | `0xE40E` | L6432–L6436 | Table | `MISSING` |  |
| `soundEffectSlot1_lineCompletedInit` | `0xE41A` | L6439–L6444 | Table | `MISSING` |  |
| `soundEffectSlot1_lineCompletedPlaying` | `0xE425` | L6445–L6449 | Table | `MISSING` |  |
| `soundEffectSlot1_levelUpPlaying` | `0xE4D1` | L6524–L6534 | Table | `MISSING` |  |
| `soundEffectSlot1_levelUpInit` | `0xE4EC` | L6537–L6541 | Table | `MISSING` |  |
| `soundEffectSlot2_mediumBuzz` | `0xE50B` | L6547–L6550 | Table | `MISSING` |  |
| `soundEffectSlot2_lowBuzz` | `0xE513` | L6551–L6555 | Table | `MISSING` |  |
| `soundEffectSlot3_donk` | `0xE534` | L6568–L6572 | Table | `MISSING` |  |
| `soundEffectSlot3Playing_advance` | `0xE53B` | L6573–L6575 | Table | `MISSING` |  |
| `soundEffectSlot3Playing_ret` | `0xE550` | L6583–L6585 | Entry | `MISSING` |  |
| `updateSoundEffectSlot3_apu` | `0xE551` | L6586–L6599 | Table | `MISSING` |  |
| `soundEffectSlot3_fallingAlien` | `0xE56E` | L6600–L6607 | Table | `MISSING` |  |
| `updateMusic` | `0xE583` | L6613–L6635 | Yes | `MISSING` |  |
| `updateMusicFrame_setChanLo` | `0xE5D0` | L6643–L6664 | Yes | `MISSING` |  |
| `updateMusicFrame_setChanLoOffset` | `0xE637` | L6706–L6739 | Yes | `MISSING` |  |
| `musicLoOffset_setLo` | `0xE677` | L6745–L6759 | Entry | `MISSING` |  |
| `loadMusicTrack` | `0xE6D7` | L6780–L6809 | Yes | `MISSING` |  |
| `initSq12IfTrashedBySoundEffect` | `0xE735` | L6831–L6849 | Yes | `MISSING` |  |
| `initSq12IfTrashedBySoundEffect_ret` | `0xE765` | L6850–L6853 | Entry | `MISSING` |  |
| `updateMusicFrame_setChanVol` | `0xE766` | L6854–L6921 | Yes | `MISSING` |  |
| `updateMusicFrame_ret` | `0xE7FE` | L6943–L6945 | Entry | `MISSING` |  |
| `musicGetNextInstructionByte` | `0xEA41` | L7293–L7299 | Yes | `MISSING` |  |

### CORE_LOGIC (51 Routines)

| Routine Name | Address | Lines | JSR Called | Status | Notes |
|:---|:---:|:---:|:---:|:---:|:---|
| `branchOnGameMode` | `0x8161` | L432–L441 | Yes | `MISSING` |  |
| `L830B` | `0x830B` | L683–L798 | Entry | `MISSING` |  |
| `makePlayer1Active` | `0x8776` | L1269–L1287 | Yes | `MISSING` |  |
| `makePlayer2Active` | `0x8792` | L1288–L1306 | Yes | `MISSING` |  |
| `savePlayer1State` | `0x87AE` | L1307–L1324 | Yes | `MISSING` |  |
| `savePlayer2State` | `0x87C8` | L1325–L1338 | Yes | `MISSING` |  |
| `endTypeBInit` | `0x8875` | L1433–L1435 | Entry | `MISSING` |  |
| `L8B9D` | `0x8B9D` | L1802–L1831 | Entry | `MISSING` |  |
| `isPositionValid` | `0x948B` | L2392–L2456 | Yes | `MISSING` |  |
| `L9996` | `0x9996` | L3055–L3061 | Entry | `MISSING` |  |
| `L9C94` | `0x9C94` | L3516–L3528 | Entry | `MISSING` |  |
| `gameMode_startDemo` | `0x9DF6` | L3735–L3745 | Table | `MISSING` |  |
| `endingAnimation` | `0x9E3A` | L3790–L3801 | Yes | `MISSING` |  |
| `endingAnimationB` | `0x9E49` | L3802–L3944 | Entry | `MISSING` |  |
| `add100Points` | `0x9F62` | L3945–L3956 | Yes | `MISSING` |  |
| `L9FE9` | `0x9FE9` | L4010–L4014 | Entry | `MISSING` |  |
| `ending_initTypeBVars` | `0xA463` | L4623–L4663 | Yes | `MISSING` |  |
| `ending_typeB` | `0xA52E` | L4739–L4748 | Entry | `MISSING` |  |
| `ending_typeBConcert` | `0xA53B` | L4749–L4906 | Yes | `MISSING` |  |
| `ending_computeTypeBCathedralYTableIndex` | `0xA6AE` | L4980–L4991 | Yes | `MISSING` |  |
| `LA72C` | `0xA72C` | L5046–L5051 | Entry | `MISSING` |  |
| `endingAnimationA` | `0xA926` | L5165–L5196 | Entry | `MISSING` |  |
| `selectEndingScreen` | `0xA96E` | L5197–L5225 | Yes | `MISSING` |  |
| `ending_selected` | `0xA9A5` | L5226–L5233 | Entry | `MISSING` |  |
| `LAA82` | `0xAA82` | L5351–L5355 | Yes | `MISSING` |  |
| `copyAddrAtReturnAddressToTmp_incrReturnAddrBy2` | `0xAB21` | L5446–L5467 | Yes | `MISSING` |  |
| `pollController_actualRead` | `0xAB69` | L5493–L5514 | Yes | `MISSING` |  |
| `addExpansionPortInputAsControllerInput` | `0xAB8B` | L5515–L5525 | Yes | `MISSING` |  |
| `pollController` | `0xAB9D` | L5526–L5540 | Yes | `MISSING` |  |
| `LAC0D` | `0xAC0D` | L5585–L5594 | Entry | `MISSING` |  |
| `LAC67` | `0xAC67` | L5631–L5634 | Entry | `MISSING` |  |
| `memset_page` | `0xAC6A` | L5635–L5653 | Yes | `MISSING` |  |
| `switch_s_plus_2a` | `0xAC82` | L5654–L5683 | Yes | `MISSING` |  |
| `setMMC1Control` | `0xACA3` | L5684–L5695 | Yes | `MISSING` |  |
| `changeCHRBank0` | `0xACB7` | L5696–L5710 | Yes | `MISSING` |  |
| `changeCHRBank1` | `0xACCB` | L5711–L5725 | Yes | `MISSING` |  |
| `changePRGBank` | `0xACDF` | L5726–L5740 | Yes | `MISSING` |  |
| `copyToSq1Channel` | `0xE04B` | L5903–L5905 | Yes | `MISSING` |  |
| `copyToTriChannel` | `0xE04F` | L5906–L5908 | Yes | `MISSING` |  |
| `copyToNoiseChannel` | `0xE053` | L5909–L5911 | Yes | `MISSING` |  |
| `copyToSq2Channel` | `0xE057` | L5912–L5914 | Yes | `MISSING` |  |
| `LE1D8` | `0xE1D8` | L6107–L6113 | Entry | `MISSING` |  |
| `LE253` | `0xE253` | L6173–L6179 | Yes | `MISSING` |  |
| `LE265` | `0xE265` | L6180–L6187 | Entry | `MISSING` |  |
| `setNoiseLo` | `0xE2D3` | L6240–L6243 | Entry | `MISSING` |  |
| `loadNoiseLo` | `0xE2D7` | L6244–L6247 | Yes | `MISSING` |  |
| `LE33B` | `0xE33B` | L6312–L6320 | Yes | `MISSING` |  |
| `LE51B` | `0xE51B` | L6556–L6567 | Entry | `MISSING` |  |
| `storeDeref1AndContinue` | `0xE72A` | L6822–L6830 | Entry | `MISSING` |  |
| `playNoise` | `0xE9EB` | L7244–L7254 | Yes | `MISSING` |  |
| `playDmc` | `0xEA03` | L7255–L7292 | Yes | `MISSING` |  |

### GAMEPLAY_STATE (15 Routines)

| Routine Name | Address | Lines | JSR Called | Status | Notes |
|:---|:---:|:---:|:---:|:---:|:---|
| `gameModeState_updatePlayer1` | `0x8174` | L442–L450 | Table | `MISSING` |  |
| `gameModeState_updatePlayer2` | `0x8186` | L451–L461 | Table | `MISSING` |  |
| `branchOnPlayStatePlayer1` | `0x81B2` | L474–L488 | Yes | `MISSING` |  |
| `branchOnPlayStatePlayer2` | `0x81D9` | L495–L509 | Yes | `MISSING` |  |
| `gameModeState_initGameBackground` | `0x85F0` | L1092–L1167 | Table | `MISSING` |  |
| `gameModeState_initGameBackground_finish` | `0x8693` | L1168–L1182 | Entry | `MISSING` |  |
| `gameModeState_initGameState` | `0x86DC` | L1190–L1268 | Table | `MISSING` |  |
| `gameModeState_updateCountersAndNonPlayerState` | `0x8884` | L1441–L1464 | Table | `MISSING` |  |
| `gameModeState_handleGameOver` | `0x9CBF` | L3545–L3595 | Table | `MISSING` |  |
| `gameModeState_checkForResetKeyCombo` | `0x9E16` | L3756–L3770 | Table | `MISSING` |  |
| `gameModeState_vblankThenRunState2` | `0x9E27` | L3771–L3776 | Table | `MISSING` |  |
| `playState_incrementPlayState` | `0x9E37` | L3785–L3786 | Table | `MISSING` |  |
| `playState_noop` | `0x9E39` | L3787–L3789 | Table | `MISSING` |  |
| `playState_bTypeGoalCheck` | `0xA3F2` | L4545–L4584 | Table | `MISSING` |  |
| `playState_bTypeGoalCheck_ret` | `0xA42B` | L4585–L4588 | Entry | `MISSING` |  |

### MATH_BCD (9 Routines)

| Routine Name | Address | Lines | JSR Called | Status | Notes |
|:---|:---:|:---:|:---:|:---:|:---|
| `gameMode_playAndEndingHighScore_jmp` | `0x815D` | L428–L431 | Table | `MISSING` |  |
| `gameMode_playAndEndingHighScore` | `0x819B` | L462–L473 | Yes | `MISSING` |  |
| `playState_updateLinesAndStatistics` | `0x9B58` | L3329–L3377 | Table | `MISSING` |  |
| `showHighScores` | `0x9FF2` | L4015–L4020 | Yes | `MISSING` |  |
| `handleHighScoreIfNecessary` | `0xA0EE` | L4116–L4159 | Yes | `MISSING` |  |
| `copyHighScoreNameToNextIndex` | `0xA192` | L4208–L4238 | Yes | `MISSING` |  |
| `copyHighScoreScoreToNextIndex` | `0xA1C1` | L4239–L4258 | Yes | `MISSING` |  |
| `copyHighScoreLevelToNextIndex` | `0xA1E0` | L4259–L4271 | Yes | `MISSING` |  |
| `highScoreEntryScreen` | `0xA201` | L4276–L4438 | Entry | `MISSING` |  |

### MATH_RNG (2 Routines)

| Routine Name | Address | Lines | JSR Called | Status | Notes |
|:---|:---:|:---:|:---:|:---:|:---|
| `generateNextPseudorandomNumber` | `0xAB47` | L5468–L5485 | Yes | `VERIFIED` | Verified with LFSR bit rotation tests and seed manipulation |
| `unreferenced_soundRng` | `0xE09E` | L5963–L5978 | Entry | `VERIFIED` | Verified with audio RNG seed and tmp storage checks |

### MENUS_UI (13 Routines)

| Routine Name | Address | Lines | JSR Called | Status | Notes |
|:---|:---:|:---:|:---:|:---:|:---|
| `gameMode_legalScreen` | `0x8200` | L516–L558 | Table | `MISSING` |  |
| `gameMode_titleScreen` | `0x824F` | L559–L635 | Table | `MISSING` |  |
| `render_mode_legal_and_title_screens` | `0x82B1` | L636–L656 | Table | `MISSING` |  |
| `gameMode_gameTypeMenu` | `0x82D1` | L657–L682 | Table | `MISSING` |  |
| `gameMode_levelMenu` | `0x83D7` | L799–L844 | Table | `MISSING` |  |
| `gameMode_levelMenu_processPlayer1Navigation` | `0x8436` | L845–L919 | Entry | `MISSING` |  |
| `gameMode_levelMenu_handleLevelHeightNavigation` | `0x84AE` | L920–L1065 | Yes | `MISSING` |  |
| `render_mode_menu_screens` | `0x85DA` | L1078–L1091 | Table | `MISSING` |  |
| `soundEffectSlot1_menuOptionSelectPlaying_ret` | `0xE3B2` | L6377–L6379 | Entry | `MISSING` |  |
| `soundEffectSlot1_menuOptionSelectPlaying` | `0xE3B3` | L6380–L6391 | Table | `MISSING` |  |
| `soundEffectSlot1_menuOptionSelectInit` | `0xE3CA` | L6392–L6395 | Table | `MISSING` |  |
| `soundEffectSlot1_menuScreenSelectInit` | `0xE473` | L6485–L6492 | Table | `MISSING` |  |
| `soundEffectSlot1_menuScreenSelectPlaying` | `0xE481` | L6493–L6501 | Table | `MISSING` |  |

### PLAYER_INPUT (4 Routines)

| Routine Name | Address | Lines | JSR Called | Status | Notes |
|:---|:---:|:---:|:---:|:---:|:---|
| `pollControllerButtons` | `0x9D51` | L3630–L3724 | Yes | `MISSING` |  |
| `demoButtonsTable_indexIncr` | `0x9DE8` | L3725–L3734 | Yes | `MISSING` |  |
| `gameModeState_startButtonHandling` | `0xA37F` | L4475–L4544 | Table | `MISSING` |  |
| `diffOldAndNewButtons` | `0xABBD` | L5541–L5553 | Entry | `MISSING` |  |

### PLAYFIELD (11 Routines)

| Routine Name | Address | Lines | JSR Called | Status | Notes |
|:---|:---:|:---:|:---:|:---:|:---|
| `initPlayfieldIfTypeB` | `0x87DC` | L1339–L1343 | Yes | `MISSING` |  |
| `copyPlayfieldRowToVRAM` | `0x9725` | L2704–L2756 | Yes | `MISSING` |  |
| `updateLineClearingAnimation` | `0x977F` | L2757–L2827 | Yes | `MISSING` |  |
| `noop_disabledVramRowIncr` | `0x9874` | L2878–L2895 | Yes | `MISSING` |  |
| `playState_updateGameOverCurtain` | `0x9A11` | L3130–L3187 | Table | `MISSING` |  |
| `playState_checkForCompletedRows` | `0x9A6B` | L3188–L3279 | Table | `MISSING` |  |
| `playState_receiveGarbage` | `0x9B03` | L3280–L3326 | Table | `MISSING` |  |
| `updatePlayfield` | `0x9CAF` | L3532–L3544 | Yes | `MISSING` |  |
| `soundEffectSlot0_gameOverCurtainInit` | `0xE2F8` | L6267–L6271 | Table | `MISSING` |  |
| `soundEffectSlot1_lineClearingInit` | `0xE42E` | L6450–L6454 | Table | `MISSING` |  |
| `soundEffectSlot1_lineClearingPlaying` | `0xE43B` | L6458–L6461 | Table | `MISSING` |  |

### RENDER (19 Routines)

| Routine Name | Address | Lines | JSR Called | Status | Notes |
|:---|:---:|:---:|:---:|:---:|:---|
| `render` | `0x804B` | L290–L296 | Yes | `MISSING` |  |
| `loadSpriteIntoOamStaging` | `0x8C27` | L1861–L1903 | Yes | `MISSING` |  |
| `render_mode_play_and_demo` | `0x94EE` | L2457–L2671 | Table | `MISSING` |  |
| `twoDigsToPPU` | `0x9712` | L2691–L2703 | Yes | `MISSING` |  |
| `updatePaletteForLevel` | `0x9808` | L2833–L2870 | Yes | `MISSING` |  |
| `render_mode_ending_animation` | `0x9F95` | L3975–L4009 | Table | `MISSING` |  |
| `render_mode_congratulations_screen` | `0xA344` | L4443–L4474 | Table | `MISSING` |  |
| `ending_typeBConcertPatchToPpuForHeight` | `0xA4B1` | L4664–L4712 | Yes | `MISSING` |  |
| `patchToPpu` | `0xA507` | L4713–L4733 | Yes | `MISSING` |  |
| `render_ending` | `0xA527` | L4734–L4738 | Yes | `MISSING` |  |
| `ending_typeBCathedralSetSprite` | `0xA690` | L4961–L4979 | Yes | `MISSING` |  |
| `render_endingUnskippable` | `0xA7FD` | L5098–L5106 | Yes | `MISSING` |  |
| `_updatePpuMask` | `0xAA59` | L5323–L5327 | Entry | `MISSING` |  |
| `_updatePpuCtrl` | `0xAA7C` | L5346–L5350 | Entry | `MISSING` |  |
| `copyCurrentScrollAndCtrlToPPU` | `0xAA8A` | L5356–L5363 | Yes | `MISSING` |  |
| `bulkCopyToPpu` | `0xAA98` | L5364–L5367 | Yes | `MISSING` |  |
| `copyToPpu` | `0xAAF2` | L5417–L5423 | Entry | `MISSING` |  |
| `copyOamStagingToOam` | `0xAB5E` | L5486–L5492 | Yes | `MISSING` |  |
| `memset_ppu_page_and_more` | `0xAC1C` | L5595–L5611 | Yes | `MISSING` |  |

### SYSTEM (6 Routines)

| Routine Name | Address | Lines | JSR Called | Status | Notes |
|:---|:---:|:---:|:---:|:---:|:---|
| `nmi` | `0x8005` | L248–L287 | Table | `MISSING` |  |
| `sleep_for_14_vblanks` | `0xA44D` | L4594–L4601 | Yes | `MISSING` |  |
| `sleep_for_a_vblanks` | `0xA459` | L4602–L4609 | Yes | `MISSING` |  |
| `waitForVBlankAndEnableNmi` | `0xAA6B` | L5336–L5342 | Yes | `MISSING` |  |
| `disableNmi` | `0xAA78` | L5343–L5345 | Yes | `MISSING` |  |
| `reset` | `0xFF00` | L7692–L7769 | Table | `MISSING` |  |

### TETRIMINO (19 Routines)

| Routine Name | Address | Lines | JSR Called | Status | Notes |
|:---|:---:|:---:|:---:|:---:|:---|
| `playState_playerControlsActiveTetrimino` | `0x81CF` | L489–L494 | Table | `MISSING` |  |
| `playState_player2ControlsActiveTetrimino` | `0x81F6` | L510–L515 | Table | `MISSING` |  |
| `rotate_tetrimino` | `0x88AB` | L1465–L1502 | Yes | `MISSING` |  |
| `drop_tetrimino` | `0x8914` | L1530–L1600 | Yes | `MISSING` |  |
| `shift_tetrimino` | `0x89AE` | L1616–L1667 | Yes | `MISSING` |  |
| `stageSpriteForCurrentPiece` | `0x8A0A` | L1668–L1758 | Yes | `MISSING` |  |
| `stageSpriteForNextPiece` | `0x8BCE` | L1832–L1846 | Yes | `MISSING` |  |
| `playState_spawnNextTetrimino` | `0x988E` | L2896–L2947 | Table | `MISSING` |  |
| `chooseNextTetrimino` | `0x98EB` | L2948–L2963 | Yes | `VERIFIED` | Verified with demo table streams and gameplay mode branching |
| `pickRandomTetrimino` | `0x9903` | L2964–L2992 | Entry | `VERIFIED` | Verified with spawn count, anti-repetition reroll, and modulo-7 tests |
| `useNewSpawnID` | `0x9938` | L3001–L3004 | Entry | `MISSING` |  |
| `incrementPieceStat` | `0x9969` | L3030–L3054 | Yes | `VERIFIED` | Verified with 2-byte BCD decimal adjust (+6) and digit carry |
| `playState_lockTetrimino` | `0x99A2` | L3062–L3129 | Table | `MISSING` |  |
| `playState_unassignOrientationId` | `0x9E2F` | L3777–L3784 | Table | `MISSING` |  |
| `soundEffectSlot1_lockTetriminoInit` | `0xE384` | L6350–L6356 | Table | `MISSING` |  |
| `soundEffectSlot1_shiftTetriminoInit` | `0xE390` | L6357–L6363 | Table | `MISSING` |  |
| `soundEffectSlot1_rotateTetrimino_ret` | `0xE3D0` | L6396–L6398 | Entry | `MISSING` |  |
| `soundEffectSlot1_rotateTetriminoInit` | `0xE3D1` | L6399–L6404 | Table | `MISSING` |  |
| `soundEffectSlot1_rotateTetriminoPlaying` | `0xE3DD` | L6405–L6425 | Table | `MISSING` |  |
