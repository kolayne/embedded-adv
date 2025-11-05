# Visionfive2

Playing around with Visionfive2.

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
