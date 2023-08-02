FROM susicam-base
RUN apt install -y uuid-dev qtbase5-dev wget sudo udev libcanberra-gtk-module libcanberra-gtk3-module
WORKDIR /home/susicam
COPY . .
WORKDIR /home
RUN wget --progress=bar:force:noscroll https://www.ximea.com/downloads/recent/XIMEA_Linux_SP.tgz
RUN tar xzf XIMEA_Linux_SP.tgz
WORKDIR /home/package/scripts
RUN sed -i '/^[^#]/ s/\(^.*udevadm control --reload.*$\)/#\ \1/' install_steps
WORKDIR /home/package
RUN ./install
RUN echo "echo 0 > /sys/module/usbcore/parameters/usbfs_memory_mb" >> /etc/rc.local
# build susicam
WORKDIR /home/susicam/build
RUN cmake -D Caffe_DIR=/home/caffe/build -D OpenCV_DIR=/usr/include/opencv4/opencv2 -D Ximea_Include_Dir=/opt/XIMEA/include -D Ximea_Lib=/usr/lib/libm3api.so.2.0.0 ..
RUN make all -j

CMD QT_GRAPHICSSYSTEM="native" QT_X11_NO_MITSHM=1 /home/susicam/build/susiCam /home/caffe/models/susi/imec_patchnet_4_LAYER_in_vivo.prototxt /home/caffe/models/susi/model_20SNR_20stain_3patch.caffemodel /home/caffe/models/susi/white.tif /home/caffe/models/susi/dark.tif