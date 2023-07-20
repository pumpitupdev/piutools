#!/usr/bin/env bash



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

# Make sure we have Docker, our environment is set up, etc.
if ! docker info > /dev/null 2>&1 ; then
  echo "Docker is not running"
  exit 1
fi

# Check if "pumpos_classic" image is present
if ! docker image inspect pumpos_classic > /dev/null 2>&1 ; then
  # Build the "pumpos_classic" image using Dockerfile
  docker build -t pumpos_classic -f ./tools/PumpOS_Classic.Dockerfile .
fi

# Load Our Native Paths
piutools_native_path=$(realpath $(pwd))
piutools_native_rom_path=$piutools_native_path/roms/$game_name
piutools_native_config_path=$piutools_native_path/config/$game_name/$game_version.conf

piutools_native_save_path=$piutools_native_path/save/$game_name/$game_version
if [ ! -d "$piutools_native_save_path" ]; then
    mkdir -p "$piutools_native_save_path"
fi

# Load the configuration file
# Extract the variables from the [CONTAINER] section
container_vars=$(grep -A 1000 "\[CONTAINER\]" "$piutools_native_config_path" | sed -n '/\[/{:a;n;/^\[/b;p;ba}')
# Parse the variables into shell variables
#echo $container_vars
eval "$container_vars"

# Set up Our Environment Paths
piutools_root=/opt/piutools
piutools_bin=$piutools_root/bin
piutools_plugins_path=$piutools_bin/plugins
piutools_config_path=$piutools_bin/config/$game_name/$game_version.conf
piutools_rom=$piutools_root/rom
piutools_save=$piutools_root/save
piutools_mnt=$piutools_root/mnt
piutools_tmp=$piutools_root/tmp

#run_command= RUN_STRACE, RUN_GDB, RUN_GAME,RUN_LTRACE

# ---- Detect WSL or Native to Change Docker Options
if grep -qEi "(Microsoft|WSL)" /proc/version &> /dev/null ; then
    docker_args="run --device /dev/fuse --cap-add SYS_ADMIN --rm -it"
    # Add the Graphics Support Stuff
    docker_args+=" --gpus all -v /tmp/.X11-unix:/tmp/.X11-unix -e DISPLAY=$DISPLAY"
    # Add the Sound Stuff
    docker_args+=" -e PULSE_SERVER=$PULSE_SERVER -v /mnt/wslg/:/mnt/wslg/"
else
    docker_args="run --device /dev/fuse --cap-add SYS_ADMIN --rm -it --add-host host.docker.internal:host-gateway"
    # Add the Graphics Support Stuff
    docker_args+=" -v /dev/dri:/dev/dri -v /tmp/.X11-unix:/tmp/.X11-unix -e DISPLAY=$DISPLAY"
    # Add the Sound Stuff
    docker_args+=" -e PULSE_SERVER=unix:${XDG_RUNTIME_DIR}/pulse/native -v /run/user/$(id -u):/run/user/$(id -u) --group-add $(getent group audio | cut -d: -f3)"
fi


# ---- END WSL ----
# Add Our PIUTools Mounts
docker_args+=" -v $piutools_native_path:$piutools_bin"
docker_args+=" -v $piutools_native_rom_path:$piutools_rom:ro"
docker_args+=" -v $piutools_native_save_path:$piutools_save"
# Add Our PIUTools Envars
docker_args+=" -e PIUTOOLS_GAME_NAME=$game_name"
docker_args+=" -e PIUTOOLS_GAME_VERSION=$game_version"
docker_args+=" -e PIUTOOLS_PATH=$piutools_bin"
docker_args+=" -e PIUTOOLS_CONFIG_PATH=$piutools_config_path"
docker_args+=" -e PIUTOOLS_PLUGIN_PATH=$piutools_plugins_path"
docker_args+=" -e PIUTOOLS_ROM_PATH=$piutools_rom"
docker_args+=" -e PIUTOOLS_SAVE_PATH=$piutools_save"
docker_args+=" -e PIUTOOLS_TMP_PATH=$piutools_tmp"
docker_args+=" -e PIUTOOLS_MNT_PATH=$piutools_mnt"
docker_args+=" -e PIUTOOLS_ROMS=$game_roms"
docker_args+=" -e PIUTOOLS_EXE_PATH=$exe_path"
docker_args+=" -e PIUTOOLS_EXE_ARGS=$exe_args"
docker_args+=" -e PIUTOOLS_GAME_DIR=$exe_game_dir"
docker_args+=" -e PIUTOOLS_DEBUG=1"
docker_args+=" -e RUN_GAME=1"
#docker_args+=" -e RUN_GDB=1"
#docker_args+=" -e RUN_STRACE=1"
#docker_args+=" -e RUN_LTRACE=1"

docker_args+=" pumpos_classic $piutools_bin/tools/bootstrap_game.sh"

echo $docker_args

exec docker $docker_args

