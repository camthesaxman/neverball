#!/bin/sh
#
# Removes all Android build artifacts
#

# Directory containing this script
THISDIR=$(dirname "$0")

# Delete build artifacts
rm -rf \
    "$THISDIR/app/.externalNativeBuild" \
    "$THISDIR/app/build" \

# Delete libraries
cd "$THISDIR/app/jni" && find . ! -name 'Android.mk' -exec rm -f {} + \;
