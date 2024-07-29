FROM nvidia/cuda:12.2.0-devel-ubuntu22.04

# Avoid Docker build freeze due to region selection
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=Europe/Berlin
RUN apt update && apt-get -y install tzdata

# Basic tools
RUN apt update && apt install -y \
    build-essential \
    sudo \
    udev \
    wget \
    libcanberra-gtk-module  \
    libcanberra-gtk3-module  \
    cmake \
    xvfb \
    uuid-dev  \
    libgl1-mesa-dev  \
    git

RUN apt install -y  \
    libmsgpack-dev  \
    qt6-base-dev  \
    libqt6svg6-dev  \
    libgtest-dev \
    gcovr \
    libopencv-dev \
    --no-install-recommends libboost-all-dev

# install xiAPI
WORKDIR /home
RUN wget --progress=bar:force:noscroll https://www.ximea.com/downloads/recent/XIMEA_Linux_SP.tgz
RUN tar xzf XIMEA_Linux_SP.tgz
WORKDIR /home/package/scripts
RUN sed -i '/^[^#]/ s/\(^.*udevadm control --reload.*$\)/#\ \1/' install_steps
WORKDIR /home/package
RUN ./install
RUN echo "echo 0 > /sys/module/usbcore/parameters/usbfs_memory_mb" >> /etc/rc.local

# install BLOSC2
WORKDIR /home
RUN git clone https://github.com/Blosc/c-blosc2.git
WORKDIR /home/c-blosc2
RUN git checkout v2.15.0
WORKDIR /home/c-blosc2/build
RUN cmake -DCMAKE_INSTALL_PREFIX=/usr .. && \
    cmake --build . --target install --parallel

# build susicam
WORKDIR /home/susicam
COPY . .
WORKDIR /home/susicam/cmake-build
RUN cmake --version
RUN cmake -D ENABLE_COVERAGE=ON ..
RUN xvfb-run -a --server-args="-screen 0 1024x768x24" make all -j

# run tests
ENV QT_QPA_PLATFORM offscreen
RUN xvfb-run -a --server-args="-screen 0 1024x768x24" ctest --output-on-failure
RUN gcovr --html --exclude-unreachable-branches --print-summary -o coverage.html -e '/.*cmake-build.*/.*' --root ../

# run application
CMD QT_GRAPHICSSYSTEM="native" QT_X11_NO_MITSHM=1 /home/susicam/build/susiCam /home/caffe/models/susi/imec_patchnet_4_LAYER_in_vivo.prototxt /home/caffe/models/susi/model_20SNR_20stain_3patch.caffemodel /home/caffe/models/susi/white.tif /home/caffe/models/susi/dark.tif