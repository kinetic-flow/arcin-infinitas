# arcin-infinitas
arcin firmware and configuration tool for multiple games, including beatmania IIDX Infinitas (2020 ver)

![screenshot](https://raw.githubusercontent.com/minsang-github/arcin-infinitas-conf/ce8058caf97a84ac9981a455a71a1bc7173a6317/res/screenshot.png)

## What is this?
A custom arcin firmware, written for broad compatibility with various games, including the new infinitas 120hz version. No key config or joy2key needed - everything will work with the game out of the gate. It is natively compatible with Infinitas, HID-light, LR2, and beatoraja. Additional games can be played using keyboard mode, with fully customizable keyboard input.

**WARNING: only use with arcin boards. Do not use the firmware flashing tool or the config tool with the official Konami controller plugged in. I'm not responsible for any damage.**

## Quick HOW-TO

### Where do I download?

[Check out the releases page](https://github.com/minsang-github/arcin-infinitas/releases).

### How do I flash the firmware?
* Unplug your controller. Hold down buttons 1 and 2 while you plug it back in. If you see button 1 flashing - you are now in flashing mode.
* Run the executable (arcin_infinitas_new.exe or arcin_infinitas_new.exe) in Windows. You might need to do it a few times until it works.

### How do I set up the wiring?
* Ensure your turntable is connected to QE1.
* Black & white keys must be wired to b1-b7. This will give you the most responsive input. Avoid b8 and b9 (they have extra debouncing enabled)
* Start and select to b10 or b11.
* Other buttons - like the two buttons at the top edge of DAO RED - can be wired to b8 and b9 to be used as extra E buttons.

### How do I use it with Infinitas?
* Launch the configuration tool (arcin_conf_infinitas.exe)
    * Set turntable mode to Analog only
* Start the Infinitas launcher, enter Settings
* **Press "デフォルトに戻す" to reset all keybinds. This is important and you MUST do this!!**
    * This firmware pretends to be the official infinitas controller, so the game detects this automatically and uses the correct key binds. Even if the key binds "seem" wrong in the UI, ignore it.
    * Do not change any key binds in the settings - it doesn't seem to work.
* If you have trouble with the game recognizing your controller, ensure all other game controllers are disconnected. If you ever installed third party software to make PS4 DualShock work, that might be interferring as well.

## Features

In the beginning, this firmware was set out to address the turntable sensitivy issues with Infinitas when running at 120hz. The game has since been patched. This project now aims to be a highly configurable firmware that is compatbile with a wide array of games.

On top of the "arcin_flash_config_alpha5" firmware, the following changes are made:

* Changes the hardware ID (USB VID / PID) to be the same as Konami infinitas controller. This allows the game to automatically detect it & use the analog turntable.
* Remaps E1-E4 buttons correctly; customizable in the configuration tool
* Supports both analog and digital turntable input
* Optional double-click / triple-click select button feature (like DJ DAO)
* Button debouncing with customizable millisecond window
* Switch between 1000hz / 250hz polling mode
* Keyboard mode for games without proper gamepad support
* Runtime mode switching via button combinations (hold start+select+button)
* Control over turntable LED - reactive mode, HID-light mode
* Experimental WS2812B support

In addition to Infinitas, this firmware should be perfectly fine with HID-light and beatoraja (analog turntable) configurations. If you enable digital turntable in the configuration tool, it will also work in LR2 for playing BMS. Keyboard mode can be used for any games that do not have full support for gamepads (DJMAX Respect V, Muse Dash...).

## Configuration tool

Press "Refresh" to update the list of devices. Press "Load" to load settings from your controller. Press "Save" to save your settings to your controller.

Hovering over some of the options will show you a tooltip with helpful text.

The following configurations are recommended:

* Infinitas, beatoraja    
    * 1000hz, but some have reported 250hz being better for Infinitas
    * No debouncing
    * Analog turntable ONLY (leaving digital turntable on will confuse the game)
    * Turntable sensitivity 1:3 for 250hz, 1:4 for 1000hz
    * Controller ONLY
* LR2
    * 1000hz polling rate
    * 4-5 frames of debouncing
    * Digital turntable
    * Turntable sensitivity 1:3 or 1:4
    * Controller ONLY
* HID-light
    * 1000hz polling rate
    * No debouncing
    * Analog turntable
    * Turntable sensitivity 1:4
* DJMAX Respect V
    * 1000hz polling rate
    * Keyboard ONLY (game has poor gamepad support)
    * Use one of the DJMAX presets in the keyboard settings.

## Keyboard mode

![screenshot](https://raw.githubusercontent.com/minsang-github/arcin-infinitas-conf/master/res/keyboard.png)

When keyboard mode is enabled, button pressees will register as keystrokes. Configure each key codes in the menu as shown above.

## Turntable (QE1) sensitivity
Here is an explanation of the sensitivty options.

1:1 means the rotary encoder counts from 0 and up to (but not including) 256. This value is directly reported as the joystick X-axis.

1:N increases the range in which the encoder counts up to at the hardware level. 1:2 means 0 to 512, 1:4 means 0 to 1024, etc. In other words, you are setting finer granularity for each tick as N increases. Effectively, going from 1:2 to 1:4 makes so that twice the amount of rotation of the encoder is required to input a full rotation.

N:1 means the rotary encoder counts from 0 and up to (but not including) 256, but the value reported to your computer will be amplified by a factor of N. 4:1 means it will move 4x as much, and so on. This comes at a loss of granularity (values will "jump" between each tick).

For DAO turntables, it is recommended that you stick to values between 1:2 and 1:5, inclusive. 1:2 is super sensitive, 1:3 is sensitive, 1:4 and 1:5 are like stock DAO.

## Runtime mode switching

First of all, turn on "Enable mode switching" in the config tool. Then:

* Holding Start + Select + 1 for 3 seconds will switch between input modes (controller <=> keyboard). Key 2 or 4 will flash to indicate which mode you are in. 
* Holding Start + Select + 3 for 3 seconds will switch between turntable modes (=> analog only => digital only => analog reversed =>). Key 2, 4, or 6 will flash to indicate which mode you are in.
* Holding Start + Select + 5 for 3 seconds will enable or disable all LEDs.

Note that when you use the mode switching button combinations, the changes are not permanently saved; when the controller is unplugged, things will revert back to what was set in the configuration tool. This is intentional!

## WS2812B control

WS2812B light strips can be controlled over button 9 pins. This is currently experimental. See https://github.com/minsang-github/rhythmgame-docs/wiki/WS2812B-on-arcin for details.

## For developers - build instructions
See [BUILDING.md](https://github.com/minsang-github/arcin-infinitas/blob/master/BUILDING.md).

## Disclaimer

The repository owner or contributors are not liable for any damage caused to your hardware.
This repository is a fork of zyp's respository, specifically at this commit: https://cgit.jvnv.net/arcin/commit/?h=conf&id=1c211c70f4a23a770724ce435e223d442e5fb78e
