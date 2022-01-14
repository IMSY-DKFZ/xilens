## Additional steps for installation on ubuntu 20

**Installation of Qt version 5.14.2**

```bash
wget http://download.qt.io/official_releases/qt/5.14/5.14.2/qt-opensource-linux-x64-5.14.2.run
chmod +x qt-opensource-linux-x64-5.14.2.run ./qt-opensource-linux-x64-5.14.2.run 
```

**CMake configuration in Qt**
thefollowing configuration has to be added manually in CMake configuration before building

```
Caffe_DIR: /<path-to>/caffe/build
OpenCV_DIR: /usr/share/OpenCV
Ximea_Include_Dir: /opt/XIMEA/include
Ximea_Lib: /usr/lib/libm3api.so.2.0.0
```

**Installation of gcc version 6**
Package for ubuntu 18

```bash
sudo apt update
sudo apt install gcc-6 g++-6
```



**Install opencv**

```bash
locate opencv
sudo apt install python3-opencv
```

**Remove built from caffe**
To remove the built from caffe go to the path where caffe is located and do the following.

```bash
rm -r built
```

To disabe opencv in caffe built go open Cmake gui and turn opencv off.

---
## FAQ

**Error**
1. undefined referenc to uuid_generate 
Set flags in Qt Creator CMakeList.txt and add the following.

```
set(CMAKE_CXX_FLAGS "-luuid")
find_package( OpenCV REQUIRED COMPONENTS core imgproc highgui)
find_package(Boost REQUIRED python system thread timer chrono log filesystem)
include(version.cmake)
```

Add the following to: set (susiCamLib_src ${PROJECT_SOURCE_DIR}/mainwindow.cpp

```
${PROJECT_SOURCE_DIR}/version.cpp
${PROJECT_SOURCE_DIR}/displaySaturation.cpp
${PROJECT_SOURCE_DIR}/displayDemo.cpp
```

Add the following to: set (susiCamLib_hdr ${PROJECT_SOURCE_DIR}/mainwindow.h

```
${PROJECT_SOURCE_DIR}/displaySaturation.h
${PROJECT_SOURCE_DIR}/displayDemo.h
```

Notice:
If you did clear all in Qt Creator and the error massage is still there do this:

```bash
rm -r build-susicam-Desktop_Qt_5_14_2_GCC_64bit-Debug
mkdir build-susicam-Desktop_Qt_5_14_2_GCC_64bit-Debug
```

2. Libopencv_imgcodes.so.4.2.0: undefined reference to 'TIFFReadRGBAStripe@IBTIFF_4.0'
3. no rule to make target 'home/s025n/anaconda3/lib/libhdf5_cpp.sp', needed by 'libsusiCamLib.so'
Deactivate libtiff in conda

```bash
conda uninstall libtiff
```

