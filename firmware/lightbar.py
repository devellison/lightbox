import board
import digitalio
import neopixel_write


class LightBar:
    """
    This class is for controlling lightbars in a lightbox.

    Each light bar section has:
        1 Neopixel GBRW strip
        1 Neopixel UV strip
        1 12V IR strip.
    The GBRW strip has 4 channels of pixels (Green, Blue, Red, White)
    The UV strip has 3 channels of pixels (All UV, but 3 of them)
    The IR strip is just on/off 12V for the entire strip from a relay.

    GBRW and UV strip are chained with the GBRW strip first.
    Sections are chained ( so it ends up GBRW->UV->GBRW->UV->etc. )

    Each section as its own DIO line for the relay control for IR.

    Currently the GBRW strip has 9 LEDs, UV has 7 LEDs.

    Pull configurable stuff into init call parameters if we make more.

    In general, my assumption is that we'll only use full on colors, so
    brightness is simplified.
    """

    def __init__(self):
        # Strip types - right now we have 3 types per section
        self.STRIP_TYPE_GBRW = 0
        self.STRIP_TYPE_UV = 1
        self.STRIP_TYPE_IR = 2

        self.ALL = -1           # Do all (sections, etc)

        self.NUM_GBRW_LEDS = 9  # Using 9 LEDs per section for GBRW
        self.NUM_UV_LEDS = 7    # Using 7 LEDS per section for UV
        self.NUM_GBRW_CHANNELS = 4
        self.NUM_UV_CHANNELS = 3
        self.NUM_SECTIONS = 4  # Initially 4 strips chained GBRW_1|UV_1|IR_1|GBRW_2|UV_2|etc

        # RP2040 Feather board
        self.LED_PIN = board.A0
        self.IR_PINS = [board.D9, board.D10, board.D11, board.D12]

        self.GBRW_LEN = self.NUM_GBRW_LEDS * self.NUM_GBRW_CHANNELS
        self.UV_LEN = self.NUM_UV_LEDS * self.NUM_UV_CHANNELS

        self.ArrayLength = self.NUM_SECTIONS * (self.GBRW_LEN + self.UV_LEN)
        # Initialize the buffers and turn the LEDs off
        self.raw_pixel_array = bytearray(self.ArrayLength)

        # ir is per strip rather than per pixel
        self.raw_ir_array = bytearray(self.NUM_SECTIONS)

        # Set up main neopixel pin
        self.pixel_pin = digitalio.DigitalInOut(self.LED_PIN)
        self.pixel_pin.direction = digitalio.Direction.OUTPUT
        neopixel_write.neopixel_write(self.pixel_pin, self.raw_pixel_array)

        # set up IR pins, 1 per section
        self.ir_pins = []
        for cur_ir in range(0, self.NUM_SECTIONS):
            self.ir_pins.append(digitalio.DigitalInOut(self.IR_PINS[cur_ir]))
            self.ir_pins[cur_ir].direction = digitalio.Direction.OUTPUT
            self.ir_pins[cur_ir].value = 0

    # Sets 1 or more pixels.
    #   section = 0-3 index of which strip, or self.ALL for all sections
    #   strip_type = STRIP_TYPE_*
    #   index = 0-based index of LED in the section's strip, or ALL to set all
    #   value = LED value (byte or tuple depending on number of channels in strip_type)
    def SetPixel(self, section: int, strip_type: int, index: int, value):
        offset = 0
        start = 0
        count = 1

        if section == self.ALL:
            section_start = 0
            section_count = self.NUM_SECTIONS
        else:
            section_start = section
            section_count = 1

        if strip_type == self.STRIP_TYPE_GBRW:
            offset = 0
            if index == self.ALL:
                count = self.NUM_GBRW_LEDS
            else:
                start = index * self.NUM_GBRW_CHANNELS
        elif strip_type == self.STRIP_TYPE_UV:
            offset = self.NUM_GBRW_LEDS * self.NUM_GBRW_CHANNELS
            if index == self.ALL:
                count = self.NUM_UV_LEDS
            else:
                start = index * self.NUM_UV_CHANNELS
        elif strip_type == self.STRIP_TYPE_IR:
            # IR is just a relay toggle, we ignore index
            for curSection in range(section_start, section_start + section_count):
                self.raw_ir_array[curSection] = value
            return

        for curSection in range(section_start, section_start + section_count):
            section_off = offset + curSection * \
                (self.NUM_GBRW_LEDS*self.NUM_GBRW_CHANNELS +
                 self.NUM_UV_LEDS*self.NUM_UV_CHANNELS)
            for curIndex in range(start, start + count):
                if (strip_type == self.STRIP_TYPE_GBRW):
                    idxOff = curIndex * self.NUM_GBRW_CHANNELS + section_off
                    self.raw_pixel_array[idxOff] = value[1]
                    self.raw_pixel_array[idxOff+1] = value[0]
                    self.raw_pixel_array[idxOff+2] = value[2]
                    self.raw_pixel_array[idxOff+3] = value[3]
                else:
                    idxOff = curIndex * self.NUM_UV_CHANNELS + section_off
                    self.raw_pixel_array[idxOff] = value[0]
                    self.raw_pixel_array[idxOff+1] = value[1]
                    self.raw_pixel_array[idxOff+2] = value[2]

    def UpdatePixels(self):
        neopixel_write.neopixel_write(self.pixel_pin, self.raw_pixel_array)
        for cur_ir in range(0, self.NUM_SECTIONS):
            self.ir_pins[cur_ir].value = self.raw_ir_array[cur_ir]

    def SetGBRW(self, color, section=None):
        if (section == None):
            section = self.ALL
        self.SetPixel(section, self.STRIP_TYPE_GBRW, self.ALL, color)
        self.SetPixel(section, self.STRIP_TYPE_UV,   self.ALL, [0, 0, 0])
        self.SetPixel(section, self.STRIP_TYPE_IR,   self.ALL, 0)
        self.UpdatePixels()

    def SetUv(self, color, section=None):
        if (section == None):
            section = self.ALL
        self.SetPixel(section, self.STRIP_TYPE_GBRW, self.ALL, [0, 0, 0, 0])
        self.SetPixel(section, self.STRIP_TYPE_UV,   self.ALL, color)
        self.SetPixel(section, self.STRIP_TYPE_IR,   self.ALL, 0)
        self.UpdatePixels()

    def SetIr(self, color, section=None):
        if (section == None):
            section = self.ALL
        self.SetPixel(section, self.STRIP_TYPE_GBRW, self.ALL, [0, 0, 0, 0])
        self.SetPixel(section, self.STRIP_TYPE_UV,   self.ALL, [0, 0, 0])
        self.SetPixel(section, self.STRIP_TYPE_IR,   self.ALL, color)
        self.UpdatePixels()

    def Off(self, section=None):
        if (section == None):
            section = self.ALL
        self.SetPixel(section, self.STRIP_TYPE_GBRW, self.ALL, [0, 0, 0, 0])
        self.SetPixel(section, self.STRIP_TYPE_UV,   self.ALL, [0, 0, 0])
        self.SetPixel(section, self.STRIP_TYPE_IR,   self.ALL, 0)
        self.UpdatePixels()

    def SetGBRWFields(self, color, top, bottom):
        if top:
            self.SetGBRW(color, 0)
            self.SetGBRW(color, 1)
        else:
            self.Off(0)
            self.Off(1)

        if bottom:
            self.SetGBRW(color, 2)
            self.SetGBRW(color, 3)
        else:
            self.Off(2)
            self.Off(3)

    def SetUvFields(self, color, top, bottom):
        if top:
            self.SetUv(color, 0)
            self.SetUv(color, 1)
        else:
            self.Off(0)
            self.Off(1)

        if bottom:
            self.SetUv(color, 2)
            self.SetUv(color, 3)
        else:
            self.Off(2)
            self.Off(3)

    def SetIrFields(self, color, top, bottom):
        if top:
            self.SetIr(color, 0)
            self.SetIr(color, 1)
        else:
            self.Off(0)
            self.Off(1)

        if bottom:
            self.SetIr(color, 2)
            self.SetIr(color, 3)
        else:
            self.Off(2)
            self.Off(3)

    # This works as long as all colors are full on/off,
    # which I think is probably how I'll use this for vision but we'll see...
    def UpdateBrightness(self, bright):
        for i in range(0, len(self.raw_pixel_array)):
            if self.raw_pixel_array[i] > 0:
                self.raw_pixel_array[i] = bright
        self.UpdatePixels()
