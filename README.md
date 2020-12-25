# arcin-infinitas
arcin firmware and configuration tool for multiple games, including beatmania IIDX Infinitas (2020 ver)

### What is this?
A custom arcin firmware, written for broad compatibility with various games, including the new infinitas 120hz version. No key config or joy2key needed - everything will work with the game out of the gate. It is natively compatible with Infinitas, HID-light, LR2, and beatoraja. Additional games can be played using keyboard mode, with fully customizable keyboard input.

### Demo

![screenshot](https://raw.githubusercontent.com/minsang-github/arcin-infinitas-conf/c29a50343532a660fc1e9a92380b83ffd167aef0/res/screenshot.png)

[![screenshot](https://raw.githubusercontent.com/minsang-github/arcin-infinitas-conf/e9d2e171b3c819968162a47f1296eef2a8701e3d/res/video_thumbnail.png)](https://www.youtube.com/watch?v=y9fGt4nZhUo)

# Quick HOW-TO

**WARNING: only use with arcin boards. Do not use the firmware flashing tool or the config tool with the official Konami controller plugged in. I'm not responsible for any damage.**

### Where do I download?

[Check out the releases page](https://github.com/minsang-github/arcin-infinitas/releases).

### How do I flash the firmware?
* Run one of the executables in Windows.
    * arcin_infinitas_new.exe is for upgrading from other arcin firmware.
    * arcin_infinitas_upgrade.exe is for upgrading from an older version of arcin-infinitas.
    * You might need to run them a couple times until it works.
* If you run into any trouble: unplug your controller, hold down buttons 1 and 2 while you plug it back in. If you see button 1 flashing - you are now in flashing mode. Try running the flashing tool again from there.
    * It's possible for WS2812B on button 9 to prevent you from entering flashing mode. You can try with it unplugged.

### How do I set up the wiring?
* Ensure your turntable is connected to QE1.
* Black & white keys must be wired to B1-B7. This will give you the most responsive input.
* Start and select to B10 or B11.
* Other buttons - like the two buttons at the top edge of DAO RED - can be wired to B8 and B9 to be used as extra E-buttons.
* WS2812B - see [here](https://github.com/minsang-github/arcin-infinitas/wiki/WS2812B).

### How do I use it with Infinitas?
* Launch the configuration tool, load, set turntable mode to Analog only, save.
* Start the Infinitas launcher, enter Settings
* **Press "デフォルトに戻す" to reset all keybinds. This is important and you MUST do this!!**
    * This firmware pretends to be the official infinitas controller, so the game detects this automatically and uses the correct key binds. Even if the key binds "seem" wrong in the UI, ignore it.
    * Do not change any key binds in the settings - it doesn't seem to work.
* If you have trouble with the game recognizing your controller, ensure all other game controllers are disconnected. If you ever installed third party software to make PS4 DualShock work, that might be interferring as well.

# Features

In the beginning, this firmware was set out to address the turntable sensitivy issues with Infinitas when running at 120hz. The game has since been patched to fix this issue... but now this project now aims to be a highly configurable firmware that is compatbile with a wide array of games.

The goal is to be the only firmware you'll ever need for your IIDX controller for all your games, without the need to flash other firmware for each game.

* Uses the same hardware ID (USB VID / PID) as Konami's Infinitas controller, allowing the game to automatically detect the controlelr as the official one.
* Turntable features:
    * Analog input with sensitivity adjustment
    * Optimized digital turntable mode for LR2, fixing misfire issues with full-size turntables
* Button input features:
    * Optional double-click / triple-click select button feature (like DJ DAO)
    * Reassign E1, E2, E3, E4 buttons
    * Button debouncing with customizable millisecond window
* LED control:
    * Control over turntable LED - reactive mode, HID-light mode
    * Experimental WS2812B support (see beta releases)
* Other features:
    * Switch between 1000hz / 250hz polling mode
    * Keyboard mode for games without proper gamepad support
    * Runtime mode switching via button combinations (hold start+select+button)


## Configuration tool

Press "Refresh" to update the list of devices. Press "Load" to load settings from your controller. Press "Save" to save your settings to your controller.

Hovering over some of the options will show you a tooltip with helpful text.

The following settings are recommended:

* IIDX / Infinitas, beatoraja
    * controller mode, analog turntable, no debouncing, 
* LR2
    * controller mode, digital turntable, 4 frames of debouncing
* DJMAX Respect V
    * keyboard mode (game has poor gamepad support)
    * Use one of the DJMAX presets in the keyboard settings.

## Turntable (QE1) sensitivity
See [wiki page here](https://github.com/minsang-github/arcin-infinitas/wiki/Turntable-sensitivity-explained).

## Runtime mode switching

First of all, turn on "Enable mode switching" in the config tool. Then:

* Holding Start + Select + 1 for 3 seconds will switch between input modes (controller <=> keyboard). Key 2 or 4 will flash to indicate which mode you are in. 
* Holding Start + Select + 3 for 3 seconds will switch between turntable modes (=> analog only => digital only => analog reversed =>). Key 2, 4, or 6 will flash to indicate which mode you are in.
* Holding Start + Select + 5 for 3 seconds will enable or disable all LEDs.

Note that when you use the mode switching button combinations, the changes are not permanently saved; when the controller is unplugged, things will revert back to what was set in the configuration tool. This is intentional!

## WS2812B control

WS2812B light strips can be controlled over button 9 pins. This is currently experimental and only available in beta releases. [See here for details](https://github.com/minsang-github/arcin-infinitas/wiki/WS2812B).

# Disclaimers

For developers: see [BUILDING.md](https://github.com/minsang-github/arcin-infinitas/blob/master/BUILDING.md) for build instructions.

The repository owner or contributors are not liable for any damage caused to your hardware.

This repository is a fork of zyp's arcin code base, specifically at [this commit](https://cgit.jvnv.net/arcin/commit/?h=conf&id=1c211c70f4a23a770724ce435e223d442e5fb78e).

Borrows code from the [FastLED project](https://github.com/FastLED/FastLED)

Some WS2812B features inspired by [Roxy-Firmware project](https://github.com/veroxzik/roxy-firmware).
