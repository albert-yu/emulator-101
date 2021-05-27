# CC = clang

EXE = intel8080
SRC_DIR = src
OBJ_DIR = obj

SRC = $(wildcard $(SRC_DIR)/*.c)
OBJ = $(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

CFLAGS += -Wall
CPPFLAGS += -Iinclude

CPPFLAGS += -I/usr/include/SDL2
LDLIBS += -lSDL2

.PHONY: all clean debug

all: $(EXE) $(LIBOUT)

$(EXE): $(OBJ)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(DEBUG) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(OBJ): | $(OBJ_DIR)

$(OBJ_DIR):
	mkdir $(OBJ_DIR)

# debug: DEBUG = -DDEBUG
debug: DEBUG = -g -O0

debug: all

clean:
	$(RM) $(OBJ)
