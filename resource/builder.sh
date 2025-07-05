#!/bin/sh

set -e

export CMAKE_TOOLCHAIN_FILE=$VCPKG_INSTALLATION_ROOT/scripts/buildsystems/vcpkg.cmake

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release || cat build/vcpkg_installed/vcpkg/issue_body.md

cmake --build build --target package --config Release

pushd build
FILE=$(ls comic_reader-*.zip)
if [ "$RUNNER_OS" = "Linux" ]; then
	rm -rf AppDir

	export APPIMAGE_EXTRACT_AND_RUN=1
	export NO_STRIP=true
	export UPDATE_INFORMATION='gh-releases-zsync|Suraj-Yadav|comic_reader|latest|Comic_Reader-*x86_64.AppImage.zsync'

	NAME="${FILE%.*}"
	unzip $FILE
	mv $NAME AppDir

	pushd /tmp
	wget -c https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
	chmod +x linuxdeploy-*.AppImage
	popd

	/tmp/linuxdeploy-*.AppImage --appdir AppDir --desktop-file ../resource/comic_reader.desktop --icon-file ../resource/comic_reader.png -e AppDir/bin/comic_reader --output appimage
else
	mv $FILE comic_reader-win64.zip
fi
popd
