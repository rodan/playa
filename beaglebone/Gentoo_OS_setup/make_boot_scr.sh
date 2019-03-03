#!/bin/bash

mkimage -C none -A arm -T script -d boot_spi0.cmd boot_spi0.scr
mkimage -C none -A arm -T script -d boot_univ.cmd boot_univ.scr
mkimage -C none -A arm -T script -d boot_playa.cmd boot_playa.scr
