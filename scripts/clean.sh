#!/bin/bash

SCRIPT_DIR=$(dirname -- "$( readlink -f -- "$0"; )")
SOURCE_DIR=${SCRIPT_DIR}/..

docker stop vietvodao-videohub-mysql
docker rm vietvodao-videohub-mysql

make -C ${SOURCE_DIR}/back clean
make -C ${SOURCE_DIR}/front clean

find ${SOURCE_DIR} -name "*.o" -delete -print
find ${SOURCE_DIR} -name "*.log" -delete -print
