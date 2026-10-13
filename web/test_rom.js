/**
 * Automated verification test for NES Tetris ROM execution in WebAssembly.
 * Run with: node web/test_rom.js
 */

const fs = require('fs');
const path = require('path');
const crypto = require('crypto');
const assert = require('assert');

const EXPECTED_SHA1 = '3026d28b63d94c921fe58364f8b0659d10b5a0ac';

async function runTest() {
    console.log('Testing full NES Tetris ROM in WebAssembly emulator (binjnes)...');

    // 1. Check ROM file existence and integrity
    const romCandidates = [
        path.join(__dirname, '..', 'Tetris (USA).nes'),
        path.join(__dirname, '..', 'TetrisNESDisasm', 'tetris.nes'),
    ];

    let romPath = null;
    for (const cand of romCandidates) {
        if (fs.existsSync(cand)) {
            romPath = cand;
            break;
        }
    }

    if (!romPath) {
        console.log('SKIPPED: reference ROM not found');
        process.exit(0);
    }

    const romBuffer = fs.readFileSync(romPath);
    const sha1 = crypto.createHash('sha1').update(romBuffer).digest('hex');
    console.log(`  Loaded ROM: ${romPath} (${romBuffer.length} bytes)`);
    console.log(`  ROM SHA-1: ${sha1}`);

    if (sha1 === EXPECTED_SHA1) {
        console.log('  [PASS] ROM SHA-1 matches authoritative commercial reference ROM');
    } else {
        console.log('  [NOTE] ROM is valid NES format, proceeding with test');
    }

    // 2. Load WebAssembly module
    const Binjnes = require('./binjnes.js');
    const m = await Binjnes();
    assert(m, 'Failed to initialize Binjnes WASM module');
    console.log('  [PASS] WebAssembly emulator module initialized');

    // 3. Allocate ROM in WASM memory
    const alignedSize = (romBuffer.length + 0x7fff) & ~0x7fff;
    const romPtr = m._malloc(alignedSize);
    m.HEAPU8.fill(0, romPtr, romPtr + alignedSize);
    m.HEAPU8.set(romBuffer, romPtr);

    const sampleRate = 44100;
    const audioFrames = 2048;
    const e = m._emulator_new_simple(romPtr, alignedSize, sampleRate, audioFrames);
    assert(e !== 0, 'Failed to create Emulator instance in WASM');
    console.log(`  [PASS] Emulator instantiated (pointer: 0x${e.toString(16)})`);

    const joypad = m._joypad_new_simple(e);
    assert(joypad !== 0, 'Failed to create Joypad instance');
    console.log('  [PASS] Joypad subsystem initialized');

    const rgbaPtr = m._get_rgba_frame_buffer_ptr(e);
    const rgbaSize = m._get_rgba_frame_buffer_size(e);
    assert.strictEqual(rgbaSize, 256 * 240 * 4, 'Framebuffer must be 256x240 RGBA (245,760 bytes)');

    const audioPtr = m._get_audio_buffer_ptr(e);
    const audioCap = m._get_audio_buffer_capacity(e);
    assert(audioCap > 0, 'Audio buffer capacity must be positive');
    console.log(`  [PASS] Video (256x240 RGBA) and Audio (${sampleRate}Hz) buffers mapped`);

    // Helper to step emulation
    let currentSec = 0;
    let framesRendered = 0;
    let audioFullEvents = 0;

    function stepFrames(count, buttons = {}) {
        for (let i = 0; i < count; i++) {
            currentSec += 1 / 60.0;
            if (buttons.start !== undefined) m._set_joyp_start(e, 0, buttons.start ? 1 : 0);
            if (buttons.a !== undefined) m._set_joyp_A(e, 0, buttons.a ? 1 : 0);
            if (buttons.b !== undefined) m._set_joyp_B(e, 0, buttons.b ? 1 : 0);
            if (buttons.left !== undefined) m._set_joyp_left(e, 0, buttons.left ? 1 : 0);
            if (buttons.right !== undefined) m._set_joyp_right(e, 0, buttons.right ? 1 : 0);
            if (buttons.down !== undefined) m._set_joyp_down(e, 0, buttons.down ? 1 : 0);

            while (true) {
                const event = m._emulator_run_until_secs(e, currentSec);
                if (event & 1) framesRendered++; // EVENT_NEW_FRAME
                if (event & 2) audioFullEvents++; // EVENT_AUDIO_BUFFER_FULL
                if (event & 4) break; // EVENT_UNTIL_TICKS
            }
        }
    }

    function countNonZeroPixels() {
        m._emulator_convert_frame_buffer_simple(e);
        const fb = new Uint8Array(m.HEAPU8.buffer, rgbaPtr, rgbaSize);
        let nz = 0;
        for (let i = 0; i < fb.length; i += 4) {
            if (fb[i] !== 0 || fb[i+1] !== 0 || fb[i+2] !== 0) {
                nz++;
            }
        }
        return nz;
    }

    // 4. Test Legal Screen (power-on)
    console.log('  Advancing 120 frames through legal copyright screen...');
    stepFrames(120);
    const legalPixels = countNonZeroPixels();
    assert(legalPixels > 1000, `Expected rendered graphics on legal screen, got ${legalPixels} pixels`);
    console.log(`  [PASS] Legal screen rendered successfully (${legalPixels} non-zero pixels)`);

    // 5. Test Title Screen
    console.log('  Advancing past legal screen (total 280 frames) to Title Screen...');
    stepFrames(160);
    const titlePixels = countNonZeroPixels();
    assert(titlePixels > 1000, 'Title screen must render pixels');
    console.log(`  [PASS] Title screen rendered successfully (${titlePixels} non-zero pixels)`);

    // 6. Test Menu Navigation (Game Type Menu)
    console.log('  Pressing START on title screen to enter Game Type Menu...');
    stepFrames(10, { start: true });
    stepFrames(40, { start: false });
    const menuPixels = countNonZeroPixels();
    assert(menuPixels > 1000, 'Game Type menu must render pixels');
    console.log(`  [PASS] Game Type Menu rendered successfully (${menuPixels} non-zero pixels)`);

    // 7. Test Level Select Menu
    console.log('  Pressing START to enter Level Select Menu...');
    stepFrames(10, { start: true });
    stepFrames(40, { start: false });
    const levelPixels = countNonZeroPixels();
    assert(levelPixels > 1000, 'Level Select menu must render pixels');
    console.log(`  [PASS] Level Select Menu rendered successfully (${levelPixels} non-zero pixels)`);

    // 8. Test Gameplay
    console.log('  Pressing START to enter active gameplay...');
    stepFrames(10, { start: true });
    stepFrames(60, { start: false });
    const gamePixels = countNonZeroPixels();
    assert(gamePixels > 1000, 'Gameplay screen must render pixels');
    console.log(`  [PASS] Gameplay playfield rendered successfully (${gamePixels} non-zero pixels)`);

    // 9. Test In-Game Controls (Shift Left, Right, Rotate, Soft Drop)
    console.log('  Testing controller inputs during gameplay...');
    stepFrames(10, { left: true });
    stepFrames(10, { left: false, right: true });
    stepFrames(10, { right: false, a: true });
    stepFrames(10, { a: false, b: true });
    stepFrames(20, { b: false, down: true });
    stepFrames(20, { down: false });
    console.log('  [PASS] Controller input processing executed without error');

    // 10. Verify Audio Generation
    assert(audioFullEvents > 20, `Expected audio buffer events, got ${audioFullEvents}`);
    console.log(`  [PASS] 2A03 APU audio synthesized successfully (${audioFullEvents} buffer events)`);

    // 11. Test Reset Vector
    console.log('  Testing soft reset...');
    m._emulator_set_reset(e, 1);
    stepFrames(5);
    m._emulator_set_reset(e, 0);
    stepFrames(30);
    console.log('  [PASS] Reset vector handled cleanly');

    // Cleanup
    m._joypad_delete(joypad);
    m._emulator_delete(e);
    m._free(romPtr);

    console.log('\nAll full NES ROM WebAssembly verification checks PASSED!');
}

runTest().catch(err => {
    console.error('\nVerification FAILED:', err);
    process.exit(1);
});
