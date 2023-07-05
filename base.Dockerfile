FROM nvidia/cuda:10.1-cudnn7-devel-ubuntu18.04

# Avoid Docker build freeze due to region selection
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=Europe/Berlin
RUN apt update && apt-get -y install tzdata

# Basic tools
RUN apt update && apt install -y \
     build-essential \
     curl \
     git \
     rsync \
     tree \
     libgl1-mesa-glx libglib2.0-0 \
     libprotobuf-dev libleveldb-dev libsnappy-dev libopencv-dev libhdf5-serial-dev protobuf-compiler \
     --no-install-recommends libboost-all-dev \
     libgflags-dev libgoogle-glog-dev liblmdb-dev \
     libatlas-base-dev \
     gcc-6 g++-6
RUN export CC=/usr/bin/gcc-6 && export CXX=/usr/bin/g++-6
RUN apt install -y python3-pip
RUN python3 -m pip install --upgrade pip
RUN pip3 install setuptools-scm
RUN pip3 install scikit-build
RUN pip3 install cmake --upgrade

WORKDIR /home/caffe
COPY caffe/ .

# build protoc file
RUN protoc src/caffe/proto/caffe.proto --cpp_out=.
RUN mv src/caffe/proto/caffe.pb.h include/caffe/proto

# build caffe
RUN rm -rf build && mkdir build
WORKDIR /home/caffe/build
RUN cmake -D BUILD_python=OFF -D USE_OPENCV=OFF -DCUDA_ARCH_NAME=Manual -DCUDA_ARCH_BIN="50 52 60 61" ..
RUN make all -j