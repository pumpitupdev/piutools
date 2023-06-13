#!/usr/bin/env bash

set -euo pipefail

BUILDENV_IMG_NAME=${1:-piutools_buildenv}
BUILDENV_IMG_VERSION=${2:-latest}

repo_name="${BUILDENV_IMG_NAME}:${BUILDENV_IMG_VERSION}"

GID="$(id -g ${UID})"

docker build -t ${repo_name} .
docker run --rm -v $PWD:/piutools \
    -e "CFLAGS=${CFLAGS:-""}" \
    -e "MY_UID=${UID}" \
    -e "MY_GID=${GID}" \
    ${repo_name} make
