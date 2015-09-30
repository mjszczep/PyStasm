.. PyStasm documentation master file, created by
   sphinx-quickstart on Wed Sep 30 05:54:01 2015.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

PyStasm Documentation
=====================

.. toctree::
   :maxdepth: 2

.. automodule:: stasm

A minimal example with PyStasm looks like this:

.. code-block:: python

    import os.path
    import cv2
    import stasm
    
    path = os.path.join(stasm.DATADIR, 'testface.jpg')
    
    img = cv2.imread(path, cv2.IMREAD_GRAYSCALE)
    
    if img is None:
        print("Cannot load", path)
        raise SystemExit

    landmarks = stasm.search_single(img)
    
    if len(landmarks) == 0:
        print("No face found in", path)
    else:
        landmarks = stasm.force_points_into_image(landmarks, img)
        for point in landmarks:
            img[round(point[1])][round(point[0])] = 255
    
    cv2.imshow("stasm minimal", img)
    cv2.waitKey(0)

Functions
---------
.. autofunction:: stasm.init
.. autofunction:: stasm.open_image
.. autofunction:: stasm.search_auto
.. autofunction:: stasm.search_single
.. autofunction:: stasm.search_pinned
.. autofunction:: stasm.lasterr
.. autofunction:: stasm.force_points_into_image
.. autofunction:: stasm.convert_shape

Exceptions
----------
.. autoexception:: StasmException

Data
----
.. py:data:: DATADIR
   :annotation: = <installation-dependent>
.. py:data:: SHAPE17
   :annotation: = 17
.. py:data:: BIOID
   :annotation: = 20
.. py:data:: AR
   :annotation: = 22
.. py:data:: XM2VTS
   :annotation: = 68
.. py:data:: MUCT76
   :annotation: = 76
.. py:data:: NLANDMARKS
   :annotation: = 77
.. py:data:: STASM_VERSION
   :annotation: = '4.1.0'

.. Indices and tables
.. ==================
..  
..  * :ref:`genindex`
..  * :ref:`modindex`
..  * :ref:`search`

