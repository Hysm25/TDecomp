CC = emcc
CFLAGS = -O2 -std=c11 -Wall -Wextra -Werror -pedantic -I ../include
LDFLAGS = -s WASM=1 \
          -s ALLOW_MEMORY_GROWTH=1 \
          -s MODULARIZE=1 \
          -s EXPORT_NAME='createTDecompModule' \
          -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap","getValue","setValue","HEAPU8"]' \
          -s NO_EXIT_RUNTIME=1

SRCS = tdecomp_wasm.c \
       ../src/nes.c \
       ../src/math/bcd.c \
       ../src/math/rng.c \
       ../src/game/audio.c \
       ../src/game/ending.c \
       ../src/game/highscore.c \
       ../src/game/input.c \
       ../src/game/menu.c \
       ../src/game/playfield.c \
       ../src/game/render.c \
       ../src/game/state.c \
       ../src/game/system.c \
       ../src/game/tetrimino.c

TARGET = tdecomp.js

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) $(LDFLAGS) -o $(TARGET)

clean:
	rm -f tdecomp.js tdecomp.wasm

.PHONY: all clean
