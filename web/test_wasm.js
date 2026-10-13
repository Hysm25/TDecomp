/**
 * Test script for TDecomp WebAssembly JavaScript API.
 * Run with: node web/test_wasm.js
 */

const assert = require('assert');
const path = require('path');
const { TDecompGame, BUTTONS } = require('./tdecomp_api.js');
const createTDecompModule = require('./tdecomp.js');

async function runTests() {
    console.log('Testing TDecomp WebAssembly module...');

    const game = await TDecompGame.create(createTDecompModule);
    assert(game, 'Failed to create TDecompGame instance');
    console.log('  [PASS] WASM Module loaded successfully');

    // 1. Initialize Type A game at Level 0
    game.init(0, 0);
    assert.strictEqual(game.getLevel(), 0, 'Level should be 0');
    assert.strictEqual(game.getScore(), 0, 'Score should be 0');
    assert.strictEqual(game.getLines(), 0, 'Lines should be 0');
    assert.strictEqual(game.isGameOver(), false, 'Game should not be over');
    assert.strictEqual(game.getCurrentX(), 5, 'Initial spawn X should be 5');
    assert.strictEqual(game.getCurrentY(), 0, 'Initial spawn Y should be 0');
    console.log('  [PASS] Initial game state verified');

    // 2. Playfield verification
    const pf = game.getPlayfield();
    assert.strictEqual(pf.length, 200, 'Playfield should have 200 cells');
    assert.strictEqual(pf[0], 0xEF, 'Top-left playfield cell should be empty (0xEF)');
    assert.strictEqual(game.getPlayfieldCell(0, 0), 0xEF, 'Cell (0,0) should be empty');
    console.log('  [PASS] Playfield buffer verified');

    // 3. Discrete actions
    const initialPiece = game.getCurrentPiece();
    assert(initialPiece >= 0 && initialPiece <= 18, 'Piece orientation should be in 0..18');

    // Move left
    const leftMoved = game.actionLeft();
    assert.strictEqual(leftMoved, 1, 'actionLeft should succeed');
    assert.strictEqual(game.getCurrentX(), 4, 'Piece X should be 4 after moving left');

    // Move right
    const rightMoved = game.actionRight();
    assert.strictEqual(rightMoved, 1, 'actionRight should succeed');
    assert.strictEqual(game.getCurrentX(), 5, 'Piece X should be 5 after moving right');

    // Rotate CW
    const rotCW = game.actionRotateCW();
    assert.strictEqual(rotCW, 1, 'actionRotateCW should succeed');

    // Check mino coordinates calculation
    const minos = game.getCurrentPieceMinos();
    assert.strictEqual(minos.length, 4, 'Current piece should have 4 minos');
    minos.forEach(mino => {
        assert(typeof mino.x === 'number', 'Mino X should be a number');
        assert(typeof mino.y === 'number', 'Mino Y should be a number');
        assert(typeof mino.tile === 'number', 'Mino tile should be a number');
    });
    console.log('  [PASS] Piece actions and mino coordinates verified');

    // Hard drop
    const drops = game.actionHardDrop();
    assert(drops > 0, 'actionHardDrop should drop at least 1 cell');
    assert.strictEqual(game.getPlayState(), 2, 'Play state should be LOCK_TETRIMINO (2)');
    console.log(`  [PASS] Hard drop locked piece after ${drops} cells`);

    // 4. Stepping frames through lock and next piece spawn
    for (let f = 0; f < 30; f++) {
        game.stepFrame(0);
    }
    assert.strictEqual(game.getPlayState(), 1, 'New piece should be spawned and in CONTROL_ACTIVE (1)');
    assert.strictEqual(game.getCurrentY(), 0, 'New piece should spawn at Y=0');
    console.log('  [PASS] Frame stepping and next piece spawn verified');

    // 5. Holding Down button in frame loop
    const scoreBefore = game.getScore();
    for (let f = 0; f < 100; f++) {
        game.stepFrame(BUTTONS.DOWN);
    }
    console.log(`  [PASS] Frame loop with DOWN input ran smoothly, score=${game.getScore()}`);

    // 6. Type B game verification
    game.init(5, 1);
    assert.strictEqual(game.getLevel(), 5, 'Type B level should be 5');
    assert.strictEqual(game.getLines(), 25, 'Type B initial line goal should be 25');
    console.log('  [PASS] Type B initialization verified');

    console.log('\nAll WebAssembly API tests PASSED successfully!');
}

runTests().catch(err => {
    console.error('Test FAILED:', err);
    process.exit(1);
});
