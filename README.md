Network speed indicator for Unity
=================================

![](https://raw.github.com/GGleb/indicator-netspeed-unity/master/screenshot.png)

Usage
-----

```
sudo apt-get install build-essential libgtop2-dev libgtk-3-dev libappindicator3-dev git-core
git clone https://github.com/GGleb/indicator-netspeed-unity.git
cd indicator-netspeed-unity
make
sudo make install
indicator-netspeed-unity &
```

Deb
-----

```
sudo apt-get install fakeroot
delete line (	glib-compile-schemas $(DESTDIR)/usr/share/glib-2.0/schemas/ ) in Makefile
dpkg-buildpackage -rfakeroot -b

```

The indicator will be put left of all your other indicators. If this is undesirable, the ordering
index can be changed in gsettings:/apps/indicators/netspeed-unity (use dconf-editor).


Credits
-------

Originally written by Marius Gedminas <marius@gedmin.as>

Contributors:

- Tobias Brandt <tob.brandt@gmail.com>
- Stefan Bethge (stefan at lanpartei.de)
- Gleb Golovachev <golovachev.gleb@gmail.com>

