#!/bin/sh

map() { ./main disc MAP; }
lsdisc() { ./main disc LS; }

make

./main disc COPYTO test_files/wedkarz.txt

map
lsdisc

./main disc COPYTO test_files/szturmowiec.png

map
lsdisc

./main disc RM wedkarz.txt

map
lsdisc

./main disc RM tekst.txt

./main disc COPYTO test_files/legenda.txt

map
lsdisc

./main disc COPYTO test_files/wedkarz.txt

map
lsdisc

./main disc COPYTO test_files/tekst.txt

map
lsdisc

./main disc COPYTO test_files/jarek.jpg

map
lsdisc

./main disc RM legenda.txt

map
lsdisc

./main disc COPYTO test_files/vader.jpg

map
lsdisc

./main disc RM szturmowiec.png

map
lsdisc

./main disc COPYTO test_files/vader.jpg

map
lsdisc

./main disc COPYTO test_files/szturmowiec.png