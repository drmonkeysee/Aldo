OS := $(shell uname)
SRC_DIR := src
TEST_DIR := test
BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj

MODULES := emu
SRC_FILES := $(wildcard $(SRC_DIR)/*.c) \
		$(foreach MDL,$(MODULES),$(wildcard $(SRC_DIR)/$(MDL)/*.c))
TEST_FILES := $(wildcard $(TEST_DIR)/*.c) \
		$(foreach MDL,$(MODULES),$(wildcard $(TEST_DIR)/$(MDL)/*.c))

OBJ_FILES := $(subst $(SRC_DIR),$(OBJ_DIR),$(SRC_FILES:.c=.o))
TEST_OBJ_FILES := $(addprefix $(OBJ_DIR)/,$(TEST_FILES:.c=.o))
DEP_FILES := $(OBJ_FILES:.o=.d)

OBJ_DIRS := $(OBJ_DIR) $(foreach MDL,$(MODULES),$(OBJ_DIR)/$(MDL))
TEST_OBJ_DIRS := $(OBJ_DIR)/$(TEST_DIR) \
			$(foreach MDL,$(MODULES),$(OBJ_DIR)/$(TEST_DIR)/$(MDL))
TEST_DEPS := $(addprefix $(OBJ_DIR)/,dis.o emu/bus.o emu/bytes.o emu/cart.o \
		emu/cpu.o emu/decode.o emu/mappers.o)

PRODUCT := aldo
TESTS := $(PRODUCT)tests
TARGET := $(BUILD_DIR)/$(PRODUCT)
TESTS_TARGET := $(BUILD_DIR)/$(TESTS)

CFLAGS := -Wall -Wextra -std=c17
SRC_CFLAGS := -pedantic
TEST_CFLAGS := -Wno-unused-parameter -iquote$(SRC_DIR)
SP := strip

ifdef XCF
CFLAGS += $(XCF)
endif

ifdef XLF
LDFLAGS += $(XLF)
endif

.PHONY: release debug check run clean

release: CFLAGS += -Werror -Os -flto -DNDEBUG
ifneq ($(OS), Darwin)
release: SPFLAGS := -s
endif
release: $(TARGET)
	$(SP) $(SPFLAGS) $(TARGET)

debug: CFLAGS += -g -O0 -DDEBUG
debug: $(TARGET)

check: CFLAGS += -g -O0 -DDEBUG
check: $(TESTS_TARGET)
	$(TESTS_TARGET)

ifeq ($(OS), Darwin)
$(TARGET): LDFLAGS := -L/opt/homebrew/opt/ncurses/lib
$(TARGET): LDLIBS := -lpanel -lncurses
else
$(TARGET): LDLIBS := -lm -lpanelw -lncursesw
endif
$(TARGET): $(OBJ_FILES)
	$(CC) $^ -o $@ $(LDFLAGS) $(LDLIBS)

$(TESTS_TARGET): LDLIBS := -lcinytest
ifneq ($(OS), Darwin)
$(TESTS_TARGET): LDFLAGS := -L/usr/local/lib -Wl,-rpath,/usr/local/lib
$(TESTS_TARGET): LDLIBS += -lm
endif
$(TESTS_TARGET): $(TEST_OBJ_FILES) $(TEST_DEPS)
	$(CC) $^ -o $@ $(LDFLAGS) $(LDLIBS)

-include $(DEP_FILES)

ifeq ($(OS), Darwin)
$(OBJ_DIR)/%.o: SRC_CFLAGS += -I/opt/homebrew/opt/ncurses/include
else
$(OBJ_DIR)/%.o: SRC_CFLAGS += -D_POSIX_C_SOURCE=200112L
endif
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIRS)
	$(CC) $(CFLAGS) $(SRC_CFLAGS) -MMD -c $< -o $@

ifeq ($(OS), Darwin)
$(OBJ_DIR)/$(TEST_DIR)/%.o: TEST_CFLAGS += -pedantic -Wno-gnu-zero-variadic-macro-arguments
endif
$(OBJ_DIR)/$(TEST_DIR)/%.o: $(TEST_DIR)/%.c | $(TEST_OBJ_DIRS)
	$(CC) $(CFLAGS) $(TEST_CFLAGS) -MMD -c $< -o $@

$(OBJ_DIRS) $(TEST_OBJ_DIRS):
	mkdir -p $@

run: debug
	$(TARGET) $(FILE)

clean:
	$(RM) -r $(BUILD_DIR)
