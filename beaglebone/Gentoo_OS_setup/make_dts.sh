#!/bin/bash

dtc -O dtb -o BB-SPI0-01-00A0.dtbo -b 0 -@ BB-SPI0-01-00A0.dts
dtc -O dtb -o univ-bbb-EVA-00A0.dtbo -b 0 -@ univ-bbb-EVA-00A0.dts
dtc -O dtb -o playa-bbb-00A0.dtbo -b 0 -@ playa-bbb-00A0.dts
