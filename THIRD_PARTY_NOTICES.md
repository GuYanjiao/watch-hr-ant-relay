# Third-party notices

## Zephyr BLE discovery flow

`firmware/src/main.c` adapts the discovery flow from Zephyr's `samples/bluetooth/central_hr/src/main.c`.

Copyright (c) 2015–2016 Intel Corporation. Licensed under Apache License 2.0; the license text is retained in [LICENSES/Apache-2.0.txt](LICENSES/Apache-2.0.txt), and the source header retains attribution.

## External dependencies

Nordic nRF Connect SDK, Zephyr, ANT Add-on, pyserial and Unicorn are installed separately and remain subject to their respective licenses. ANT profiles and the binary ANT stack are not vendored in this repository. Consult the ANT Add-on source headers and its LICENSE.txt before distributing dependent binaries.

The UF2 software reset request follows the [Adafruit nRF52 Bootloader documentation](https://github.com/adafruit/Adafruit_nRF52_Bootloader#how-to-use).
