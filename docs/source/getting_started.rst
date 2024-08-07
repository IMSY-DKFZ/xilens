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

   sudo xargs -a requirements.txt apt-get install -y

You will also have to install the xiAPI package provided by XIMEA

.. code:: bash

   wget --progress=bar:force:noscroll https://www.ximea.com/downloads/recent/XIMEA_Linux_SP.tgz
   tar xzf XIMEA_Linux_SP.tgz
   cd package
   sudo ./install

We use `BLOSC2 <https://www.blosc.org/c-blosc2/c-blosc2.html>`_ to store all images to a single file while using
lossless compression. To install it you simply need to run the following commands from a folder of your choosing.
Keep in mind that to install the package in /usr you will need to run the install command with :code:`sudo`.

.. code:: bash

    RUN git clone https://github.com/Blosc/c-blosc2.git
    cd c-blosc2
    git checkout v2.15.0
    mkdir build && cd build
    cmake -DCMAKE_INSTALL_PREFIX=/usr ..
    cmake --build . --target install --parallel

Finally, from the root directory of ``susicam`` do the following.

.. code:: bash

   mkdir build
   cd build
   cmake -DCMAKE_INSTALL_PREFIX=/usr  ..
   make all -j
   ctest # to check that all tests pass
   sudo make install # installs the desktop app on the system and can be accessed from the app launcher

The application can be uninstalled from the system by doing :code:`sudo make uninstall` form the build directory

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
After doing `sudo make install` from the build directory, the desktop app should be available through the app launcher
of your system.
Alternatively, you can run :code:`./SUSICAM` from the build directory in a  terminal.

Docker image
==================

.. attention::
    Running a Qt application inside docker is not straight forward. Building the docker image can serve to test your
    developed code to make sure that it will work in other systems, however running the application inside docker is still
    under development.

    .. code:: bash

       docker compose --verbose build --progress plain
       docker run -it --privileged -e DISPLAY=$DISPLAY -v /tmp/.X11-unix:/tmp/.X11-unix --gpus all --device /dev/bus/usb/ -e QT_X11_NO_MITSHM=1 -e QT_GRAPHICSSYSTEM="native" susicam
