#!/bin/bash

set -ex

if [ "$#" -ne 1 ]; then
    echo "Pass the target you want to build for"
    exit 1
fi
target="$1"

./setup_third_party_libs.sh $target

flutter precache
flutter test
flutter build $target --release

if [[ "$target" == "windows" ]]; then
    7z a ComicReaderWin.zip ./build/windows/runner/Release/*
elif [[ "$target" == "macos" ]]; then
    pushd build/macos/Build/Products/Release/
    zip -r ComicReaderMac.zip "Comic Reader.app"
    popd
fi
