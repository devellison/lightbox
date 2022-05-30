Lightbox is for a custom lightbox system with a camera

- Install dependencies:
  - Windows:
    - Visual Studio 2019 (use the native 64-bit terminal)  (Others may work, TBD)
    - Set up OpenCV 4.5.5, including videoio, highgui, and core 
      (current binary release didn't seem to have these, so yeah - gotta build it)
- Set up hardware
- Install CircuitPython to Adafruit Feather RP2040
  https://learn.adafruit.com/adafruit-feather-rp2040-pico/circuitpython
- Copy `code.py` and `lightbar.py` to Adafruit Feather RP2040's root directory
- Configure and compile app (GUI mode, as esp. in Windows you may need to help it find OpenCV build directory)
  - Configure: `cmake-gui -S . -B build`
  - Compile:   `cmake --build build --config Release`
  - Test:      `cmake --build build --config Release -t RUN_TESTS`
- Run it!
  - Windows: build\Release\lightbox.exe
  - Linux: TBD
