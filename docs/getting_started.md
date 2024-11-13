# Getting started

## Installing ``XiLens`` via ``apt``
You can install ``XiLens`` by downloading the corresponding ``.deb`` package from our [release page](https://github.com/IMSY-DKFZ/xilens/releases).
And then install it by doing:

```bash
sudo apt install ./xilens*.deb
```

This will install ``XiLens`` and its dependencies that are available in ``apt``.

!!! warning "Dependencies"
    ``XiLens`` still depends on ``BLOSC2`` and `XiAPI`. These packages are not available via `apt`, which is why they
    are not installed by `apt` when installing the `.deb` package.
    These additional dependencies can be installed by running:

    ```bash
    chmod +x install_dependencies.sh
    sudo ./install_dependencies.sh --user
    ```
    These dependencies should only be needed to be installed once, when updating `XiLens` you would only need to
    install the `.deb` package.

## Updating `XiLens`
To update `XiLens`, you only need to either re-build it, or simply download the latest `.deb` package from our [release page](https://github.com/IMSY-DKFZ/xilens/releases)
and re-install it.

## Build `XiLens` from source

[![image](https://asciinema.org/a/YSmGYswUqGmYGk969rmQUb0mV.svg)](https://asciinema.org/a/YSmGYswUqGmYGk969rmQUb0mV)

### Install dependencies

To build ``XiLens`` you only need to install the dependencies and run the
build as in the following commands.

!!! warning "Installation directory"

    You should run this from a directory where BLOSC2 and XiAPI will be downloaded.

```bash
chmod +x install_dependencies.sh
sudo ./install_dependencies.sh --user
```

This takes care of installing dependencies including the xiAPI package provided by XIMEA and BLOSC2.

!!! warning "User vs. Development dependencies"

    The previous script will install ``BLOSC2`` and ``XiAPI`` on your system!
    If you are building the application for development purposes, you should remove the flag `--user` to install the
    development dependencies instead.

### Build application
From the root directory of ``XiLens`` do the following.

```bash
mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr  ..
make all -j
ctest # to check that all tests pass
sudo make install # installs the desktop app on the system and can be accessed from the app launcher
```

!!! note "Static build"

    ``XiLens`` can be built statically by adding a flag during configuration ``cmake -DBUILD_STATIC -DCMAKE_INSTALL_PREFIX=/usr ..``

The application can be uninstalled from the system by doing ``sudo make uninstall`` form the build directory

## Enable connection to cameras
### Increase USB buffer limit
!!! important "USB buffer size"

    After building ``XiLens``, you have to increase the buffer size for the
    data transfer via USB. This can be done every time you start your
    computer by running the following command.

```bash
sudo tee /sys/module/usbcore/parameters/usbfs_memory_mb >/dev/null <<<0
```

To prevent having to run this command every time you restart your PC, you can create a daemon service that would start
automatically every time you start your computer.

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

You should see that the service is marked as ``active``.

### Add user to ``pugdev`` group

After doing the following, you will have to log-out and log-in again for this tot ake effect.

```bash
sudo usermod -aG plugdev $(whoami)
```

## Launching the application
After doing `sudo make install` from the build directory, the desktop app should be available through the app launcher
of your system.
Alternatively, you can run ``./xilens`` from the build directory in a  terminal.

## Uninstalling ``XiLens``
If you installed `XiLens` with the `.deb` package, then you can uninstall it by doing ``sudo apt uninstall xilens``.
If you build the app from source, you can uninstall it by doing the following from the directory where `XiLens` was built.

```bash
sudo make uninstall
```

## Docker image

!!! important "Qt inside a docker image"

    Running a Qt application inside docker is not straight forward. Building the docker image can serve to test your
    developed code to make sure that it will work in other systems, however running the application inside docker is still
    under development.

    ```bash
    docker compose --verbose build --progress plain
    docker run -it --privileged -e DISPLAY=$DISPLAY -v /tmp/.X11-unix:/tmp/.X11-unix --gpus all --device /dev/bus/usb/ -e QT_X11_NO_MITSHM=1 -e QT_GRAPHICSSYSTEM="native" xilens
    ```
