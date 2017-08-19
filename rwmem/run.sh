#!/bin/sh
#socat -u FILE:bin/rwmem TCP:192.168.2.2:6053
cat bin/rwmem | nc 192.168.2.2 6053
