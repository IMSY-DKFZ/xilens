FROM ubuntu:22.04

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

# build xilens
WORKDIR /home/xilens
COPY . .

# install dependencies
RUN chmod +x install_dependencies.sh
RUN ./install_dependencies.sh

# build and install package
WORKDIR /home/xilens/cmake-build
RUN cmake --version
RUN cmake -D ENABLE_COVERAGE=ON ..
RUN xvfb-run -a --server-args="-screen 0 1024x768x24" make all -j
RUN xvfb-run -a --server-args="-screen 0 1024x768x24" make package -j
RUN dpkg -i xilens*.deb

# run tests
ENV QT_QPA_PLATFORM offscreen
RUN xvfb-run -a --server-args="-screen 0 1024x768x24" ctest --output-on-failure
RUN gcovr --html --exclude-unreachable-branches --print-summary -o coverage.html -e '/.*cmake-build.*/.*' --root ../

# run application
CMD QT_GRAPHICSSYSTEM="native" QT_X11_NO_MITSHM=1 /home/xilens/build/xilens /home/caffe/models/susi/imec_patchnet_4_LAYER_in_vivo.prototxt /home/caffe/models/susi/model_20SNR_20stain_3patch.caffemodel /home/caffe/models/susi/white.tif /home/caffe/models/susi/dark.tif
