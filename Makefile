OS := $(shell uname)
SRC_DIR := src
CLI_DIR := cli
GUI_DIR := gui
EXT_DIR := ext
IMGUI_DIR := imgui
TEST_DIR := test
BUILD_DIR := build
CLI_PATH := $(SRC_DIR)/$(CLI_DIR)
GUI_PATH := $(SRC_DIR)/$(GUI_DIR)
IMGUI_PATH := $(EXT_DIR)/$(IMGUI_DIR)
OBJ_PATH := $(BUILD_DIR)/obj
CLI_OBJ_PATH := $(OBJ_PATH)/$(CLI_DIR)
GUI_OBJ_PATH := $(OBJ_PATH)/$(GUI_DIR)
IMGUI_OBJ_PATH := $(OBJ_PATH)/$(IMGUI_DIR)
TEST_OBJ_PATH := $(OBJ_PATH)/$(TEST_DIR)
OBJ_PATHS := $(OBJ_PATH) $(CLI_OBJ_PATH) $(GUI_OBJ_PATH) $(IMGUI_OBJ_PATH) $(TEST_OBJ_PATH)

LIB_SRC := $(wildcard $(SRC_DIR)/*.c)
CLI_SRC := $(wildcard $(CLI_PATH)/*.c)
GUI_SRC := $(wildcard $(GUI_PATH)/*.c) $(wildcard $(GUI_PATH)/*.cpp)
IMGUI_SRC := $(wildcard $(IMGUI_PATH)/*.cpp)
TEST_SRC := $(wildcard $(TEST_DIR)/*.c)

LIB_OBJ := $(subst $(SRC_DIR),$(OBJ_PATH),$(LIB_SRC:.c=.o))
CLI_OBJ := $(subst $(SRC_DIR),$(OBJ_PATH),$(CLI_SRC:.c=.o))
GUI_OBJ := $(subst $(SRC_DIR),$(OBJ_PATH),$(addsuffix .o,$(basename $(GUI_SRC))))
IMGUI_OBJ := $(subst $(EXT_DIR),$(OBJ_PATH),$(IMGUI_SRC:.cpp=.o))
TEST_OBJ := $(addprefix $(OBJ_PATH)/,$(TEST_SRC:.c=.o))

DEP_FILES := $(LIB_OBJ:.o=.d) $(CLI_OBJ:.o=.d) $(GUI_OBJ:.o=.d) $(IMGUI_OBJ:.o=.d)
TEST_DEPS := $(CLI_OBJ_PATH)/argparse.o

PRODUCT := aldo
LIB_TARGET := $(BUILD_DIR)/lib$(PRODUCT).a
CLI_TARGET := $(BUILD_DIR)/$(PRODUCT)c
GUI_TARGET := $(BUILD_DIR)/$(PRODUCT)gui
TESTS_TARGET := $(BUILD_DIR)/$(PRODUCT)tests

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
CXXFLAGS := -Wall -Wextra -pedantic -std=c++20
ifneq ($(OS), Darwin)
CFLAGS += -D_POSIX_C_SOURCE=200112L -Wno-format-zero-length
endif

DEBUG_COMPILE := -g -O0 -DDEBUG
RELEASE_COMPILE := -Werror -Os -flto -DNDEBUG

LDFLAGS := -L$(BUILD_DIR)
LDLIBS := -l$(PRODUCT)
SP := strip

ifdef XCF
CFLAGS += $(XCF)
endif

ifdef XLF
LDFLAGS += $(XLF)
endif

.PHONY: bcdtest check clean debug debug-gui debug-lib empty ext extclean nesdiff nestest \
	purge release release-gui release-lib run test version

empty:
	$(info Please specify a make target)

ext:
	$(MAKE) -C $@

extclean:
	$(MAKE) clean -C ext

release: CFLAGS += $(RELEASE_COMPILE)
ifneq ($(OS), Darwin)
release: SPFLAGS := -s
endif
release: $(CLI_TARGET)
	$(SP) $(SPFLAGS) $<

release-gui: CFLAGS += $(RELEASE_COMPILE)
release-gui: CXXFLAGS += $(RELEASE_COMPILE)
ifneq ($(OS), Darwin)
release-gui: SPFLAGS := -s
endif
release-gui: $(GUI_TARGET)
	$(SP) $(SPFLAGS) $<

release-lib: CFLAGS += $(RELEASE_COMPILE)
release-lib: $(LIB_TARGET)

debug: CFLAGS += $(DEBUG_COMPILE)
debug: $(CLI_TARGET)

debug-gui: CFLAGS += $(DEBUG_COMPILE)
debug-gui: CXXFLAGS += $(DEBUG_COMPILE)
debug-gui: $(GUI_TARGET)

debug-lib: CFLAGS += $(DEBUG_COMPILE)
debug-lib: $(LIB_TARGET)

run: debug
	$(CLI_TARGET) $(FILE)

check: test nestest nesdiff bcdtest

test: CFLAGS += $(DEBUG_COMPILE)
test: $(TESTS_TARGET)
	$<

nestest: $(NESTEST_ROM) debug
	$(RM) $(TRACE_CMP)
	$(CLI_TARGET) -btvz -H@c66e -Hjam -H3s -rc000 $<

nesdiff: $(NESTEST_CMP) $(TRACE_CMP)
	diff -y --suppress-common-lines $^ > $(NESTEST_DIFF); \
	DIFF_RESULT=$$?; \
	echo "$$(wc -l < $(NESTEST_DIFF)) lines differ"; \
	exit $$DIFF_RESULT

bcdtest: $(BCDTEST_ROM) debug
	$(CLI_TARGET) -bDv -g$(TEST_DIR)/bcdtest.brk $<
	hexdump -C system.ram | head -n1 | awk '{ print "ERROR =",$$2; \
	if ($$2 == 0) print "BCD Pass!"; else { print "BCD Fail :("; exit 1 }}'

clean:
	$(RM) -r $(BUILD_DIR)

purge: clean
	$(RM) $(PURGE_ASSETS)

$(LIB_TARGET): $(LIB_OBJ)
	$(AR) $(ARFLAGS) $@ $?

ifeq ($(OS), Darwin)
$(CLI_TARGET): CFLAGS += -I/opt/homebrew/opt/ncurses/include
$(CLI_TARGET): LDFLAGS += -L/opt/homebrew/opt/ncurses/lib
$(CLI_TARGET): LDLIBS += -lpanel -lncurses
else
$(CLI_TARGET): LDLIBS += -lm -lpanelw -lncursesw
endif
$(CLI_TARGET): $(CLI_OBJ) | $(LIB_TARGET)
	$(CC) $^ -o $@ $(LDFLAGS) $(LDLIBS)

ifeq ($(OS), Darwin)
$(GUI_TARGET):
	$(error Make target not supported on macOS; use Xcode project instead)
else
$(GUI_TARGET): LDLIBS += -lSDL2
$(GUI_TARGET): $(GUI_OBJ) $(IMGUI_OBJ) | $(LIB_TARGET)
	$(CXX) $^ -o $@ $(LDFLAGS) $(LDLIBS)
endif

$(TESTS_TARGET): LDLIBS += -lcinytest
ifneq ($(OS), Darwin)
$(TESTS_TARGET): LDFLAGS += -L/usr/local/lib -Wl,-rpath,/usr/local/lib
$(TESTS_TARGET): LDLIBS += -lm
endif
$(TESTS_TARGET): $(TEST_OBJ) $(TEST_DEPS) | $(LIB_TARGET)
	$(CC) $^ -o $@ $(LDFLAGS) $(LDLIBS)

-include $(DEP_FILES)

$(OBJ_PATH)/%.o: $(SRC_DIR)/%.c | $(OBJ_PATH) $(CLI_OBJ_PATH) $(GUI_OBJ_PATH)
	$(CC) $(CFLAGS) -pedantic -MMD -c $< -o $@

ifneq ($(OS), Darwin)
$(OBJ_PATH)/%.o: CXXFLAGS += -Wno-missing-field-initializers
endif
$(OBJ_PATH)/%.o: $(SRC_DIR)/%.cpp | $(GUI_OBJ_PATH)
	$(CXX) $(CXXFLAGS) -Wconversion -iquote$(SRC_DIR) -iquote$(IMGUI_PATH) -MMD -c $< -o $@

$(OBJ_PATH)/%.o: $(EXT_DIR)/%.cpp | $(IMGUI_OBJ_PATH)
	$(CXX) $(CXXFLAGS) -MMD -c $< -o $@

ifeq ($(OS), Darwin)
$(TEST_OBJ_PATH)/%.o: CFLAGS += -pedantic -Wno-gnu-zero-variadic-macro-arguments
endif
$(TEST_OBJ_PATH)/%.o: $(TEST_DIR)/%.c | $(TEST_OBJ_PATH)
	$(CC) $(CFLAGS) -Wno-unused-parameter -iquote$(CLI_PATH) -MMD -c $< -o $@

$(OBJ_PATHS):
	mkdir -p $@

$(NESTEST_ROM) $(NESTEST_LOG):
	cd $(TEST_DIR) && curl -O '$(NESTEST_HTTP)/$(@F)'

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
