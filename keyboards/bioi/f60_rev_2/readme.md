# BIOI F60 rev 2
![f60_rev_2](https://img.alicdn.com/imgextra/i3/51502588/O1CN01oOD8WN1UzLzfN7SKK_!!51502588.jpg)

60% multiple layout hotswap keyboard PCBA for Cherry MX compatible switches.

* Keyboard Maintainer: [Basic I/O Instruments (Scott Wei)](https://github.com/scottywei)
* Hardware Supported: [Basic I/O Instruments (Scott Wei)](https://github.com/scottywei)
* Hardware Availability: [Basic I/O Instruments Taobao Store](https://item.taobao.com/item.htm?id=820730728892)

Make example for this keyboard (after setting up your build environment):

    make bioi/f60_rev_2:default

Flashing this keyboard:

Copy the built .uf2 file to the USB mass storage device that appears after entering the bootloader.

See the [build environment setup](https://docs.qmk.fm/#/getting_started_build_tools) and the [make instructions](https://docs.qmk.fm/#/getting_started_make_guide) for more information. Brand new to QMK? Start with our [Complete Newbs Guide](https://docs.qmk.fm/#/newbs).

## Bootloader

Enter the bootloader in 3 ways:

* **Bootmagic reset**: Hold down the key at (0,0) in the matrix (usually the top left key or Escape) and plug in the keyboard
* **Physical reset button**: **DOUBLE PRESS** the `RESET` button on the back of the PCB
* **Keycode in layout**: Press the key mapped to `QK_BOOT` if it is available