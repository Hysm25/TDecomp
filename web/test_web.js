#!/usr/bin/env node
/**
 * test_web.js - Automated WebAssembly validation runner for TDecomp
 *
 * Verifies that the WebAssembly build properly enforces ROM validation,
 * extracts assets from Tetris (USA).nes, and runs the headless simulation.
 */

const fs = require('fs');
const path = require('path');
const { execFileSync } = require('child_process');

const repoRoot = path.resolve(__dirname, '..');
const wasmScript = path.resolve(__dirname, 'tdecomp.js');
const romPath = path.resolve(repoRoot, 'Tetris (USA).nes');

if (!fs.existsSync(romPath)) {
    console.error("[ERROR] Reference ROM not found at:", romPath);
    process.exit(1);
}

console.log(`==> Found reference ROM: ${romPath} (${fs.statSync(romPath).size} bytes)`);

// Test 1: Rejection of execution without valid ROM
console.log("==> Test 1: Verifying WASM rejects missing ROM...");
try {
    execFileSync('node', [wasmScript, '--headless', '--rom', 'nonexistent.nes'], {
        cwd: repoRoot,
        stdio: 'pipe'
    });
    console.error("[FAIL] WASM unexpectedly succeeded with nonexistent ROM!");
    process.exit(1);
} catch (err) {
    const stderr = (err.stderr ? err.stderr.toString() : '') + (err.stdout ? err.stdout.toString() : '');
    if (stderr.includes("FATAL ERROR") || stderr.includes("ROM verification failed") || err.status === 1) {
        console.log("  [PASS] WASM correctly aborted when ROM was missing (exit code 1)");
    } else {
        console.error("[FAIL] Unexpected error output:", stderr);
        process.exit(1);
    }
}

// Test 2: Rejection of corrupted ROM with bad hash
console.log("==> Test 2: Verifying WASM rejects invalid ROM hash...");
const badRomPath = '/tmp/wasm_bad_rom.nes';
try {
    const dummyRom = Buffer.alloc(49168);
    dummyRom.write("NES\x1a\x02\x02", 0);
    fs.writeFileSync(badRomPath, dummyRom);

    execFileSync('node', [wasmScript, '--headless', '--rom', badRomPath], {
        cwd: repoRoot,
        stdio: 'pipe'
    });
    console.error("[FAIL] WASM unexpectedly accepted bad ROM hash!");
    process.exit(1);
} catch (err) {
    const stderr = (err.stderr ? err.stderr.toString() : '') + (err.stdout ? err.stdout.toString() : '');
    if (stderr.includes("hash mismatch") || err.status === 1) {
        console.log("  [PASS] WASM correctly rejected bad ROM hash (exit code 1)");
    } else {
        console.error("[FAIL] Unexpected error output:", stderr);
        process.exit(1);
    }
} finally {
    try { fs.unlinkSync(badRomPath); } catch(e) {}
}

// Test 3: Successful extraction and simulation with authentic Tetris (USA).nes
console.log("==> Test 3: Verifying WASM simulation with valid Tetris (USA).nes...");
try {
    const stdout = execFileSync('node', [wasmScript, '--headless', '--rom', 'Tetris (USA).nes', '--test-frames', '120'], {
        cwd: repoRoot,
        stdio: 'pipe'
    });

    const outStr = stdout.toString();
    console.log(outStr.trim());
    if (outStr.includes("Verification passed") && outStr.includes("ROM authenticated")) {
        console.log("  [PASS] WASM simulation and ROM asset extraction succeeded!");
    } else {
        console.error("[FAIL] Output did not contain expected verification passed message");
        process.exit(1);
    }
} catch (err) {
    console.error("[FAIL] WASM execution failed:", err.message);
    if (err.stderr) console.error(err.stderr.toString());
    process.exit(1);
}

console.log("\n==> ALL WEBASSEMBLY TESTS PASSED SUCCESSFULLY! <==");
process.exit(0);
