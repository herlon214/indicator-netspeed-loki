CC=gcc
CFLAGS=-g -Wall -Wfatal-errors -std=c99 $(shell pkg-config --cflags --libs gtk+-3.0 appindicator3-0.1 libgtop-2.0)

all: indicator-netspeed-unity

indicator-netspeed-unity: indicator-netspeed-unity.c
	$(CC) $< $(CFLAGS) -o $@

clean:
	rm -f *.o indicator-netspeed-unity

install:
	install -Dm 755 -s indicator-netspeed-unity $(DESTDIR)/usr/bin/indicator-netspeed-unity
	install -Dm 644 indicator-netspeed-unity.gschema.xml $(DESTDIR)/usr/share/glib-2.0/schemas/indicator-netspeed-unity.gschema.xml
	install -Dm 644 indicator-netspeed-unity.desktop $(DESTDIR)/etc/xdg/autostart/indicator-netspeed-unity.desktop
	install -Dm 644 indicator-netspeed-unity.desktop $(DESTDIR)/usr/share/applications/indicator-netspeed-unity.desktop
	install -Dm 644 resources/indicator-netspeed-idle-d.svg $(DESTDIR)/usr/share/pixmaps/indicator-netspeed-unity/indicator-netspeed-idle-d.svg
	install -Dm 644 resources/indicator-netspeed-idle-l.svg $(DESTDIR)/usr/share/pixmaps/indicator-netspeed-unity/indicator-netspeed-idle-l.svg
	install -Dm 644 resources/indicator-netspeed-receive-d.svg $(DESTDIR)/usr/share/pixmaps/indicator-netspeed-unity/indicator-netspeed-receive-d.svg
	install -Dm 644 resources/indicator-netspeed-receive-l.svg $(DESTDIR)/usr/share/pixmaps/indicator-netspeed-unity/indicator-netspeed-receive-l.svg
	install -Dm 644 resources/indicator-netspeed-transmit-d.svg $(DESTDIR)/usr/share/pixmaps/indicator-netspeed-unity/indicator-netspeed-transmit-d.svg
	install -Dm 644 resources/indicator-netspeed-transmit-l.svg $(DESTDIR)/usr/share/pixmaps/indicator-netspeed-unity/indicator-netspeed-transmit-l.svg
	install -Dm 644 resources/indicator-netspeed-transmit-receive-d.svg $(DESTDIR)/usr/share/pixmaps/indicator-netspeed-unity/indicator-netspeed-transmit-receive-d.svg
	install -Dm 644 resources/indicator-netspeed-transmit-receive-l.svg $(DESTDIR)/usr/share/pixmaps/indicator-netspeed-unity/indicator-netspeed-transmit-receive-l.svg
	#glib-compile-schemas $(DESTDIR)/usr/share/glib-2.0/schemas/
	install -Dm 755 install_nethogs.sh $(DESTDIR)/usr/share/indicator-netspeed-unity/install_nethogs.sh
	bash potomo.sh $(DESTDIR)/usr/share/locale

uninstall:
	rm $(DESTDIR)/usr/bin/indicator-netspeed-unity
	rm $(DESTDIR)/usr/share/glib-2.0/schemas/indicator-netspeed-unity.gschema.xml
	rm $(DESTDIR)/etc/xdg/autostart/indicator-netspeed-unity.desktop
	rm $(DESTDIR)/usr/share/applications/indicator-netspeed-unity.desktop
	rm $(DESTDIR)/usr/share/pixmaps/indicator-netspeed-unity/indicator-netspeed-idle-d.svg
	rm $(DESTDIR)/usr/share/pixmaps/indicator-netspeed-unity/indicator-netspeed-idle-l.svg
	rm $(DESTDIR)/usr/share/pixmaps/indicator-netspeed-unity/indicator-netspeed-receive-d.svg
	rm $(DESTDIR)/usr/share/pixmaps/indicator-netspeed-unity/indicator-netspeed-receive-l.svg
	rm $(DESTDIR)/usr/share/pixmaps/indicator-netspeed-unity/indicator-netspeed-transmit-d.svg
	rm $(DESTDIR)/usr/share/pixmaps/indicator-netspeed-unity/indicator-netspeed-transmit-l.svg
	rm $(DESTDIR)/usr/share/pixmaps/indicator-netspeed-unity/indicator-netspeed-transmit-receive-d.svg
	rm $(DESTDIR)/usr/share/pixmaps/indicator-netspeed-unity/indicator-netspeed-transmit-receive-l.svg
	rmdir $(DESTDIR)/usr/share/pixmaps/indicator-netspeed-unity/
	rm $(DESTDIR)/usr/share/indicator-netspeed-unity/install_nethogs.sh
	rmdir $(DESTDIR)/usr/share/indicator-netspeed-unity/
	glib-compile-schemas $(DESTDIR)/usr/share/glib-2.0/schemas/
