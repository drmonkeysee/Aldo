OS := $(shell uname)
SRC_DIR := src
BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj

MODULES := emu
SRC_FILES := $(wildcard $(SRC_DIR)/*.c) $(foreach MDL,$(MODULES),$(wildcard $(SRC_DIR)/$(MDL)/*.c))
OBJ_FILES := $(subst $(SRC_DIR),$(OBJ_DIR),$(SRC_FILES:.c=.o))
DEP_FILES := $(OBJ_FILES:.o=.d)
OBJ_DIRS := $(OBJ_DIR) $(foreach MDL,$(MODULES),$(OBJ_DIR)/$(MDL))
PRODUCT := aldo
TARGET := $(BUILD_DIR)/$(PRODUCT)

CFLAGS := -Wall -Wextra -pedantic -std=c17
SP := strip

ifeq ($(OS), Darwin)
CFLAGS += -I/usr/local/opt/ncurses/include
LDFLAGS := -L/usr/local/opt/ncurses/lib
LDLIBS := -lncurses
else
CFLAGS += -D_POSIX_C_SOURCE=199309L
LDLIBS := -lncursesw
SPFLAGS := -s
endif

ifdef XCF
CFLAGS += $(XCF)
endif

ifdef XLF
LDFLAGS += $(XLF)
endif

.PHONY: release debug run clean

release: CFLAGS += -Werror -Os -flto -DNDEBUG
release: $(TARGET)
	$(SP) $(SPFLAGS) $(TARGET)

debug: CFLAGS += -g -O0 -DDEBUG
debug: $(TARGET)

$(TARGET): $(OBJ_FILES)
	$(CC) $^ -o $@ $(LDFLAGS) $(LDLIBS)

-include $(DEP_FILES)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIRS)
	$(CC) $(CFLAGS) -MMD -c $< -o $@

$(OBJ_DIRS):
	mkdir -p $@

run: debug
	$(TARGET)

clean:
	$(RM) -r $(BUILD_DIR)
