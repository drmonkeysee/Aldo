OS := $(shell uname)
SRC_DIR := src
TEST_DIR := test
BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj

SRC_FILES := $(wildcard $(SRC_DIR)/*.c)
TEST_FILES := $(wildcard $(TEST_DIR)/*.c)

OBJ_FILES := $(subst $(SRC_DIR),$(OBJ_DIR),$(SRC_FILES:.c=.o))
TEST_OBJ_FILES := $(addprefix $(OBJ_DIR)/,$(TEST_FILES:.c=.o))
DEP_FILES := $(OBJ_FILES:.o=.d)

TEST_OBJ_DIR := $(OBJ_DIR)/$(TEST_DIR)
TEST_DEPS := $(addprefix $(OBJ_DIR)/,bus.o bytes.o cart.o cpu.o \
		decode.o dis.o mappers.o)

PRODUCT := aldo
TESTS := $(PRODUCT)tests
TARGET := $(BUILD_DIR)/$(PRODUCT)
TESTS_TARGET := $(BUILD_DIR)/$(TESTS)
NESTEST_CMP := nestest-cmp.log
TRACE_CMP := trace-cmp.log

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

.PHONY: check clean debug nesdiff nestest release run

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
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(SRC_CFLAGS) -MMD -c $< -o $@

ifeq ($(OS), Darwin)
$(TEST_OBJ_DIR)/%.o: TEST_CFLAGS += -pedantic -Wno-gnu-zero-variadic-macro-arguments
endif
$(TEST_OBJ_DIR)/%.o: $(TEST_DIR)/%.c | $(TEST_OBJ_DIR)
	$(CC) $(CFLAGS) $(TEST_CFLAGS) -MMD -c $< -o $@

$(OBJ_DIR) $(TEST_OBJ_DIR):
	mkdir -p $@

run: debug
	$(TARGET) $(FILE)

nestest: debug
	rm -f $(TRACE_CMP)
	$(TARGET) -bt -Hc66e -rc000 nestest.nes

$(NESTEST_CMP):
	# Strip out PPU column (unimplemented) and Accumulator cue (implied)
	sed -e 's/PPU:.\{3\},.\{3\} //' -e 's/ A /   /' nestest.log > $@

$(TRACE_CMP):
	sed -e 's/:/ /' -e 's/>/=/' -e 's/S:/SP:/' -e 's/CPU/CYC/' \
		-e 's/[+-][[:digit:]]\{1,3\} @ /$$/' \
		-e 's/\(P:\)\(..\) (.\{8\})/\10x\2/' trace.log \
		| awk 'BEGIN { FS=" A:" } NR > 1 \
		{ p=substr($$2, match($$2, /0x../), 4); \
		mask="echo $$((" p " & 0xef))"; mask | getline p; close(mask); \
		p = sprintf("%X", p); sub(/0x../, p, $$2); \
		printf "%-47s%s%s\n", $$1, FS, $$2 }' > $@

nesdiff: $(NESTEST_CMP) $(TRACE_CMP)
	echo "$$(diff $^ | wc -l) lines differ"

clean:
	$(RM) -r $(BUILD_DIR)
