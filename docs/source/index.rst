================
XiLens /ksɪlɛns/
================
Welcome to the :code:`XiLens` documentation. This project aims at providing an application
that can be used to record data from all the **XIMEA** camera families.

.. image:: ../../resources/ui-animation.gif

.. note::

    Main features:

    * Compatible with all XIMEA cameras: spectral, RGB & gray cameras
    * Highly optimized data storing at video-rate: n-dimensional arrays with `BLOSC2 <https://www.blosc.org/c-blosc2/c-blosc2.html>`_
    * Multi-instance run for recordings with multiple cameras in parallel
    * Long-term stability, tested for recordings of 24 hours at over 20 fps
    * Camera temperature logged automatically during recording
    * Compatible with Linux systems
    * Automatic tests of non-UI components through google tests

.. important::

    This applications has mainly been developed to be used in **UNIX systems**, however it might also be possible
    to use it in **Windows systems**.


Getting started
===============
Installing dependencies, building :code:`XiLens` and how to use :code:`XiLens` is all introduced in the getting started section:

.. toctree::
    :maxdepth: 1
    :caption: Getting started

    getting_started.rst

    faq.rst

    developer_guidelines.rst

Application details
===================
More details on the implementation and supported features of the application can be found in the following pages:

.. toctree::
    :maxdepth: 1
    :caption: Application details

    supported_cameras.rst

Development logic
=================
The :code:`XiLens` application relies at its core on many `Qt <https://www.qt.io/product/qt6>`_ components for the GUI, while
adding a custom look for the interface. :code:`XiLens` is developed based on the principles that usability and performance are
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
