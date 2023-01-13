#!/bin/sh

eval "$(/home/linuxbrew/.linuxbrew/bin/brew shellenv)"

## Download dependencies
cd recorder/bin

# scons
brew list scons || brew install scons

# openh264
wget http://ciscobinary.openh264.org/libopenh264-2.3.1-linux64.7.so.bz2
bzip2 -dv libopenh264-2.3.1-linux64.7.so.bz2
rm -v libopenh264-2.3.1-linux64.7.so.bz2 || true
mv -v libopenh264-*.so libopenh264.so

# ffmpeg stuff
brew list ffmpeg || brew install ffmpeg
FFMPEG_LIB_PATH=$(brew --prefix ffmpeg)/lib
cp -v $FFMPEG_LIB_PATH/libavcodec.so .
cp -v $FFMPEG_LIB_PATH/libavformat.so .
cp -v $FFMPEG_LIB_PATH/libavutil.so .
cp -v $FFMPEG_LIB_PATH/libswresample.so .
cp -v $FFMPEG_LIB_PATH/libswscale.so .

## Build

# Get api.json
cd ../../godot-cpp/godot-headers
wget -O api.json https://gist.github.com/naturecodevoid/0cb1403491d9a44cc3c27498ce6cedc0/raw/09d02e418de05356ff0500d16a14aa4a1794c1cf/godot-3.5.1-api.json

# Build godot-cpp
cd ..
scons platform=linux target=release generate_bindings=yes

# Build recorder
cd ..
scons platform=linux

# Create zip
rm -v recorder.zip || true
zip -r recorder recorder
