#!/bin/bash

SCRIPT_DIR=$(dirname -- "$( readlink -f -- "$0"; )")
SOURCE_DIR=${SCRIPT_DIR}/..

docker run -d \
    --name videohub-mysql \
    -p 3306:3306 \
    -e MYSQL_DATABASE=video \
    -e MYSQL_ROOT_PASSWORD=6y51^HsXrNcx \
    mysql:9.0.1
