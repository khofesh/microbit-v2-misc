# microbit-v2-samples

[![Native Build Status](https://github.com/lancaster-university/microbit-v2-samples/actions/workflows/build.yml/badge.svg)](https://github.com/lancaster-university/microbit-v2-samples/actions/workflows/build.yml) [![Docker Build Status](https://github.com/lancaster-university/microbit-v2-samples/actions/workflows/docker-image.yml/badge.svg)](https://github.com/lancaster-university/microbit-v2-samples/actions/workflows/docker-image.yml)

This repository provides the necessary tooling to compile a C/C++ CODAL program for the micro:bit V2 and generate a HEX file that can be downloaded to the device.

## Raising Issues
Any issues regarding the micro:bit are gathered on the [lancaster-university/codal-microbit-v2](https://github.com/lancaster-university/codal-microbit-v2) repository. Please raise yours there too.

# Installation
You need some open source pre-requisites to build this repo. You can either install these tools yourself, or use the docker image provided below.

- [GNU Arm Embedded Toolchain](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads)
- [Git](https://git-scm.com)
- [CMake](https://cmake.org/download/)
- [Python 3](https://www.python.org/downloads/)

We use Ubuntu Linux for most of our tests. You can also install these tools easily through the package manager:

```
    sudo apt install gcc
    sudo apt install git
    sudo apt install cmake
    sudo apt install gcc-arm-none-eabi binutils-arm-none-eabi
```

## Yotta
For backwards compatibility with [microbit-samples](https://github.com/lancaster-university/microbit-samples) users, we also provide a yotta target for this repository.

## Docker
You can use the [Dockerfile](https://github.com/lancaster-university/microbit-v2-samples/blob/master/Dockerfile) provided to build the samples, or your own project sources, without installing additional dependencies.

Run the following command to build the image locally; the .bin and .hex files from a successful compile will be placed in a new `out/` directory:

```
    docker build -t microbit-tools --output out .
```

To omit the final output stage (for CI, for example) run without the `--output` arguments:

```
    docker build -t microbit-tools .
```

# CODAL libraries as git submodules
In this repo, the CODAL libraries in `libraries/` are tracked as git submodules. `build.py` doesn't download them on the fly. The `libraries` line in `.gitignore` is commented out so git can see them.

| Path | Repository |
|------|------------|
| `libraries/codal-microbit-v2` | https://github.com/lancaster-university/codal-microbit-v2 |
| `libraries/codal-core` | https://github.com/lancaster-university/codal-core |
| `libraries/codal-nrf52` | https://github.com/lancaster-university/codal-nrf52 (has a nested `nrfx` submodule) |
| `libraries/codal-microbit-nrf5sdk` | https://github.com/microbit-foundation/codal-microbit-nrf5sdk |

Clone with submodules:

```
    git clone --recurse-submodules <repo-url>
    # or, after a plain clone:
    git submodule update --init --recursive
```

How they were added (note: `--recursive` is not an option of `git submodule add`, and each submodule needs its own folder inside `libraries/`):

```
    git submodule add https://github.com/lancaster-university/codal-microbit-v2 libraries/codal-microbit-v2
    git submodule add https://github.com/lancaster-university/codal-core libraries/codal-core
    git submodule add https://github.com/lancaster-university/codal-nrf52 libraries/codal-nrf52
    git submodule add https://github.com/microbit-foundation/codal-microbit-nrf5sdk libraries/codal-microbit-nrf5sdk
    git submodule update --init --recursive
```

To update one, or pin it to a tag:

```
    cd libraries/codal-microbit-v2 && git fetch && git checkout <tag-or-commit> && cd ../..
    git add libraries/codal-microbit-v2 && git commit -m "Bump codal-microbit-v2"
```

> `build.py` may check out the versions listed in `target.json` (under `libraries/codal-microbit-v2`) for these folders. Keep the submodule commits in line with that file, or build with CMake directly.

# Building
- Clone this repository (see the submodule section above)
- In the root of this repository type `python build.py`
- The hex file will be built `MICROBIT.hex` and placed in the root folder.

# Flashing the board
When you plug in the micro:bit V2 with a USB **data** cable, it shows up as a drive called `MICROBIT`. To flash it, copy the `.hex` file onto that drive.

### Copy to the drive (the usual way)
On Fedora the drive is usually mounted at `/run/media/$USER/MICROBIT`:

```
    cp MICROBIT.hex /run/media/$USER/MICROBIT/ && sync
```

The yellow LED on the back flashes while it copies. Then the drive unmounts, comes back, and your program runs.

If the drive isn't auto-mounted:

```
    lsblk -o NAME,LABEL,MOUNTPOINT | grep -i microbit
    udisksctl mount -b /dev/sdX     # use the device shown above
```

Build and flash in one line:

```
    python build.py && cp MICROBIT.hex /run/media/$USER/MICROBIT/ && sync
```

### pyOCD (no drive needed; also works for debugging)

```
    pip install pyocd
    pyocd flash -t nrf52833 MICROBIT.hex
```

On Linux you may need udev rules to run it without root (see the pyOCD docs).

### If it doesn't work
- **A `FAIL.TXT` file appears on the drive:** the hex is bad or is for the wrong board. Rebuild and make sure it's a V2 build.
- **The board isn't detected:** try another cable or port, and update the DAPLink firmware. To do that, hold the reset button while plugging in; a `MAINTENANCE` drive appears, and you copy the latest firmware from microbit.org onto it.
- **Your program doesn't start:** press the reset button on the back.

# Developing
You will find a simple main.cpp in the `source` folder which you can edit. CODAL will also compile any other C/C++ header files our source files with the extension `.h .c .cpp` it finds in the source folder.

The `samples` folder contains a number of simple sample programs that utilise you may find useful.

## Developer codal.json

There is an example `coda.dev.json` file which enables "developer builds" (clones dependencies from the latest commits, instead of the commits locked in the `codal-microbit-v2` tag), and adds extra CODAL flags that enable debug data to be printed to serial.
To use it, simply copy the additional json entries into your `codal.json` file, or you can replace the file completely (`mv coda.dev.json codal.json`).

# Debugging
If you are using Visual Studio Code, there is a working debugging environment already set up for you, allowing you to set breakpoints and observe the micro:bit's memory. To get it working, follow these steps:

1. Install either [OpenOCD](http://openocd.org) or [PyOCD](https://github.com/pyocd/pyOCD).
2. Install the [`marus25.cortex-debug` VS Code extension](https://marketplace.visualstudio.com/items?itemName=marus25.cortex-debug).
3. Build your program.
4. Click the Run and Debug option in the toolbar.
5. Two debugging options are provided: one for OpenOCD, and one for PyOCD. Select the correct one depending on the debugger you installed.

This should launch the debugging environment for you. To set breakpoints, you can click to the left of the line number of where you want to stop.

# Compatibility
This repository is designed to follow the principles and APIs developed for the first version of the micro:bit. We have also included a compatibility layer so that the vast majority of C/C++ programs built using [microbit-dal](https://www.github.com/lancaster-university/microbit-dal) will operate with few changes.

# Documentation
API documentation is embedded in the code using doxygen. We will produce integrated web-based documentation soon.
