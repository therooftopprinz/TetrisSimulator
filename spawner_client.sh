#!/bin/sh
# Run from the repository root after: cmake -S . -B build && cmake --build build --target spawner
objcopy -O binary --only-section=.rodata build/client client.rodata
touch log.bin
SPAWNER=build/logless-build/spawner
if ! test -x "$SPAWNER"; then
  SPAWNER=build/_deps/logless_fc-build/spawner
fi
exec "$SPAWNER" client.rodata log.bin noexiteof