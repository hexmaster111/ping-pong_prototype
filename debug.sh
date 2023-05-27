#!/bin/bash
echo "Building... for debug"
cc src/main.c -o build/main -lSDL2  -lSDL2_ttf -g
echo "Done."
echo "Lauching Debugger..."
gdb build/main
echo "Done."

