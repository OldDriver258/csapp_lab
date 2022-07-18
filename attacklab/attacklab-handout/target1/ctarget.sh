#!/usr/bin/bash
./hex2raw < phase1.txt > phase1.raw
./ctarget -q < phase1.raw

./hex2raw < phase2.txt > phase2.raw
./ctarget -q < phase2.raw

./hex2raw < phase3.txt > phase3.raw
./ctarget -q < phase3.raw

./hex2raw < phase4.txt > phase4.raw
./rtarget -q < phase4.raw

./hex2raw < phase5.txt > phase5.raw
./rtarget -q < phase5.raw

