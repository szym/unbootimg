Unbootimg
=========

When working on Android, I found it useful to be able to unpack a boot image,
replace a few files and repack it. unbootimg is the perfect complement to the
mkbootimg tool that is used to create boot images.

unbootimg takes a boot image and produces all that is necessary in order to
create a new image using mkbootimg. In particular it extracts the kernel command
line and the addresses of the image parts, and creates a command line for
mkbootimg to regenerate the image as close to the original one. It even verifies
the SHA1 checks!

unpack and repack
-----------------

`unpack.sh` and `repack.sh` are trivial shell wrappers for unbootimg that also
take care of the ramdisk packaging: cpio and gzip. They are designed to
complement, so that this would simply recreate boot.img without changes:

    unpack.sh boot.img
    repack.sh boot.img

Building
--------

If you already have the AOSP repo and managed to build mkbootimg, adding
unbootimg is easy.

- Download the [unbootimg source](https://github.com/szym/unbootimg/zipball/master)
- Unpack it to your `mydroid/repo/system/core/mkbootimg`. Note: it overwrites
Android.mk to include unbootimg.
- Build it:

    cd mydroid/repo
    . build/envsetup.sh
    make mkbootimg unbootimg
    # sadly that can't install unpack.sh and repack.sh so we need:
    mmm system/core/mkbootimg

- Put `mydroid/repo/out/host/linux-x86/bin` in your path. Easiest way to do this
is:

    setpaths  # this is defined by Android build system `. build/envsetup.sh`

unbootimg depends (like mkbootimg) on libmincrypt for computing the SHA1, but
this can easily be disabled in the source code.

Usage
-----

    # on the phone: copy the boot or recovery image, for example
    dd if=/dev/mtd/mtd3 of=/sdcard/recovery-backup.img
    # on the host: pull the backup
    adb pull /sdcard/recovery-backup.img .
    # unpack it
    unpack.sh recovery-backup.img
    # edit as you wish:
    # cmdline: recovery-backup.img-mk
    # kernel:  recovery-backup.img-kernel
    # ramdisk: recovery-backup.img-ramdisk/
    # repack it
    repack.sh recovery-backup.img
    # send to phone
    adb push recovery-backup.img /sdcard
    # flash it using flash_image

