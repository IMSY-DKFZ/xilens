# `XiLens` /ksɪlɛns/

Welcome to the ``XiLens`` documentation. This project aims at providing an application
that can be used to record data from all the `XIMEA` camera families.

![image](resources/ui-animation.gif)

!!! note "Main features"

    * Compatible with all `XIMEA` cameras: spectral, RGB & gray cameras.
    * Highly optimized data storing at video-rate: n-dimensional arrays with  [BLOSC2](https://www.blosc.org/c-blosc2/c-blosc2.html).
    * Multi-instance run for recordings with multiple cameras in parallel.
    * Long-term stability, tested for recordings of 24 hours at over 20 fps.
    * Camera temperature logged automatically during recording.
    * Compatible with Linux systems.
    * Automatic tests of non-UI components through google tests.

!!! info "OS requirements"

    This applications has mainly been developed to be used in **UNIX systems**, however it might also be possible
    to use it in **Windows systems** with minimal changes.
    In addition, the communication with the `XIMEA` cameras requires a USB3 connection for fast image acquisition.
    We also recommend as minimum a `i5` CPU and an SSD for storing the data when using a single camera. If multiple
    cameras should be connected tot he same computer, we recommend using an `i7` or `i9` CPU.


## Acknowledgments
The ``XiLens`` application relies at its core on many [Qt](https://www.qt.io/product/qt6) components for the GUI, while
adding a custom look for the interface. ``XiLens`` is developed based on the principles that usability and performance are
prioritized.

To get the maximum speed while recording data with the `XIMEA` cameras, the [Boost](https://www.boost.org/) library is
used for multi-threading most components of the applications such as displaying images, recording data, etc.

For maintainability, displaying images is done with [OpenCV](https://opencv.org/get-started/).
