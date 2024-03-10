#!/bin/sh

ARCH=$(arch)
BASEDIR=$(dirname $0)
SUBDIR=""
if [ "$ARCH" = "x86_64" ]; then
  SUBDIR="x64"
else
  SUBDIR="x86"
fi

if [ -d "/usr/local/lib" ]; then
  cp -f "/usr/local/lib/libuv.a" "$BASEDIR/lib/$SUBDIR/"
  cp -f "/usr/local/lib/libuv.la" "$BASEDIR/lib/$SUBDIR/"
  cp -f "/usr/local/lib/libuv.so.1" "$BASEDIR/lib/$SUBDIR/"
fi

echo Ok.
