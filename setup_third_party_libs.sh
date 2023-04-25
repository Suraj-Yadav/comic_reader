#!/bin/bash

set -ex

if [ "$#" -ne 1 ]; then
    echo "Pass the target you want to build for"
    exit 1
fi
target="$1"

folder="./prebuilt"
mkdir -p "$folder"

if [[ "$target" == "windows" ]]; then
    url='http://www.libarchive.org/downloads/libarchive-v3.6.2-amd64.zip'
    if ! [ -d "$folder/libarchive" ]; then
        curl -o "$folder/libarchive.zip" "$url"
        7z x "$folder/libarchive.zip" -o"$folder" -y
        rm -rf "$folder/libarchive.zip"
    fi
fi
