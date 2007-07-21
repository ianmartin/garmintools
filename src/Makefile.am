garmintoolsincludedir = $(includedir)

garmintoolsinclude_HEADERS = garmin.h

lib_LTLIBRARIES = libgarmin.la

libgarmin_la_SOURCES = \
	usb_comm.c \
	byte_util.c \
	unpack.c \
	pack.c \
	protocol.c \
	command.c \
	packet_id.c \
	print.c \
	scan.c \
	datatype.c \
	symbol_name.c \
	run.c

# Updating version info:
#
# version is c:r:a (current:revision:age)
#
# Any change at all => c:r+1:a
# Interfaces changed/added/removed => c+1:0:a
# Interfaces added => c+1:0:a+1
# Interfaces removed => c+1:0:0

libgarmin_la_LDFLAGS = \
	-version-info 3:0:1

bin_PROGRAMS = \
	garmin_save_runs \
	garmin_dump \
	garmin_get_info \
	garmin_gmap

AM_CFLAGS = $(USB_CFLAGS) -Wall

garmin_save_runs_SOURCES = garmin_save_runs.c

garmin_save_runs_LDADD = $(lib_LTLIBRARIES) @LDFLAGS@ @PROG_LIBS@

garmin_dump_SOURCES = garmin_dump.c

garmin_dump_LDADD = $(lib_LTLIBRARIES) @LDFLAGS@ @PROG_LIBS@

garmin_get_info_SOURCES = garmin_get_info.c

garmin_get_info_LDADD = $(lib_LTLIBRARIES) @LDFLAGS@ @PROG_LIBS@

garmin_gmap_SOURCES = garmin_gmap.c

garmin_gmap_LDADD = $(lib_LTLIBRARIES) @LDFLAGS@ @PROG_LIBS@ -lm
