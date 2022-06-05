Lightbox is for a custom lightbox system with a camera.

It's a research project and under early active hacking.
More details TBA.

The Zebral Camera Library is being developed while working on the lightbox,
and can be built as part of the lightbox project or separately - look in
./camera, or in the ./lib and ./include directories of an install package.


- Install dependencies:
  - Windows:
    - Visual Studio 2019 (use the native 64-bit terminal)  (Others may work, TBD)
    - Set up OpenCV 4.5.5, including videoio, highgui, and core 
      (current binary release didn't seem to have these, so yeah - gotta build it)
  - Linux:
    - Currently using gcc 9
    - Using OpenCV 4.2 from Ubuntu's distribution
  - Build tools and deps for both:
    - Doxygen, graphviz, cpack, cmake-format, clang-format
- Set up hardware
- Install CircuitPython to Adafruit Feather RP2040
  https://learn.adafruit.com/adafruit-feather-rp2040-pico/circuitpython
- Copy `code.py` and `lightbar.py` from the `firmware` directory to Adafruit Feather RP2040's root directory
- Configure and compile app (GUI mode, as esp. in Windows you may need to help it find OpenCV build directory)
  - Windows:
    - Configure: `cmake-gui -S . -B build`
    - Compile:   `cmake --build build --config Release`
    - Test:      `cmake --build build --config Release -t RUN_TESTS`
    - Package:   'cmake --build build --config Release -t package'
  - Linux: 
    - Configure: 'cmake-gui -S . -B build'
    - Compile:   'cmake --build build --config Release'
    - Test:      'cmake --build build --config Release -t test'
    - Package:   'cd build; cpack'
- Run it!
  - Windows: build\Release\lightbox.exe
  - Linux: build/lightbox
