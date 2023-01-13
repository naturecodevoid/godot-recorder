#!/bin/sh

## Download dependencies
cd recorder/bin

# scons
brew list scons || brew install scons

# openh264
wget http://ciscobinary.openh264.org/libopenh264-2.3.1-mac-x64.dylib.bz2
bzip2 -dv libopenh264-2.3.1-mac-x64.dylib.bz2
rm -v libopenh264-2.3.1-mac-x64.dylib.bz2 || true
mv -v libopenh264-*.dylib libopenh264.dylib
install_name_tool -id recorder/bin/libopenh264.dylib libopenh264.dylib

# ffmpeg stuff
brew list ffmpeg || brew install ffmpeg
FFMPEG_LIB_PATH=$(brew --prefix ffmpeg)/lib

cp -v $FFMPEG_LIB_PATH/libavcodec.dylib .
install_name_tool -id recorder/bin/libavcodec.dylib libavcodec.dylib

cp -v $FFMPEG_LIB_PATH/libavformat.dylib .
install_name_tool -id recorder/bin/libavformat.dylib libavformat.dylib

cp -v $FFMPEG_LIB_PATH/libavutil.dylib .
install_name_tool -id recorder/bin/libavutil.dylib libavutil.dylib

cp -v $FFMPEG_LIB_PATH/libswresample.dylib .
install_name_tool -id recorder/bin/libswresample.dylib libswresample.dylib

cp -v $FFMPEG_LIB_PATH/libswscale.dylib .
install_name_tool -id recorder/bin/libswscale.dylib libswscale.dylib

## Build

# Get api.json
cd ../../godot-cpp/godot-headers
wget -O api.json https://gist.github.com/naturecodevoid/0cb1403491d9a44cc3c27498ce6cedc0/raw/09d02e418de05356ff0500d16a14aa4a1794c1cf/godot-3.5.1-api.json

# Build godot-cpp
cd ..
scons platform=osx target=release generate_bindings=yes

# Build recorder
cd ..
scons platform=osx

# Create zip
rm -v recorder.zip || true
zip -r recorder recorder
