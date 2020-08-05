#!/bin/sh

DIST_NAME="arcin-inf-$(date +%Y)-$(date +%m)-$(date +%d).zip"

set -e
rm -rf ./arcin.elf
mkdir -p ./bin
rm -rf ./bin/*
scons
./arcin-utils/hidloader_append.py ./arcin.elf arcin-utils/hidloader_v2.exe ./bin/arcin_new_infinitas_1000hz.exe
mkdir -p ./pkg
rm -rf ./pkg/*
cp COPYING ./pkg/license.txt
cp readme.txt ./pkg
cp ./bin/* ./pkg
mkdir -p ./dist
pushd ./pkg
zip -qrXT9 ../dist/${DIST_NAME} * -z <<< "$DIST_NAME"
popd