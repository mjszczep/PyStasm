#!/usr/bin/env python
"""Python wrapper for finding features in faces.

Stasm is a C++ software library for finding features in faces.
For more information, visit http://www.milbo.users.sonic.net/stasm/
"""

DOCLINES = __doc__.split('\n')

import sys
import os
import fnmatch

if sys.version_info[0] >= 3:
    import builtins
else:
    import __builtin__ as builtins

try:
    from setuptools import setup, Extension
    from setuptools.command.build_ext import build_ext
    using_setuptools = True
except ImportError:
    from distutils.core import setup, Extension
    from distutils.command.build_ext import build_ext
    using_setuptools = False

def recursive_glob(path, match):
    matches = []
    for root, dirnames, filenames in os.walk(path):
        for filename in fnmatch.filter(filenames, match):
            matches.append(os.path.join(root, filename))
    return matches

cv_libs = ['opencv_core',
           'opencv_imgproc',
           'opencv_objdetect']
if sys.platform == 'win32':
    cv_libs = [lib + '300' for lib in cv_libs]

cflags = {'msvc': ['/EHsc']}

class stasm_build_ext(build_ext):
    def build_extensions(self):
        c = self.compiler.compiler_type
        if c == 'unix':
            # Remove -Wstrict-prototypes since we're compiling C++
            so = self.compiler.compiler_so
            if '-Wstrict-prototypes' in so:
                so.remove('-Wstrict-prototypes')
        if c in cflags:
            for e in self.extensions:
                e.extra_compile_args = cflags[c]
        build_ext.build_extensions(self)

    def finalize_options(self):
        build_ext.finalize_options(self)
        # See http://stackoverflow.com/a/21621689/2509873
        builtins.__NUMPY_SETUP__ = False
        import numpy
        self.include_dirs.append(numpy.get_include())

metadata = dict(
        name='PyStasm',
        version='0.3.0',
        author='Matthew Szczepankiewicz',
        author_email='mjszczep@buffalo.edu',
	ext_modules=[
            Extension('stasm._stasm',
                      sources = recursive_glob('src', '*.cpp'),
                      depends = recursive_glob('src', '*.h'),
                      libraries = cv_libs,
                      language = 'C++',
                      )
            ],
        headers=recursive_glob('src', '*.h') + recursive_glob('src', '*.mh'),
        cmdclass={'build_ext': stasm_build_ext},
        packages=['stasm'],
        package_data={'stasm' : ['LICENSE.txt', os.path.join('data','*.*')]},
        include_package_data=True,
        url='http://github.com/mjszczep/PyStasm',
        license='Simplified BSD',
        description=DOCLINES[0],
        long_description='\n'.join(DOCLINES[2:]),
        platforms=['Linux', 'Windows'],
        classifiers=[
            'Programming Language :: C++',
            'Programming Language :: Python :: 2',
            'Programming Language :: Python :: 3',
            'Operating System :: Microsoft :: Windows',
            'Operating System :: Unix',
            'Topic :: Software Development :: Libraries',
            'Topic :: Scientific/Engineering :: Image Recognition',
            'Intended Audience :: Developers',
            'Intended Audience :: Science/Research',
            'Development Status :: 3 - Alpha',
            'License :: OSI Approved :: BSD License',
            ],
        setup_requires=[
            'numpy>=1.7'
            ],
        install_requires=[
            'numpy>=1.7',
            ##'opencv' # Specify this if possible but I don't think it is.
            ],
)

if using_setuptools:
    metadata['zip_safe']=False

if __name__ == '__main__':
    setup(**metadata)
