# Process this file with autoconf to produce a configure script.

AC_INIT([garmintools],[0.10],[dave@daveb.net])
AC_CONFIG_SRCDIR(src/garmin.h)
AC_CONFIG_HEADERS(src/config.h:src/config.h.in)
AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LIBTOOL

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h stdlib.h string.h sys/time.h termios.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T
AC_STRUCT_TM

# Checks for library functions.
AC_FUNC_SETVBUF_REVERSED
AC_FUNC_STRFTIME
AC_CHECK_FUNCS([memset strerror])

# Check endianness
AC_C_BIGENDIAN

# Checks for libusb
AC_PATH_PROG(LIBUSB_CONFIG,libusb-config)
if test -n "${LIBUSB_CONFIG}"; then
   USB_LIBS="`$LIBUSB_CONFIG --libs`"
   USB_CFLAGS="`$LIBUSB_CONFIG --cflags`"
else
   AC_MSG_ERROR(cannot build garmintools without libusb)
fi

# Checks for python
AC_ARG_WITH(python,[  --with-python		  compile python bindings.],
[
if test "${withval}" = "yes"; then
   PYTHON_SUBDIRS="python" 
   AC_CHECK_PROG(HAVE_PYTHON, "python", "yes")
   if test "${HAVE_PYTHON}" != "yes"; then
      AC_MSG_ERROR([No usable python interpreter found])
   fi
fi
])

PROG_LIBS="$USB_LIBS"

AC_SUBST(USB_CFLAGS)
AC_SUBST(USB_LIBS)
AC_SUBST(PROG_LIBS)
AC_SUBST(PYTHON_SUBDIRS)

AC_CONFIG_FILES([Makefile \
  src/Makefile \
  doc/Makefile \
  doc/man/Makefile \
  extras/Makefile])

if test "${HAVE_PYTHON}" = "yes"; then
   AC_CONFIG_FILES([python/Makefile])
fi

AC_OUTPUT
