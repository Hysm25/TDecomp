# binjnes — WebAssembly NES Emulation Core

This directory contains the WebAssembly emulation core adapted from **[binjnes](https://github.com/binji/binjnes)** by Ben Smith (binji), distributed under the terms of the **MIT License** (see [LICENSE](LICENSE)).

### Upstream Information

- **Upstream Repository**: https://github.com/binji/binjnes
- **Author**: Ben Smith (binji)
- **License**: MIT License
- **Mapper Support**: Full support for Mapper 1 (MMC1 / SxROM, 32KB PRG, 16KB CHR) used by NES Tetris (USA).
- **Subsystems**:
  - **CPU**: Cycle-accurate Ricoh 2A03 (NMOS 6502 core with decimal mode disabled).
  - **PPU**: Scanline-accurate Ricoh 2C02 (256×240 resolution, nametables, fine scrolling, sprite evaluation, and 64-color NES palette).
  - **APU**: Cycle-accurate sound synthesis for Pulse 1, Pulse 2, Triangle, Noise, and DMC channels generating 44.1 kHz float audio.
  - **Memory & Mappers**: Built-in cartridge database (CartDB) with CRC checksum matching and SxROM bank switching.
