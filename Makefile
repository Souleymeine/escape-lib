CC = gcc

BUILD_DIR = build
SRC_DIR = src
OUT_NAME = test

SOURCES = $(wildcard $(SRC_DIR)/*.c)

cc_is_gcc_or_clang = false
cc_is_msvc = false
ifeq ($(CC),gcc)
	cc_is_gcc_or_clang = true
else ifeq ($(CC),clang)
	cc_is_gcc_or_clang = true
else ifeq ($(CC),cl)
	cc_is_msvc = true
else ifeq ($(CC),cl.exe)
	cc_is_msvc = true
endif

DEBUG_OPTS   =
RELEASE_OPTS =
ifeq ($(cc_is_gcc_or_clang),true)
	DEBUG_OPTS   = -I. -std=c2x -Wall -Wextra -DDEBUG -O0 -g3
	RELEASE_OPTS = -I. -std=c2x -Wall -Wextra -Werror -O2
else ifeq ($(cc_is_msvc),true)
	DEBUG_OPTS   = -std:clatest -Wall -DDEBUG -Od -ZI
	RELEASE_OPTS = -std:clatest -Wall -WX -O2
endif

ifneq ($(OS),Windows_NT)
	DEBUG_OPTS += -fsanitize=address,leak,undefined
endif

# Produce temp files XOR Compile
TMP_OPTS     =
ifeq ($(tmp),true)
	ifeq ($(cc_is_gcc_or_clang),true)
		TMP_OPTS += -S -fverbose-asm -save-temps -w
		ifeq ($(CC), gcc)
			TMP_OPTS += -fdump-tree-optimized
		else ifeq ($(CC), clang)
		# Disables asm output as the -S flag produces textual IR when combined with -emit-llvm
		# See https://releases.llvm.org/7.0.0/tools/clang/docs/Toolchain.html
		# Uncomment the line below if you want to generate LLVM IR instead of assembly when using clang.
		#	TMP_OPTS += -emit-llvm
		endif
	else ifeq ($(cc_is_msvc),true)
		TMP_OPTS += -P -Fi
	endif
else
	ifeq ($(cc_is_gcc_or_clang),true)
		DEBUG_OPTS   += -o $(OUT_NAME)
		RELEASE_OPTS += -o $(OUT_NAME)
	endif
endif

# Thanks to :
# https://stackoverflow.com/questions/20763629/test-whether-a-directory-exists-inside-a-makefile

debug: | $(BUILD_DIR)
	cd build && $(CC) $(addprefix ../, $(SOURCES)) $(DEBUG_OPTS) $(TMP_OPTS)

release: | $(BUILD_DIR)
	cd build && $(CC) $(addprefix ../, $(SOURCES)) $(RELEASE_OPTS) $(TMP_OPTS)

clean:
ifeq ($(shell test -d $(BUILD_DIR) && echo 1 || echo 0), 1)
	rm -r $(BUILD_DIR)
endif

$(BUILD_DIR):
	mkdir $@

