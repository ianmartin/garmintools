
EXTRA_DIST = setup.py garmintools

clean:
	rm -rf build

all:
	python setup.py build --usb_libs="@USB_LIBS@" --usb_cflags="@USB_CFLAGS@" --libdir=$(libdir)

install-exec-hook:
	python setup.py install --prefix=$(DESTDIR)/$(prefix)

