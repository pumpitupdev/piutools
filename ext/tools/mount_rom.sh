#!/bin/bash

# Check that the script was called with one argument
if [ "$#" -ne 1 ]; then
  echo "Usage: $0 image_file"
  exit 1
fi

# Create a directory for the mount point with the name of the image file (without the extension)
mnt="$PIUTOOLS_MNT_PATH/$(basename "$1" .sqfs)"

mkdir -p "$mnt"

# Mount the image file using squashfuse
squashfuse "$1" "$mnt"

cp -r --symbolic-link --remove-destination $mnt/* /