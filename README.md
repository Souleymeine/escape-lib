# NOT READY FOR USE. THERE IS ABSOLUTELY NO WARRANTY.

TODO : Describe project and its motivations


This project uses [**semantic versioning**](https://semver.org/), enforced by `build.zig` ([more info about the Zig build system](https://ziglang.org/learn/build-system/))

## Developpement scheme
This project's branches names match the following regular expression : `(feature/|rework/|bugfix/|goal/)[a-zA-Z][a-zA-Z0-9]*`.
Depending on the category, the branch
- **goal**    : fully implements a draft for a desired feature or use case with existing library code, with the goal of re-implementing said draft with a future **feature** branches. A single **goal** branch may have multiple follow up **feature** branches, or none, if the draft is deemed acceptable as-is and thus only adds one example in `test/`.
- **feature** : adds a new feature, incrementing the minor or the major version if it includes breaking changes.
- **bugfix**  : fixes a bug, incrementing the patch version.
- **rework**  : reviews how an already implemented feature works or deletes one.
- **meta**    : reworks or adds files for the project's infrastructure (CI, build system, tooling, etc.)

The merge history should look like a repetition of these steps.

The goal of this developpement scheme is to keep a clear path to a stable and simple API, for both me, the original author (Souleymeine) and other contributors in the long run (I would be sincerely honored if there are any).

A hypothetical example of `goal` -> `feature` branches:

- ***Added by `goal/text-style-handling`***
```C
void cool_test()
{
    printf(
        "\x1b[?1049h\x1b[H"
        "\x1b[3mThe excitement of creating something new and hopefully useful flows down your veins\n"
        "as you prepare for the very first commit.\x1b[23m\n\n\n\n"
        "\x1b[1;33m		You are filled with determination.\x1b[m");
    getchar();
    printf("\x1b[?1049l");
}
```

- ***Added by `feature/color-handling`, `feature/style-handling`, `feature/state-and-input-handling` after `goal/text-style-handling` has been merged***
```C
void cool_test()
{
    init_flags(TERM_NO_ECHO | TERM_HIDE_CURSOR | TERM_ALTBUF);

    addstr("The excitement of creating something new and hopefully useful flows down your veins as you prepare for the very first commit.",
           0, 0, STYLE_ITALIC, OFFSCR_WRAPPED);
    addstr("You are filled with determination.",
           5, 8, CLR_CODE(CLRCODE_RED), STYLE_BOLD | STYLE_UNDERLINE, OFFSCR_CUT);
    refresh();

    waitfor(KEY_RETURN);
    cleanup_term();
}
```
The result of both is the same, but the second is at least more convinient, more efficient (introduces less latency by generating the smallest string possible for the requested output) and is more comprehensive.

## Some useful naming conventions and constructs
I hate when I try to understand an open source project's source code and I'm greeted with unspoken, unseen, baffling gibberish.
This library aims to be as readable as possible without being too verbose, C is a [spartan language](https://www.kernel.org/doc/html/v4.10/process/coding-style.html#naming) after all. However, some constructs/naming conentions are also welcome, as long they are few and well defined.
The only things required to understand this library's source code are the following:
- The C programming language in its C23 standard (last stable standard at the time of this commit).
- These naming conventions you'll come across quite often:
    - `esc` is short for **esc**ape, the library's name, which is a funny abbreviation for *escape sequence*, because that's what the library deals with.
    - [`cu`](https://developer.mozilla.org/docs/Glossary/Code_unit) refers to a UTF-8 for **c**ode **u**nit
    - [`cp`](https://developer.mozilla.org/docs/Glossary/Code_point) refers to a Unicode **c**ode**p**oint
- These macros:
    - `ESC_ERRUNION(T)` expands to a tagged union (struct containing union + which value is active) to declare functions that can return an error (of some enum type defined as such) or some value of type `T`.


## TODO
**This list is not exaustive and is subject to impeding change. It also does not include "*goals*" (defined above)**

- [x] Use zig as a build system rather than make (painful, painful indeed after so many hours writing a Makefile, but necessary). [See video](https://youtu.be/i9nFvSpcCzo?si=yxOfo1hWYExjidIT).
    *"Replacing your dependency on (C)make by zig, and now this new dependency also cross compiles"*, **marvelous**.
- [ ] Cross-platform terminal-independant escape sequences management : [replacement for termcaps/terminfo](https://lobste.rs/s/m1j4b4/terminfo_at_this_point_time_is_net)
- [ ] Asbtraction layer above device/kernel specific interaction mechanism with stdin/stdout and the console/tty itself (for instance, a crossplatform `getch`)
- [ ] IO events and resizing.
- [ ] Find the best algorithm for `srefresh` (non-heuristic if possible)
- [ ] "Immediate" mode, as opposed to retained mode when using `TERM_ALTBUF`
- [ ] UI layout (as en extension)
- [ ] Image processing (as an extension)
- [ ] Make screen a unique and global data structure, no need to have multiple
- [ ] Bindings for [clay](https://github.com/nicbarker/clay/tree/main/renderers/terminal) and implementation
- [x] Limit the usage of the standard library to bit utilities (stdbit.h) and system calls (no C runtime, like malloc, printf, fopen, ...)
- [ ] statically link against musl or homemade syscall lib to only get syscalls and possibly inline them in libescape with LTO and get rid of glibc entirely
- [ ] Better support for CJK characters, mostly full width characters
- [ ] Support for combining diacritical marks, mostly arabic but also phonetics
- [ ] Account for other terminal emulators on Windows other than *conhost* and *Terminal*
- [ ] Support WASM for [Xterm.js](https://xtermjs.org/)
- [ ] Support OpenGL/Vulkan surfaces
- [ ] Parallel screen rendering?
- [ ] 24bit color driver for Linux/BSD's tty?
- [ ] Try compiling and running with [Fil-C](https://fil-c.org/)
## After version 1.0.0 (stable)
- [ ] API for grapheme clusters in core
Bindings for
- [ ] Zig
- [ ] C++
- [ ] Python
- [ ] Rust
- [ ] OCaml
- [ ] Java
- [ ] C#
