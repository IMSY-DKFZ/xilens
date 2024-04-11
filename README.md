The susicam library is only supported in Linux systems, more support is provided for Ubuntu.

NOTE: You will need `CUDA` library and drivers installed in your computer in order to use susicam.

## Build susicam from source

To build susicam you only need to install the dependencies and run the build as in the following commands.

```bash
sudo apt install libopencv-dev libboost-all-dev libgtest-dev gcovr libqt6svg6-dev qt6-base-dev cmake g++ wget 
```

You will also have to install the xiAPI package provided my XIMEA

```bash
wget --progress=bar:force:noscroll https://www.ximea.com/downloads/recent/XIMEA_Linux_SP.tgz
tar xzf XIMEA_Linux_SP.tgz
cd package 
sudo ./install
```

Finally, from the home directory of `susicam` do the following. Notice that the specified paths have been tested in 
`Ubuntu 22.04`. If your distribution is different, the specific paths might differ.

```bash
mkdir build
cd build
cmake -D OpenCV_DIR=/usr/include/opencv4/opencv2 -D Ximea_Include_Dir=/opt/XIMEA/include -D Ximea_Lib=/usr/lib/libm3api.so.2.0.0 ..
make all -j
ctest # to check that all tests pass 
```

# Increase USB buffer limit
After building `susicam`, you have to increase the buffer size for the data transfer via USB.  This can be done every 
time you start your computer by running the following command. 

```bash
sudo tee /sys/module/usbcore/parameters/usbfs_memory_mb >/dev/null <<<0
```

Alternatively, you can create a daemon service that would start automatically every time you start your computer.
```bash
sudo nano /etc/systemd/system/usb-buffer-size.service
```
You will need to paste the following content in the file opened above
```bash
[Unit]
Description=Increase USB Buffer Size

[Service]
ExecStart=/bin/bash -c 'sudo tee /sys/module/usbcore/parameters/usbfs_memory_mb >/dev/null <<<0'
Type=oneshot
RemainAfterExit=yes

[Install]
WantedBy=multi-user.target
```
Then you have to enable and start the service
```bash
sudo systemctl enable usb-buffer-size.service
sudo systemctl start usb-buffer-size.service
service usb-buffer-size status
```
You should see that the service is marked as `active`.

# Launching the application
You can start the application by doing `./SUSICAM` from the terminal. Alternatively you can create an application 
launcher by copying the `susicam.deskptop` and `icon.png` files to `~/.local/share/applications`. After copying these
files, you will have to modify the paths inside `~/.local/share/applications/susicam.deskptop` to represent the 
full path to the executable and the `icon.png` file:

```bash
Exec=/home/<user-name>/<path-to-build-dir>/SUSICAM
Icon=/home/<user-name>/.local/share/applications/icon.png
```

## Build docker image
Running a Qt application inside docker is not straight forward. Building the docker image can serve to test your 
developed code to make sure that it will work in other systems, however running the application inside docker is still 
under development. 

```bash
docker compose --verbose build --progress plain
docker run -it --privileged -e DISPLAY=$DISPLAY -v /tmp/.X11-unix:/tmp/.X11-unix --gpus all --device /dev/bus/usb/ -e QT_X11_NO_MITSHM=1 -e QT_GRAPHICSSYSTEM="native" susicam
```

---
## FAQ

1. Errors related to libtiff conflicts:
`Libopencv_imgcodes.so.4.2.0: undefined reference to 'TIFFReadRGBAStripe@IBTIFF_4.0'`
`no rule to make target '../anaconda3/lib/libhdf5_cpp.sp', needed by 'libsusiCamLib.so'`


**Solution:**
```bash
conda uninstall libtiff
```

2. Error: Linking error of `libuuid`
```bash
can not link -luuid
```
You most likely are missing the `uuid-dev` package, to fix this you cna install it via:
```bash
sudo apt install uuid-dev
```

3. Error: Number of XIMAE devices found:0

```bash
number of ximea devices found: 0
xiAPI: ---- xiOpenDevice API:V4.25.03.00 started ----
xiAPI: EAL_DeviceEnumerator::OpenDevice - no camera with index 0 is available
xiAPI: EAL_DeviceEnumerator::OpenDevice - can not find device by ID 00000000
xiAPI: xiAPI error: Expected XI_OK in:../API/xiFAPI/interfaces/01_top/xifapi_Top.cpp xiOpenDevice/Line:86
```
This is due to the fact that your user is not a member of the correct group `plugdev`. To fix this issue, run the following command and restart your PC. 
```bash
sudo usermod -aG plugdev $(whoami)
```