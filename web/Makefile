CC = emcc
CFLAGS = -O2 -std=c11 -Wall -Wextra -I emulator
LDFLAGS = -s WASM=1 \
          -s ALLOW_MEMORY_GROWTH=1 \
          -s MODULARIZE=1 \
          -s EXPORT_NAME='Binjnes' \
          -s NO_EXIT_RUNTIME=1 \
          -s EXPORTED_FUNCTIONS=@emulator/exported.json \
          -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap","setValue","getValue","UTF8ToString","HEAPU8","HEAP32","HEAPF32"]'

SRCS = emulator/memory.c \
       emulator/cartdb.c \
       emulator/emulator.c \
       emulator/joypad.c \
       emulator/rewind.c \
       emulator/wrapper.c

TARGET = binjnes.js

all: $(TARGET)

$(TARGET): $(SRCS) emulator/exported.json
	$(CC) $(CFLAGS) $(SRCS) $(LDFLAGS) -o $(TARGET)

clean:
	rm -f binjnes.js binjnes.wasm tdecomp.js tdecomp.wasm

.PHONY: all clean
