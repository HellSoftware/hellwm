WAYLAND_PROTOCOLS=$(shell pkg-config --variable=pkgdatadir wayland-protocols)
WAYLAND_SCANNER=$(shell pkg-config --variable=wayland_scanner wayland-scanner)
LIBS=\
	 $(shell pkg-config --cflags --libs "wlroots >= 0.18.0-dev") \
	 $(shell pkg-config --cflags --libs wayland-server) \
	 $(shell pkg-config --cflags --libs xkbcommon)

wlr-layer-shell-unstable-v1-protocol.h:
	$(WAYLAND_SCANNER) server-header \
		protocol/wlr-layer-shell-unstable-v1.xml $@

xdg-shell-protocol.h:
	$(WAYLAND_SCANNER) server-header \
		$(WAYLAND_PROTOCOLS)/stable/xdg-shell/xdg-shell.xml $@

hellwm: src/hellwm.c src/server.c src/config.c include/config.h include/server.h xdg-shell-protocol.h wlr-layer-shell-unstable-v1-protocol.h 
	$(CC) $(CFLAGS) \
		-g -Werror -I. -I./src \
		-DWLR_USE_UNSTABLE \
		-o $@ $< \
		$(LIBS)

clean:
	rm -f hellwm xdg-shell-protocol.h xdg-shell-protocol.c wlr-layer-shell-unstable-v1-protocol.h logfile.txt

.DEFAULT_GOAL=hellwm
.PHONY: clean
