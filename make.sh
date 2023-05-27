#!/bin/bash
echo "Building..."
cc src/main.c -o build/main -lSDL2  -lSDL2_ttf
echo "Done."

