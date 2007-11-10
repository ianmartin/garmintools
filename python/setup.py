#! /usr/bin/env python

import sys

sources = ['pygarmin.c']
include_dirs = ['../src']
library_dirs = []
libraries = ['garmin']

argv = []

for arg in sys.argv :

    usb_libs = arg.split('--usb_libs=')
    if len(usb_libs) == 2 and usb_libs[0] == '' and usb_libs[1] != '' :
        for lib in usb_libs[1].split(' ') :
            if lib[1] == 'L' :
                library_dirs.append(lib.replace('-L', ''))
            if lib[1] == 'l' :
                libraries.append(lib.replace('-l', ''))

    usb_cflags = arg.split('--usb_cflags=')
    if len(usb_cflags) == 2 and usb_cflags[0] == '' and usb_cflags[1] != '' :
        for cflag in usb_cflags[1].split(' ') :
            if cflag[1] == 'I' :
                include_dirs.append(cflag.replace('-I', ''))

    libdir = arg.split('--libdir=')
    if len(libdir) == 2 and libdir[0] == '' and libdir[1] != '' :
        library_dirs.append(libdir[1])

    if arg.count("--prefix=") == 1 :
        argv = [arg]

sys.argv = sys.argv[0:2] + argv

from distutils.core import setup, Extension

extension = Extension('garmintools.pygarmin',
                      include_dirs = include_dirs,
                      libraries = libraries,
                      library_dirs = library_dirs,
                      sources = sources)

setup (name = 'garmintools',
       version = '0.1',
       description = 'Python bindings for garmintools',
       ext_modules = [extension],
       packages = ['garmintools'])
