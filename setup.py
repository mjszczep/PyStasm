#!/usr/bin/env python
"""Python wrapper around the Stasm library.

Stasm is a software library for finding features in faces.
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

cflags = {'msvc' : ['/EHsc']}

class stasm_build_ext(build_ext):

    def build_extensions(self):
        c = self.compiler.compiler_type
        so = self.compiler.compiler_so
        if c in cflags:
            so.extend(cflags[c])
        # Remove -Wstrict-prototypes since we're compiling C++
        if '-Wstrict-prototypes' in so:
            so.remove('-Wstrict-prototypes')
        build_ext.build_extensions(self)

    def finalize_options(self):
        build_ext.finalize_options(self)
        # See http://stackoverflow.com/a/21621689/2509873
        builtins.__NUMPY_SETUP__ = False
        import numpy
        self.include_dirs.append(numpy.get_include())

metadata = dict(
        name='PyStasm',
        version='0.2.1',
        author='Matthew Szczepankiewicz',
        author_email='mjszczep@buffalo.edu',
	ext_modules=[
            Extension('stasm._stasm',
                      sources = recursive_glob('src', '*.cpp'),
                      depends = recursive_glob('src', '*.h'),
                      #include_dirs = ['include'],
		      #On Windows, these should be of the form eg opencv_core300. I think.
                      libraries = ['opencv_core',
                                   'opencv_imgproc',
                                   'opencv_objdetect',
                                   ],
                      #library_dirs = ['lib'],
                      language = 'C++',
                      )
            ],
        headers = recursive_glob('src', '*.h') + recursive_glob('src', '*.mh'),
        cmdclass = {'build_ext': stasm_build_ext},
        packages = ['stasm'],
        package_data = {'stasm' : ['LICENSE.txt', os.path.join('data','*.*')]},
        include_package_data=True,
        url='http://github.com/mjszczep/PyStasm',
        license='Simplified BSD',
        description=DOCLINES[0],
        long_description='\n'.join(DOCLINES[2:]),
        platforms=['Linux'], #Windows support coming soon
        classifiers=[
            'Programming Language :: C++',
            'Programming Language :: Python :: 2',
            'Programming Language :: Python :: 3',
            #'Operating System :: Microsoft :: Windows', #Coming soon
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
