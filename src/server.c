#include <assert.h>
#include <complex.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <wchar.h>
#include <wlr/backend.h>
#include <wlr/render/allocator.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_pointer.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_subcompositor.h>
#include <wlr/types/wlr_layer_shell_v1.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/log.h>
#include <xkbcommon/xkbcommon-keysyms.h>
#include <xkbcommon/xkbcommon.h>

#ifdef XWAYLAND
#include <wlr/xwayland.h>
#include <xcb/xcb.h>
#include <xcb/xcb_icccm.h>
#endif

#include "../include/config.h"
#include "../include/server.h"
#include "../include/layer_shell.h"

#define HELLWM_INFO  "INFO"
#define HELLWM_ERROR "ERROR"
#define HELLWM_LOG   "LOG"

// Execute command in new thread
static void exec_cmd(char *command)
{
	if (fork() == 0)
	{
			execl("/bin/sh", "/bin/sh", "-c", command, (void *)NULL);
	}
}

// LOG
void hellwm_log(char *log_type ,const char *format, ...)
{


	char *helwm_log_filename = "logfile.txt";
	va_list args;
   va_start(args, format);
   vprintf(format, args);
   va_end(args);
	printf("\n");

	FILE *logfile = fopen(helwm_log_filename,"a");

	fseek(logfile, 0L, SEEK_END);
	if (ftell(logfile) == 0)
	{
		fprintf(logfile, "******** HELLWM LOGFILE ********\n");
 	}
	fprintf(logfile, "\n%s: ", log_type);
	vfprintf(logfile, format, args);
	fclose(logfile);
}

void hellwm_log_flush()
{
	char *helwm_log_filename = "logfile.txt";
	if (remove(helwm_log_filename)==0)
	{
		hellwm_log(HELLWM_LOG, "logfile deleted"); 
	}
	else
	{
		hellwm_log(HELLWM_ERROR, "unable to delete logfile");
	}
}

void hellwm_toplevel_add_to_list(struct hellwm_server *server, struct hellwm_toplevel *new_toplevel)
{
	if (server->alltoplevels)
	{
		struct hellwm_toplevel_list *instance = malloc(sizeof(struct hellwm_toplevel_list));
		instance->size = 0;
		instance->last_id = 0;
		instance->current_id = 0;
		server->alltoplevels = instance;
	}
	struct hellwm_toplevel_list_element **elements = 
		(struct hellwm_toplevel_list_element**)realloc(server->alltoplevels->list,
				(server->alltoplevels->size + 1) * sizeof(struct hellwm_toplevel_list_element**));

	elements = server->alltoplevels->list;
	elements[server->alltoplevels->size]->toplevel = new_toplevel;

	server->alltoplevels->list = elements;
	server->alltoplevels->size += 1;
	server->alltoplevels->last_id = server->alltoplevels->current_id;
	server->alltoplevels->current_id = server->alltoplevels->size-1;
}

void hellwm_toplevel_remove_from_list(struct wlr_xdg_toplevel *toplevel)
{
	free(toplevel); // idkkk
}

static void hellwm_resize_toplevel_by(struct hellwm_server *server, int32_t w, int32_t h)
{
	struct wlr_surface *focused_surface =
		server->seat->keyboard_state.focused_surface;
	
	struct wlr_xdg_toplevel *toplevel = wlr_xdg_toplevel_try_from_wlr_surface(focused_surface);

	wlr_xdg_toplevel_set_resizing(toplevel,true);

	w = w + toplevel->current.width;
	h = h + toplevel->current.height;

	if (w<=0 || h<=0 )
		return;

	wlr_xdg_toplevel_set_size(
			wlr_xdg_toplevel_try_from_wlr_surface(focused_surface),
			w,
			h
	);

	wlr_xdg_toplevel_set_resizing(toplevel, false);
}

void hellwm_toggle_fullscreen_toplevel(struct hellwm_server *server)
{
	struct wlr_xdg_toplevel *toplevel = wlr_xdg_toplevel_try_from_wlr_surface(
			server->seat->keyboard_state.focused_surface);

	wlr_xdg_toplevel_set_fullscreen(
			toplevel,
			!toplevel->current.fullscreen);
}

static void focus_toplevel(struct hellwm_toplevel *toplevel, struct wlr_surface *surface) {
	/* Note: this function only deals with keyboard focus. */
	if (toplevel == NULL) {
		return;
	}
	struct hellwm_server *server = toplevel->server;
	struct wlr_seat *seat = server->seat;
	struct wlr_surface *prev_surface = seat->keyboard_state.focused_surface;
	if (prev_surface == surface) {
		/* Don't re-focus an already focused surface. */
		return;
	}
	
	if (prev_surface) {
		/*
		 * Deactivate the previously focused surface. This lets the client know
		 * it no longer has focus and the client will repaint accordingly, e.g.
		 * stop displaying a caret.
		 */
		struct wlr_xdg_toplevel *prev_toplevel =
			wlr_xdg_toplevel_try_from_wlr_surface(prev_surface);
		if (prev_toplevel != NULL) {
			wlr_xdg_toplevel_set_activated(prev_toplevel, false);
			// idk
			//wlr_xdg_toplevel_send_close(prev_toplevel);
		}
	}
	struct wlr_keyboard *keyboard = wlr_seat_get_keyboard(seat);
	/* Move the toplevel to the front */
	wlr_scene_node_raise_to_top(&toplevel->scene_tree->node);
	wl_list_remove(&toplevel->link);
	wl_list_insert(&server->toplevels, &toplevel->link);
	/* Activate the new surface */
	wlr_xdg_toplevel_set_activated(toplevel->xdg_toplevel, true);
	/*
	 * Tell the seat to have the keyboard enter this surface. wlroots will keep
	 * track of this and automatically send key events to the appropriate
	 * clients without additional work on your part.
	 */
	if (keyboard != NULL) {
		wlr_seat_keyboard_notify_enter(seat, toplevel->xdg_toplevel->base->surface,
			keyboard->keycodes, keyboard->num_keycodes, &keyboard->modifiers);
	}
}

static void destroy_toplevel(struct hellwm_server *server) {

	if (wl_list_length(&server->toplevels) < 1)
		return;
	
	struct wlr_seat *seat = server->seat;
	struct wlr_surface *prev_surface = seat->keyboard_state.focused_surface;
	struct wlr_xdg_toplevel *prev_toplevel =
			wlr_xdg_toplevel_try_from_wlr_surface(prev_surface);

	struct hellwm_toplevel *next_toplevel =
			wl_container_of(server->toplevels.prev, next_toplevel, link);

	focus_toplevel(next_toplevel, next_toplevel->xdg_toplevel->base->surface);

	/* focus_toplevel(server->alltoplevels->list[server->alltoplevels->
			last_id]->toplevel,server->alltoplevels->list[server->
			alltoplevels->last_id]->toplevel->xdg_toplevel->base->surface);
	*/
	wlr_xdg_toplevel_send_close(prev_toplevel);
}

static void keyboard_handle_modifiers(
		struct wl_listener *listener, void *data) {
	/* This event is raised when a modifier key, such as shift or alt, is
	 * pressed. We simply communicate this to the client. */
	struct hellwm_keyboard *keyboard =
		wl_container_of(listener, keyboard, modifiers);
	/*
	 * A seat can only have one keyboard, but this is a limitation of the
	 * Wayland protocol - not wlroots. We assign all connected keyboards to the
	 * same seat. You can swap out the underlying wlr_keyboard like this and
	 * wlr_seat handles this transparently.
	 */
	wlr_seat_set_keyboard(keyboard->server->seat, keyboard->wlr_keyboard);
	/* Send modifiers to the client. */
	wlr_seat_keyboard_notify_modifiers(keyboard->server->seat,
		&keyboard->wlr_keyboard->modifiers);
}

static bool handle_keybinding(struct hellwm_server *server, xkb_keysym_t sym) {
	/*
	 * Here we handle compositor keybindings. This is when the compositor is
	 * processing keys, rather than passing them on to the client for its own
	 * processing.
	 *
	 * This function assumes Meta key is held down.
	 */
		
	//if (config_status == 1)
	//{
		// function to reload keybinding config file;
	//}

	/* 
	 * hardcoded keybindings that will be loaded from config file
	 * in the future 
	*/
	switch (sym) {
	case XKB_KEY_Return:
		exec_cmd("kitty");
		break;
	case XKB_KEY_q:
		destroy_toplevel(server);
		break;
	case XKB_KEY_e:
		exec_cmd("nemo");	
		break;
	case XKB_KEY_b:
		exec_cmd("firefox");
		break;
	case XKB_KEY_p:
		exec_cmd("pavucontrol");
		break;
	case XKB_KEY_Escape:
		wl_display_terminate(server->wl_display);
		break;
	case XKB_KEY_l:
		hellwm_resize_toplevel_by(server, 50, 0);
		break;
	case XKB_KEY_h:
		hellwm_resize_toplevel_by(server, -50, 0);
		break;
	case XKB_KEY_k:
		hellwm_resize_toplevel_by(server, 0, -50);
		break;
	case XKB_KEY_j:
		hellwm_resize_toplevel_by(server, 0, 50);
		break;
	case XKB_KEY_f:
		hellwm_toggle_fullscreen_toplevel(server);	
		break;
	case XKB_KEY_c:
		/* Cycle to the next toplevel */
		if (wl_list_length(&server->toplevels) < 2) {
			break;
		}
		struct hellwm_toplevel *next_toplevel =
			wl_container_of(server->toplevels.prev, next_toplevel, link);
		focus_toplevel(next_toplevel, next_toplevel->xdg_toplevel->base->surface);
		break;
	default:
		return false;
	}
	return true;
}

static void keyboard_handle_key(
		struct wl_listener *listener, void *data) {
	/* This event is raised when a key is pressed or released. */
	struct hellwm_keyboard *keyboard =
		wl_container_of(listener, keyboard, key);
	struct hellwm_server *server = keyboard->server;
	struct wlr_keyboard_key_event *event = data;
	struct wlr_seat *seat = server->seat;

	/* Translate libinput keycode -> xkbcommon */
	uint32_t keycode = event->keycode + 8;
	/* Get a list of keysyms based on the keymap for this keyboard */
	const xkb_keysym_t *syms;
	int nsyms = xkb_state_key_get_syms(
			keyboard->wlr_keyboard->xkb_state, keycode, &syms);

	bool handled = false;
	uint32_t modifiers = wlr_keyboard_get_modifiers(keyboard->wlr_keyboard);
	if ((modifiers & WLR_MODIFIER_LOGO) &&
			event->state == WL_KEYBOARD_KEY_STATE_PRESSED) {
		/* If SUPER key is held down and this button was _pressed_, we attempt to
		 * process it as a compositor keybinding. */
		for (int i = 0; i < nsyms; i++) {
			handled = handle_keybinding(server, syms[i]);
		}
	}

	if (!handled) {
		/* Otherwise, we pass it along to the client. */
		wlr_seat_set_keyboard(seat, keyboard->wlr_keyboard);
		wlr_seat_keyboard_notify_key(seat, event->time_msec,
			event->keycode, event->state);
	}
}

static void keyboard_handle_destroy(struct wl_listener *listener, void *data) {
	/* This event is raised by the keyboard base wlr_input_device to signal
	 * the destruction of the wlr_keyboard. It will no longer receive events
	 * and should be destroyed.
	 */
	struct hellwm_keyboard *keyboard =
		wl_container_of(listener, keyboard, destroy);
	wl_list_remove(&keyboard->modifiers.link);
	wl_list_remove(&keyboard->key.link);
	wl_list_remove(&keyboard->destroy.link);
	wl_list_remove(&keyboard->link);
	free(keyboard);
}

static void server_new_keyboard(struct hellwm_server *server,
		struct wlr_input_device *device) {
	struct wlr_keyboard *wlr_keyboard = wlr_keyboard_from_input_device(device);

	struct hellwm_keyboard *keyboard = calloc(1, sizeof(*keyboard));
	keyboard->server = server;
	keyboard->wlr_keyboard = wlr_keyboard;

	/* We need to prepare an XKB keymap and assign it to the keyboard. This
	 * assumes the defaults (e.g. layout = "us"). */
	struct xkb_context *context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
	struct xkb_keymap *keymap = xkb_keymap_new_from_names(context, NULL,
		XKB_KEYMAP_COMPILE_NO_FLAGS);

	wlr_keyboard_set_keymap(wlr_keyboard, keymap);
	xkb_keymap_unref(keymap);
	xkb_context_unref(context);
	wlr_keyboard_set_repeat_info(wlr_keyboard, 50, 250);

	/* Here we set up listeners for keyboard events. */
	keyboard->modifiers.notify = keyboard_handle_modifiers;
	wl_signal_add(&wlr_keyboard->events.modifiers, &keyboard->modifiers);
	keyboard->key.notify = keyboard_handle_key;
	wl_signal_add(&wlr_keyboard->events.key, &keyboard->key);
	keyboard->destroy.notify = keyboard_handle_destroy;
	wl_signal_add(&device->events.destroy, &keyboard->destroy);

	wlr_seat_set_keyboard(server->seat, keyboard->wlr_keyboard);

	/* And add the keyboard to our list of keyboards */
	wl_list_insert(&server->keyboards, &keyboard->link);
}

static void server_new_pointer(struct hellwm_server *server,
		struct wlr_input_device *device) {
	/* We don't do anything special with pointers. All of our pointer handling
	 * is proxied through wlr_cursor. On another compositor, you might take this
	 * opportunity to do libinput configuration on the device to set
	 * acceleration, etc. */
	wlr_cursor_attach_input_device(server->cursor, device);
}

static void
server_new_touch(struct hellwm_server *server, struct wlr_input_device *device)
{
	wlr_cursor_attach_input_device(server->cursor, device);
}

static void
server_new_tablet(struct hellwm_server *server, struct wlr_input_device *device)
{
	wlr_cursor_attach_input_device(server->cursor, device);
}

static void server_new_input(struct wl_listener *listener, void *data) {
	/* This event is raised by the backend when a new input device becomes
	 * available. */
	struct hellwm_server *server =
		wl_container_of(listener, server, new_input);
	struct wlr_input_device *device = data;
	switch (device->type) {
	case WLR_INPUT_DEVICE_KEYBOARD:
		server_new_keyboard(server, device);
		break;
	case WLR_INPUT_DEVICE_POINTER:
		server_new_pointer(server, device);
		break;
	case WLR_INPUT_DEVICE_TABLET:
		server_new_tablet(server,device);
		break;
	case WLR_INPUT_DEVICE_TOUCH:
		server_new_touch(server, device);
		break;
	default:
		break;
	}
	/* We need to let the wlr_seat know what our capabilities are, which is
	 * communiciated to the client. In TinyWL we always have a cursor, even if
	 * there are no pointer devices, so we always include that capability. */
	uint32_t caps = WL_SEAT_CAPABILITY_POINTER;
	if (!wl_list_empty(&server->keyboards)) {
		caps |= WL_SEAT_CAPABILITY_KEYBOARD;
	}
	wlr_seat_set_capabilities(server->seat, caps);
}

static void seat_request_cursor(struct wl_listener *listener, void *data) {
	struct hellwm_server *server = wl_container_of(
			listener, server, request_cursor);
	/* This event is raised by the seat when a client provides a cursor image */
	struct wlr_seat_pointer_request_set_cursor_event *event = data;
	struct wlr_seat_client *focused_client =
		server->seat->pointer_state.focused_client;
	/* This can be sent by any client, so we check to make sure this one is
	 * actually has pointer focus first. */
	if (focused_client == event->seat_client) {
		/* Once we've vetted the client, we can tell the cursor to use the
		 * provided surface as the cursor image. It will set the hardware cursor
		 * on the output that it's currently on and continue to do so as the
		 * cursor moves between outputs. */
		wlr_cursor_set_surface(server->cursor, event->surface,
				event->hotspot_x, event->hotspot_y);
	}
}

static void seat_request_set_selection(struct wl_listener *listener, void *data) {
	/* This event is raised by the seat when a client wants to set the selection,
	 * usually when the user copies something. wlroots allows compositors to
	 * ignore such requests if they so choose, but in hellwm we always honor
	 */
	struct hellwm_server *server = wl_container_of(
			listener, server, request_set_selection);
	struct wlr_seat_request_set_selection_event *event = data;
	wlr_seat_set_selection(server->seat, event->source, event->serial);
}

static struct hellwm_toplevel *desktop_toplevel_at(
		struct hellwm_server *server, double lx, double ly,
		struct wlr_surface **surface, double *sx, double *sy) {
	/* This returns the topmost node in the scene at the given layout coords.
	 * We only care about surface nodes as we are specifically looking for a
	 * surface in the surface tree of a hellwm_toplevel. */
	struct wlr_scene_node *node = wlr_scene_node_at(
		&server->scene->tree.node, lx, ly, sx, sy);
	if (node == NULL || node->type != WLR_SCENE_NODE_BUFFER) {
		return NULL;
	}
	struct wlr_scene_buffer *scene_buffer = wlr_scene_buffer_from_node(node);
	struct wlr_scene_surface *scene_surface =
		wlr_scene_surface_try_from_buffer(scene_buffer);
	if (!scene_surface) {
		return NULL;
	}

	*surface = scene_surface->surface;
	/* Find the node corresponding to the hellwm_toplevel at the root of this
	 * surface tree, it is the only one for which we set the data field. */
	struct wlr_scene_tree *tree = node->parent;
	while (tree != NULL && tree->node.data == NULL) {
		tree = tree->node.parent;
	}
	return tree->node.data;
}

static void reset_cursor_mode(struct hellwm_server *server) {
	/* Reset the cursor mode to passthrough. */
	server->cursor_mode = HELLWM_CURSOR_PASSTHROUGH;
	server->grabbed_toplevel = NULL;
}

static void process_cursor_move(struct hellwm_server *server, uint32_t time) {
	/* Move the grabbed toplevel to the new position. */
	struct hellwm_toplevel *toplevel = server->grabbed_toplevel;
	wlr_scene_node_set_position(&toplevel->scene_tree->node,
		server->cursor->x - server->grab_x,
		server->cursor->y - server->grab_y);
}

static void process_cursor_resize(struct hellwm_server *server, uint32_t time) {
	/*
	 * Resizing the grabbed toplevel can be a little bit complicated, because we
	 * could be resizing from any corner or edge. This not only resizes the
	 * toplevel on one or two axes, but can also move the toplevel if you resize
	 * from the top or left edges (or top-left corner).
	 *
	 * Note that some shortcuts are taken here. In a more fleshed-out
	 * compositor, you'd wait for the client to prepare a buffer at the new
	 * size, then commit any movem ent that was prepared.
	 */
	struct hellwm_toplevel *toplevel = server->grabbed_toplevel;
	double border_x = server->cursor->x - server->grab_x;
	double border_y = server->cursor->y - server->grab_y;
	int new_left = server->grab_geobox.x;
	int new_right = server->grab_geobox.x + server->grab_geobox.width;
	int new_top = server->grab_geobox.y;
	int new_bottom = server->grab_geobox.y + server->grab_geobox.height;

	if (server->resize_edges & WLR_EDGE_TOP) {
		new_top = border_y;
		if (new_top >= new_bottom) {
			new_top = new_bottom - 1;
		}
	} else if (server->resize_edges & WLR_EDGE_BOTTOM) {
		new_bottom = border_y;
		if (new_bottom <= new_top) {
			new_bottom = new_top + 1;
		}
	}
	if (server->resize_edges & WLR_EDGE_LEFT) {
		new_left = border_x;
		if (new_left >= new_right) {
			new_left = new_right - 1;
		}
	} else if (server->resize_edges & WLR_EDGE_RIGHT) {
		new_right = border_x;
		if (new_right <= new_left) {
			new_right = new_left + 1;
		}
	}

	struct wlr_box geo_box;
	wlr_xdg_surface_get_geometry(toplevel->xdg_toplevel->base, &geo_box);
	wlr_scene_node_set_position(&toplevel->scene_tree->node,
		new_left - geo_box.x, new_top - geo_box.y);

	int new_width = new_right - new_left;
	int new_height = new_bottom - new_top;
	wlr_xdg_toplevel_set_size(toplevel->xdg_toplevel, new_width, new_height);
}

static void process_cursor_motion(struct hellwm_server *server, uint32_t time) {
	/* If the mode is non-passthrough, delegate to those functions. */
	if (server->cursor_mode == HELLWM_CURSOR_MOVE) {
		process_cursor_move(server, time);
		return;
	} else if (server->cursor_mode == HELLWM_CURSOR_RESIZE) {
		process_cursor_resize(server, time); 
		return;
	}

	/* Otherwise, find the toplevel under the pointer and send the event along. */
	double sx, sy;
	struct wlr_seat *seat = server->seat;
	struct wlr_surface *surface = NULL;
	struct hellwm_toplevel *toplevel = desktop_toplevel_at(server,
			server->cursor->x, server->cursor->y, &surface, &sx, &sy);
	if (!toplevel) {
		/* If there's no toplevel under the cursor, set the cursor image to a
		 * default. This is what makes the cursor image appear when you move it
		 * around the screen, not over any toplevels. */
		wlr_cursor_set_xcursor(server->cursor, server->cursor_mgr, "default");
	}
	if (surface) {
		/*
		 * Send pointer enter and motion events.
		 *
		 * The enter event gives the surface "pointer focus", which is distinct
		 * from keyboard focus. You get pointer focus by moving the pointer over
		 * a window.
		 *
		 * Note that wlroots will avoid sending duplicate enter/motion events if
		 * the surface has already has pointer focus or if the client is already
		 * aware of the coordinates passed.
		 */
		wlr_seat_pointer_notify_enter(seat, surface, sx, sy);
		wlr_seat_pointer_notify_motion(seat, time, sx, sy);
	} else {
		/* Clear pointer focus so future button events and such are not sent to
		 * the last client to have the cursor over it. */
		wlr_seat_pointer_clear_focus(seat);
	}
}

static void server_cursor_motion(struct wl_listener *listener, void *data) {
	/* This event is forwarded by the cursor when a pointer emits a _relative_
	 * pointer motion event (i.e. a delta) */
	struct hellwm_server *server =
		wl_container_of(listener, server, cursor_motion);
	struct wlr_pointer_motion_event *event = data;
	/* The cursor doesn't move unless we tell it to. The cursor automatically
	 * handles constraining the motion to the output layout, as well as any
	 * special configuration applied for the specific input device which
	 * generated the event. You can pass NULL for the device if you want to move
	 * the cursor around without any input. */
	wlr_cursor_move(server->cursor, &event->pointer->base,
			event->delta_x, event->delta_y);
	process_cursor_motion(server, event->time_msec);
}

static void server_cursor_motion_absolute(
		struct wl_listener *listener, void *data) {
	/* This event is forwarded by the cursor when a pointer emits an _absolute_
	 * motion event, from 0..1 on each axis. This happens, for example, when
	 * wlroots is running under a Wayland window rather than KMS+DRM, and you
	 * move the mouse over the window. You could enter the window from any edge,
	 * so we have to warp the mouse there. There is also some hardware which
	 * emits these events. */
	struct hellwm_server *server =
		wl_container_of(listener, server, cursor_motion_absolute);
	struct wlr_pointer_motion_absolute_event *event = data;
	wlr_cursor_warp_absolute(server->cursor, &event->pointer->base, event->x,
		event->y);
	process_cursor_motion(server, event->time_msec);
}

static void server_cursor_button(struct wl_listener *listener, void *data) {
	/* This event is forwarded by the cursor when a pointer emits a button
	 * event. */
	struct hellwm_server *server =
		wl_container_of(listener, server, cursor_button);
	struct wlr_pointer_button_event *event = data;
	/* Notify the client with pointer focus that a button press has occurred */
	wlr_seat_pointer_notify_button(server->seat,
			event->time_msec, event->button, event->state);
	double sx, sy;
	struct wlr_surface *surface = NULL;
	struct hellwm_toplevel *toplevel = desktop_toplevel_at(server,
			server->cursor->x, server->cursor->y, &surface, &sx, &sy);
	if (event->state == WL_POINTER_BUTTON_STATE_RELEASED) {
		/* If you released any buttons, we exit interactive move/resize mode. */
		reset_cursor_mode(server);
	} else {
		/* Focus that client if the button was _pressed_ */
		focus_toplevel(toplevel, surface);
	}
}

static void server_cursor_axis(struct wl_listener *listener, void *data) {
	/* This event is forwarded by the cursor when a pointer emits an axis event,
	 * for example when you move the scroll wheel. */
	struct hellwm_server *server =
		wl_container_of(listener, server, cursor_axis);
	struct wlr_pointer_axis_event *event = data;
	/* Notify the client with pointer focus of the axis event. */
	wlr_seat_pointer_notify_axis(server->seat,
			event->time_msec, event->orientation, event->delta,
			event->delta_discrete, event->source, event->relative_direction);
}

static void server_cursor_frame(struct wl_listener *listener, void *data) {
	/* This event is forwarded by the cursor when a pointer emits an frame
	 * event. Frame events are sent after regular pointer events to group
	 * multiple events together. For instance, two axis events may happen at the
	 * same time, in which case a frame event won't be sent in between. */
	struct hellwm_server *server =
		wl_container_of(listener, server, cursor_frame);
	/* Notify the client with pointer focus of the frame event. */
	wlr_seat_pointer_notify_frame(server->seat);
}

static void output_frame(struct wl_listener *listener, void *data) {
	/* This function is called every time an output is ready to display a frame,
	 * generally at the output's refresh rate (e.g. 60Hz). */
	struct hellwm_output *output = wl_container_of(listener, output, frame);
	struct wlr_scene *scene = output->server->scene;

	struct wlr_scene_output *scene_output = wlr_scene_get_scene_output(
		scene, output->wlr_output);

	/* Render the scene if needed and commit the output */
	wlr_scene_output_commit(scene_output, NULL);

	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	wlr_scene_output_send_frame_done(scene_output, &now);
}

static void output_request_state(struct wl_listener *listener, void *data) {
	/* This function is called when the backend requests a new state for
	 * the output. For example, Wayland and X11 backends request a new mode
	 * when the output window is resized. */
	struct hellwm_output *output = wl_container_of(listener, output, request_state);
	const struct wlr_output_event_request_state *event = data;
	wlr_output_commit_state(output->wlr_output, event->state);
}

static void output_destroy(struct wl_listener *listener, void *data) {
	struct hellwm_output *output = wl_container_of(listener, output, destroy);
	struct hellwm_server *server = output->server;
	
	/*for (int i=0;i<server->outputs_list->count;i++)
	{
		if (server->outputs_list->outputs[i]->wlr_output->name == output->wlr_output->name)
		{
			free(server->outputs_list->outputs[i]);
			server->outputs_list->count--;
		}
	}*/

	wl_list_remove(&output->frame.link);
	wl_list_remove(&output->request_state.link);
	wl_list_remove(&output->destroy.link);
	wl_list_remove(&output->link);
	free(output);
}

static void server_new_output(struct wl_listener *listener, void *data) {
	hellwm_log(HELLWM_LOG, "server_new_output() got called");

	struct hellwm_server *server =
		wl_container_of(listener, server, new_output);
	struct wlr_output *wlr_output = data;

	wlr_output_init_render(wlr_output, server->allocator, server->renderer);

	struct wlr_output_state state;
	wlr_output_state_init(&state);
	wlr_output_state_set_enabled(&state, true);

	struct wlr_output_mode *mode = wlr_output_preferred_mode(wlr_output);

	if (mode != NULL) {
		wlr_output_state_set_mode(&state, mode);
	}

	/* this should be read by config, im working on it... -_- */
	if (!strcmp(wlr_output->name,"DP-2")) {
		wlr_output_state_set_transform(&state, WL_OUTPUT_TRANSFORM_90);
	}

	/* Atomically applies the new output state. */
	wlr_output_commit_state(wlr_output, &state);
	wlr_output_state_finish(&state);

	/* Allocates and configures our state for this output */
	struct hellwm_output *output = calloc(1, sizeof(*output));
	output->wlr_output = wlr_output;
	output->server = server;

	/*  if outputs list is not allocated, do it and add output to the list  
	if (server->outputs_list==NULL)
	{
		struct hellwm_outputs_list *list = malloc(sizeof(struct hellwm_outputs_list));
		list->count=0;
		server->outputs_list=list;
	}
	server->outputs_list->outputs = realloc(server->outputs_list->outputs, (server->outputs_list->count + 1) * sizeof(struct hellwm_output));
   server->outputs_list->outputs[server->outputs_list->count] = output;
	server->outputs_list->count++;
	*/

	/* Sets up a listener for the frame event. */
	output->frame.notify = output_frame;
	wl_signal_add(&wlr_output->events.frame, &output->frame);

	/* Sets up a listener for the state request event. */
	output->request_state.notify = output_request_state;
	wl_signal_add(&wlr_output->events.request_state, &output->request_state);

	/* Sets up a listener for the destroy event. */
	output->destroy.notify = output_destroy;
	wl_signal_add(&wlr_output->events.destroy, &output->destroy);

	wl_list_insert(&server->outputs, &output->link);
	
	hellwm_log("New output: ",wlr_output->name, wlr_output->description);

	/* Adds this to the output layout. The add_auto function arranges outputs
	 * from left-to-right in the order they appear. A more sophisticated
	 * compositor would let the user configure the arrangement of outputs in the
	 * layout.
	 *
	 * The output layout utility automatically adds a wl_output global to the
	 * display, which Wayland clients can see to find out information about the
	 * output (such as DPI, scale factor, manufacturer, etc).
	 */
	struct wlr_output_layout_output *l_output = wlr_output_layout_add_auto(server->output_layout,
		wlr_output);
	struct wlr_scene_output *scene_output = wlr_scene_output_create(server->scene, wlr_output);
	wlr_scene_output_layout_add_output(server->scene_layout, l_output, scene_output);
}

static void xdg_toplevel_map(struct wl_listener *listener, void *data) {
	/* Called when the surface is mapped, or ready to display on-screen. */
	struct hellwm_toplevel *toplevel = wl_container_of(listener, toplevel, map);

	wl_list_insert(&toplevel->server->toplevels, &toplevel->link);

	focus_toplevel(toplevel, toplevel->xdg_toplevel->base->surface);
}

static void xdg_toplevel_unmap(struct wl_listener *listener, void *data) {
	/* Called when the surface is unmapped, and should no longer be shown. */
	struct hellwm_toplevel *toplevel = wl_container_of(listener, toplevel, unmap);

	/* Reset the cursor mode if the grabbed toplevel was unmapped. */
	if (toplevel == toplevel->server->grabbed_toplevel) {
		reset_cursor_mode(toplevel->server);
	}

	wl_list_remove(&toplevel->link);
}

static void xdg_toplevel_commit(struct wl_listener *listener, void *data) {
	/* Called when a new surface state is committed. */
	struct hellwm_toplevel *toplevel = wl_container_of(listener, toplevel, commit);

	if (toplevel->xdg_toplevel->base->initial_commit) {
		/* When an xdg_surface performs an initial commit, the compositor must
		 * reply with a configure so the client can map the surface. hellwm
		 * configures the xdg_toplevel with 0,0 size to let the client pick the
		 * dimensions itself. */
		wlr_xdg_toplevel_set_size(toplevel->xdg_toplevel, 0, 0);
	}
}

static void xdg_toplevel_destroy(struct wl_listener *listener, void *data) {
	/* Called when the xdg_toplevel is destroyed. */
	struct hellwm_toplevel *toplevel = wl_container_of(listener, toplevel, destroy);
	struct hellwm_server *server = toplevel->server;

	wl_list_remove(&toplevel->map.link);
	wl_list_remove(&toplevel->unmap.link);
	wl_list_remove(&toplevel->commit.link);
	wl_list_remove(&toplevel->destroy.link);
	wl_list_remove(&toplevel->request_move.link);
	wl_list_remove(&toplevel->request_resize.link);
	wl_list_remove(&toplevel->request_maximize.link);
	wl_list_remove(&toplevel->request_fullscreen.link);

   //focus_toplevel(focus_toplevel(server->alltoplevels->list[server->alltoplevels->current_id]->toplevel),server->alltoplevels->list[server->alltoplevels->current_id]->toplevel));
	free(toplevel);
	//TODO free server->alltoplevels	
}

static void begin_interactive(struct hellwm_toplevel *toplevel,
		enum hellwm_cursor_mode mode, uint32_t edges) {
	/* This function sets up an interactive move or resize operation, where the
	 * compositor stops propegating pointer events to clients and instead
	 * consumes them itself, to move or resize windows. */
	struct hellwm_server *server = toplevel->server;
	struct wlr_surface *focused_surface =
		server->seat->pointer_state.focused_surface;
	if (toplevel->xdg_toplevel->base->surface !=
			wlr_surface_get_root_surface(focused_surface)) { 
		/* Deny move/resize requests from unfocused clients. */
		return;
	}
	server->grabbed_toplevel = toplevel;
	server->cursor_mode = mode;

	if (mode == HELLWM_CURSOR_MOVE) {
		server->grab_x = server->cursor->x - toplevel->scene_tree->node.x;
		server->grab_y = server->cursor->y - toplevel->scene_tree->node.y;
	} else {
		struct wlr_box geo_box;
		wlr_xdg_surface_get_geometry(toplevel->xdg_toplevel->base, &geo_box);

		double border_x = (toplevel->scene_tree->node.x + geo_box.x) +
			((edges & WLR_EDGE_RIGHT) ? geo_box.width : 0);
		double border_y = (toplevel->scene_tree->node.y + geo_box.y) +
			((edges & WLR_EDGE_BOTTOM) ? geo_box.height : 0);
		server->grab_x = server->cursor->x - border_x;
		server->grab_y = server->cursor->y - border_y;

		server->grab_geobox = geo_box;
		server->grab_geobox.x += toplevel->scene_tree->node.x;
		server->grab_geobox.y += toplevel->scene_tree->node.y;

		server->resize_edges = edges;
	}
}

static void xdg_toplevel_request_move(
		struct wl_listener *listener, void *data) {
	/* This event is raised when a client would like to begin an interactive
	 * move, typically because the user clicked on their client-side
	 * decorations. Note that a more sophisticated compositor should check the
	 * provided serial against a list of button press serials sent to this
	 * client, to prevent the client from requesting this whenever they want. */
	struct hellwm_toplevel *toplevel = wl_container_of(listener, toplevel, request_move);
	begin_interactive(toplevel, HELLWM_CURSOR_MOVE, 0);
}

static void xdg_toplevel_request_resize(
		struct wl_listener *listener, void *data) {
	/* This event is raised when a client would like to begin an interactive
	 * resize, typically because the user clicked on their client-side
	 * decorations. Note that a more sophisticated compositor should check the
	 * provided serial against a list of button press serials sent to this
	 * client, to prevent the client from requesting this whenever they want. */
	struct wlr_xdg_toplevel_resize_event *event = data;
	struct hellwm_toplevel *toplevel = wl_container_of(listener, toplevel, request_resize);
	begin_interactive(toplevel, HELLWM_CURSOR_RESIZE, event->edges);
}

static void xdg_toplevel_request_maximize(
		struct wl_listener *listener, void *data) {
	/* This event is raised when a client would like to maximize itself,
	 * typically because the user clicked on the maximize button on client-side
	 * decorations. hellwm doesn't support maximization, but to conform to
	 * xdg-shell protocol we still must send a configure.
	 * wlr_xdg_surface_schedule_configure() is used to send an empty reply.
	 * However, if the request was sent before an initial commit, we don't do
	 * anything and let the client finish the initial surface earetup. */
	struct hellwm_toplevel *toplevel =
		wl_container_of(listener, toplevel, request_maximize);
	if (toplevel->xdg_toplevel->base->initialized) {
		wlr_xdg_surface_schedule_configure(toplevel->xdg_toplevel->base);
	}
}

static void
xdg_toplevel_request_fullscreen(
		struct wl_listener *listener, void *data)
{
	hellwm_log(HELLWM_LOG, "xdg_toplevel_request_fullscreen() called");
	/* Just as with request_maximize, we must send a configure here. */
	
	struct hellwm_toplevel *toplevel =
		wl_container_of(listener, toplevel, request_fullscreen);

	if (toplevel->xdg_toplevel->base->initialized) {
		wlr_xdg_surface_schedule_configure(toplevel->xdg_toplevel->base);
	}
	wlr_xdg_toplevel_set_fullscreen(
			toplevel->xdg_toplevel,
			!toplevel->xdg_toplevel->current.fullscreen);
}

static void server_new_xdg_toplevel(struct wl_listener *listener, void *data) {
	/* This event is raised when a client creates a new toplevel (application window). */
	struct hellwm_server *server = wl_container_of(listener, server, new_xdg_toplevel);
	struct wlr_xdg_toplevel *xdg_toplevel = data;

	/* Allocate a hellwm_toplevel for this surface */
	struct hellwm_toplevel *toplevel = calloc(1, sizeof(*toplevel));
	toplevel->server = server;
	toplevel->xdg_toplevel = xdg_toplevel;
	toplevel->scene_tree =
		wlr_scene_xdg_surface_create(&toplevel->server->scene->tree, xdg_toplevel->base);
	toplevel->scene_tree->node.data = toplevel;
	xdg_toplevel->base->data = toplevel->scene_tree;


	/* TODO fix this failed attempt to create system of focusing last focused window. */
	//hellwm_toplevel_add_to_list(server, toplevel);

	/* Listen to the various events it can emit */
	toplevel->map.notify = xdg_toplevel_map;
	wl_signal_add(&xdg_toplevel->base->surface->events.map, &toplevel->map);
	toplevel->unmap.notify = xdg_toplevel_unmap;
	wl_signal_add(&xdg_toplevel->base->surface->events.unmap, &toplevel->unmap);
	toplevel->commit.notify = xdg_toplevel_commit;
	wl_signal_add(&xdg_toplevel->base->surface->events.commit, &toplevel->commit);

	toplevel->destroy.notify = xdg_toplevel_destroy;
	wl_signal_add(&xdg_toplevel->events.destroy, &toplevel->destroy);

	/* cotd */
	toplevel->request_move.notify = xdg_toplevel_request_move;
	wl_signal_add(&xdg_toplevel->events.request_move, &toplevel->request_move);
	toplevel->request_resize.notify = xdg_toplevel_request_resize;
	wl_signal_add(&xdg_toplevel->events.request_resize, &toplevel->request_resize);
	toplevel->request_maximize.notify = xdg_toplevel_request_maximize;
	wl_signal_add(&xdg_toplevel->events.request_maximize, &toplevel->request_maximize);
	toplevel->request_fullscreen.notify = xdg_toplevel_request_fullscreen;
	wl_signal_add(&xdg_toplevel->events.request_fullscreen, &toplevel->request_fullscreen);
}

static void xdg_popup_commit(struct wl_listener *listener, void *data) {
	/* Called when a new surface state is committed. */
	struct hellwm_popup *popup = wl_container_of(listener, popup, commit);

	if (popup->xdg_popup->base->initial_commit) {
		/* When an xdg_surface performs an initial commit, the compositor must
		 * reply with a configure so the client can map the surface.
		 * hellwm sends an empty configure. A more sophisticated compositor
		 * might change an xdg_popup's geometry to ensure it's not positioned
		 * off-screen, for example. */
		wlr_xdg_surface_schedule_configure(popup->xdg_popup->base);
	}
}

static void xdg_popup_destroy(struct wl_listener *listener, void *data) {
	/* Called when the xdg_popup is destroyed. */
	struct hellwm_popup *popup = wl_container_of(listener, popup, destroy);

	wl_list_remove(&popup->commit.link);
	wl_list_remove(&popup->destroy.link);

	free(popup);
}

static void server_new_xdg_popup(struct wl_listener *listener, void *data) {
	/* This event is raised when a client creates a new popup. */
	struct wlr_xdg_popup *xdg_popup = data;

	struct hellwm_popup *popup = calloc(1, sizeof(*popup));
	popup->xdg_popup = xdg_popup;

	struct wlr_xdg_surface *parent = wlr_xdg_surface_try_from_wlr_surface(xdg_popup->parent);
	assert(parent != NULL);
	struct wlr_scene_tree *parent_tree = parent->data;
	xdg_popup->base->data = wlr_scene_xdg_surface_create(parent_tree, xdg_popup->base);

	popup->commit.notify = xdg_popup_commit;
	wl_signal_add(&xdg_popup->base->surface->events.commit, &popup->commit);

	popup->destroy.notify = xdg_popup_destroy;
	wl_signal_add(&xdg_popup->events.destroy, &popup->destroy);
}

void hellwm_setup(struct hellwm_server *server)
{
    /* testing config */
	if (true)
	{
		hellwm_config config={NULL,0};
		hellwm_config_load("config/config.conf", &config);
		
		pause();
	}
	 
	//	FUTURE SETUP OF EVERYTING
	
	wlr_log_init(WLR_DEBUG, NULL);
	server->wl_display = wl_display_create();
	
	server->backend = wlr_backend_autocreate(wl_display_get_event_loop(
				server->wl_display), NULL);
	if (server->backend == NULL) {
		hellwm_log(HELLWM_ERROR, "Failed to create wlr_backend");
		exit(EXIT_FAILURE);
	}
	
	server->renderer = wlr_renderer_autocreate(server->backend);
	if (server->renderer == NULL) {
		hellwm_log(HELLWM_ERROR, "Failed to create wlr_renderer");
		exit(EXIT_FAILURE);
	}

	wlr_renderer_init_wl_display(server->renderer, 
			server->wl_display);

	server->allocator = wlr_allocator_autocreate(server->backend,
		server->renderer);
	if (server->allocator == NULL) {
		hellwm_log(HELLWM_ERROR, "Failed to create wlr_allocator");	
		exit(EXIT_FAILURE);
	}

	wlr_compositor_create(server->wl_display
			, 5, server->renderer);
	server->compositor = wlr_compositor_create(server->wl_display,
		6,server->renderer);
	wlr_subcompositor_create(server->wl_display);
	wlr_data_device_manager_create(server->wl_display);

	server->output_layout = wlr_output_layout_create(
			server->wl_display);

	wl_list_init(&server->outputs);
	server->new_output.notify = server_new_output;
	wl_signal_add(&server->backend->events.new_output, 
			&server->new_output);

	server->scene = wlr_scene_create();
	server->scene_layout = wlr_scene_attach_output_layout(server->scene, 
			server->output_layout);

	wl_list_init(&server->toplevels);
	server->xdg_shell = wlr_xdg_shell_create(
			server->wl_display, 3);
	server->new_xdg_toplevel.notify = server_new_xdg_toplevel;
	wl_signal_add(&server->xdg_shell->events.new_toplevel, 
			&server->new_xdg_toplevel);
	
	server->new_xdg_popup.notify = server_new_xdg_popup;
	wl_signal_add(&server->xdg_shell->events.new_popup,
			&server->new_xdg_popup);

	server->layer_shell = wlr_layer_shell_v1_create(server->wl_display,
		4);
	server->new_layer_surface.notify = server_new_layer_surface;
	wl_signal_add(&server->layer_shell->events.new_surface,
		&server->new_layer_surface);

	
	server->cursor = wlr_cursor_create();
	wlr_cursor_attach_output_layout(server->cursor, server->output_layout);

	server->cursor_mgr = wlr_xcursor_manager_create(NULL, 24);

	server->cursor_mode = HELLWM_CURSOR_PASSTHROUGH;
	server->cursor_motion.notify = server_cursor_motion;
	wl_signal_add(&server->cursor->events.motion, 
			&server->cursor_motion);
	server->cursor_motion_absolute.notify = server_cursor_motion_absolute;
	wl_signal_add(&server->cursor->events.motion_absolute,
			&server->cursor_motion_absolute);
	server->cursor_button.notify = server_cursor_button;
	wl_signal_add(&server->cursor->events.button, 
			&server->cursor_button);
	server->cursor_axis.notify = server_cursor_axis;
	wl_signal_add(&server->cursor->events.axis,
			&server->cursor_axis);
	server->cursor_frame.notify = server_cursor_frame;
	wl_signal_add(&server->cursor->events.frame,
			&server->cursor_frame);
	
	wl_list_init(&server->keyboards);
	server->new_input.notify = server_new_input;
	wl_signal_add(&server->backend->events.new_input,
			&server->new_input);
	server->seat = wlr_seat_create(server->wl_display, "seat0");
	server->request_cursor.notify = seat_request_cursor;
	wl_signal_add(&server->seat->events.request_set_cursor,
			&server->request_cursor);
	server->request_set_selection.notify = seat_request_set_selection;
	wl_signal_add(&server->seat->events.request_set_selection,
			&server->request_set_selection);

	/* Add a Unix socket to the Wayland display. */
	server->socket = wl_display_add_socket_auto(server->wl_display);
	if (!server->socket) {
		wlr_backend_destroy(server->backend);
		exit(EXIT_FAILURE);
	}

	if (!wlr_backend_start(server->backend)) {
		wlr_backend_destroy(server->backend);
		wl_display_destroy(server->wl_display);
		exit(EXIT_FAILURE);
	}

	/* Set the WAYLAND_DISPLAY environment variable to our socket, 
	 * XDG_CURRENT_DESKTOP to HellWM and run the startup commands if ANY. */

	setenv("WAYLAND_DISPLAY", server->socket, true);
	setenv("XDG_CURRENT_DESKTOP", "HellWM", true);	

	char **cmds_autostart; // load from config
	if (0==1) { //YES
		for (int i=0;i<sizeof(cmds_autostart);i++)
		{
			exec_cmd(cmds_autostart[i]);
		}
	}

/* Run the Wayland event loop. This does not return until you exit the
	 * compositor. Starting the backend rigged up all of the necessary event
	 * loop configuration to listen to libinput events, DRM events, generate
	 * frame events at the refresh rate, and so on. */
	wlr_log(WLR_INFO,
		"Running Wayland compositor on WAYLAND_DISPLAY=%s",server->socket);
}

void hellwm_destroy_everything(struct hellwm_server *server)
{
	wl_display_destroy_clients(server->wl_display);
	wlr_scene_node_destroy(&server->scene->tree.node);
	wlr_xcursor_manager_destroy(server->cursor_mgr);
	wlr_cursor_destroy(server->cursor);
	wlr_allocator_destroy(server->allocator);
	wlr_renderer_destroy(server->renderer);
	wlr_backend_destroy(server->backend);
	wl_display_destroy(server->wl_display);
}


