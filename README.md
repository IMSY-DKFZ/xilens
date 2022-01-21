The susicam library is only supported in Linux systems, more support is provided for Ubuntu.

NOTE: You will need `CUDA` library and drivers installed in your computer in order to use susicam.

## Build susicam

The library `caffe` is planned to be removed from `susicam`, if the version you are using still depends on `caffe` then you can build `caffe` by following the next steps.

**Build caffe**
Install dependancies:
```bash
sudo apt-get install libprotobuf-dev libleveldb-dev libsnappy-dev libopencv-dev libhdf5-serial-dev protobuf-compiler
sudo apt-get install --no-install-recommends libboost-all-dev
sudo apt-get install libgflags-dev libgoogle-glog-dev liblmdb-dev
sudo apt-get install libatlas-base-dev cmake cmake-gui
```

WARNING: The `gcc` version that you have installed depends on the Ubuntu version you have. Recent versions of Ubuntu have `gcc 9` by default. You will need to install `gcc-6` and `g++-6` in order to build susicam. This can be done by `sudo apt install gcc-6 g++-6` and then doing `export CC=/usr/bin/gcc-6 && export CXX=/usr/bin/g++-6`. You might need to remove the `build` directory from `caffe` and re-create it if you already attempted to build with another `gcc` version.

copy the `caffe` library from network drives to any desired location. You can find the library in `E130-Projekte/Biophotonics/Software/caffe`. From the `caffe` folder do the following:

```bash
rm -r build && mkdir build && cd build
cmake ..
```

If there are no errors then proceed to the following
```bash
make all -j
```
If you encounter errors during the building process, checkout the FAQ section below. If the building procedure finished you can proceed to the next step
```bash
protoc src/caffe/proto/caffe.proto --cpp_out=.
mkdir include/caffe/proto
mv src/caffe/proto/caffe.pb.h include/caffe/proto
```


**Installation of Qt version 5.14.2**
The building process has been tested with Qt 5.14.2, to install it you need to do the following:

```bash
wget http://download.qt.io/official_releases/qt/5.14/5.14.2/qt-opensource-linux-x64-5.14.2.run
chmod +x qt-opensource-linux-x64-5.14.2.run 
./qt-opensource-linux-x64-5.14.2.run 
```

**Install xiAPI library**
you can install the xiAPI library by following the instructions described [here](https://www.ximea.com/support/wiki/apis/XIMEA_Linux_Software_Package#Installation). You should install it for USB3 cameras. 

**CMake configuration in Qt**
thefollowing configuration has to be added manually in CMake configuration before building

```
Caffe_DIR: /<path-to>/caffe/build
OpenCV_DIR: /usr/share/OpenCV
Ximea_Include_Dir: /opt/XIMEA/include
Ximea_Lib: /usr/lib/libm3api.so.2.0.0
```
WARNING: The paths might depend on your ubuntu version. For example, opencv is installed in `/usr/include/opencv4/opencv2`. Also, the naming of libm3api could change to `libm3api.so.2` or `libm3api.so`.


---
## FAQ


1. Errors related to libtiff conflicts:
`Libopencv_imgcodes.so.4.2.0: undefined reference to 'TIFFReadRGBAStripe@IBTIFF_4.0'`
`no rule to make target '../anaconda3/lib/libhdf5_cpp.sp', needed by 'libsusiCamLib.so'`


**Solution:**
```bash
conda uninstall libtiff
```

2. Error Common for cuda versions above 10.0:

```bash    
CMake Error: The following variables are used in this project, but they are set to NOTFOUND.
Please set them or make sure they are set and tested correctly in the CMake files:
CUDA_cublas_LIBRARY (ADVANCED)
linked by target "caffe" in directory ../caffe/src/caffe
CUDA_cublas_device_LIBRARY (ADVANCED)
linked by target "caffe" in directory ../caffe/src/caffe
```

**Solution:** 
cublas is no longer dispatched with cuda above 10.0 so cmake can not find it try upgrading `cmake`:
```bash    
sudo apt purge cmake
sudo snap intall cmake --classic
```    
If this does not work try also downgrading cuda to version 10

3. Error Common for cuda 10 installations:
```bash
error: ‘cudaGraph_t’ has not been declared
```
**Solution**
Try modifying the line in `/usr/include/cudnn.h`: `#include "driver_types.h"` for `#include <driver_types.h>`

4. Error: Common for cuda 10 installations
```bash
error: expected ‘)’ before ‘*’ token cuda_runtime_api.h
```
**Solution:**

Definitions in cuda runtime api have to me modified. Try this:
```bash
sudo gedit /usr/local/cuda/include/cuda_runtime_api.h
```
Then add before this line:
```cpp
typedef void (CUDART_CB *cudaStreamCallback_t)(cudaStream_t stream, cudaError_t status, void *userData);
```
the following code:
```cpp
#ifdef _WIN32
#define CUDART_CB __stdcall
#else
#define CUDART_CB
#endif
```