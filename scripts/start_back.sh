#!/bin/bash

SCRIPT_DIR=$(dirname -- "$( readlink -f -- "$0"; )")
SOURCE_DIR=${SCRIPT_DIR}/..

export MYSQL_ROOT_PASSWORD="6y51^HsXrNcx"
#DB_IP=`docker inspect -f '{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' vietvodao-videohub-mysql`
DB_IP=127.0.0.1
export MYSQL_DB_URL=${DB_IP}
(cd ${SOURCE_DIR}/back && ./server) >${SCRIPT_DIR}/back.log 2>&1
