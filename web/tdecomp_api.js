/**
 * TDecomp JavaScript API for WebAssembly NES Tetris Engine.
 *
 * Provides a clean object-oriented interface over the compiled WebAssembly core.
 * Works seamlessly in both browser and Node.js environments.
 */

const BUTTONS = {
    RIGHT:  0x01,
    LEFT:   0x02,
    DOWN:   0x04,
    UP:     0x08,
    START:  0x10,
    SELECT: 0x20,
    B:      0x40,
    A:      0x80,
};

const PIECES = {
    T: 0,
    J: 1,
    Z: 2,
    O: 3,
    S: 4,
    L: 5,
    I: 6,
};

const PIECE_NAMES = ['T', 'J', 'Z', 'O', 'S', 'L', 'I'];

// Authentic NES Tetris piece to orientation spawn map
const PIECE_SPAWN_ORIENTATIONS = {
    0: 2,  // T -> tDown
    1: 7,  // J -> jDown
    2: 8,  // Z -> zHoriz
    3: 10, // O -> oFixed
    4: 11, // S -> sHoriz
    5: 14, // L -> lDown
    6: 18, // I -> iHoriz
};

class TDecompGame {
    /**
     * @param {object} module - Initialized Emscripten WASM module
     */
    constructor(module) {
        this.module = module;

        // Bind C functions
        this._init = module.cwrap('tdecomp_wasm_init', 'number', ['number', 'number']);
        this._reset = module.cwrap('tdecomp_wasm_reset', 'number', []);
        this._stepFrame = module.cwrap('tdecomp_wasm_step_frame', 'number', ['number']);
        this._actionLeft = module.cwrap('tdecomp_wasm_action_left', 'number', []);
        this._actionRight = module.cwrap('tdecomp_wasm_action_right', 'number', []);
        this._actionRotateCW = module.cwrap('tdecomp_wasm_action_rotate_cw', 'number', []);
        this._actionRotateCCW = module.cwrap('tdecomp_wasm_action_rotate_ccw', 'number', []);
        this._actionDrop = module.cwrap('tdecomp_wasm_action_drop', 'number', []);
        this._actionHardDrop = module.cwrap('tdecomp_wasm_action_hard_drop', 'number', []);
        this._getPlayfieldPtr = module.cwrap('tdecomp_wasm_get_playfield', 'number', []);
        this._getPlayfieldCell = module.cwrap('tdecomp_wasm_get_playfield_cell', 'number', ['number', 'number']);
        this._getCurrentPiece = module.cwrap('tdecomp_wasm_get_current_piece', 'number', []);
        this._getCurrentX = module.cwrap('tdecomp_wasm_get_current_x', 'number', []);
        this._getCurrentY = module.cwrap('tdecomp_wasm_get_current_y', 'number', []);
        this._getNextPiece = module.cwrap('tdecomp_wasm_get_next_piece', 'number', []);
        this._getScore = module.cwrap('tdecomp_wasm_get_score', 'number', []);
        this._getLines = module.cwrap('tdecomp_wasm_get_lines', 'number', []);
        this._getLevel = module.cwrap('tdecomp_wasm_get_level', 'number', []);
        this._getPlayState = module.cwrap('tdecomp_wasm_get_play_state', 'number', []);
        this._isGameOver = module.cwrap('tdecomp_wasm_is_game_over', 'number', []);
        this._getMinoX = module.cwrap('tdecomp_wasm_get_mino_x', 'number', ['number', 'number']);
        this._getMinoY = module.cwrap('tdecomp_wasm_get_mino_y', 'number', ['number', 'number']);
        this._getMinoTile = module.cwrap('tdecomp_wasm_get_mino_tile', 'number', ['number', 'number']);
        this._getStat = module.cwrap('tdecomp_wasm_get_stat', 'number', ['number']);
        this._getHighScore = module.cwrap('tdecomp_wasm_get_high_score', 'number', ['number']);
    }

    /**
     * Factory method to load and instantiate TDecompGame.
     * @param {Function} [moduleFactory] - Optional createTDecompModule factory function
     * @returns {Promise<TDecompGame>}
     */
    static async create(moduleFactory) {
        let factory = moduleFactory;
        if (!factory && typeof createTDecompModule === 'function') {
            factory = createTDecompModule;
        }
        if (!factory && typeof require !== 'undefined') {
            factory = require('./tdecomp.js');
        }
        if (!factory) {
            throw new Error('createTDecompModule factory function not found');
        }

        const moduleInstance = await factory();
        return new TDecompGame(moduleInstance);
    }

    /**
     * Initialize a new game.
     * @param {number} [startLevel=0] - Starting level (0 - 19)
     * @param {number} [gameType=0] - 0 for Type A (marathon), 1 for Type B (25 lines)
     */
    init(startLevel = 0, gameType = 0) {
        return this._init(startLevel, gameType);
    }

    /**
     * Reset the game with the same level and type.
     */
    reset() {
        return this._reset();
    }

    /**
     * Advance the game by one frame (1/60th second).
     * @param {number} [buttonMask=0] - Bitmask of pressed controller buttons
     * @returns {number} 0 if game is ongoing, 1 if game over
     */
    stepFrame(buttonMask = 0) {
        return this._stepFrame(buttonMask);
    }

    /** Move active piece left 1 column */
    actionLeft() {
        return this._actionLeft();
    }

    /** Move active piece right 1 column */
    actionRight() {
        return this._actionRight();
    }

    /** Rotate active piece clockwise */
    actionRotateCW() {
        return this._actionRotateCW();
    }

    /** Rotate active piece counter-clockwise */
    actionRotateCCW() {
        return this._actionRotateCCW();
    }

    /** Drop active piece 1 row */
    actionDrop() {
        return this._actionDrop();
    }

    /** Instantly drop active piece to lowest valid position and lock */
    actionHardDrop() {
        return this._actionHardDrop();
    }

    /**
     * Get a copy of the 10x20 playfield (200 bytes).
     * Empty cells are 0xEF or 0xFF. Placed blocks are tile IDs (0x7B, 0x7C, 0x7D, 0x78, etc.).
     * @returns {Uint8Array}
     */
    getPlayfield() {
        const ptr = this._getPlayfieldPtr();
        return new Uint8Array(this.module.HEAPU8.buffer, ptr, 200).slice();
    }

    /**
     * Get a single cell from the playfield.
     * @param {number} x - Column (0 - 9)
     * @param {number} y - Row (0 - 19)
     * @returns {number} Tile ID (0xEF is empty)
     */
    getPlayfieldCell(x, y) {
        return this._getPlayfieldCell(x, y);
    }

    /** Current piece orientation ID (0 - 18) */
    getCurrentPiece() {
        return this._getCurrentPiece();
    }

    /** Current piece X origin (0 - 9) */
    getCurrentX() {
        return this._getCurrentX();
    }

    /** Current piece Y origin (0 - 19) */
    getCurrentY() {
        return this._getCurrentY();
    }

    /** Next piece preview ID (0 - 6 or orientation ID) */
    getNextPiece() {
        return this._getNextPiece();
    }

    /** Current player score (decimal integer) */
    getScore() {
        return this._getScore();
    }

    /** Total cleared lines (decimal integer) */
    getLines() {
        return this._getLines();
    }

    /** Current level (0 - 29+) */
    getLevel() {
        return this._getLevel();
    }

    /** Current play state (1 = control active, 2 = locking, 4 = line clear anim, etc.) */
    getPlayState() {
        return this._getPlayState();
    }

    /** Returns true if game has ended */
    isGameOver() {
        return this._isGameOver() !== 0;
    }

    /**
     * Calculates the absolute coordinates of the 4 minos of an orientation.
     * @param {number} orientation - Orientation ID (0 - 18)
     * @param {number} originX - Piece origin X
     * @param {number} originY - Piece origin Y
     * @returns {Array<{x: number, y: number, tile: number}>}
     */
    getMinoCoords(orientation, originX, originY) {
        const minos = [];
        for (let i = 0; i < 4; i++) {
            const relX = this._getMinoX(orientation, i);
            const relY = this._getMinoY(orientation, i);
            const tile = this._getMinoTile(orientation, i);
            minos.push({
                x: originX + relX,
                y: originY + relY,
                tile: tile,
            });
        }
        return minos;
    }

    /**
     * Get mino coordinates of the current active falling piece.
     * @returns {Array<{x: number, y: number, tile: number}>}
     */
    getCurrentPieceMinos() {
        const orientation = this.getCurrentPiece();
        if (orientation > 18) return [];
        return this.getMinoCoords(orientation, this.getCurrentX(), this.getCurrentY());
    }

    /**
     * Get spawn mino coordinates for previewing a piece.
     * @param {number} pieceId - Piece ID (0 - 6) or orientation
     * @returns {Array<{x: number, y: number, tile: number}>}
     */
    getPreviewMinos(pieceId) {
        let orient = pieceId;
        if (pieceId in PIECE_SPAWN_ORIENTATIONS) {
            orient = PIECE_SPAWN_ORIENTATIONS[pieceId];
        }
        if (orient > 18) orient = 0;
        return this.getMinoCoords(orient, 0, 0);
    }

    /**
     * Cleared count for a piece type (0 - 6).
     * @param {number} pieceId
     * @returns {number}
     */
    getStat(pieceId) {
        return this._getStat(pieceId);
    }

    /**
     * Top high score (rank 0, 1, or 2).
     * @param {number} [rank=0]
     * @returns {number}
     */
    getHighScore(rank = 0) {
        return this._getHighScore(rank);
    }
}

// Module export for Node.js / CommonJS and browser window
if (typeof module !== 'undefined' && module.exports) {
    module.exports = {
        TDecompGame,
        BUTTONS,
        PIECES,
        PIECE_NAMES,
    };
}
if (typeof window !== 'undefined') {
    window.TDecompGame = TDecompGame;
    window.TDECOMP_BUTTONS = BUTTONS;
    window.TDECOMP_PIECES = PIECES;
}
