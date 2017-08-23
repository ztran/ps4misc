#!/bin/sh
cat bin/elf_dec | nc 192.168.2.2 5053
#socat -u FILE:bin/elf_dec TCP:192.168.2.2:5053
