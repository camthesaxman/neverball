#!/bin/bash
#
# Builds the Android APK
#

# Directory containing this script
THISDIR=$(dirname "$0")


# Sanity check for Android SDK and NDK environment variables
if [ -z "$ANDROID_HOME" ]
then
    echo 'error: environment variable ANDROID_HOME is not set'
    exit 1
fi
if [ -z "$ANDROID_NDK_HOME" ]
then
    echo 'error: environment variable ANDROID_NDK_HOME is not set'
    exit 1
fi

# Symlink data into assets
if [ ! -d "$THISDIR/app/src/main/assets/data" ]
then
    pushd "$THISDIR/app/src/main/assets"
    ln -vs ../../../../../data data
    popd
fi

# Download required libraries

pushd "$THISDIR/app/jni"

if [ ! -d SDL2-2.0.10 ]
then
    echo 'Downloading SDL2...'
    wget https://www.libsdl.org/release/SDL2-2.0.10.tar.gz -O SDL2-2.0.10.tar.gz
    tar xf SDL2-2.0.10.tar.gz
fi

if [ ! -d SDL2_ttf-2.0.15 ]
then
    echo 'Downloading SDL2_ttf...'
    wget https://www.libsdl.org/projects/SDL_ttf/release/SDL2_ttf-2.0.15.tar.gz -O SDL2_ttf-2.0.15.tar.gz
    tar xf SDL2_ttf-2.0.15.tar.gz
fi

if [ ! -d libpng-android ]
then
    echo 'Downloading libpng-android...'
    git clone https://github.com/julienr/libpng-android.git libpng-android
fi

if [ ! -d libjpeg-turbo ]
then
    echo 'Downloading libjpeg-turbo...'
    git clone git://git.linaro.org/people/tomgall/libjpeg-turbo/libjpeg-turbo.git -b android-95ed21e84965f859da0792558742f9cbf9d4ac7a libjpeg-turbo
    # Not sure what this 'cutils' thing is. Remove it since it's not needed.
    sed -e '/LOCAL_SHARED_LIBRARIES := libcutils/d' -i libjpeg-turbo/Android.mk
fi

if [ ! -d libvorbis-1.3.6 ]
then
    echo 'Downloading libvorbis...'
    wget https://downloads.xiph.org/releases/vorbis/libvorbis-1.3.6.tar.xz -O libvorbis-1.3.6.tar.xz
    tar xf libvorbis-1.3.6.tar.xz
fi

if [ ! -d libogg-1.3.4 ]
then
    echo 'Downloading libogg...'
    wget http://downloads.xiph.org/releases/ogg/libogg-1.3.4.tar.gz -O libogg-1.3.4.tar.gz
    tar xf libogg-1.3.4.tar.gz
fi

popd

# Build data
cd "$THISDIR/.." && make sols

# Build app
export APP_PLATFORM='android-14'
cd "$THISDIR" && ./gradlew assembleDebug