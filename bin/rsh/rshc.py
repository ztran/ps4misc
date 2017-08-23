#! /usr/bin/python

import socket
import os
import sys
import time
import struct

# connect to 8099
# sendfilename
# sendfile
# socketclose
def print_help():
    print("exec [ip] [programpath] (args...)")
    print("put [ip] [localfile] [remotepath]")
    print("get [ip] [remotepath] [localfile]")
    exit()

if len(sys.argv) < 3:
    print_help();

if (sys.argv[1] == "exec"):
    if len(sys.argv) < 4:
        print_help();

    remotehost = sys.argv[2]
    remotepath = sys.argv[3]
    extraargs = sys.argv[4:]
    print("extraargs: ")
    print(extraargs)

    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    # now connect to the web server on port 80 - the normal http port
    s.connect((remotehost, 6070))
    s.send(struct.pack("I", 4)) #header size
    s.send(struct.pack("I", 0x1)) #cmd

    s.send(struct.pack("I", len(remotepath)))
    s.send(remotepath)

    s.send(struct.pack("I", len(extraargs)))

    for a in extraargs:
        s.send(struct.pack("I", len(a)))
        s.send(a)

    while(1):
        r = s.recv()
        if r < 1:
            print("connection closed")
            break;
        print(r)


# r = readall(socket, &filename_size, 4);
# if (r != 0)
#     return -1;
# r = readall(socket, &datasize, 8);
# if (r != 0)
#     return -1;

# filename = malloc(filename_size);
# r = readall(socket, filename, filename_size);
# if (r != 0)
#     return -1;

# char * buf = malloc(4096 * 10);

# int f = open(buf, O_WRONLY | O_CREAT);

if (sys.argv[1] == "put"):
    if len(sys.argv) < 3:
        print_help();

    #print(sys.argv)
    localfile = sys.argv[-2]
    remotepath = sys.argv[-1]
    print("put %s -> %s" % (localfile, remotepath))

    msg = open(localfile, "rb").read()

    time.sleep(0.2)

    # create an INET, STREAMing socket
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    # now connect to the web server on port 80 - the normal http port
    s.connect((sys.argv[-3], 6070))

    s.send(struct.pack("I", 4)) #header size
    s.send(struct.pack("I", 0x2)) #cmd

    s.send(struct.pack("I", len(remotepath)))
    s.send(struct.pack("L", len(msg)))
    s.send(remotepath)

    MSGLEN = len(msg)

    totalsent = 0
    while totalsent < MSGLEN:
        sent = s.send(msg[totalsent:])
        if sent == 0:
            raise RuntimeError("socket connection broken")
        totalsent = totalsent + sent

    s.close()

if (sys.argv[1] == "get"):
    if len(sys.argv) < 3:
        print_help();

    #print(sys.argv)
    localfile = sys.argv[-1]
    remotepath = sys.argv[-2]
    print("get %s -> %s" % (remotepath, localfile))

    f = open(localfile, "wb")
    msg = ""

    time.sleep(0.2)

    # create an INET, STREAMing socket
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    # now connect to the web server on port 80 - the normal http port
    s.connect((sys.argv[-3], 6070))
    s.send(remotepath)
    s.send("\x00")

    MSGLEN = len(msg)

    totalsent = 0
    while totalsent < MSGLEN:
        sent = s.send(msg[totalsent:])
        if sent == 0:
            raise RuntimeError("socket connection broken")
        totalsent = totalsent + sent

    f.close()
    s.close()
