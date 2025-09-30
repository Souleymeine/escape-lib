# ~~~ Configurable options (overrideable from cmdline) ~~~
CONFIG = build.default.cfg
ifneq ($(file < build.cfg),)
CONFIG = build.cfg
endif

include $(CONFIG)

override missing-opt-err = $(error Please specify "$(1)" in $(CONFIG) or from the commandline, e.g. `$$ make $(MAKECMDGOALS) $(1)=...`)
override missing-ver-err = $(error Missing $(1) number in $(CONFIG))

# Checking for missing options
ifndef OPTIM_LVL
$(call missing-opt-err,OPTIM_LVL)
endif
ifndef DEBUG_VERB_LVL
$(call missing-opt-err,DEBUG_VERB_LVL)
endif
ifndef STRIP
$(call missing-opt-err,STRIP)
endif
ifndef LTO
$(call missing-opt-err,LTO)
endif

ifndef MAJOR_VER
$(call missing-ver-err,MAJOR_VER)
endif
ifndef MINOR_VER
$(call missing-ver-err,MINOR_VER)
endif
ifndef PATCH_VER
$(call missing-ver-err,PATCH_VER)
endif
ifndef PRE_RELEASE_VER
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

OBJ_FLAGS     := -MMD -MP -c $(CFLAGS)
BASE_FLAGS    := -std=c23 -Wall -Wextra -I$(include_dir)
DEBUG_FLAGS   := -g$(DEBUG_VERB_LVL) -O0 -DDEBUG $(fsan)
RELEASE_FLAGS := -O$(OPTIM_LVL) -Werror -DNDEBUG
STRIP_FLAG    := $(if $(filter true yes 1,$(STRIP)),-s)
LTO_FLAG      := $(if $(filter true yes 1,$(LTO)),-flto)

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


# ~~~ "Standard targets", mostly cleaning rules ~~~
# See https://www.gnu.org/software/make/manual/html_node/Standard-Targets.html
clean:
	-rm -rf $(build_dir) libescape*{.a,.so} test-*-{sr,sd,dr,dd}
mostlyclean:
	-rm -rf $(build_dir)
cleanlib:
	-rm -f libescape*{.a,.so}
cleantest:
	-rm -f test-*-{sr,sd,dr,dd}

distclean:
	-rm -rf $(dist_tarball)

dist: $(dist_tarball)
$(dist_tarball): libescape_g.so libescape.so libescape_g.a libescape.a
	mkdir $(dist_dir)
	ln -s $(addprefix ../, README.md $(notdir $(include_dir)) $^) $(dist_dir)
	ln -s ../LICENSE.md $(dist_dir)/COPYING
	tar -hcJf $(dist_tarball) $(dist_dir)
	rm -rf $(dist_dir)

# For the meaning of suffixes, see test targets bellow
clean-sr:
	-rm -rf $(build_dir)/static/release libescape.a
clean-sd:
	-rm -rf $(build_dir)/static/debug libescape_g.a
clean-dr:
	-rm -rf $(build_dir)/dynamic/release libescape.so
clean-dd:
	-rm -rf $(build_dir)/dynamic/debug libescape_g.so

# ~~~ Library targets ~~~
libescape_g.so: $(dyn_debug_objs)
	$(CC) -shared $^ -o $@

libescape.so: $(dyn_release_objs)
	$(CC) $(LTO_FLAG) $(STRIP_FLAG) -shared $^ -o $@

libescape_g.a: $(stc_debug_objs)
	$(AR) -rcs $@ $?

libescape.a: $(stc_release_objs)
	$(AR) -rcs $@ $?

# ~~~ Pattern rules for tests of all build types ~~~
# 1st prefix letter - [s]tatic | [d]ynamic
# 2nd prefix letter - [d]ebug  | [r]elease
test-%-sr: $(test_dir)/%.c libescape.a
	$(CC) $(BASE_FLAGS) $(LTO_FLAG) $(RELEASE_FLAGS) $(STRIP_FLAG) $< -o $@ $(lastword $^)

test-%-sd: $(test_dir)/%.c libescape_g.a
	$(CC) $(BASE_FLAGS) $(DEBUG_FLAGS) $< -o $@ $(lastword $^)

test-%-dr: $(test_dir)/%.c libescape.so
	$(CC) $(BASE_FLAGS) $(RELEASE_FLAGS) $(STRIP_FLAG) -Wl,-rpath=$(lastword $^) ./$(lastword $^) $< -o $@

test-%-dd: $(test_dir)/%.c libescape_g.so
	$(CC) $(BASE_FLAGS) $(DEBUG_FLAGS) -Wl,-rpath=$(lastword $^) ./$(lastword $^) $< -o $@

# ~~~ Pattern rules for running tests of all build types (the most useful of rules in this Makefile) ~~~
run-%-sr: test-%-sr
	./$<
run-%-sd: test-%-sd
	./$<
run-%-dr: test-%-dr
	./$<
run-%-dd: test-%-dd
	./$<

.PHONY: clean mostlyclean cleanlib cleantest distclean clean-sr clean-sd clean-dr clean-dd
.PRECIOUS: test-%-sr test-%-sd test-%-dr test-%-dd

# ~~~ "every" rules to test all that can be built ~~~
every-%-test: $(addsuffix -%,$(addprefix test-,$(basename $(notdir $(test_sources)))));
everything: every-sr-test every-sd-test every-dr-test every-dd-test

# ~~~ Directory targets ~~~
$(build_dir)/static/debug $(build_dir)/static/release $(build_dir)/dynamic/debug $(build_dir)/dynamic/release:
	mkdir -p "$@"

# ~~~ Pattern rules for objects of all build types ~~~
$(build_dir)/dynamic/debug/%.o: $(src_dir)/%.c | $(build_dir)/dynamic/debug
	$(CC) $(OBJ_FLAGS) $(BASE_FLAGS) $(DEBUG_FLAGS) -fPIC $< -o $@

$(build_dir)/dynamic/release/%.o: $(src_dir)/%.c | $(build_dir)/dynamic/release
	$(CC) $(OBJ_FLAGS) $(BASE_FLAGS) $(LTO_FLAG) $(RELEASE_FLAGS) -fPIC $< -o $@

$(build_dir)/static/debug/%.o: $(src_dir)/%.c | $(build_dir)/static/debug
	$(CC) $(OBJ_FLAGS) $(BASE_FLAGS) $(DEBUG_FLAGS) $< -o $@

$(build_dir)/static/release/%.o: $(src_dir)/%.c | $(build_dir)/static/release
	$(CC) $(OBJ_FLAGS) $(BASE_FLAGS) $(LTO_FLAG) $(RELEASE_FLAGS) $< -o $@

# ~~~ Specific object targets based inlcude directives (generated by $(CC) where the objects live) ~~~
-include $(dyn_debug_objs:.o=.d)
-include $(dyn_release_objs:.o=.d)
-include $(stc_debug_objs:.o=.d)
-include $(stc_release_objs:.o=.d)

