# TDecomp Technical Documentation & Historical Reports

This directory contains technical reports, feasibility studies, and historical audit documentation produced during the reverse-engineering, decompilation, and ROM integration phases of the `TDecomp` project.

---

## Technical Reports Index

### 1. [REPOSITORY_AUDIT.md](REPOSITORY_AUDIT.md)
- **Scope**: Repository audit, file census, build configuration hardening, and test harness review.
- **Key Outcomes**: Hardened compiler flags to strict `-std=c11 -Wall -Wextra -Werror -pedantic`, standardized test return codes, eliminated compiler warning suppressions, and verified multi-level test suite integrity.

### 2. [ROM_BUILD_REPORT.md](ROM_BUILD_REPORT.md)
- **Scope**: NES ROM build pipeline, assembly rebuild verification, and initial C-to-6502 feasibility.
- **Key Outcomes**: Established out-of-tree assembly build (`tetris_baseline.nes`), proved bit-exact PRG/CHR payload matching against the reference cartridge (`Tetris (USA).nes`), and verified cc65 compiler capabilities.

### 3. [C_TO_6502_INTEGRATION_REPORT.md](C_TO_6502_INTEGRATION_REPORT.md)
- **Scope**: Incremental routine-level replacement of native 6502 assembly with object code compiled from target-ready C.
- **Key Outcomes**: Successfully integrated `copyOamStagingToOam` ($AB5E, 11 bytes) and `muteAudio` ($E271, 22 bytes) into `tetris_experimental.nes` with bit-exact matching; evaluated gameplay RNG (`generateNextPseudorandomNumber`), optimized C footprint to 54 bytes, and documented the in-place slot collision blocker.

### 4. [BANK_LEVEL_C_PORT_FEASIBILITY.md](BANK_LEVEL_C_PORT_FEASIBILITY.md)
- **Scope**: Bank-level / cluster-level C decompilation without requiring byte-for-byte machine code parity.
- **Key Outcomes**: Audited the High Score subsystem cluster ($A0EE–$A200, 275 bytes); determined in-place replacement is blocked by compiler expansion (636B C footprint / 2.31x expansion); proved and verified relocatable bank integration into unreferenced cartridge slack (`unreferenced_data1` at $D6C9) with an assembly trampoline at $A0EE, producing verified `tetris_bank_exp.nes` (49,168 bytes).

---

## Primary Project Documentation (Root Directory)

- [README.md](../README.md): Project overview, architecture, capabilities, build instructions, and limitations.
- [DECOMPILATION_PROGRESS.md](../DECOMPILATION_PROGRESS.md): Complete chronological batch tracking and routine verification census.
- [ROUTINE_INVENTORY.md](../ROUTINE_INVENTORY.md): Comprehensive inventory of all 171 standalone routines, 111 alternate entry points, and 260 authoritative data tables.
- [CODEBASE_HEALTH_REPORT.md](../CODEBASE_HEALTH_REPORT.md): Current codebase health, cleanup findings, tool audit, and maintainability summary.
