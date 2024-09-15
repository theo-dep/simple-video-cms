#!/bin/bash

SCRIPT_DIR=$(dirname -- "$( readlink -f -- "$0"; )")
SOURCE_DIR=${SCRIPT_DIR}/..

export SERVER_URL="localhost:5000"
(cd ${SOURCE_DIR}/front && ./server) >${SCRIPT_DIR}/front.log 2>&1
