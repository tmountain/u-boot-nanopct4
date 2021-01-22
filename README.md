# u-boot-nanopct4

After many attempts, I have not been able to get a working u-boot for the
NanoPC-T4 using the upstream u-boot source code. I
[reported this issue](https://lists.denx.de/pipermail/u-boot/2021-January/437950.html)
to the u-boot mailing list, but I did not receive a reply.

As such, I resorted to looking at the Armbian u-boot build process since that distribution ships
with a u-boot that works with the NanoPC-T4.

In my investigation, I discovered that the Armbian u-boot is heavily patched with the
patches applied against the v2020.10 u-boot official release.

Many of the patches applied in the [Armbian build process](https://github.com/armbian/build) are
not directly applicable to the NanoPC-T4, so I created a
[patch](https://github.com/tmountain/u-boot-nanopct4/blob/main/vendor/patches/v2020.10.patch),
which I believe represents the minimum changes necessary to make the u-boot v2020.10 release
work with the NanoPC-T4.

This repository is the result of that work and represents two distinct goals:

* Curating a reproducible / stable u-boot build specifically for the NanoPC-T4.
* Establishing a basis by which a Nix u-boot can be built for this specific SoC.

The Nix build is a work in progress, but in the meantime, the following instructions detail
how to build u-boot images from inside of an x86 Ubuntu/Focal docker container.

```
$ docker run -ti ubuntu:focal
# apt-get update

# apt-get -y install build-essential flex bison gcc-aarch64-linux-gnu git \
  device-tree-compiler python3 gcc-arm-none-eabi bc

# cd /root

# git clone https://github.com/tmountain/u-boot-nanopct4

# cd u-boot-nanopct4

# make CROSS_COMPILE=aarch64-linux-gnu- nanopc-t4-rk3399_defconfig
# make u-boot-dtb.bin CROSS_COMPILE=aarch64-linux-gnu-

# tools/mkimage -n rk3399 -T rksd -d vendor/rkbin/blobs/rk3399_ddr_800MHz_v1.24.bin idbloader.bin
# cat vendor/rkbin/blobs/rk3399_miniloader_v1.19.bin >> idbloader.bin

# export PATH=$PATH:`pwd`/vendor/rkbin/bin
# trust_merger --replace bl31.elf vendor/rkbin/blobs/rk3399_bl31_v1.30.elf trust.ini
# loaderimage --pack --uboot ./u-boot-dtb.bin uboot.img

# dd if=idbloader.bin of=/dev/mmcblk2 seek=64 conv=notrunc
# dd if=uboot.img of=/dev/mmcblk2 seek=16384 conv=notrunc
# dd if=trust.bin of=/dev/mmcblk2 seek=24576 conv=notrunc
# sync
```
