#!/bin/bash

# Build dylib for x86_64.
mkdir build_x86_64 && cd build_x86_64
../configure --host=x86_64-apple-darwin --disable-static --enable-shared \
            CFLAGS="-arch x86_64 -mmacosx-version-min=10.15" \
            LDFLAGS="-arch x86_64"
make
cp lib/.libs/libcurl.4.dylib ../libcurl_x86_64.dylib
cd ..

# Build dylib for arm64.
mkdir build_arm64 && cd build_arm64
../configure --host=arm64-apple-darwin --disable-static --enable-shared \
            CFLAGS="-arch arm64 -mmacosx-version-min=10.15" \
            LDFLAGS="-arch arm64"
make
cp lib/.libs/libcurl.4.dylib ../libcurl_arm64.dylib
cd ..

# Combine the above two dylibs.
lipo -create libcurl_x86_64.dylib libcurl_arm64.dylib -output libcurl_universal

# Make a framework.
mkdir -p libcurl.framework/Versions/A/Headers
mkdir -p libcurl.framework/Versions/A/Resources
cp libcurl_universal libcurl.framework/Versions/A/libcurl
cp -R include/curl/*.h libcurl.framework/Versions/A/Headers/
cd libcurl.framework
ln -s Versions/A/libcurl libcurl
ln -s Versions/A/Headers Headers
ln -s Versions/A/Resources Resources
ln -s A Versions/Current
cd ..

# Clear build cache.
rm -rf build_x86_64 build_arm64
rm -f libcurl_x86_64.dylib libcurl_arm64.dylib
rm -f libcurl_universal
