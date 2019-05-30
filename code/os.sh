#!/bin/bash

# up -- user program
#
# Runs a user program with NachOS.
# Example: make; os.sh up create -- compiles and the runs the user program create
if [ $1 == "up" ]; then
  ./userprog/nachos -x ./userland/$2
fi

# updeb -- user program debug
#
# Runs a user program with custom debugging on.
# Example: make; os.sh updeb halt
if [ $1 == "updeb" ]; then
  ./userprog/nachos -d c -x ./userland/$2 
fi

# updeb -- user program debug
#
# Runs a user program with gdb to debug it.
# Example: make; os.sh upgdb halt
if [ $1 == "upgdb" ]; then
  gdb --args ./userprog/nachos -x ./userland/$2
fi

# updebui -- user program debug, with user interface
#
# Runs a user program with gdb and a user interface 
# to debug it.
# Example: make; os.sh updebui test_io
if [ $1 == "updebui" ]; then
  gdb -tui --args ./userprog/nachos -x ./userland/$2
fi