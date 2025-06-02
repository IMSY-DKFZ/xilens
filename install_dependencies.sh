#!/bin/bash
set -e

# Default dependencies file
DEPENDENCIES_FILE="dev-requirements.txt"

# Check if the --user argument is passed
if [[ $1 == "--user" ]]; then
    DEPENDENCIES_FILE="user-requirements.txt"
fi

# Install apt dependencies
xargs -a "$DEPENDENCIES_FILE" apt install --no-install-recommends -y

# install XiAPI
wget --progress=bar:force:noscroll https://www.ximea.com/getattachment/ab5baacf-e806-4b9d-b3d4-7eedf0f092b8/XIMEA_Linux_SP.tgz
tar xzf XIMEA_Linux_SP.tgz
cd package || exit
sed -i '/^[^#]/ s/\(^.*udevadm control --reload.*$\)/#\ \1/' scripts/install_steps
./install
cd ..
