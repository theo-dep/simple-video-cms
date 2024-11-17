#!/bin/bash

SCRIPT_DIR=$(dirname -- "$( readlink -f -- "$0"; )")
SOURCE_DIR=${SCRIPT_DIR}/..

COMMON_DIR=${SOURCE_DIR}/builder
CXXFLAGS="-DDEBUG_LOG -I${COMMON_DIR}/third-party -I${COMMON_DIR}"

pushd ${SOURCE_DIR}
declare -a dirs=("builder" "back" "front")
for dir in "${dirs[@]}"
do
    echo Analyze ${dir}...
    make -C ${dir} -j -k analyze CXXFLAGS="${CXXFLAGS}"
done
popd
