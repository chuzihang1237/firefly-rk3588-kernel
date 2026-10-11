#!/bin/bash

DEV_NAME=$(cat "/proc/device-tree/model")
DTB_NAME=$(realpath /boot/rk-kernel.dtb | awk -F '/' '{print $NF}')

echo "DEVICE NAME: $DEV_NAME"
echo "DTB NAME   : $DTB_NAME"

if uname -m | grep -q aarch64 ;then
        DEV_ARCH=arm64
        DTB_NAME=rockchip/$DTB_NAME
else
        DEV_ARCH=armhf
fi

mkdir -p arch/$DEV_ARCH/configs/
cp .config arch/$DEV_ARCH/configs/firefly_defconfig

make ARCH=$DEV_ARCH firefly_defconfig
make ARCH=$DEV_ARCH $DTB_NAME

if [ "$1" == "-s" ];then
        mv arch/$DEV_ARCH/boot/dts/$DTB_NAME /boot/
else
        mv arch/$DEV_ARCH/boot/dts/$DTB_NAME ./
fi
