#!/bin/bash

set -ex

if [ "$#" -ne 1 ]; then
    echo "Pass the target you want to build for"
    exit 1
fi
target="$1"

folder="./assets/third_party"
mkdir -p "$folder"

if [[ "$target" == "windows" ]]; then
    url='http://www.libarchive.org/downloads/libarchive-v3.6.2-amd64.zip'
    if ! [ -d "$folder/libarchive" ]; then
        curl -o "$folder/libarchive.zip" "$url"
        7z x "$folder/libarchive.zip" -o"$folder" -y
        rm -rf "$folder/libarchive.zip"
    fi
elif [[ "$target" == "macos" ]]; then
    git clone --depth 1 --branch v3.6.2 https://github.com/libarchive/libarchive.git
    cd libarchive
    cmake . -DENABLE_WERROR=OFF -DENABLE_TEST=OFF -DENABLE_CAT=OFF -DENABLE_TAR=OFF -DENABLE_CPIO=OFF -DCMAKE_INSTALL_PREFIX=../assets/third_party/libarchive -DCMAKE_BUILD_TYPE=Release
    make install -j $(nproc)
    find ../assets/third_party/libarchive
fi
