FROM nvidia/cuda:12.2.0-devel-ubuntu22.04

# Avoid Docker build freeze due to region selection
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=Europe/Berlin
RUN apt update && apt-get -y install tzdata

# Basic tools
RUN apt update && apt install -y \
     build-essential \
     curl \
     libopencv-dev \
     libgtest-dev \
     --no-install-recommends libboost-all-dev
RUN export CC=/usr/bin/gcc-6 && export CXX=/usr/bin/g++-6
RUN apt install -y python3-pip
RUN python3 -m pip install --upgrade pip
RUN pip3 install setuptools-scm
RUN pip3 install scikit-build
RUN pip3 install cmake --upgrade
