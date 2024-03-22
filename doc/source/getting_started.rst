===============
Getting started
===============

.. note::
    In the future, you will need the ``CUDA`` library and drivers installed in your computer in order to use susicam.
    This is mainly because we are aiming at supporting the use of PyTorch models.

Build susicam from source
=========================

To build susicam you only need to install the dependencies and run the
build as in the following commands.

.. code:: bash

   sudo apt install libopencv-dev libboost-all-dev libgtest-dev gcovr qt6-base-dev cmake g++ wget

You will also have to install the xiAPI package provided my XIMEA

.. code:: bash

   wget --progress=bar:force:noscroll https://www.ximea.com/downloads/recent/XIMEA_Linux_SP.tgz
   tar xzf XIMEA_Linux_SP.tgz
   cd package 
   sudo ./install

Finally, from the home directory of ``susicam`` do the following. Notice
that the specified paths have been tested in ``Ubuntu 22.04``. If your
distribution is different, the specific paths might differ.

.. code:: bash

   mkdir build
   cd build
   cmake -D OpenCV_DIR=/usr/include/opencv4/opencv2 -D Ximea_Include_Dir=/opt/XIMEA/include -D Ximea_Lib=/usr/lib/libm3api.so.2.0.0 ..
   make all -j
   ctest # to check that all tests pass 

Increase USB buffer limit
=========================
.. important::
    After building ``susicam``, you have to increase the buffer size for the
    data transfer via USB. This can be done every time you start your
    computer by running the following command.

.. code:: bash

   sudo tee /sys/module/usbcore/parameters/usbfs_memory_mb >/dev/null <<<0

To prevent having to run this command every time you restart your PC, you can create a daemon service that would start
automatically every time you start your computer.

.. code:: bash

   sudo nano /etc/systemd/system/usb-buffer-size.service

You will need to paste the following content in the file opened above

.. code:: bash

   [Unit]
   Description=Increase USB Buffer Size

   [Service]
   ExecStart=/bin/bash -c 'sudo tee /sys/module/usbcore/parameters/usbfs_memory_mb >/dev/null <<<0'
   Type=oneshot
   RemainAfterExit=yes

   [Install]
   WantedBy=multi-user.target

Then you have to enable and start the service

.. code:: bash

   sudo systemctl enable usb-buffer-size.service
   sudo systemctl start usb-buffer-size.service
   service usb-buffer-size status

You should see that the service is marked as ``active``.

Launching the application
=========================
You can start the application by doing :code:`./SUSICAM` from the terminal. Alternatively you can create an application
launcher by copying the :code:`susicam.deskptop` and :code:`icon.png` files to :code:`~/.local/share/applications`. After copying these
files, you will have to modify the paths inside :code:`~/.local/share/applications/susicam.deskptop` to represent the
full path to the executable and the :code:`icon.png` file:

.. code:: bash

    Exec=/home/<user-name>/<path-to-build-dir>/SUSICAM
    Icon=/home/<user-name>/.local/share/applications/icon.png

Once you have done this, you should be able to launch the SUSICAM application from the application launcher and pin it
to your task bar for easy access.

Docker image
==================

.. attention::
    Running a Qt application inside docker is not straight forward. Building the docker image can serve to test your
    developed code to make sure that it will work in other systems, however running the application inside docker is still
    under development.

    .. code:: bash

       docker compose --verbose build --progress plain
       docker run -it --privileged -e DISPLAY=$DISPLAY -v /tmp/.X11-unix:/tmp/.X11-unix --gpus all --device /dev/bus/usb/ -e QT_X11_NO_MITSHM=1 -e QT_GRAPHICSSYSTEM="native" susicam

