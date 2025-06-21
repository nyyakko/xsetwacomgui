# XSetWacomGUI

This is a frontend for the `xsetwacom` utility.

![image](https://github.com/user-attachments/assets/911f735e-d48d-4105-b63b-e0baadd2b07e)

# Building

## Dependencies

Before building, make sure you have the following depencies installed on your system:

* opengl development package
* glfw development package
* xrandr
* xsetwacom

You will also need a [C++23 compiler](https://github.com/llvm/llvm-project/releases) and [cmake](https://cmake.org/) installed.

```bash
python configure.py && python build.py
```

to build and install the release version,

```bash
python configure.py release && python build.py
python install.py
```

## Documentation

For cli documentation, read the docs available at the [documentation](documentation/) folder.
