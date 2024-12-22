#!/bin/bash

SCRIPT_DIR=$(dirname -- "$( readlink -f -- "$0"; )")
SOURCE_DIR=${SCRIPT_DIR}/..

COMMON_DIR=${SOURCE_DIR}/builder
THIRD_PARTY_DIR=${COMMON_DIR}/third-party

gcc -g -Wall -Wextra -Werror -I${THIRD_PARTY_DIR} -c ${THIRD_PARTY_DIR}/sqlite3.c -o ${COMMON_DIR}/sqlite3.o
