CC=gcc
CFLAGS=

all:
	python get_layouts.py > keyboards.h
	$(CC) system-setup-keyboard.c `pkg-config --cflags --libs hal glib-2.0` $(CFLAGS) -o system-setup-keyboard 

install:
	install -D -m 755 system-setup-keyboard $(DESTDIR)/usr/bin/system-setup-keyboard
	install -D -m 644 10-x11-keymap.fdi $(DESTDIR)/usr/share/hal/fdi/policy/10osvendor/10-x11-keymap.fdi

clean:
	rm -f system-setup-keyboard keyboards.h
