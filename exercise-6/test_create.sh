#!/bin/sh
make

# Create a disc with 1MB
./main disc CREATE 1048576

./main disc MAP

./main disc LS
