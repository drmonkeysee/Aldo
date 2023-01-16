OS := $(shell uname)
SRC_DIR := src
CLI_DIR := cli
TEST_DIR := test
BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj

SRC_FILES := $(wildcard $(SRC_DIR)/*.c) $(wildcard $(SRC_DIR)/$(CLI_DIR)/*.c)
TEST_FILES := $(wildcard $(TEST_DIR)/*.c)

OBJ_FILES := $(subst $(SRC_DIR),$(OBJ_DIR),$(SRC_FILES:.c=.o))
TEST_OBJ_FILES := $(addprefix $(OBJ_DIR)/,$(TEST_FILES:.c=.o))
DEP_FILES := $(OBJ_FILES:.o=.d)

OBJ_DIRS := $(OBJ_DIR) $(OBJ_DIR)/$(CLI_DIR)
TEST_OBJ_DIR := $(OBJ_DIR)/$(TEST_DIR)
TEST_DEPS := $(addprefix $(OBJ_DIR)/,$(CLI_DIR)/argparse.o bus.o bytes.o cart.o \
		 cpu.o debug.o decode.o dis.o haltexpr.o mappers.o)

PRODUCT := aldoc
TESTS := $(PRODUCT)tests
TARGET := $(BUILD_DIR)/$(PRODUCT)
TESTS_TARGET := $(BUILD_DIR)/$(TESTS)

NESTEST_HTTP := https://raw.githubusercontent.com/christopherpow/nes-test-roms/master/other
NESTEST_ROM := $(TEST_DIR)/nestest.nes
NESTEST_LOG := $(TEST_DIR)/nestest.log
NESTEST_CMP := $(TEST_DIR)/nestest-cmp.log
NESTEST_DIFF := $(TEST_DIR)/nestest.diff
TRACE_LOG := trace.log
TRACE_CMP := $(TEST_DIR)/trace-cmp.log
BCDTEST_ROM := $(TEST_DIR)/bcdtest.rom
PURGE_ASSETS := $(NESTEST_ROM) $(NESTEST_LOG) $(NESTEST_CMP) $(NESTEST_DIFF) \
		$(TRACE_CMP) $(BCDTEST_ROM) $(TRACE_LOG) system.ram

CFLAGS := -Wall -Wextra -Wconversion -std=c17 -iquote$(SRC_DIR)
ifneq ($(OS), Darwin)
CFLAGS += -Wno-format-zero-length
endif

SRC_CFLAGS := -pedantic
TEST_CFLAGS := -Wno-unused-parameter -iquote$(SRC_DIR)/$(CLI_DIR)
SP := strip

ifdef XCF
CFLAGS += $(XCF)
endif

ifdef XLF
LDFLAGS += $(XLF)
endif

.PHONY: bcdtest check clean debug empty ext extclean nesdiff nestest purge \
	release run test version

empty:
	$(info Please specify a make target)

ext:
	$(MAKE) -C $@

extclean:
	$(MAKE) clean -C ext

release: CFLAGS += -Werror -Os -flto -DNDEBUG
ifneq ($(OS), Darwin)
release: SPFLAGS := -s
endif
release: $(TARGET)
	$(SP) $(SPFLAGS) $(TARGET)

debug: CFLAGS += -g -O0 -DDEBUG
debug: $(TARGET)

run: debug
	$(TARGET) $(FILE)

check: test nestest nesdiff bcdtest

test: CFLAGS += -g -O0 -DDEBUG
test: $(TESTS_TARGET)
	$(TESTS_TARGET)

nestest: $(NESTEST_ROM) debug
	$(RM) $(TRACE_CMP)
	$(TARGET) -btvz -H@c66e -Hjam -H3s -rc000 $<

nesdiff: $(NESTEST_CMP) $(TRACE_CMP)
	diff -y --suppress-common-lines $^ > $(NESTEST_DIFF); \
	DIFF_RESULT=$$?; \
	echo "$$(wc -l < $(NESTEST_DIFF)) lines differ"; \
	exit $$DIFF_RESULT

bcdtest: $(BCDTEST_ROM) debug
	$(TARGET) -bDv -g$(TEST_DIR)/bcdtest.brk $<
	hexdump -C system.ram | head -n1 | awk '{ print "ERROR =",$$2; \
	if ($$2 == 0) print "BCD Pass!"; else { print "BCD Fail :("; exit 1 }}'

clean:
	$(RM) -r $(BUILD_DIR)

purge: clean
	$(RM) $(PURGE_ASSETS)

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
$(TEST_OBJ_DIR)/%.o: TEST_CFLAGS += -pedantic -Wno-gnu-zero-variadic-macro-arguments
endif
$(TEST_OBJ_DIR)/%.o: $(TEST_DIR)/%.c | $(TEST_OBJ_DIR)
	$(CC) $(CFLAGS) $(TEST_CFLAGS) -MMD -c $< -o $@

$(OBJ_DIRS) $(TEST_OBJ_DIR):
	mkdir -p $@

$(NESTEST_ROM) $(NESTEST_LOG):
	cd $(TEST_DIR) && curl -O '$(NESTEST_HTTP)/$(notdir $@)'

$(NESTEST_CMP): $(NESTEST_LOG)
	# NOTE: strip out PPU column (unimplemented) and Accumulator cue (implied)
	sed -e 's/PPU:.\{3\},.\{3\} //' -e 's/ A /   /' $< > $@

$(TRACE_CMP):
	# NOTE: convert aldo trace log format to nestest format
	sed -e 's/:/ /' -e 's/>/=/' -e 's/S:/SP:/' -e 's/CPU/CYC/' \
		-e 's/[+-][[:digit:]]\{1,3\} @ /$$/' \
		-e 's/\(P:\)\(..\) (.\{8\})/\10x\2/' \
		-e 's/\*ISC/\*ISB/' -e 's/FLT/FF/' $(TRACE_LOG) \
		| awk 'BEGIN { FS=" A:" } NR > 1 \
		{ p=substr($$2, match($$2, /0x../), 4); \
		mask="echo $$((" p " & 0xef))"; mask | getline p; close(mask); \
		p = sprintf("%X", p); sub(/0x../, p, $$2); \
		printf "%-47s%s%s\n", $$1, FS, $$2 }' > $@

$(BCDTEST_ROM):
	python3 $(TEST_DIR)/bcdtest.py

ifdef DATE
version: DATE_FLAG := -d $(DATE)
endif
ifdef VER
version: VER_FLAG := -v $(VER)
endif
version:
	tools/version.sh $(DATE_FLAG) $(VER_FLAG)
