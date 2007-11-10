
EXTRA_DIST = setup.py.in garmintools

install-exec-hook:
	python setup.py build --usb_libs="@USB_LIBS@" --usb_cflags="@USB_CFLAGS@" --libdir=$(libdir)
	python setup.py install --prefix=$(DESTDIR)/$(prefix)

