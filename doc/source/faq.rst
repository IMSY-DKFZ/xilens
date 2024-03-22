===
FAQ
===
This section describes some common errors that might appear when building SUSICAM. Solutions to such errors are also
provided.

libtiff conflicts
-----------------
.. error::

    .. code:: bash

        Libopencv_imgcodes.so.4.2.0: undefined reference to 'TIFFReadRGBAStripe@IBTIFF_4.0'
        no rule to make target '../anaconda3/lib/libhdf5_cpp.sp', needed by 'libsusiCamLib.so'

.. hint::

    .. code:: bash

       conda uninstall libtiff


Linking error of ``libuuid``
----------------------------

.. error::
    .. code:: bash

       can not link -luuid

.. hint::

    You most likely are missing the ``uuid-dev`` package, to fix this you
    cna install it via:

    .. code:: bash

       sudo apt install uuid-dev

Number of XIMAE devices found:0
--------------------------------------
.. error::

    .. code:: bash

       number of ximea devices found: 0
       xiAPI: ---- xiOpenDevice API:V4.25.03.00 started ----
       xiAPI: EAL_DeviceEnumerator::OpenDevice - no camera with index 0 is available
       xiAPI: EAL_DeviceEnumerator::OpenDevice - can not find device by ID 00000000
       xiAPI: xiAPI error: Expected XI_OK in:../API/xiFAPI/interfaces/01_top/xifapi_Top.cpp xiOpenDevice/Line:86

.. hint::
    This is due to the fact that your user is not a member of the correct
    group ``plugdev``. To fix this issue, run the following command and
    restart your PC.

    .. code:: bash

       sudo usermod -aG plugdev $(whoami)

    You will need to log out and log back in or restart your computer for this to take effect