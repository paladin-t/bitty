#!/bin/sh

ARCH=$(arch)
BASEDIR=$(dirname $0)
SUBDIR=""
if [ "$ARCH" = "x86_64" ]; then
  SUBDIR="x64"
else
  SUBDIR="x86"
fi

if [ -d "$BASEDIR/lib/.libs" ]; then
  cp -f "$BASEDIR/lib/.libs/libcurl.a" "$BASEDIR/$SUBDIR/"
  cp -f "$BASEDIR/lib/.libs/libcurl.la" "$BASEDIR/$SUBDIR/"
  cp -f "$BASEDIR/lib/.libs/libcurl.lai" "$BASEDIR/$SUBDIR/"
  cp -f "$BASEDIR/lib/.libs/libcurl.so" "$BASEDIR/$SUBDIR/"
  cp -f "$BASEDIR/lib/.libs/libcurl.so.4" "$BASEDIR/$SUBDIR/"
  cp -f "$BASEDIR/lib/.libs/libcurl.so.4.7.0" "$BASEDIR/$SUBDIR/"
fi

echo Ok.
