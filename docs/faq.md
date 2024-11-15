# FAQ

This section describes some common errors that might appear when building ``XiLens``. Solutions to such errors are
also provided.

## Known Limitation
Even though `XiLens` has been developed with robustness as a guidance principle, and is thoroughly tested, we are aware
that some limitations exist. Below we list the ones we are awa of, if you find new limitations that are not listed here
please contact us.

!!! info "File format"
    The default format of the files used to store the images from all `XIMEA` cameras is an n-dimensional array
    through `BLOSC2`. This allows storing all images in one single file, at video-rate, and in a lossless compressed fashion.
    We strongly believe that this file format is ideal for storing large amounts of data, which is currently the only
    supported file format by `XiLens`. These files can however be read easily using the python library `blosc2` and
    can also be transformed to other formats is necessary. You can find more details in our documentation.

!!! warning "File corruption"
    We have noticed that when the connection between a camera and the computer is interrupted, `XiLens` might end abruptly.
    This error emanates from the `XiAPI` that is used to communicate with the cameras. This can sometimes also result in
    an `illegal memory access`, which by nature is difficult to mitigate since the point at which the camera is accidentally
    disconnected from the system cannot be determined beforehand. This can sometimes lead to corruption of the recording file.
    To mitigate this, make sure to secure the connection between the camera and the computer, and never disconnect
    the camera while recording data.

!!! info "Application look in Ubuntu 24.04"
    `XiLens` might have a strange look in ubuntu 24.04 that does not feel native. This is because
    `XiLens` uses `Qt` for the main `Ui` components. Unfortunately `Qt` only recently added support for `Wayland` in
    version `6.8`, which is not yet available via `apt`. You can install the latest version of Qt manually.
    Alternatively, You can also overwrite the following variable to change the app look `export QT_QPA_PLATFORM=xcb`
    to make it look more native to Ubuntu.

## Camera support
Camera support can be obtained from XIMEA through their [ticketing system](https://desk.ximea.com). When you create a
ticket, it is always a good Idea to attach the report from the [XiCop diagnostics tool](https://www.ximea.com/support/wiki/allprod/Saving_a_diagnostic_log_using_xiCop).
In Ubuntu, you can do this by running:

``` bash
/opt/XIMEA/bin/xiCOP -save_diag
```

The report is then stored in the file ``xicop_report.xml`` in the current working directory.


## Linking error of ``libuuid``
!!! failure "Linking error of `libuuid`"
    ``` bash
    can not link -luuid
    ```

!!! success "Solution"

    You most likely are missing the ``uuid-dev`` package, to fix this you
    cna install it via:

    ``` bash
    sudo apt install uuid-dev
    ```

## Number of XIMEA devices found:0
!!! failure "Number of XIMEA devices found:0"

    ``` bash
    number of ximea devices found: 0
    xiAPI: ---- xiOpenDevice API:V4.25.03.00 started ----
    xiAPI: EAL_DeviceEnumerator::OpenDevice - no camera with index 0 is available
    xiAPI: EAL_DeviceEnumerator::OpenDevice - can not find device by ID 00000000
    xiAPI: xiAPI error: Expected XI_OK in:../API/xiFAPI/interfaces/01_top/xifapi_Top.cpp xiOpenDevice/Line:86
    ```

!!! success "Solution"

    This is due to the fact that your user is not a member of the correct
    group ``plugdev``. To fix this issue, run the following command and
    restart your PC.

    ``` bash
    sudo usermod -aG plugdev $(whoami)
    ```

    You will need to log out and log back in or restart your computer for this to take effect
