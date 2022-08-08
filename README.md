# Lightbox Project

Lightbox is for a custom lightbox system with a camera.
It's a research project and under early active hacking.
More details TBA.  

[You can see what I'm likely working on here](https://github.com/users/devellison/projects/1).

It doesn't do much yet - base firmware in `./firmware` and the start of the camera
library in `./zebral/camera` are the most developed.

## NOTE: Zebral has been split to its own repo.
https://github.com/devellison/zebral
## Firmware is its own submodule too:
https://github.com/devellison/zebral_esp32cam

It is now used as a submodule, so when cloning:
 - If you've cloned lightbox, you'll need to update the submodules like:<br>
   ```git submodule update --init --recursive```
 - If you're cloning lightbox, add the recursive switch:<br>
   ```git clone --recursive https://github.com/devellison/lightbox.git```

To update Zebral (it's a work in progress):
 - `git submodule update --remote`

## Firmware
I've switched from using an RP2040 Feather for LED control to using an ESP32-CAM with a little additional hardware.
Code for the firmware is in firmware/zebral_esp32cam, some basic schematics are in there as well.
It provides a web interface to camera, gpio, and leds.  Very much work in progress there.

## Building
Currently developing it with:
- C++20, Clang-12, MSVC 2019 (2022 for CI)
- 64-bit Windows 11 / C++/WinRT and Ubuntu 22.04 (20.04 for CI)/ V4L2
- [Adafruit Feather RP2040](https://learn.adafruit.com/adafruit-feather-rp2040-pico/circuitpython)
- [Wiki has more hardware details](https://github.com/devellison/lightbox/wiki)

- Install dependencies:
  - As it's actively being developed, might check the [workflows](https://github.com/devellison/lightbox/tree/main/.github/workflows) for latest requirements for building.
  - Windows:
    - Visual Studio 2019 (CMake 3.20+)  (Others may work - 2022 in CI)
    - Set up OpenCV 4.5.5, including videoio, highgui, and core 
      (current binary release didn't seem to have these, but choco did)
    - Doxygen, graphviz, cmake-format (pip install), clang-format
  - Linux:
    - Currently using Clang-12 (CMake 3.23)
    - Using OpenCV 4.2 from Ubuntu's distribution
    - libfmt (on Linux) should be built and installed
    - Google test is downloaded by CMake currently
  - Build tools and deps for both:    
    - CMake v3.20+ (you can get it from kitware for ubuntu)
    - OpenMP (optional, not using it much yet)
    - Doxygen, graphviz, cpack, cmake-format (pip install), clang-format
    - Python 3.10.5+ and PyBind11 for python bindings
    - Numpy and OpenCV-python (pip installs) are required for the python tests
- Set up hardware (Web Camera, Feather RP2040, many many LEDs, relay boards, TBD)
    - See the Wiki, will doc wiring/etc. as it gets more solidified.
- Install CircuitPython to Adafruit Feather RP2040
- Copy `code.py` and `lightbar.py` from the `firmware` directory to Adafruit Feather RP2040's root directory
- Configure and compile app using normal CMake commands, or just run the scripts from the base directory.
  - `build_win.bat` for Windows
  - `./build_linux.sh` for Linux
- Run it!
  - Windows: `build\app\Release\lightbox_app.exe`
  - Linux: `build/app/lightbox_app`
