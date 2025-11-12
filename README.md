# XSetWacomGUI

A graphical `xsetwacom` wrapper for ease of use.

<img width="806" height="839" alt="image" src="https://github.com/user-attachments/assets/0432fbae-9017-4452-954f-16d59d31aa50" />

# Building

> [!WARNING]
> Unless you're willing to contribute, make sure to always build from the `stable` branch!

## Dependencies

Before building, make sure you have the following depencies installed on your system:

* freetype development package
* udev development package
* opengl development package

and, of course, a X11 installation with `xsetwacom` available.

You will also need a [C++23 compiler](https://github.com/llvm/llvm-project/releases) and [cmake](https://cmake.org/) installed.

```bash
python configure.py && python build.py
```

to build and install the release version,

```bash
python configure.py release && python build.py
python install.py
```

# Documentation

For cli documentation, read the docs available at the [documentation](documentation/) folder.

# Contributing

Just try to follow the general styling of the code and make sure to work in the `devel` branch. For submitting translations, check the [languages](resources/languages/) folder.
