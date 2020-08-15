#!/bin/sh

DIST_NAME="arcin-new-infinitas-conf-$(date +%Y)-$(date +%m)-$(date +%d).zip"

set -e
rm -rf ./arcin.elf

mkdir -p ./bin
rm -rf ./bin/*
mkdir -p ./bin_dbg
rm -rf ./bin_dbg/*

export ARCIN_INFINITAS_250HZ_MODE=0

scons
./arcin-utils/hidloader_append.py ./arcin.elf arcin-utils/hidloader_v2.exe ./bin/arcin_infinitas_new_1000hz.exe
./arcin-utils/hidloader_append_dev.py ./arcin.elf arcin-utils/hidloader_v2.exe ./bin_dbg/arcin_infinitas_new_1000hz_upgrade.exe

export ARCIN_INFINITAS_250HZ_MODE=1

scons
./arcin-utils/hidloader_append.py ./arcin.elf arcin-utils/hidloader_v2.exe ./bin/arcin_infinitas_new_250hz.exe
./arcin-utils/hidloader_append_dev.py ./arcin.elf arcin-utils/hidloader_v2.exe ./bin_dbg/arcin_infinitas_new_250hz_upgrade.exe

mkdir -p ./pkg
rm -rf ./pkg/*

cp COPYING ./pkg/license.txt
cp readme.txt ./pkg
cp ./bin/* ./pkg
cp ../arcin-conf/arcin-infinitas-conf/dist/arcin_conf_infinitas.exe ./pkg

mkdir -p ./dist

pushd ./pkg
zip -qrXT9 ../dist/${DIST_NAME} * -z <<< "$DIST_NAME"
popd