# Experiments with the Visionfive2 board

## Context and miscellaneous info

### What is this document?

Here I document how to do various things with the StarFive VisionFive 2 board.
Subtitles in this README are actions you might want to perform, and the sections
describe how to do it.

I wrote this with the primary goal to just document my experiments, so that it's
stored somewhere or in case I need it in the future. But maybe you can find something
useful here too.

This text complements the official documentation but, unfortunately, cannot include
information from it for licensing reasons.

### Boot mode

You can select the boot mode according to the
[documentation](https://doc-en.rvspace.org/VisionFive2/Quick_Start_Guide/VisionFive2_SDK_QSG/boot_mode_settings.html).

Most of the document below uses the SDIO boot mode.

### Communicating with the board

The most low-level communication interface supported by the board is UART. One way to
communicate with it is to turn your computer into a UART terminal with a UART-USB adaptor
(a cheap piece of hardware, which is not included with the board).

Connect your UART-enabled device to the board according to the
[pinout diagram](https://doc-en.rvspace.org/VisionFive2/Quick_Start_Guide/VisionFive2_QSG/pinout_diagram%20-%20vf2.html)
or as demonstrated in the
[Recovering the Bootloader](https://doc-en.rvspace.org/VisionFive2/Quick_Start_Guide/VisionFive2_SDK_QSG/recovering_bootloader%20-%20vf2.html)
section of the official documentation.

To communicate with the board from a Linux device (e.g., when connected with an adapter),
use a utility such as `cu` (e.g., the `cu` package on Ubuntu or `uucp` on Arch Linux).
Configure appropriate permissions (add the current user to the group that owns the
corresponding device on `/dev/`) and launch the command, e.g.:
```sh
$ cu --line /dev/ttyUSB0 -s 115200
```

(use the appropriate baud rate, it is 115200 for my board)

## Booting the board with a pre-built distro

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

With the above, a bunch of things are built, including OpenSBI and U-Boot. They can
be flashed on an SDK. If you already have the pre-built distro or an image build with
this SDK on the SD card, the bootloader image can be written to partition 2, e.g.
(specify correct path and device number!):
```sh
dd if=.../VisionFive2/work/visionfive2_fw_payload.img of=/dev/mmcblkN2 \
  iflag=fullblock conv=notrunc,fsync oflag=direct status=progress bs=16M
```

To build the whole image, make the `sdimg` target (i.e., `make sdimg -j"$(nproc)"`),
which will create the `sdcard.img` file. Flash it onto your SD card, e.g. (specify
correct path and device number):
```sh
dd if=.../VisionFive2/work/sdcard.img of=/dev/mmcblkN \
  iflag=fullblock conv=notrunc,fsync oflag=direct status=progress bs=16M
```

## Running custom bare-metal code instead of the bootloader

Having built the SDK, perform the following steps (hello_world_bare.S is used as an example).

**Step 0**. Using the SDK and the docker container, build the image as usual with `make`
(in fact, `make uboot` is sufficient).

**Step 1**. Build a static executable that will be your binary:

```sh
$ # 1.1. Compile the object file:
$ VisionFive2/work/buildroot_initramfs/host/bin/riscv64-buildroot-linux-gnu-as hello_world_bare.S -o hello_world_bare.o
$ # 1.2. Link a static executable
$ VisionFive2/work/buildroot_initramfs/host/bin/riscv64-buildroot-linux-gnu-ld --no-dynamic-linker -static -nostdlib -o hello_world_bare -s hello_world_bare.o
```

**Step 2**. Slip our executable into the build process and have it build the image.

```sh
$ # 2. Copy your executable to replace the u-boot binary file.
$ #    Note the copy via `objcopy` with output format `binary`. ELF metadata is discarded.
$ #    The command below is based on a command that is run in the `u-boot` build process (at the time of writing).
$ VisionFive2/work/buildroot_initramfs/host/bin/riscv64-linux-objcopy --gap-fill=0xff -O binary hello_world_bare VisionFive2/work/u-boot/u-boot.bin
```

**Step 3**. Finally, go back to the SDK container and build the image (`make -j$(nproc)`).

---

Verify the success injection of your code into the image by running (in `VisionFive2/`):
```sh
$ strings work/visionfive2_fw_payload.img | grep Hello
# Should display the string containing 'Hello' from your executable
```

You may now boot the board with this image using one of the methods described in
sections above or below.

> [~~And as above so below...~~](https://youtu.be/JwtEOp7pC1A)

## Booting via Ethernet

U-boot allows to boot an image that is downloaded from the ethernet.

On your computer, launch a tftp server in the `work/` directory.
E.g., on my Arch machine with the `tftp-hpa` package:
```sh
sudo in.tftpd --ipv4 -L --address 0.0.0.0 --secure work/
```

<hr>

Boot the board in either flash mode (recommended by the vendor) or with an
SD card containing OpenSBI and U-Boot.

Connect the board to a router/switch/computer with an ethernet cable.

If you connected the board to a device that is _not_ running a DHCP server
(e.g., a typical computer), the network must be configured manually.
On your computer configure an ip address a subnet. E.g., via NetworkManager:
```sh
$ nmcli con add id "Wired manual" type eth
$ nmcli con edit "Wired manual"
nmcli> set ipv4.method manual
nmcli> set ipv4.addresses 10.0.0.13/24
nmcli> save
nmcli> activate
```
Then, in the U-Boot console:
```sh
setenv netmask 255.255.255.0
setenv ipaddr 10.0.0.3
setenv serverip 10.0.0.13
```
Note: the values `3` and `13` in ip addresses are arbitrary. The netmask must correspond
to the mask set in ipv4.addresses (`/24`). `serverip` must correspond to the address
set on the server.

If you connected the board to a device that _is_ running a DHCP server
(e.g., a router), the network will be configured automatically, and you only
need to set `serverip` to the address of your computer where tftp server is running.

<hr>

Next, in the U-boot console, load the necessary files from tftp into memory.

The standard way to do it is to use the [FIT file](https://docs.u-boot.org/en/v2024.04/usage/fit/source_file_format.html)
which can be built with the SDK (`make fit`). Load it to the default address
(stored in the `$loadaddr` environment variable) with:
```
tftpboot $loadaddr image.fit
```

Alternatively, you may separately load the flattened device tree (dtb) file, the kernel
image, and the initramfs image. To load them to their default addresses, according
to the environment, do:
```
# The dtb below is the default one, but others exist
tftpboot $fdt_addr_r linux/arch/riscv/boot/dts/starfive/jh7110-starfive-visionfive-2-wm8960.dtb
# This command downloads the compressed image format; omit `.gz` to download the
# uncompressed one.
tftpboot $kernel_addr_r linux/arch/riscv/boot/Image.gz
tftpboot $ramdisk_addr_r initramfs.cpio.gz
```

Note that you may load the files at different addresses, as long as the kernel
is properly aligned and all the loads are in the valid (not reserved) memory.

To check reserved memory information, run `bdinfo` in u-boot. Possible output:
```
boot_params = 0x0000000000000000
DRAM bank   = 0x0000000000000000
-> start    = 0x0000000040000000
-> size     = 0x0000000200000000
flashstart  = 0x0000000000000000
flashsize   = 0x0000000000000000
flashoffset = 0x0000000000000000
baudrate    = 115200 bps
relocaddr   = 0x00000000f7ef0000
reloc off   = 0x00000000b7cf0000
Build       = 64-bit
current eth = ethernet@16030000
ethaddr     = 6c:cf:39:00:83:a2
IP addr     = 10.0.0.3
fdt_blob    = 0x00000000f76d41f0
new_fdt     = 0x00000000f76d41f0
fdt_size    = 0x000000000000bbe0
Video       = dc8200@29400000 active
FB base     = 0x00000000fe000000
FB size     = 0x0x32
lmb_dump_all:
 memory.cnt  = 0x1
 memory[0]	[0x40000000-0x23fffffff], 0x200000000 bytes flags: 0
 reserved.cnt  = 0x1
 reserved[0]	[0x40000000-0x4005ffff], 0x00060000 bytes flags: 0
```
The last lines here describe the available memory and its reserved regions.

<hr>

Having loaded the kernel, run the following commands:
```
# (only if you loaded image.fit) Load/move stuff from image.fit
bootm start $loadaddr  # You may omit `$loadaddr`
# (only if you loaded image.fit) Prepare kernel (e.g., uncompress)
bootm loados $loadaddr  # You may omit `$loadaddr`
# Configure FDT address and memory + visionfive-specific stuff
run chipa_set_linux
# Set appropriate CPU voltage value as an FDT property
run cpu_vol_set
# Specify the partition to be used as root when linux boots.
# E.g., if using an SD card with SDK image or a pre-built distro image:
setenv sdev_blk mmcblk${devnum}p${rootpart}
# Add the root partition to the kernel cmd-line args
setenv bootargs $bootargs root=/dev/$sdev_blk
# Boot!
booti $kernel_addr_r $ramdisk_addr_r:$filesize $fdt_addr_r
```
where in the last command the appropriate addresses and the ramdisk image size
are provided. Note that the `filesize` environment variable is set by `tftpboot`
to the size of the last downloaded file on every run.

This boots the kernel!

### Wlan driver

There is an ESWIN wlan adapter that comes with the board. Its driver will not
load automatically in the system built from the SDK (unlike the pre-built distro image).
To be able to use the wlan adapter, when the system boots, run (as root):
```sh
$ insmod /lib/modules/5.15.0/kernel/drivers/net/wireless/eswin/wlan_ecr6600u_usb.ko
```
(the kernel version or the exact path to the driver may change in the future)

## Patching initramfs

To make a quick patch to initramfs (e.g., the init script), perform the following steps:

```sh
$ cd .../VisionFive2/work/  # Go to SDK dir /work
$ mkdir repack-initramfs && cd repack-initramfs
$ gunzip --keep ../initramfs.cpio.gz
$ sudo cpio < ../initramfs.cpio --extract --make-directories  # Needs root privileges to set correct file ownerships
$ # Modify files however you want. E.g., if you want to get a shell early in the init process:
$ vim init  # Modify `init`, e.g., as follows:
# 1. After /dev is mounted, add `exec < /dev/console > /dev/console 2>&1`
#    to ensure that stdin/stdout/stderr are through `/dev/console`, which corresponds
#    to the primary UART interface.
# 2. Add a line such as `/bin/sh` or `exec /bin/sh` to run the shell for manual intervention.
$ find . | cpio -H newc --create | gzip -9 --stdout > ../initramfs.cpio.gz  # Create the new initramfs archive
```

The file `initramfs.cpio.gz` now contains your patched initramfs, which you can load
following the steps described before. Note that `image.fit` was not updated, so loading
the initramfs file directly is necessary (see the instructions above).

## Boot the system via UART

### What is UART boot

Unlike other boot modes of the board, the UART boot mode does not read the bootloader
from any storage, but reads it right from the UART, expecting it to be transferred
via the XMODEM or YMODEM protocol. The received data is used as a temporary bootloader,
which is just executed without being stored permanently.

### How to send via X/Y/ZMODEM

To perform the boot, you will need to send files via the XMODEM and YMODEM protocols
a few times. You can do this by using `cu` along with an external XMODEM sending utility
(called `sx` in some distributions). For Arch Linux I used the `lrzsz-sx` utility from the
`lrzsz` package. You may use alternative tools, such as `minicom` (which, however, also
relies on external tools for X/Y/ZMODEM implementations).

When you're in `cu` and the other end is continuously sending the `C` character, it indicates
that it is ready for the transfer. To send a file, press Enter (to make sure that, from the
`cu`'s perspective, you are at a new line) and type a command such as the following
(ignore the `C`s that keep appearing as you type):
```shell
~+ lrzsz-sx --xmodem -vv path/to/file
```
where (meaning of options depends on the tool! the following is for lrzsz):

-   `~` is the default escape sequence of `cu`, allowing you to give a command to `cu`,
    rather than sending the typed text via the connection;

-   `+` means "run the following command, connecting both its stdin and stdout to the
    channel" (see the cu(1) manual page for details) (note that X/Y/ZMODEM require
    bidirectional communication to send one file in one direction);

-   `lrzsz-sx` is the name of your XMODEM sending tool (if you are using a different
    one, check out its `--help` or manual page, since it may provide different options!);

-   `--xmodem` to use XMODEM; `--ymodem` to use YMODEM. The tool is supposed to guess
    the protocol to be used based on the name you launch it with (e.g., lrzsz-s**x**) but,
    depending on your packager, it might not work right, and this feature is broken on
    Arch Linux, as of writing;

-   `-vv` (optional) regulates output verbosity, to see submission progress, put `-vv` in
    xmodem mode and `-vvv` in ymodem mode;

-   `path/to/file` the file that you want to send. Note: cu launches the command after `~+`
    via a shell, so watch out for characters in the path that need to be escaped.
    You may also use globbing.

Note: if you are having trouble with transmission via XMODEM, the discussion at
[starfive-tech/VisionFive2#13](https://github.com/starfive-tech/VisionFive2/issues/13)
might help.

## Steps

To boot the board via UART, perform the following.

**Step 0**. Connect to UART via `cu`
(see section [Communicating with the board](#communicating-with-the-board)).

**Step 1**. Boot the board in the UART boot mode (see section [Boot mode](#boot-mode)).
If booted correctly, you should see the `C` characters appearing repeatedly
on the UART connection.

**Step 2**. Send (as described above) the U-Boot SPL (Secondary Program Loader),
which is a small early bootloader that is used to load the main part of U-Boot
(you can learn more about U-Boot booting stages in its
[documentation](https://docs.u-boot.org/en/stable/usage/spl_boot.html)).
<br>
This corresponds to file `VisionFive2/work/u-boot-spl.bin.normal.out` in the built SDK.
<br>
Should be sent via XMODEM, although YMODEM works too in this stage for this file
(but not for all files - apparently, YMODEM is partially by the board's firmware).

**Step 3**. Once the upload is done, the board starts executing the SPL: it wants the following
u-boot binary, and will be ready to accept, which is indicated by `C` characters
starting to appear again. Then, send the OpenSBI+U-Boot image
<br>
It corresponds to the `VisionFive2/work/visionfive2_fw_payload.img` file in the built SDK.
<br>
Must be sent via YMODEM. A transfer via XMODEM succeeds, but then, surprisingly enough,
SPL fails to boot the received file. This behavior is consistent, and I have ensured that
it is not a transfer issue, since checksums are checked by the receiver. I see no explanation
for this other than a bug in SPL.

---

After the upload is complete, the board should proceed by printing the OpenSBI header
and bring up U-Boot console (attempts autoboot by default, unless a key is pressed...
but autoboot does not succeed in the UART mode).

**TODO:** Suggest steps to perform in U-Boot console

**TODO:** Can we directly upload `image.fit` (most likely, via YMODEM)?

## Updating the board's flash memory

Flashing bootloaders onto the board's built-in memory (flash).

In this section, we use the UART boot mode (see the previous section)
but instead of U-Boot we boot the official recovery tool, which acts
as a one-time bootloader, to update the contents of flash. The (closed-source) recovery
tool is provided in the
[official GitHub repository](https://github.com/starfive-tech/Tools/blob/master/recovery/).
You should probably use the latest version. Be careful, though: if your board is not
"devkits", presumably, you should use the regular "-recovery-" rather than
"-devkits-recovery-".

To update the bootloaders stored in flash, perform the following steps.

**Step 0**. Connect to UART via `cu`
(see section [Communicating with the board](#communicating-with-the-board)).

**Step 1**. Boot the board in the UART boot mode (see section [Boot mode](#boot-mode)).
If booted correctly, you should see the `C` characters appearing repeatedly
on the UART connection.

**Step 2**. Send the official recovery image via XMODEM, as described above.
Sending via YMODEM fails with transfer errors.
<br>
Note: if you are having trouble with transmission via XMODEM, the discussion at
[starfive-tech/VisionFive2#13](https://github.com/starfive-tech/VisionFive2/issues/13)
might help.

**Step 3**. Once the transfer is complete, the board loads and runs it. The output might
look something like:

```
JH7110 secondboot version: 230322-c514da9
CPU freq: 1250MHz
idcode: 0x1860C8
ddr 0x00000000, 4M test
ddr 0x00400000, 8M test
DDR clk 2133M, size 8GB

*********************************************************
****************** JH7110 program tool ******************
*********************************************************
0: update 2ndboot/SPL in flash
1: update 2ndboot/SPL in emmc
2: update fw_verif/uboot in flash
3: update fw_verif/uboot in emmc
4: update otp, caution!!!!
5: exit
NOTE: current xmodem receive buff = 0x40000000, 'load 0x********' to change.
select the function to test: 
```

Select 0 if you want to update the SPL (Secondary Program Loader),
an early bootloader that loads U-Boot (you can learn more about U-Boot booting
stages in its [documentation](https://docs.u-boot.org/en/stable/usage/spl_boot.html)).
This corresponds to file `VisionFive2/work/u-boot-spl.bin.normal.out` in the built SDK.

Select 2 if you want to update the main binary (U-Boot and OpenSBI).
This corresponds to file `VisionFive2/work/visionfive2_fw_payload.img` in the built SDK.
Note that you may also upload your modified bootloader with your custom bare-metal code,
as described in section
[Running custom bare-metal code instead of the bootloader](#running-custom-bare-metal-code-instead-of-the-bootloader).

As for other options, I never tried updating otp and did not have success with
updating partitions in eMMC.

**Step 4**. After selecting the partition to update, send the corresponding file via XMODEM.
YMODEM is not supported (the transfer will not begin).

**Step 5**. After the file transfer is complete, wait for the tool to write the file
to the corresponding partition. It will then report success and display the prompt again.

---

You may now switch to the flash boot mode and reboot the board.
