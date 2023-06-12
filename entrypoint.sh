#!/usr/bin/env bash

set -euo pipefail

MY_UID="${MY_UID:-"0"}"
MY_GID="${MY_GID:-"0"}"

# maintain permissions of calling user
if [ "x$MY_UID" != "x0" ]; then
    groupadd -g $MY_GID somegroup
    useradd -g $MY_UID someuser
    echo "running as sudo ($MY_UID:$MY_GID): $@"
    env
    sudo --preserve-env=CFLAGS -g somegroup -u someuser "$@"
else
    exec "$@"
fi
