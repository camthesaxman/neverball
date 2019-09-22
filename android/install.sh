#!/bin/sh
#
# Installs the APK onto a connected Android device
#

# Directory containing this script
THISDIR=$(dirname "$0")

adb install -r "$THISDIR/app/build/outputs/apk/debug/app-debug.apk"