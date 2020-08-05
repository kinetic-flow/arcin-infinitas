#!/bin/sh

DIST_NAME="arcin-new-infinitas-$(date +%Y)-$(date +%m)-$(date +%d).zip"

set -e
rm -rf ./arcin.elf
mkdir -p ./bin
rm -rf ./bin/*
export ARCIN_INF_SENSITIVE_TT=1
export ARCIN_INF_FLIP_START_SELECT=0
scons
./arcin-utils/hidloader_append.py ./arcin.elf arcin-utils/hidloader_v2.exe ./bin/arcin_new_infinitas_sensitive_tt.exe
export ARCIN_INF_SENSITIVE_TT=0
export ARCIN_INF_FLIP_START_SELECT=0
scons
./arcin-utils/hidloader_append.py ./arcin.elf arcin-utils/hidloader_v2.exe ./bin/arcin_new_infinitas_normal_tt.exe
export ARCIN_INF_SENSITIVE_TT=1
export ARCIN_INF_FLIP_START_SELECT=1
scons
./arcin-utils/hidloader_append.py ./arcin.elf arcin-utils/hidloader_v2.exe ./bin/arcin_new_infinitas_sensitive_tt_startsel_flipped.exe
export ARCIN_INF_SENSITIVE_TT=0
export ARCIN_INF_FLIP_START_SELECT=1
scons
./arcin-utils/hidloader_append.py ./arcin.elf arcin-utils/hidloader_v2.exe ./bin/arcin_new_infinitas_normal_tt_startsel_flipped.exe
mkdir -p ./pkg
rm -rf ./pkg/*
cp COPYING ./pkg/license.txt
cp readme.txt ./pkg
cp ./bin/* ./pkg
mkdir -p ./dist
pushd ./pkg
zip -qrXT9 ../dist/${DIST_NAME} * -z <<< "$DIST_NAME"
popd