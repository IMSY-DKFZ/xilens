#!/bin/bash
set -e

# install apt dependencies
xargs -a requirements.txt apt install --no-install-recommends -y

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
git checkout v2.15.0
mkdir build
cd build || exit
cmake -DCMAKE_INSTALL_PREFIX=/usr .. && \
cmake --build . --target install --parallel