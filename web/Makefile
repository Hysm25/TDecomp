# WebAssembly build for TDecomp using Emscripten and raylib HTML5 support

CC := emcc
EMMAKE ?= emmake
MAKE ?= make

PROJECT_ROOT := $(abspath ..)
RAYLIB_DIR ?= $(CURDIR)/raylib
RAYLIB_LIB := $(RAYLIB_DIR)/src/libraylib.a

# Allow using cached or system raylib source if available
ifneq ($(wildcard /tmp/raylib-src/src/libraylib.a),)
    RAYLIB_DIR := /tmp/raylib-src
    RAYLIB_LIB := /tmp/raylib-src/src/libraylib.a
endif

# Decompiled core library sources
CORE_SOURCES := \
    $(PROJECT_ROOT)/src/nes.c \
    $(PROJECT_ROOT)/src/math/rng.c \
    $(PROJECT_ROOT)/src/math/bcd.c \
    $(PROJECT_ROOT)/src/game/tetrimino.c \
    $(PROJECT_ROOT)/src/game/playfield.c \
    $(PROJECT_ROOT)/src/game/input.c \
    $(PROJECT_ROOT)/src/game/state.c \
    $(PROJECT_ROOT)/src/game/menu.c \
    $(PROJECT_ROOT)/src/game/highscore.c \
    $(PROJECT_ROOT)/src/game/render.c \
    $(PROJECT_ROOT)/src/game/ending.c \
    $(PROJECT_ROOT)/src/game/audio.c \
    $(PROJECT_ROOT)/src/game/system.c

# Native presentation and raylib adapter sources
APP_SOURCES := \
    $(PROJECT_ROOT)/native/main.c \
    $(PROJECT_ROOT)/native/game_adapter.c \
    $(PROJECT_ROOT)/native/renderer.c \
    $(PROJECT_ROOT)/native/audio.c

ALL_SOURCES := $(CORE_SOURCES) $(APP_SOURCES)

INCLUDES := \
    -I$(PROJECT_ROOT)/include \
    -I$(PROJECT_ROOT)/native \
    -I$(RAYLIB_DIR)/src

CFLAGS := \
    -std=c11 \
    -Wall \
    -Wextra \
    -O2 \
    -DPLATFORM_WEB \
    $(INCLUDES)

LDFLAGS := \
    $(RAYLIB_LIB) \
    -s USE_GLFW=3 \
    -s ALLOW_MEMORY_GROWTH=1 \
    -s EXPORTED_RUNTIME_METHODS=ccall,cwrap \
    --shell-file shell.html

TARGET := tdecomp.html

.PHONY: all clean serve test

all: $(TARGET) index.html

# Download and compile raylib for WebAssembly if not present
$(RAYLIB_DIR):
	@echo "==> Fetching raylib 5.0 repository for WebAssembly build..."
	git clone --depth 1 --branch 5.0 https://github.com/raysan5/raylib.git $(RAYLIB_DIR)

$(RAYLIB_LIB): | $(RAYLIB_DIR)
	@echo "==> Compiling raylib static library for WebAssembly (PLATFORM_WEB)..."
	$(MAKE) -C $(RAYLIB_DIR)/src PLATFORM=PLATFORM_WEB -j$(shell nproc 2>/dev/null || echo 2)

$(TARGET): $(ALL_SOURCES) $(RAYLIB_LIB) shell.html
	@echo "==> Compiling TDecomp WebAssembly module with emcc..."
	$(CC) $(CFLAGS) $(ALL_SOURCES) $(LDFLAGS) -o $(TARGET)
	@echo "==> Build complete: $(TARGET) (along with tdecomp.js and tdecomp.wasm)"

index.html: $(TARGET)
	@cp -f $(TARGET) index.html

test: $(TARGET)
	@echo "==> Running automated Node.js WebAssembly verification test..."
	node tdecomp.js --headless --test-frames 120
	@echo "==> WebAssembly test passed successfully!"

serve: all
	@echo "==> Starting local HTTP server at http://localhost:8080..."
	python3 -m http.server 8080 --directory $(CURDIR)

clean:
	rm -f tdecomp.html tdecomp.js tdecomp.wasm index.html
