#!/bin/bash

# Script to build and run the hardware version on Raspberry Pi
# This script should be run on the Raspberry Pi

echo "Building hardware version for Raspberry Pi..."
make clean
make hardware

if [ $? -ne 0 ]; then
    echo "Build failed!"
    exit 1
fi

echo ""
echo "Build successful!"
echo ""
echo "To test clearing the display:"
echo "  sudo ./bin/test_clear_hardware --color 1    # Clear to white"
echo "  sudo ./bin/test_clear_hardware --color 0    # Clear to black"
echo ""
echo "Note: sudo is required for GPIO/SPI access"
echo "Note: Display refresh can take up to 32 seconds"
echo ""
echo "Running test now (clearing to white)..."
sudo ./bin/test_clear_hardware --color 1