#!/bin/sh
set -euo pipefail

# Runs the bare loader for debugging purposes

usage() {
    echo "Usage: $0 <game name> <game version> <command> [command args]"
    echo "e.g.:"
    echo -e "\t$0 pro1 r5 ./piu"
    echo -e "\t$0 nx 108 gdb ./piu"
    echo -e "\t$0 nx2 120 strace -e open ./piu"
    exit 2
}

[[ $# -lt 2 ]] && usage

PIUTOOLS_GAME_NAME="$1"
shift
PIUTOOLS_GAME_VERSION="$1"
shift

export DBGLOG=1
export PIUTOOLS_GAME_NAME PIUTOOLS_GAME_VERSION
export PIUTOOLS_PATH="$(readlink -f $(dirname $0)/..)/build"
export PIUTOOLS_EXT_PATH="$(readlink -f $(dirname $0)/..)/ext"
export PIUTOOLS_CONFIG_PATH="${PIUTOOLS_EXT_PATH}/config"
export PIUTOOLS_SAVE_PATH="${PIUTOOLS_PATH}/save"
export PIUTOOLS_PLUGIN_PATH="${PIUTOOLS_PATH}/plugins"
export LD_LIBRARY_PATH="${LD_LIBRARY_PATH:-""}:${PIUTOOLS_PATH}/libs"
LD_PRELOAD="${PIUTOOLS_PATH}/piutools.so" "$@"

