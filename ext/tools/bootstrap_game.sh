#!/usr/bin/env bash

# Deployment Script for Pump it Up Classic Via PIUTools
cat $PIUTOOLS_PATH/tools/pump_arrows.graphic
#cat $PIUTOOLS_PATH/am.graphic
#cat $PIUTOOLS_PATH/pumplogo.graphic
# Set PIUTools Stuff
mkdir $PIUTOOLS_TMP_PATH

# Read the list of gamedata roms and mount each.
IFS=',' read -ra roms_array <<< "$PIUTOOLS_ROMS"
for rom in "${roms_array[@]}"; do
  mnt="$PIUTOOLS_MNT_PATH/$(basename "$rom" .sqfs)"
  $PIUTOOLS_PATH/tools/mount_rom.sh "$PIUTOOLS_ROM_PATH/$rom"
done

# Check for a libs.sqfs in our rom directory, mount this if it exists
lib_sqfs_path=$PIUTOOLS_ROM_PATH/libs.sqfs
lib_mnt_path="$PIUTOOLS_MNT_PATH/libs"
if [ -f "$lib_sqfs_path" ]; then
    mkdir -p "$lib_mnt_path"    
    squashfuse "$lib_sqfs_path" "$lib_mnt_path"
    cp -r --symbolic-link --remove-destination $lib_mnt_path/* /
fi

# Handle Persistence Directories
rw_sqfs_path=$PIUTOOLS_ROM_PATH/rw.sqfs
rw_mnt_path="$PIUTOOLS_MNT_PATH/rw"
# If our save path is empty and we have a rw.sqfs template, mount it and 
# copy the contents out
if [ -z "$(ls -A $PIUTOOLS_SAVE_PATH)" ] && [ -f "$rw_sqfs_path" ]; then
    # Mount the squashfs image containing the read/write directories
    mkdir -p "$rw_mnt_path"
    squashfuse "$rw_sqfs_path" "$rw_mnt_path"
    # Copy our template files to our save path
    cp -R $rw_mnt_path/* $PIUTOOLS_SAVE_PATH/
fi

# Overlay the contents of our save path into root
for item in "$PIUTOOLS_SAVE_PATH"/*; do
    # Check if the item is a file
    if [ -f "$item" ]; then
        # Symlink the file to the root directory
        ln -sf "$item" "/$(basename "$item")"
    # Check if the item is a directory
    elif [ -d "$item" ]; then
        # Symlink the directory to the root directory
        ln -sfn "$item" "/$(basename "$item")"
    fi
done

# Change to our execution path.
cd $PIUTOOLS_GAME_DIR

# If we have a library directory, we'll add it to our library path.
if [ -d "$PIUTOOLS_ROM_PATH/libs" ]; then
    export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$PIUTOOLS_ROM_PATH/libs
fi

# Runs a IDA remote debug server.
if [ -n "$RUN_IDA_DBG_SERVER" ]; then
	$PIUTOOLS_PATH/tools/linux_server64 &
fi

# Based on the command, we have a few execution options.
if [ -n "$RUN_GDB" ]; then
  unset LD_PRELOAD
  gdb -ex "set env LD_PRELOAD=$PIUTOOLS_PATH/piutools.so" -ex run --args $PIUTOOLS_EXE_PATH $PIUTOOLS_EXE_ARGS
elif [ -n "$RUN_STRACE" ]; then
  LD_PRELOAD=$PIUTOOLS_PATH/piutools.so exec strace -o $PIUTOOLS_PATH/piutools_strace.txt $PIUTOOLS_EXE_PATH $PIUTOOLS_EXE_ARGS
elif [ -n "$RUN_LTRACE" ]; then
  LD_PRELOAD=$PIUTOOLS_PATH/piutools.so exec ltrace -e '*' -o $PIUTOOLS_PATH/piutools_ltrace.txt $PIUTOOLS_EXE_PATH $PIUTOOLS_EXE_ARGS
elif [ -n "$RUN_GAME" ]; then
  LD_PRELOAD=$PIUTOOLS_PATH/piutools.so exec script -q -c "$PIUTOOLS_EXE_PATH $PIUTOOLS_EXE_ARGS"
  #LD_PRELOAD=$PIUTOOLS_PATH/piutools.so exec $PIUTOOLS_EXE_PATH $PIUTOOLS_EXE_ARGS
else
  unset LD_PRELOAD
  exec /bin/bash
fi