#!/bin/sh
# Build the hardware library on a Raspberry Pi.
# Run this on the Pi itself; output is build/hardware/libinky.a
set -e

cd "$(dirname "$0")"

echo "Building hardware library for Raspberry Pi..."
rm -rf build/hardware
script/build hardware

echo ""
echo "Build successful: build/hardware/libinky.a"
