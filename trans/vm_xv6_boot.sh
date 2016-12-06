#!/bin/sh

cp vm_xv6_boot.o llboot.o
./cos_linker "llboot.o, ;llpong.o, :" ./gen_client_stub
