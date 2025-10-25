This project uses semantic versioning : https://semver.org/

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

The above example could then become :
```C
void cool_test()
{
    altbuf_visbile(true);
    cursor_visible(false);

    print_styled_wrapped("The excitement of creating something new and hopefully useful flows down your veins as you prepare for the very first commit.", 85, ITALIC);
    move_relative(5, 8);
    print_styled("You are filled with determination.", RED, BOLD);

    getchar();
    altbuf_visbile(false);
    cursor_visible(true);
}
```

In the next branches.
(Of course the latter is horrible but you get the point).

## Some naming conventions :
**cp** stands for **c**ode**p**oint
**gphm** stands for **g**ra**ph**e**m**e

Those two are ferequent in the library.
They respectively refer to the smallest bit of information in a utf8 string (codepoint) and a group of multiple codepoints that make visual character up (grapheme)

## TODO
**This list is not exaustive and is subject to impeding change. It also does not include "*goals*" (defined above)**

- [ ] Use zig as a build system rather than make (painful, painful indeed after so many hours writing a Makefile, but necessary).
    See https://youtu.be/i9nFvSpcCzo?si=yxOfo1hWYExjidIT.
    "Replacing your dependency on make by zig, and now this new dependency also cross compiles", **marvelous**.
- [ ] Binding for [clay](https://github.com/nicbarker/clay/tree/main/renderers/terminal)
- [ ] Limit the usage of the standard library to bit utilities (stdbit.h) and system calls (no C runtime, like malloc, printf, fopen, ...)
- [ ] Better support for CJK characters, mostly full width characters
- [ ] Support for combining diacritical marks, mostly arabic but also phonetics
- [ ] Account for other terminal emulators on Windows other than *conhost* and *Terminal*
