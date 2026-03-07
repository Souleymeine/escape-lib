This project uses semantic versioning : https://semver.org/

# NOT READY FOR USE. THERE IS ABSOLUTELY NO WARRANTY.

## Developpement scheme
The project will use the `feature` / `bugfix` / `test` convetion for branch names but adds another one called "goal".
Branch named `goal/something` will contain a few commits setting up a test file which will do something that should be easily achievable using the library but that is not yet implemented. Quick example :
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
Such a test could be written in a `goal/style-handling` branch so that other branches, let's say `feature/color-handling` and `feature/style-handling` can be open after the "goal" branch has been merged. This will help focus developpement into the implementation of actual features instead of trying to implement everything without a clear direction.

The above example could then become:
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
(based on actual code/projections of the current state of the library)
The result of these two is the same, whith the latter generating a smaller or equally long string.

In the next branches.
(Of course the latter is horrible but you get the point).

## Some naming conventions :
**cp** stands for **c**ode**p**oint

**gphm** stands for **g**ra**ph**e**m**e

These two are ferequent in the library.
They respectively refer to the smallest bit of information in a utf8 string (codepoint, a single byte) and a group of multiple codepoints which make a visual character up (grapheme): what we would *naturaly* refer to as a "character".

## TODO
**This list is not exaustive and is subject to impeding change. It also does not include "*goals*" (defined above)**

- [ ] Use zig as a build system rather than make (painful, painful indeed after so many hours writing a Makefile, but necessary). [See video](https://youtu.be/i9nFvSpcCzo?si=yxOfo1hWYExjidIT).
    *"Replacing your dependency on (C)make by zig, and now this new dependency also cross compiles"*, **marvelous**.
- [ ] Cross-platform terminal-independant escape sequences management : [replacement for termcaps/terminfo](https://lobste.rs/s/m1j4b4/terminfo_at_this_point_time_is_net)
- [ ] Asbtraction layer above device/kernel specific interaction mechanism with stdin/stdout and the console/tty itself (for instance, a crossplatform `getch`)
- [ ] IO events and resizing.
- [ ] Find the best algorithm for `srefresh` (non-heuristic if possible)
- [ ] "Immediate" mode, as opposed to retained mode when using `TERM_ALTBUF`
- [ ] UI layout (as en extension)
- [ ] Image processing (as an extension)
- [ ] Bindings for [clay](https://github.com/nicbarker/clay/tree/main/renderers/terminal) and implementation
- [x] Limit the usage of the standard library to bit utilities (stdbit.h) and system calls (no C runtime, like malloc, printf, fopen, ...)
- [ ] Better support for CJK characters, mostly full width characters
- [ ] Support for combining diacritical marks, mostly arabic but also phonetics
- [ ] Account for other terminal emulators on Windows other than *conhost* and *Terminal*
- [ ] Support WASM for [Xterm.js](https://xtermjs.org/)
- [ ] Support OpenGL/Vulkan surfaces
- [ ] Parallel screen rendering?
- [ ] 24bit color driver for Linux/BSD's tty?
- [ ] Try compiling and running with [Fil-C](https://fil-c.org/)
## After version 1.0.0 (stable)
Bindings for
- [ ] C++
- [ ] Python
- [ ] Zig
- [ ] Rust
- [ ] OCaml
- [ ] Java
- [ ] C#
