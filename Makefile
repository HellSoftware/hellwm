# Makefile

VERSION = 0.1
PKG_CONFIG = pkg-config

USR_LOCAL_BIN = /usr/local/bin
USR_SHARE = /usr/share

XWAYLAND =
XLIBS =
#XWAYLAND = -DXWAYLAND
#XLIBS = xcb xcb-icccm

# FLAGS
CFLAGS = -I. -DWLR_USE_UNSTABLE -D_POSIX_C_SOURCE=200809L -DVERSION=\"$(VERSION)\" $(XWAYLAND)
DEVCFLAGS = -g -pedantic -Wall -Wextra -Wdeclaration-after-statement -Wno-unused-parameter -Wno-sign-compare -Wshadow -Wunused-macros\
	-Werror=strict-prototypes -Werror=implicit -Werror=return-type -Werror=incompatible-pointer-types

# CFLAGS / LDFLAGS
PKGS      = wlroots-0.18 wayland-server xkbcommon libinput $(XLIBS)
HELLWMCFLAGS = `$(PKG_CONFIG) --cflags $(PKGS)` $(CFLAGS) $(DEVCFLAGS)
LDLIBS    = `$(PKG_CONFIG) --libs $(PKGS)` $(LIBS)

all: hellwm
hellwm: hellwm.o
	$(CC) hellwm.o $(LDLIBS) $(LDFLAGS) $(HELLWMCFLAGS) -o $@
hellwm.o: hellwm.c cursor-shape-v1-protocol.h xdg-shell-protocol.h wlr-layer-shell-unstable-v1-protocol.h

# WAYLAND
WAYLAND_SCANNER   = `$(PKG_CONFIG) --variable=wayland_scanner wayland-scanner`
WAYLAND_PROTOCOLS = `$(PKG_CONFIG) --variable=pkgdatadir wayland-protocols`

xdg-shell-protocol.h:
	$(WAYLAND_SCANNER) server-header \
		$(WAYLAND_PROTOCOLS)/stable/xdg-shell/xdg-shell.xml $@
wlr-layer-shell-unstable-v1-protocol.h:
	$(WAYLAND_SCANNER) server-header \
		protocols/wlr-layer-shell-unstable-v1.xml $@
cursor-shape-v1-protocol.h:
	$(WAYLAND_SCANNER) server-header \
		$(WAYLAND_PROTOCOLS)/staging/cursor-shape/cursor-shape-v1.xml $@

clean:
	rm -f hellwm *.o *-protocol.h

# CREATE PACKAGE
dist: clean
	mkdir -p hellwm-$(VERSION)
	cp -R LICENSE* Makefile README.md protocols hellwm.c hellwm-$(VERSION)
	tar -caf hellwm-$(VERSION).tar.gz hellwm-$(VERSION)
	rm -rf hellwm-$(VERSION)

install: hellwm
	mkdir -p $(USR_LOCAL_BIN)
	cp -f hellwm $(USR_LOCAL_BIN)
	chmod 755 $(USR_LOCAL_BIN)/hellwm # chmod u=rwx,g=rx,o=rx
	mkdir -p $(USR_SHARE)/wayland-sessions
	cp -f hellwm.desktop $(USR_SHARE)/wayland-sessions/hellwm.desktop
	chmod 644 $(USR_SHARE)/wayland-sessions/hellwm.desktop # chmod u=rwx,g=rx,o=rx
uninstall:
	rm -f $(USR_LOCAL_BIN)/hellwm $(USR_SHARE)/wayland-sessions/hellwm.desktop

.SUFFIXES: .c .o
.c.o:
	$(CC) $(CPPFLAGS) $(HELLWMCFLAGS) -c $<
