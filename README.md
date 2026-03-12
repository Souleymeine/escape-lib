# NOT READY FOR USE. THERE IS ABSOLUTELY NO WARRANTY.

TODO : rewrite all that

*libescape* is a cross platform C library that aims at manipulating terminals in the most efficient way possible, making graphics inside terminals possible and facilitating IO, TUIs and CLI tools in general, while being as efficient as possible in terms of speed, memory footprint, latency and binary size.
*libescape* is also a replacement for termcaps/terminfo (and all the tools that build on top of it, such as tput) as well as a backend for any rendering, whether that's for 3D applications using modern graphics APIs, games, or for UI layout libraries such as [Clay](https://www.nicbarker.com/clay).

The rationale for libescape could be summerized as such :

**Produce the shortest string that represents the declared state provided by the user, as fast as possible, on any terminal or terminal emulator, on any platform ; considering the user must be able to do everything that their terminal *can* do.**


The name "escape" refers to [*escape sequences*](https://en.wikipedia.org/wiki/Escape_sequence), which is what it mainly deals with to do anything else than line-by-line plain text. Using those, you can write the string returned by the library to `stdout` and see what you declared in your code. You could also write it to any file so you could store advanced ascii art in your filesystem for example!

It is an alternative to other CLI libraries and does not claim to compete with any (...yet! except [ncurses](https://invisible-island.net/ncurses/ncurses.html) maybe) and which tries to offer a solution the lack of standard in terminals and terminal emulators without forcing any paradigm on the user nor forcing them to wait for some maintainer to update a termcaps database once every decade.

*libescape* is separated into multiple sub-libraries :
- esc-core, terminal capabilities/info detection, low level terminal control and escape sequence management
- esc-io
- esc-rndr, the rendering backend
- esc-2d, depends on esc-rndr, provides a way to do TUIs as well as any 2d graphics
- esc-img, depends on esc-rndr, the in-terminal image rendering (whether that's using dedicated protocols or regular color escape sequences)
- esc-3d, depends on esc-img, provides a thin communication layer with Vulkan, OpenGL and SDL3's GPU API as well as a simple 3D software renderer

TODO : provide a graph to visualize internal dependecies in a few seconds instead of a wall of text

All those sub-libraries depend on esc-core.
This clear separation allows users to embed libescape into their projects without importing any bloat, although you could argue statically linking with LTO should be enough or even better in that regard.

It mainly helps myself (the original creator of the project) to not add any superfluous feature to what's strictly necessary for a certain use-case, whilst still allowing myself to build on top of it. That separation also ensures a certain quality in design as integrating with core, whether that's for sub-libraries or any 3rd-party code, must is as easy and *overhead-free* as possible.
The default `libescape.(dll|so|a|lib)` is an amalgamation of all the listed sub-libraries for convinience.

This project uses [**semantic versioning**](https://semver.org/), enforced by `build.zig` ([more info about the Zig build system](https://ziglang.org/learn/build-system/))

## Build from source
**For any OS on any architecture : `zig build [options]`**
That's it!

The only dependency of this project is [Zig](https://ziglang.org/), both as a programming language (see `build.zig`) and as a toolchain.
To compile the project from source, install Zig globally on your system and refer to `zig build -h` for usage.
If you prefer using a local installation of Zig, this is also possible : [Download the latest version of Zig](https://ziglang.org/download/), unpack the archive and place it under the project's root directory as `zig-bin`. You can now do `zig-bin/zig build -h`!
You can compile to and from any supported platform, that is every major OS, every major ABI and every major ISA, listed in `zig targets`.
One goal of libescape is to be as cross-platform as possible.


**HOWEVER : This project uses the C23 standard of the C programming language, which isn't widely supported at the time of this commit**.

As Zig only bundles libc implementations, **the ability to cross compile from anywhere to anything doesn't prevent compiler errors due to said libc implementations not being up to date**. We can however, differentiate compiler errors that are caused by libc implementations not being up to date with the latest standard and the library's source code not covering a certain libc implementation regardless of the version.

**If you get any compiler error other than something like**
- `unknown type name some_C23_type`
- `'<some_C23_header.h>' file not found`
- `unkown symbol C23_function` (that lives in `'<some_header_C23_(re)defined.h>'`)
**then it is a bug and *it needs to be fixed*.**


Also note that Zig is still not stable, bugs during build unrelated to the issues presented above *might* happen.

## Developpement scheme
This project's branches names match the following regular expression : `(feature/|rework/|bugfix/|goal/)[a-zA-Z][-a-zA-Z0-9]*`.

Before naming a branch, it's name can be tested with `$ echo "branch_name" | grep -E "[insert the regexp above]"`.


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
- [ ] API for grapheme clusters
Bindings for
- [ ] Zig
- [ ] C++
- [ ] Python
- [ ] Rust
- [ ] OCaml
- [ ] Java
- [ ] C#
