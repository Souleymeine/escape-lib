# ~~~ Configurable options (overrideable from cmdline) ~~~
CONFIG = build.default.cfg
ifneq ($(file < build.cfg),)
CONFIG = build.cfg
endif

include $(CONFIG)

# ~~~ Checking for missing options ~~~
override missing-opt-err       = $(error Please specify "$(1)" in $(CONFIG) or from the commandline, e.g. `$$ make $(MAKECMDGOALS) $(1)=...`)
override missing-ver-err       = $(error Missing $(1) number in $(CONFIG))
override missing-opt-war-yesno = $(warning Please specify "$(1)" in $(CONFIG) or from the commandline, e.g. `$$ make $(MAKECMDGOALS) $(1)=[yes][no]`)

ifndef      LD
$(call missing-opt-err,LD)
else ifndef OPTIM_LVL
$(call missing-opt-err,OPTIM_LVL)
else ifndef DBG_VERB_LVL
$(call missing-opt-err,DBG_VERB_LVL)
else ifndef TEMPS
$(call missing-opt-war-yesno,TEMPS)
else ifndef STRIP
$(call missing-opt-war-yesno,STRIP)
else ifndef LTO
$(call missing-opt-war-yesno,LTO)
else ifndef MAJOR_VER
$(call missing-ver-err,MAJOR_VER)
else ifndef MINOR_VER
$(call missing-ver-err,MINOR_VER)
else ifndef PATCH_VER
$(call missing-ver-err,PATCH_VER)
else ifndef PRE_RELEASE_VER
$(call missing-ver-err,PRE_RELEASE_VER)
endif

VERSION_NUMBER := $(MAJOR_VER).$(MINOR_VER).$(PATCH_VER)-$(PRE_RELEASE_VER)

# ~~~ Make options ~~~
SHELL = /bin/sh
override MAKEFLAGS += --no-builtin-rules

# ~~~ Directories ~~~
include_dir := ./include
build_dir   := ./build
src_dir     := ./src
test_dir    := ./test
dist_dir    := escape-$(VERSION_NUMBER)

# ~~~ Flags ~~~
# On Windows with mingw, gcc doesn't support any form of sanitization
# while clang (distributed with mingw or not) supports address sanitization and UB detection
ifeq ($(OS),Windows_NT)
ifeq ($(CC),clang)
fsan := -fsanitize=address,undefined
endif
else
# On GNU/linux everything is fine of course
fsan := -fsanitize=address,leak,undefined
endif

BASE_FLAGS    := -std=c23 -Wall -Wextra -I$(include_dir)
OBJ_FLAGS     := -MMD -MP -c
DEBUG_FLAGS   := -g$(DBG_VERB_LVL) -O0 -DDEBUG $(fsan)
RELEASE_FLAGS := -O$(OPTIM_LVL) -Werror -DNDEBUG
TEMPS_FLAGS   := $(if $(filter true yes 1,$(TEMPS)),-fverbose-asm -save-temps=obj)
# intermediate files may contain syntax that is not compliant with the C standard
ifeq ($(TEMPS_FLAGS),)
DEBUG_FLAGS   += -pedantic
RELEASE_FLAGS += -pedantic-errors
endif
STRIP_FLAG    := $(if $(filter true yes 1,$(STRIP)),-s)
LTO_FLAG      := $(if $(filter true yes 1,$(LTO)),-flto)
# -fuse-ld=ld doesn't work with gcc, so the flag is expanded to -fuse-ld=ld ONLY if CC is gcc and LD is not ld, or if CC is not gcc
# zig cc also seems to freak out on release test targets. Still don't know why but it will be disabled for now
USE_LD_FLAG   := $(if $(or $(and $(filter gcc,$(CC)),$(filter-out ld,$(LD))),$(filter-out gcc zig cc,$(CC))),-fuse-ld=$(LD))

RELEASE_OBJ_FLAGS := $(OBJ_FLAGS) $(BASE_FLAGS) $(RELEASE_FLAGS)
DEBUG_OBJ_FLAGS   := $(OBJ_FLAGS) $(BASE_FLAGS) $(DEBUG_FLAGS)

# ~~~ Files (sources, objects) ~~~
dist_tarball     := $(dist_dir).tar.xz
sources          := $(wildcard $(src_dir)/*.c)
test_sources     := $(wildcard $(test_dir)/*.c)
dyn_debug_objs   := $(sources:$(src_dir)/%.c=$(build_dir)/dynamic/debug/%.o)
dyn_release_objs := $(sources:$(src_dir)/%.c=$(build_dir)/dynamic/release/%.o)
stc_debug_objs   := $(sources:$(src_dir)/%.c=$(build_dir)/static/debug/%.o)
stc_release_objs := $(sources:$(src_dir)/%.c=$(build_dir)/static/release/%.o)


# Default target (declared as the first rule in the Makefile)
all: libescape.a

# ~~~ "Standard targets" and cleaning rules ~~~
# See https://www.gnu.org/software/make/manual/html_node/Standard-Targets.html
# For the meaning of suffixes, see test targets bellow
cleanlib:;    -rm -f libescape*{.a,.so}
cleantest:;   -rm -f test-*-{sr,sd,dr,dd}
mostlyclean:; -rm -rf $(build_dir)
distclean:;   -rm -rf $(dist_tarball)
clean:;       -rm -rf $(build_dir) libescape*{.a,.so,.i,.s} test-*-{sr,sd,dr,dd}
clean-temps:; -rm -f $(build_dir)/**/**/*{.i,.s,.bc} *{.i,.s,.o,.bc} *.lto_wrapper_args *.ltrans* *.res *.args.0
clean-sr:;    -rm -rf $(build_dir)/static/release libescape.a test-*-sr test-*-sr-*{.i,.s,.o}
clean-sd:;    -rm -rf $(build_dir)/static/debug libescape_g.a test-*-sd test-*-sd-*{.i,.s,.o}
clean-dr:;    -rm -rf $(build_dir)/dynamic/release libescape.so test-*-dr test-*-dr-*{.i,.s,.o}
clean-dd:;    -rm -rf $(build_dir)/dynamic/debug libescape_g.so test-*-dd test-*-dd-*{.i,.s,.o}

.PHONY: clean mostlyclean cleanlib cleantest distclean clean-sr clean-sd clean-dr clean-dd clean-temps

dist: $(dist_tarball)
$(dist_tarball): libescape_g.so libescape.so libescape_g.a libescape.a
	mkdir $(dist_dir)
	ln -s $(addprefix ../, README.md $(notdir $(include_dir)) $^) $(dist_dir)
	ln -s ../LICENSE.md $(dist_dir)/COPYING
	tar -hcJf $(dist_tarball) $(dist_dir)
	rm -rf $(dist_dir)

# ~~~ Library targets ~~~
libescape.a: $(stc_release_objs)
	$(AR) -rcs $@ $?

libescape_g.a: $(stc_debug_objs)
	$(AR) -rcs $@ $?

libescape.so: $(dyn_release_objs)
	$(CC) $(USE_LD_FLAG) $(LTO_FLAG) $(STRIP_FLAG) -shared $^ -o $@

libescape_g.so: $(dyn_debug_objs)
	$(CC) $(USE_LD_FLAG) -shared $^ -o $@

# ~~~ Pattern rules for tests of all build types ~~~
# 1st prefix letter - [s]tatic | [d]ynamic
# 2nd prefix letter - [d]ebug  | [r]elease
test-%-sr: $(test_dir)/%.c libescape.a
	$(CC) $(USE_LD_FLAG) $(BASE_FLAGS) $(RELEASE_FLAGS) $(TEMPS_FLAGS) $(STRIP_FLAG) $(LTO_FLAG) $< -o $@ $(lastword $^)

test-%-sd: $(test_dir)/%.c libescape_g.a
	$(CC) $(USE_LD_FLAG) $(BASE_FLAGS) $(DEBUG_FLAGS) $(TEMPS_FLAGS) $< -o $@ $(lastword $^)

test-%-dr: $(test_dir)/%.c libescape.so
	$(CC) $(USE_LD_FLAG) $(BASE_FLAGS) $(RELEASE_FLAGS) $(TEMPS_FLAGS) $(STRIP_FLAG) -Wl,-rpath=$(lastword $^) ./$(lastword $^) $< -o $@

test-%-dd: $(test_dir)/%.c libescape_g.so
	$(CC) $(USE_LD_FLAG) $(BASE_FLAGS) $(DEBUG_FLAGS) $(TEMPS_FLAGS) -Wl,-rpath=$(lastword $^) ./$(lastword $^) $< -o $@

# ~~~ Pattern rules for running tests of all build types (the most useful of rules in this Makefile) ~~~
run-%-sr: test-%-sr; ./$< $(arg)
run-%-sd: test-%-sd; ./$< $(arg)
run-%-dr: test-%-dr; ./$< $(arg)
run-%-dd: test-%-dd; ./$< $(arg)

.PRECIOUS: test-%-sr test-%-sd test-%-dr test-%-dd

# ~~~ "every" rules to test all that can be built ~~~
every-%-test: $(addsuffix -%,$(addprefix test-,$(basename $(notdir $(test_sources)))));
everything: every-sr-test every-sd-test every-dr-test every-dd-test;

# ~~~ Directory targets ~~~
$(build_dir)/static/release $(build_dir)/dynamic/release $(build_dir)/static/debug $(build_dir)/dynamic/debug:
	mkdir -p "$@"

# ~~~ Pattern rules for objects/asm outputs of all build types ~~~
$(build_dir)/dynamic/debug/%.o: $(src_dir)/%.c | $(build_dir)/dynamic/debug
	$(CC) $(TEMPS_FLAGS) $(DEBUG_OBJ_FLAGS) -fPIC $< -o $@

$(build_dir)/dynamic/release/%.o: $(src_dir)/%.c | $(build_dir)/dynamic/release
	$(CC) $(TEMPS_FLAGS) $(RELEASE_OBJ_FLAGS) -fPIC $< -o $@

$(build_dir)/static/debug/%.o: $(src_dir)/%.c | $(build_dir)/static/debug
	$(CC) $(TEMPS_FLAGS) $(DEBUG_OBJ_FLAGS) $< -o $@

$(build_dir)/static/release/%.o: $(src_dir)/%.c | $(build_dir)/static/release
	$(CC) $(TEMPS_FLAGS) $(RELEASE_OBJ_FLAGS) $(LTO_FLAG) $< -o $@

# ~~~ Specific object targets based inlcude directives (generated by $(CC) where the objects live) ~~~
-include $(dyn_debug_objs:.o=.d)
-include $(dyn_release_objs:.o=.d)
-include $(stc_debug_objs:.o=.d)
-include $(stc_release_objs:.o=.d)

