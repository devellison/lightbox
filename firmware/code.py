# Test for lightbox
import supervisor
import time
import lightbar
import board
from digitalio import DigitalInOut, Direction, Pull

# LightBar control and vars
lightbar = lightbar.LightBar()
bright = 255
top = True
bottom = True

# Cycle button
btn = DigitalInOut(board.A1)
btn.direction = Direction.INPUT
btn.pull = Pull.UP
btn_prev= btn.value
btn_cur = btn.value

# IR-cut filter - start with it on (blocking IR)
ir_filter = DigitalInOut(board.D6)
ir_filter.direction = Direction.OUTPUT
ir_filter.value = 1

# Cycle logic
cycle=["off","red","green","blue","yellow","magenta","cyan","uv","ir",
       "top","red","green","blue","yellow","magenta","cyan","uv","ir",
       "bottom","red","green","blue","yellow","magenta","cyan","uv","ir",
       "all"]
cycle_pos = 0

def OnHelp():
    print("Commands:")
    print("   off            Turns all lights off")
    print("   red            Turns all lights red")
    print("   green          Turns all lights green")
    print("   blue           Turns all lights blue")
    print("   white          Turns all lights white")
    print("   uv             Turns all lights uv")
    print("   ir             Turns all lights ir")
    print("   bright VALUE   Sets brightness level")
    print("   top            Sets to only use top lights")
    print("   bottom         Sets to only use bottom lights")
    print("   all            Sets to use all lights")
    print("   flton          Sets ir cut filter on")
    print("   fltoff         Sets ir cut filter off")

def set_ir_filter(enable : bool):
    if enable:
        ir_filter.value = 1
    else:
        ir_filter.value = 0


def OnCommand(tokens, lightbar, bright, top, bottom):
    if len(tokens) == 0:
        return [bright,top,bottom]
    value = tokens[0]
    if value == "top":
        top = True
        bottom = False
    elif value == "bottom":
        top = False
        bottom = True
    elif value == "all":
        top = True
        bottom = True
    elif value == "bright":
        if (len(tokens) > 1):
            bright = int(tokens[1])
            if (bright <= 0):
                bright = 1
            if (bright > 255):
                bright = 255
            lightbar.UpdateBrightness(bright)
        else:
            print("brightness needs an 8-bit value. e.g. bright 128")
    elif value == "red":
        lightbar.SetGBRWFields([bright,0,0,0],top,bottom)
    elif value == "green":
        lightbar.SetGBRWFields([0,bright,0,0],top,bottom)
    elif value == "magenta":
        lightbar.SetGBRWFields([bright,0,bright,0],top,bottom)
    elif value == "yellow":
        lightbar.SetGBRWFields([bright,bright,0,0],top,bottom)
    elif value == "cyan":
        lightbar.SetGBRWFields([0,bright,bright,0],top,bottom)
    elif value == "blue":
        lightbar.SetGBRWFields([0,0,bright,0],top,bottom)
    elif value == "white":
        lightbar.SetGBRWFields([0,0,0,bright],top,bottom)
    elif value == "uv":
        lightbar.SetUvFields([bright,bright,bright],top,bottom)
    elif value == "ir":
        # no brightness on IR right now
        lightbar.SetIrFields(255,top,bottom)
    elif value == "off":
        lightbar.Off(lightbar.ALL)
    elif value == "flton":
        set_ir_filter(True)
    elif value == "fltoff":
        set_ir_filter(False)
    elif value == "help":
        OnHelp()
    else:
        print("Unknown command: {}".format(value))

    return [bright,top,bottom]
##############################################################
# Main loop


while True:
    # If we have input from serial, receive it and process command
    # This does halt the loop once we get some input - maybe switch to asynchronous and buffering later.
    if supervisor.runtime.serial_bytes_available:
        tokens = input().strip().lower().split()
        [bright,top,bottom] = OnCommand(tokens, lightbar, bright,top,bottom)

    # Check if the button's been pressed.
    btn_cur = btn.value
    if btn_cur != btn_prev:
        # If the button just got pressed, step the cycle (so we don't need a serial interface to use)
        if not btn_cur:
            cycle_pos = (cycle_pos + 1) % len(cycle)
            command = []
            command.append(cycle[cycle_pos])
            print("OnButton: {}".format(command))
            [bright,top,bottom] = OnCommand(command, lightbar, bright, top, bottom)            
        else:
            # Sleep after release to debounce
            time.sleep(0.2)

    btn_prev = btn_cur

