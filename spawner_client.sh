#!/bin/sh

objcopy -O binary --only-section=.rodata ../client client.rodata
touch log.bin
../../Logless/build/spawner client.rodata log.bin noexiteof