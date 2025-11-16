# As of writing, the quick start guide recommends compiling on Ubuntu {16,18,20,22}.04.
# I tried compiling on Arch Linux and got lot's of compile errors
FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt update && apt install -y                                    \
    build-essential automake libtool texinfo                        \
    bison flex gawk g++ git xxd curl wget gdisk                     \
    gperf cpio bc screen texinfo unzip libgmp-dev                   \
    libmpfr-dev libmpc-dev libssl-dev libncurses-dev libglib2.0-dev \
    libpixman-1-dev libyaml-dev patchutils python3-pip zlib1g-dev   \
    device-tree-compiler dosfstools mtools kpartx rsync             \
    vim bash-completion
RUN ["/bin/bash", "-c", "eval \"$(curl -s https://packagecloud.io/install/repositories/github/git-lfs/script.deb.sh)\""]
RUN ["apt", "install", "-y", "git-lfs"]
