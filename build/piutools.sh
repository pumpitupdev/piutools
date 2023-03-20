#!/usr/bin/env bash


rom_directory=$(realpath ./rom)

# Ensure the ROM path exists.
if [ ! -e "$rom_directory" ]; then
  echo "The ROM path does not exist."
  exit 1
fi

# Get the list of possible game names from the subdirectories of "config"
games=($(find config -maxdepth 1 -type d -printf "%f\n"))

# Check if an argument is provided
if [ "$#" -lt 1 ]; then
  # Print the list of game names and exit
  echo "Please provide a game as an argument. Available games:"
  for config in "${games[@]}"; do
    # Skip the "config" directory itself
    if [ "$config" != "config" ]; then
      echo "  * $config"
    fi
  done
  exit 1
else
  # Use the provided argument as the game name
  game_name="$1"
fi

# Get the list of possible game versions from the ".conf" files
versions=($(find "config/$game_name" -maxdepth 1 -type f -name "*.conf" -printf "%f\n" | sed 's/\.conf$//'))

# Check if a second argument is provided
if [ "$#" -lt 2 ]; then
  # Print the list of game versions and exit
  echo "Please provide a game version as a second argument. Available versions for \"$game_name\":"
  for version in "${versions[@]}"; do
    echo "  * $version"
  done
  exit 1
else
  # Use the provided argument as the game version
  game_version="$2"

  # Check if the provided game_version matches any available version
  version_found=false
  for version in "${versions[@]}"; do
    if [ "$version" == "$game_version" ]; then
      version_found=true
      break
    fi
  done

  if [ "$version_found" == "false" ]; then
    echo "The provided game version \"$game_version\" doesn't match any available version for \"$game_name\"."
    echo "Available versions:"
    for version in "${versions[@]}"; do
      echo "  * $version"
    done
    exit 1
  fi
fi

# Your script continues here with the game_name and game_version variables set

# Assuming you have the game_name variable set to the selected game
game_name_file="./config/$game_name/game_name"

# Check if the game_name file exists and read its content
if [ -f "$game_name_file" ]; then
    display_name=$(cat "$game_name_file")
else
    # Fallback to using the directory name if the file is not found
    display_name="$game_name"
fi

echo "~~~~~[PIUTools Loader for $display_name $game_version]~~~~~"

rom_game_root=$(realpath "$rom_directory/$game_name")

export PIUTOOLS_GAME_NAME=$game_name
export PIUTOOLS_GAME_VERSION=$game_version

# Set PIUTOOLS Path
export PIUTOOLS_PATH=$(realpath ".")
# Set PIUTOOLS Plugin Path
export PIUTOOLS_PLUGIN_PATH=$(realpath "./plugins")
# Set a config root to access config data.
export PIUTOOLS_CONFIG_PATH=$(realpath "./config")
# Set a save root to handle machine and user saves.
export PIUTOOLS_SAVE_PATH=$(realpath "./save")
# Set a tools ROM root to handle things like hdd files, dongle files, etc.
export PIUTOOLS_ROM_PATH=$(realpath "./rom")

# Almost Showtime - Get to the ROM root.
cd "$rom_game_root"
# Set Executable Environment Stuff
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$(realpath "./libs")
export LD_PRELOAD=$LD_PRELOAD:$PIUTOOLS_PATH/piutools.so
# Set Debug Logging for Now
export DBGLOG=1

# Set Executable Path
exe_path=$rom_game_root/version/$game_version/piu

# GOGOGO
#exec ltrace -e '*' -o /mnt/c/repos/piutools_ltrace.txt $exe_path game
exec $exe_path game
