#!/bin/bash

# Check that the script was called with two arguments
if [ "$#" -ne 2 ]; then
  echo "Usage: $0 image_file.sqfs /path/to/data"
  exit 1
fi


# Create the SquashFS image file with the data directory
mksquashfs "$2" "$1" -noappend -comp lzo

echo "Done!"
