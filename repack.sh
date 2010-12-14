#!/bin/bash
# szym.net, 2010
# Simple wrapper that uses mkbootimg to resassemble a boot image
# disassembled with unbootimg (see unpack.sh).

MKBOOTIMG=mkbootimg

if [ $# -ne 1 ]; then
  echo 1>&2 Usage: $0 imagefile.img
  exit 127
fi

set -e

IMG=$1

cd ${IMG}-ramdisk
find . | cpio -o -H newc | gzip > ../${IMG}-ramdisk.cpio.gz
cd ..
bash -c "${MKBOOTIMG} `cat ${IMG}-mk`" 

