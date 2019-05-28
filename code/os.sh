#!/bin/bash

# up -- user program
#
# Runs a user program with NachOS.
# Example: make; os.sh up create -- compiles and the runs the user program create
if [ $1 == "up" ]; then
  ./userprog/nachos -x ./userland/$2
fi
