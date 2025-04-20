#!/bin/sh

set -e

export CMAKE_TOOLCHAIN_FILE=$VCPKG_INSTALLATION_ROOT/scripts/buildsystems/vcpkg.cmake

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

cmake --build build --target package --config Release

pushd build
if [ "$RUNNER_OS" = "Linux" ]; then
	FILE=$(ls comic_reader-*.zip)

	rm -rf AppDir

	export APPIMAGE_EXTRACT_AND_RUN=1
	export NO_STRIP=true

	NAME="${FILE%.*}"
	unzip $FILE
	mv $NAME AppDir

	wget https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
	chmod +x linuxdeploy-*.AppImage
	./linuxdeploy-*.AppImage --appdir AppDir --desktop-file ../resource/comic_reader.desktop --icon-file ../resource/comic_reader.png -e AppDir/bin/comic_reader --output appimage
	mv Comic_Reader-x86_64.AppImage comic_reader.AppImage
else
	mv $FILE comic_reader-win64.zip
fi
popd
