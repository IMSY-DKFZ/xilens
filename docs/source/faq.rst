===
FAQ
===
This section describes some common errors that might appear when building SUSICAM. Solutions to such errors are also
provided.

Camera support
--------------
Camera support can be obtained from XIMEA through their `ticketing system <https://desk.ximea.com>`_. When you create a
ticket, it is always a good Idea to attach the report from the `XiCop diagnostics tool <https://www.ximea.com/support/wiki/allprod/Saving_a_diagnostic_log_using_xiCop>`_.
In Ubuntu you can do this by running:

.. code:: bash

    /opt/XIMEA/bin/xiCOP -save_diag

The report is then stored in the file :code:`xicop_report.xml` in the current working directory.


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
