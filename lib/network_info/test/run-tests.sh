#!/bin/bash

TESTDIR="$(dirname -- "${0}")"
cd $TESTDIR

cabal configure && cabal build && ./dist/build/network-info-test/network-info-test
