# Visionfive2

This file documents set up of the experiments that I performed
while playing around with StarFive VisionFive 2.

## Using a pre-built distro

Download a pre-built image from [debian.starfivetech.com](https://debian.starfivetech.com/).
Follow instructions of the official
[Quick Start Guide](https://doc-en.rvspace.org/VisionFive2/Quick_Start_Guide/)
to boot the image.

## Build SDK

Building VisionFive2 SDK in a container.

First, build a docker image:

```sh
docker build . -t visionfive2-builder
```

Next, run the docker container with
```sh
docker run --rm -it -u "$(id -u):$(id -g)" -v "$PWD/:/wd/" -w /wd/ visionfive2-builder bash
```

And perform the following steps, running commands in the container shell:
 
```sh

# 1. Clone the repository
git clone --recursive --no-single-branch --branch JH7110_VisionFive2_devel \
  https://github.com/starfive-tech/VisionFive2.git

# 2. Examine the clone errors. If there are problems checking out files with
# git LFS, go to the corresponding submodule and fix the check out manually,
# downloading the necessary LFS files from the GitHub web interface.

# 3. Create a branch in the linux/ submodule
pushd VisionFive2/linux/
git checkout -t origin/JH7110_VisionFive2_devel
popd

# 4. Build firmware
cd VisionFive2/
make -j"$(nproc)"
```

The image with OpenSBI and the u-boot bootloader is built, and can then be flashed
to the SD-card. If using the pre-built distro or an image built with this SDK, the
image should be written to partition 2.

## Running custom bare-metal code instead of the bootloader

Having built the SDK, perform the following steps (hello_world_bare.S is used as an example).

Step 0. Using the SDK and the docker container, build the image as usual with `make`
(in fact, `make uboot` is sufficient).

Step 1. Build a static executable that will be your binary:

```sh
$ # 1.1. Compile the object file:
$ VisionFive2/work/buildroot_initramfs/host/bin/riscv64-buildroot-linux-gnu-as hello_world_bare.S -o hello_world_bare.o
$ # 1.2. Link a static executable
$ VisionFive2/work/buildroot_initramfs/host/bin/riscv64-buildroot-linux-gnu-ld --no-dynamic-linker -static -nostdlib -o hello_world_bare -s hello_world_bare.o
```

Step 2. Slip our executable into the build process and have it build the image.

```sh
$ # 2. Copy your executable to replace the u-boot binary file.
$ #    Note the copy via `objcopy` with output format `binary`. ELF metadata is discarded.
$ #    The command below is based on a command that is run in the `u-boot` build process (at the time of writing).
$ VisionFive2/work/buildroot_initramfs/host/bin/riscv64-linux-objcopy --gap-fill=0xff -O binary hello_world_bare VisionFive2/work/u-boot/u-boot.bin
```

Step 3. Finally, go back to the SDK container and build the image (`make -j$(nproc)`).

Verify the success injection of your code into the image by running (in `VisionFive2/`):
```sh
$ strings work/visionfive2_fw_payload.img | grep Hello
# Should display the string containing 'Hello' from your executable
```
