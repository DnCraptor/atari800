Atari emulator for ZX Murmulator dev. board (based on RP2040 Arm Cortex M0+ microcontroller)
Related hardware description: https://murmulator.ru/howto 
+ https://github.com/AlexEkb4ever/MURMULATOR_classical_scheme
Telegram canal to discuss it: https://t.me/ZX_MURMULATOR
Telegrem canal with useful things: https://t.me/ZX_MURMULATOR_DATA
Data storage with useful things: https://file.murmulator.ru/data/
N.B. current port uses PSRAM connected like this: https://github.com/DnCraptor/PICO-BK/raw/master/psram.jpg

This port is based on Atari800 emulator version 5.2.0: https://github.com/atari800/atari800
-------------------------------

This is a free and portable Atari 8-bit computer and 5200 console emulator,
originally written by David Firth and now developed by the Atari800
Development Team (please see DOC/CREDITS). This program is copyrighted and
released under the GPL (see COPYING).

Optimised for Murmulator by me:

PS2 Keyboard mapping:
F1 - UI
F2 - Option
F3 - Select
F4 - Start
F5 - Help
Ctrl + Alt + Del - Cold Restart

Joystick 0 emulation:
7  8  9
 \ | /
4--5--6
 / | \
1  2  3
Left Ctrl - fire

Joystick 1 emulation (numpad):
Q  W  E
 \ | /
A--S--D
 / | \
Z  X  C
RightCtrl - fire

Alt combinations (UI functions).

Alt + C opens cartridge menu.
Alt + D opens disk menu.
Alt + O opens sound menu.
Alt + Q performs warm reset.
Alt + R runs loaded content.
Alt + S saves emulator state.
Alt + L loads emulator state.
Alt + T opens cassette menu.
Alt + Y opens settings menu.

Save and load states.

Ctrl + F1 through F12 saves emulator state.
Shift + F1 through F12 loads emulator state.

System combinations.

Alt + Ctrl + Delete performs a cold start.

Dendy-8 joysticks:
Start + Select - UI
Start  - Start
Select - Select
A - Fire

Original project homepage: http://atari800.github.io/
