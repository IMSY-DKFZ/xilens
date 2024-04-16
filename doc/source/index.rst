=====================
SUSICAM documentation
=====================
Welcome to the SUSICAM documentation. This project aims at providing an application
that can be used to record data from all the **XIMEA** camera families.
This application aims at providing an easy to use graphical user interface (GUI) that is well documented and tested to
record data with XIMEA cameras.

.. important::

    This applications has mainly been developed to be used in **UNIX systems**, however it might also be possible
    to use it in **Windows systems**.

Main features
=============
* Support for virtually all XIMEA cameras
* Efficient data storage through `BLOSC2 <https://www.blosc.org/c-blosc2/c-blosc2.html>`_
* Multi-instance run for recordings with multiple cameras in parallel
* Long-term stability, tested for recordings of 24 hours at over 20 fps
* Unit tested using the GoogleTest framework


Getting started
===============
Installing dependencies, building SUSICAM and how to use SUSICAM is all introduced in the getting started section:

.. toctree::
    :maxdepth: 1
    :caption: Getting started

    getting_started.rst

    faq.rst

    developer_guidelines.rst

Development logic
=================
The SUSICAM application relies at its core on many `Qt <https://www.qt.io/product/qt6>`_ components for the GUI, while
adding a custom look for the interface. SUSICAM is developed based on the principles that usability and performance are
prioritized.

To get the maximum speed while recording data with the XIEMA cameras, the `Boost <https://www.boost.org/>`_ library is
used for multi-threading most components of the applications such as: displaying images, recording data, etc.

For maintainability, displaying images is done with `OpenCV <https://opencv.org/get-started/>`_.

API documentation
=================

.. toctree::
    :maxdepth: 1
    :caption: API definition and documentation

    mainwindow.rst

    displays.rst

    camera_components.rst

    constants.rst

    utils.rst



Indices and tables
==================

* :ref:`genindex`
