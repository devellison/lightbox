Lightbox is for a custom lightbox system with a camera.
It's a research project and under early active hacking.
More details TBA.  

[You can see what I'm likely working on here](https://github.com/users/devellison/projects/1).

It doesn't do much yet - base firmware in `./firmware` and the start of the camera
library in `./camera` are all that's there.

Firmware responds to serial line commands or a button. "help" for help.
Camera can capture frames via WinRT on Windows, but it's still using OpenCV
on Linux (to be replaced - most like w/V4L2 but TBD).

Currently developing it with:
- C++20, Clang-12, MSVC 2019 (2022 for CI)
- Windows 11 / C++/WinRT and Ubuntu 20.04 / V4L2
- [Adafruit Feather RP2040](https://learn.adafruit.com/adafruit-feather-rp2040-pico/circuitpython)
- [Wiki has more hardware details](https://github.com/devellison/lightbox/wiki)

The Zebral Camera Library is being developed while working on the lightbox,
and can be built as part of the lightbox project or separately - look in
./camera, or in the ./lib and ./include directories of an install package.

- Install dependencies:
  - As it's actively being developed, might check the [workflows](https://github.com/devellison/lightbox/tree/main/.github/workflows) for latest requirements for building.
  - Windows:
    - Visual Studio 2019 (CMake 3.20)  (Others may work - 2022 in CI)
    - Set up OpenCV 4.5.5, including videoio, highgui, and core 
      (current binary release didn't seem to have these, but choco did)
    - Doxygen, graphviz, cmake-format (pip install), clang-format
  - Linux:
    - Currently using Clang-12 (CMake 3.23)
    - Using OpenCV 4.2 from Ubuntu's distribution
    - libfmt (on Linux) should be built and installed
    - Google test is downloaded by the cmake currently
  - Build tools and deps for both:    
    - CMake v3.20+ (you can get it from kitware for ubuntu)
    - OpenMP (optional, not using it much yet)
    - Doxygen, graphviz, cpack, cmake-format (pip install), clang-format
- Set up hardware (Web Camera, Feather RP2040, many many LEDs, relay boards, TBD)
    - See the Wiki, will doc wiring/etc. over time.
- Install CircuitPython to Adafruit Feather RP2040
- Copy `code.py` and `lightbar.py` from the `firmware` directory to Adafruit Feather RP2040's root directory
- Configure and compile app (GUI mode, as esp. in Windows you may need to help it find OpenCV build directory)
  - Windows:
    - Configure: `cmake-gui -S . -B build`
    - Compile:   `cmake --build build --config Release`
    - Test:      `cmake --build build --config Release -t RUN_TESTS`
    - Package:   `cmake --build build --config Release -t package`
  - Linux: 
    - Configure: `cmake-gui -S . -B build`
    - Compile:   `cmake --build build --config Release`
    - Test:      `cmake --build build --config Release -t test`
    - Package:   `cd build; cpack`
- Run it!
  - Windows: `build\app\Release\lightbox_app.exe`
  - Linux: `build/app/lightbox_app`
