#!/bin/bash

SCRIPT_DIR=$(dirname -- "$( readlink -f -- "$0"; )")
SOURCE_DIR=$(realpath ${SCRIPT_DIR}/..)

TAR_FILE="backup.tar.gz"

VOLUME="$(basename ${SOURCE_DIR})_data"
if ! docker volume inspect ${VOLUME} > /dev/null 2>&1; then
  echo "${VOLUME} volume does not exist"
  exit 1
fi

CONTAINER_ID=$(docker create -v ${VOLUME}:/volume --name temp_container busybox)
if [ -z "${CONTAINER_ID}" ]; then
  echo "Fail to create the temporary backup container"
  exit 1
fi

docker run --rm --volumes-from temp_container -v ${SOURCE_DIR}:/backup busybox tar czf /backup/${TAR_FILE} -C /volume .
docker rm temp_container > /dev/null 2>&1

echo "Volume saved from ${VOLUME} to ${SOURCE_DIR}/${TAR_FILE}"
