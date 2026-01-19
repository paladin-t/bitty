#!/bin/sh

ARCH=$(arch)
BASEDIR=$(dirname $0)
SUBDIR=""
if [ "$ARCH" = "x86_64" ]; then
  SUBDIR="x64"
else
  SUBDIR="x86"
fi

if [ -d "$BASEDIR/build/.libs" ]; then
  if [ ! -d "$BASEDIR/lib/$SUBDIR/" ]; then
    mkdir -p "$BASEDIR/lib/$SUBDIR/"
  fi
  cp -f "$BASEDIR/build/.libs/libSDL2_mixer.a" "$BASEDIR/lib/$SUBDIR/"
  cp -f "$BASEDIR/build/.libs/libSDL2_mixer.la" "$BASEDIR/lib/$SUBDIR/"
  cp -f "$BASEDIR/build/.libs/libSDL2_mixer-2.0.so.0.2.2" "$BASEDIR/lib/$SUBDIR/"
  cp -f "$BASEDIR/build/.libs/libSDL2_mixer-2.0.so.0" "$BASEDIR/lib/$SUBDIR/"
  cp -f "$BASEDIR/build/.libs/libSDL2_mixer.so" "$BASEDIR/lib/$SUBDIR/"
fi

echo Ok.
