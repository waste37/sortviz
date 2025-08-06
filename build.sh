#!/bin/sh

set -e

PROGRAM_NAME=sortviz

if [ ! -f $PROGRAM_NAME ] || [ $PROGRAM_NAME -ot main.cpp ]; then
    c++ -o $PROGRAM_NAME main.cpp $(sdl2-config --cflags --libs)
    echo "Compilation succesful"
fi
./${PROGRAM_NAME}
