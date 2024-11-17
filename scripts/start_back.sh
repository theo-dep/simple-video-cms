#!/bin/bash

SCRIPT_DIR=$(dirname -- "$( readlink -f -- "$0"; )")
SOURCE_DIR=${SCRIPT_DIR}/..

(cd ${SOURCE_DIR}/back && ./server)
