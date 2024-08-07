WAYLAND_PROTOCOLS=$(shell pkg-config --variable=pkgdatadir wayland-protocols)
WAYLAND_SCANNER=$(shell pkg-config --variable=wayland_scanner wayland-scanner)
LIBS=\
	 $(shell pkg-config --cflags --libs "wlroots-0.18") \
	 $(shell pkg-config --cflags --libs wayland-server) \
	 $(shell pkg-config --cflags --libs xkbcommon) 		 \
	 $(shell pkg-config --cflags --libs lua)				 \
	 $(shell pkg-config --cflags --libs xcb)				 \

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
		  -I./include/lua			\
		  -I./include/hellcli  

SRC=\
	 ./src/hellwm.c 					 \
	 ./src/server.c 					 \
	 ./src/config.c 					 \
	 ./src/xwayland.c 				 \
	 ./src/workspaces.c 				 \
	 ./src/lua/lua_util.c			 \
	 ./src/hellcli/serv_hellcli.c	 \
	 ./src/lua/exposed_functions.c

ARGS=\
	  -DWLR_USE_UNSTABLE
# 	  -Werror \

XWAYLAND=\
	  -DXWAYLAND

noxwayland: $(HELLWM) $(SRC) xdg-shell-protocol.h wlr-layer-shell-unstable-v1-protocol.h 
		$(CC)	-g $(ARGS) $(INCLUDE) xdg-shell-protocol.h -o hellwm $< $(LIBS) 

hellwm: $(HELLWM) $(SRC) xdg-shell-protocol.h wlr-layer-shell-unstable-v1-protocol.h 
		$(CC)	-g $(ARGS) $(XWAYLAND) $(INCLUDE) xdg-shell-protocol.h -o $@ $< $(LIBS) 

hellcli: src/hellcli/hellcli.c
	$(CC)	-o $@ $<

.DEFAULT_GOAL=hellwm 

.PHONY: clean
clean:
	rm -f hellwm hellcli xdg-shell-protocol.h xdg-shell-protocol.c wlr-layer-shell-unstable-v1-protocol.h logfile.log
