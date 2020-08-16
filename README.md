# arcin-infinitas
arcin firmware and configuration tool for multiple games, including beatmania IIDX Infinitas (2020 ver)

![screenshot](https://raw.githubusercontent.com/minsang-github/arcin-infinitas-conf/master/res/screenshot.png)

## What is this?
A custom arcin firmware, written for broad compatibility with various games, including the new infinitas 120hz version. No key config or joy2key needed - everything will work with the game out of the gate. It is **compatible with Infinitas, HID-light, LR2, and beatoraja!** You can use the configuration tool to adjust options like turntable sensitivity.

## Where do I download?

[Check out the latest release](https://github.com/minsang-github/arcin-infinitas/releases/latest).

## How do I flash the firmware?

Unplug your controller. Hold down buttons 1 and 2 while you plug it back in. If you see button 1 flashing - you are now in flashing mode.

Run the executable (arcin_infinitas_new.exe or arcin_infinitas_new.exe) in Windows. You might need to do it a few times until it works.

## How do I use it with Infinitas?
* Ensure your turntable is connected to QE1, buttons to b1-b7, start and select to b10 or b11.
* Launch the configuration tool (arcin_conf_infinitas.exe)
    * Set turntable mode to Analog only.
    * Adjust turntable sensitity as needed.
    * Change E-button layout as needed.
* Start the Infinitas launcher, enter Settings
* **Press "デフォルトに戻す" to reset all keybinds. This is important and you MUST do this!!**
    * This firmware pretends to be the official infinitas controller, so the game detects this automatically and uses the correct key binds. Even if the key binds "seem" wrong in the UI, ignore it.
    * Do not change any key binds in the settings - it won't work.

# Details

This is a custom arcin firmware, mainly for playing the new version of Infinitas.

On existing infinitas firmware that people use, some people reported that their turntable is not sensitive enough in the new version of the game, as if the "deadzone" is too large. This is probably caused by the fact that the engine for Infinitas is now based on Heroic Verse to run at 120hz. This firmware aims to fix that.

On top of the "arcin_flash_config_alpha5" firmware, the following changes are made:

* Changes the hardware ID (USB VID / PID) to be the same as Konami infinitas controller. This allows the game to automatically detect it & use the analog turntable.
* Remaps E1-E4 buttons correctly (customizable in the configuration tool)
* Optional analog / digital turntable modes
* Optional double-click / triple-click select button feature (like DJ DAO)
* Optional button debouncing
* Freely switch between 1000hz / 250hz polling mode

In addition to Infinitas, this firmware should be perfectly fine with HID-light and beatoraja (analog turntable) configurations. If you enable digital turntable in the configuration tool, it will also work in LR2 for playing BMS.

This firmware is also technically compatible with the flashing tools, but the flashing tools are unable to automatically discover the controller becuase the USB IDs will be different from what they expect. To work around this, unplug the USB cable, hold down 1+2, plug in the USB cable, this will enter flashing mode and you will be able to proceed as normal.

# Configuration tool

![screenshot](https://raw.githubusercontent.com/minsang-github/arcin-infinitas-conf/master/res/screenshot.png)

Press "Refresh" to update the list of devices. Press "Load" to load settings from your controller. Press "Save" to save your settings to your controller.

Hovering over some of the options will show you a tooltip with helpful text.

The following configurations are recommended:

* Infinitas, beatoraja    
    * 1000hz, but some have reported 250hz being better for Infinitas
    * No debouncing
    * Analog turntable ONLY (leaving digital turntable on will confuse the game)
    * Turntable sensitivity 1:3 for 250hz, 1:4 for 1000hz
* LR2
    * 1000hz polling rate
    * 4-5 frames of debouncing
    * Digital turntable
    * Turntable sensitivity 1:3 or 1:4
* HID-light
    * 1000hz polling rate
    * No debouncing
    * Analog turntable
    * Turntable sensitivity 1:4
   
## Building

This section is for developers only. If you're just looking to run this on your board, download the [latest release](https://github.com/minsang-github/arcin-infinitas/releases/latest).

This is the hard part... components required to are not included in this repository, so you need to shop around.

* Get the ARM toolchain for Linux and put it in your $PATH
    * https://launchpad.net/gcc-arm-embedded/+download
    * Try to get these files from the website instead of the ones from your OS's package repository.
* Get SCons : http://scons.org/pages/download.html
    * You can get scons from yor OS's package repository (e.g., apt-get)
* git submodule init: this will pull the external laks submodule, which contains headers for the hardware used by the arcin.

To build, run

    scons

To flash, run

    ./hidflash.py arcin.elf

To create an executable for easily flashing the ELF file, grab https://github.com/theKeithD/arcin/tree/svre9/arcin-utils and then run:

    ./hidloader_append.py arcin.elf hidloader_v2.exe arcin_flash_custom.exe

I had success with Ubuntu 20.04 LTS on WSL2 (Windows Subsystem for Linux). Mind that ARM toolchian will not work in WSL1!

The configuration is hosted at https://github.com/minsang-github/arcin-infinitas-conf and that one is a pure python project.

## Disclaimer

The repository owner or contributors are not liable for any damage caused to your hardware. That being said, arcin is a fairly robust board with recovery capabilities, so you should be fine!

This repository is a fork of zyp's respository, specifically at this commit: https://cgit.jvnv.net/arcin/commit/?h=conf&id=1c211c70f4a23a770724ce435e223d442e5fb78e

Thank you for the awesome project, zyp!
