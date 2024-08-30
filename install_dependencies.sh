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
wget --progress=bar:force:noscroll https://www.ximea.com/downloads/recent/XIMEA_Linux_SP.tgz
tar xzf XIMEA_Linux_SP.tgz
cd package || exit
sed -i '/^[^#]/ s/\(^.*udevadm control --reload.*$\)/#\ \1/' scripts/install_steps
./install
cd ..

# install BLOSC2
git clone https://github.com/Blosc/c-blosc2.git
cd c-blosc2 || exit
git checkout v2.15.1
mkdir build
cd build || exit
cmake -DCMAKE_INSTALL_PREFIX=/usr .. && \
cmake --build . --target install --parallel
