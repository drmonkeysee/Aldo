OS := $(shell uname)
SRC_DIR := src
TEST_DIR := test
BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj

MODULES := emu
SRC_FILES := $(wildcard $(SRC_DIR)/*.c) $(foreach MDL,$(MODULES),$(wildcard $(SRC_DIR)/$(MDL)/*.c))
TEST_FILES := $(wildcard $(TEST_DIR)/*.c) $(foreach MDL,$(MODULES),$(wildcard $(TEST_DIR)/$(MDL)/*.c))
OBJ_FILES := $(subst $(SRC_DIR),$(OBJ_DIR),$(SRC_FILES:.c=.o))
TEST_OBJ_FILES := $(addprefix $(OBJ_DIR)/,$(TEST_FILES:.c=.o))
DEP_FILES := $(OBJ_FILES:.o=.d)
OBJ_DIRS := $(OBJ_DIR) $(foreach MDL,$(MODULES),$(OBJ_DIR)/$(MDL))
TEST_OBJ_DIRS := $(OBJ_DIR)/$(TEST_DIR) $(foreach MDL,$(MODULES),$(OBJ_DIR)/$(TEST_DIR)/$(MDL))
TEST_DEPS := $(addprefix $(OBJ_DIR)/,asm.o emu/bytes.o emu/decode.o)
PRODUCT := aldo
TESTS := $(PRODUCT)tests
TARGET := $(BUILD_DIR)/$(PRODUCT)
TESTS_TARGET := $(BUILD_DIR)/$(TESTS)

CFLAGS := -Wall -Wextra -pedantic -std=c17
SP := strip

ifeq ($(OS), Darwin)
CFLAGS += -I/usr/local/opt/ncurses/include
LDFLAGS := -L/usr/local/opt/ncurses/lib
LDLIBS := -lpanel -lncurses
else
CFLAGS += -D_POSIX_C_SOURCE=200112L
LDLIBS := -lpanelw -lncursesw
SPFLAGS := -s
endif

ifdef XCF
CFLAGS += $(XCF)
endif

ifdef XLF
LDFLAGS += $(XLF)
endif

.PHONY: release debug check run clean

release: CFLAGS += -Werror -Os -flto -DNDEBUG
release: $(TARGET)
	$(SP) $(SPFLAGS) $(TARGET)

debug: CFLAGS += -g -O0 -DDEBUG
debug: $(TARGET)

check: CFLAGS += -g -O0 -DDEBUG -Wno-gnu-zero-variadic-macro-arguments -Wno-unused-parameter -iquote$(SRC_DIR)
# TODO: can i remove extraneous flags set by ifeq above?
check: LDLIBS := -lcinytest
check: $(TESTS_TARGET)
	$(TESTS_TARGET)

$(TARGET): $(OBJ_FILES)
	$(CC) $^ -o $@ $(LDFLAGS) $(LDLIBS)

$(TESTS_TARGET): $(TEST_OBJ_FILES) $(TEST_DEPS)
	$(CC) $^ -o $@ $(LDFLAGS) $(LDLIBS)

-include $(DEP_FILES)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIRS)
	$(CC) $(CFLAGS) -MMD -c $< -o $@

$(OBJ_DIR)/$(TEST_DIR)/%.o: $(TEST_DIR)/%.c | $(TEST_OBJ_DIRS)
	$(CC) $(CFLAGS) -MMD -c $< -o $@

$(OBJ_DIRS) $(TEST_OBJ_DIRS):
	mkdir -p $@

run: debug
	$(TARGET)

clean:
	$(RM) -r $(BUILD_DIR)
