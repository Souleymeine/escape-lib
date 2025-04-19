CC = gcc

BUILD_DIR = build
SRC_DIR = src
OUT_NAME = test

SOURCES = $(wildcard $(SRC_DIR)/*.c)


# Produce temp files XOR Compile

DEBUG_OPTS   = -I. -std=c23 -Wall -Wextra -DDEBUG -O0 -g3 -fsanitize=address,leak,undefined
RELEASE_OPTS = -I. -std=c23 -Wall -Wextra -Werror -O3

TMP_OPTS     =
ifeq ($(tmp),true)
	TMP_OPTS += -S -fverbose-asm -save-temps -w
	ifeq ($(CC), gcc)
		TMP_OPTS += -fdump-tree-optimized
	else ifeq ($(CC), clang)
	# Disables asm output as the -S flag produces textual IR when combined with -emit-llvm
	# See https://releases.llvm.org/7.0.0/tools/clang/docs/Toolchain.html
	# Uncomment the line below if you want to generate LLVM IR instead of assembly when using clang.
	#	TMP_OPTS += -emit-llvm
	endif
else
	DEBUG_OPTS   += -o $(OUT_NAME)
	RELEASE_OPTS += -o $(OUT_NAME)
endif

# Thanks to :
# https://stackoverflow.com/questions/20763629/test-whether-a-directory-exists-inside-a-makefile

debug: | $(BUILD_DIR)
	cd build && $(CC) ../$(SOURCES) $(DEBUG_OPTS) $(TMP_OPTS)

release: | $(BUILD_DIR)
	cd build && $(CC) ../$(SOURCES) $(RELEASE_OPTS) $(TMP_OPTS)

clean:
ifeq ($(shell test -d $(BUILD_DIR) && echo 1 || echo 0), 1)
	rm -r $(BUILD_DIR)
endif

$(BUILD_DIR):
	mkdir $@

