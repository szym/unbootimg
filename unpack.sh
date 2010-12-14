#!/bin/bash
# szym.net, 2010
# Simple wrapper that uses unbootimg to disassemble a boot image
# and unzip the ramdisk.

UNBOOTIMG=unbootimg

if [ $# -ne 1 ]; then
  echo 1>&2 Usage: $0 imagefile.img
  exit 127
fi

set -e

IMG=$1

${UNBOOTIMG} ${IMG}
mkdir ${IMG}-ramdisk
cd ${IMG}-ramdisk
gunzip -c ../${IMG}-ramdisk.cpio.gz | cpio -i
cd ..

