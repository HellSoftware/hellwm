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

HELLWM=./src/hellwm.c

INCLUDE=\
		  -I./ 						\
		  -I./include 				\
		  -I./include/hellcli  

SRC=\
	 ./src/hellwm.c 					\
	 ./src/server.c 					\
	 ./src/config.c 					\
	 ./src/layer_shell.c 			\
	 ./src/hellcli/serv_hellcli.c

hellwm: $(HELLWM) $(SRC) xdg-shell-protocol.h wlr-layer-shell-unstable-v1-protocol.h 
		$(CC)	-g -Werror -DWLR_USE_UNSTABLE $(INCLUDE) xdg-shell-protocol.h -o $@ $< $(LIBS) 

hellcli: src/hellcli/hellcli.c
	$(CC)	-o $@ $<

.DEFAULT_GOAL=hellwm 

.PHONY: clean
clean:
	rm -f hellwm hellcli xdg-shell-protocol.h xdg-shell-protocol.c wlr-layer-shell-unstable-v1-protocol.h logfile.txt
