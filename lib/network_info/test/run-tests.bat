@echo off
cabal configure && cabal build && .\dist\build\network-info-test\network-info-test.exe
