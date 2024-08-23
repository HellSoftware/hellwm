#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>

#include <wayland-util.h>
#include <wayland-server.h>
#include <wayland-server-core.h>

#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-keysyms.h>

#include <wlroots-0.18/wlr/backend.h>
#include <wlroots-0.18/wlr/util/log.h>
#include <wlroots-0.18/wlr/util/box.h>
#include <wlroots-0.18/wlr/util/edges.h>

#include <wlroots-0.18/wlr/render/allocator.h>
#include <wlroots-0.18/wlr/render/wlr_renderer.h>

#include <wlroots-0.18/wlr/types/wlr_drm.h>
#include <wlroots-0.18/wlr/types/wlr_seat.h>
#include <wlroots-0.18/wlr/types/wlr_scene.h>
#include <wlroots-0.18/wlr/types/wlr_cursor.h>
#include <wlroots-0.18/wlr/types/wlr_buffer.h>
#include <wlroots-0.18/wlr/types/wlr_output.h>
#include <wlroots-0.18/wlr/types/wlr_pointer.h>
#include <wlroots-0.18/wlr/types/wlr_keyboard.h>
#include <wlroots-0.18/wlr/types/wlr_xdg_shell.h>
#include <wlroots-0.18/wlr/types/wlr_compositor.h>
#include <wlroots-0.18/wlr/types/wlr_data_device.h>
#include <wlroots-0.18/wlr/types/wlr_input_device.h>
#include <wlroots-0.18/wlr/types/wlr_output_layout.h>
#include <wlroots-0.18/wlr/types/wlr_subcompositor.h>
#include <wlroots-0.18/wlr/types/wlr_xcursor_manager.h>

#include <wlroots-0.18/wlr/types/wlr_screencopy_v1.h>
#include <wlroots-0.18/wlr/types/wlr_layer_shell_v1.h>
#include <wlroots-0.18/wlr/types/wlr_data_control_v1.h>
#include <wlroots-0.18/wlr/types/wlr_linux_dmabuf_v1.h>
#include <wlroots-0.18/wlr/types/wlr_xdg_activation_v1.h>
#include <wlroots-0.18/wlr/types/wlr_fractional_scale_v1.h>
#include <wlroots-0.18/wlr/types/wlr_linux_drm_syncobj_v1.h>
#include <wlroots-0.18/wlr/types/wlr_output_management_v1.h>

/* structures */
enum { scene_layer_bg, scene_layer_bottom, scene_layer_tile, scene_layer_float, scene_layer_top, scene_layer_fs, scene_layer_overlay, scene_layer_block, scene_layers_count };
enum hellwm_cursor_mode { CURSOR_MODE_NORMAL, CURSOR_MODE_MOVE, CURSOR_MODE_PRESS, CURSOR_MODE_RESIZE };

struct hellwm_server 
{
   const char *socket;
   unsigned int cursor_mode;
   double cursor_grab_x, cursor_grab_y;

   /* hellwm */
   struct hellwm_toplevel *grabbed_toplevel;
   struct hellwm_layer_surface *hellwm_layer_surface;

   /* wayland */
   struct wl_list outputs;
   struct wl_list toplevels;
   struct wl_list keyboards;

   struct wl_display *display;

   /* wlroots */
   struct wlr_box grab_geometry;
   struct wlr_box output_layout_geometry;

   struct wlr_scene *scene;
   struct wlr_scene_tree *drag_icon;
   struct wlr_scene_rect *scene_rect;
   struct wlr_scene_output_layout *scene_output_layout;
   struct wlr_scene_tree *layer_surfaces[scene_layers_count]; 

   struct wlr_seat *seat;
   struct wlr_cursor *cursor;
   struct wlr_session *session;
   struct wlr_backend *backend;
   struct wlr_renderer *renderer;
   struct wlr_xdg_shell *xdg_shell;
   struct wlr_allocator *allocator;
   struct wlr_compositor *compositor;
   struct wlr_subcompositor *subcompositor;
   struct wlr_output_layout *output_layout;
   struct wlr_xcursor_manager *cursor_manager;
   struct wlr_data_device_manager *data_device_manager;

   struct wlr_output_manager_v1 *output_manager;
   struct wlr_xdg_activation_v1 *xdg_activation; 
   struct wlr_screencopy_manager_v1 *screencopy_manager;
   struct wlr_data_control_manager_v1 *data_control_manager;


   /* listeners */
   struct wl_listener renderer_lost;

   struct wl_listener cursor_axis;
   struct wl_listener cursor_frame;
   struct wl_listener cursor_button;
   struct wl_listener cursor_motion_relative;
   struct wl_listener cursor_motion_absolute;

   struct wl_listener backend_new_input;
   struct wl_listener backend_new_output;

   struct wl_listener output_manager_test;
   struct wl_listener output_layout_update;
   struct wl_listener output_manager_apply;

   struct wl_listener seat_request_set_cursor;
   struct wl_listener seat_request_set_selection;

   struct wl_listener xdg_shell_new_popup;
   struct wl_listener xdg_shell_new_toplevel;
   struct wl_listener xdg_activation_request_activate;
};

struct hellwm_cursor
{
   struct wlr_cursor *cursor;
   struct wlr_xcursor_manager *cursor_manager;

   /* listeners */
   struct wl_listener cursor_axisnotify;
   struct wl_listener cursor_cursorframe;
   struct wl_listener cursor_buttonpress;
   struct wl_listener cursor_motionrelative;
   struct wl_listener cursor_motionabsolute;
};

struct hellwm_keyboard
{
   struct wl_list link;
   struct wlr_keyboard *wlr_keyboard;
   
   /* listeners */
   struct wl_listener key;
   struct wl_listener destroy;
   struct wl_listener modifiers;
};

struct hellwm_layer_surface
{
   int mapped;
   int isfloating;
   unsigned int type; /* layershell: background = 0; bottom = 1; top = 2; overlay = 3 */ 
   
   struct wl_list link;
   
   struct wlr_box geometry;
   struct wlr_scene_tree *scene;
   struct wlr_scene_tree *popups;
   
   struct wlr_layer_surface_v1 *layer_surface;
   struct wlr_scene_layer_surface_v1 *scene_layer;
   
   /* listeners */
   struct wl_listener unmap;
   struct wl_listener destroy;
   struct wl_listener surface_commit;
};

struct hellwm_toplevel
{
   uint32_t resize;
   unsigned int border_size;
   struct hellwm_output *output;

	struct wl_list link;

	struct wlr_box bounds;
	struct wlr_box geometry; /* layout-relative, includes border */
	struct wlr_box prev_geometry; /* prev, layout-relative, includes border */

	struct wlr_scene_tree *scene;
	struct wlr_scene_rect *border[4]; /* top, bottom, left, right */
	struct wlr_scene_tree *scene_surface;
   
   struct wlr_xdg_toplevel *xdg_toplevel;

   /* listeners */
   struct wl_listener xdg_toplevel_map;
   struct wl_listener xdg_toplevel_unmap;
   struct wl_listener xdg_toplevel_commit;
   struct wl_listener xdg_toplevel_destroy;
   struct wl_listener xdg_toplevel_set_title;
   struct wl_listener xdg_toplevel_set_app_id;
   struct wl_listener xdg_toplevel_set_parent;
   struct wl_listener xdg_toplevel_request_move;
   struct wl_listener xdg_toplevel_request_resize;
   struct wl_listener xdg_toplevel_request_maximize;
   struct wl_listener xdg_toplevel_request_minimize;
   struct wl_listener xdg_toplevel_request_fullscreen;
   struct wl_listener xdg_toplevel_request_show_window_menu;
};

struct hellwm_output
{
   struct wl_list link;
   struct wl_list layers[4]; /* hellwm_layer_surface.link */

   struct wlr_box output_area;
   struct wlr_box window_area;

   struct wlr_output *wlr_output;
   struct wlr_scene_output *scene_output;
   struct wlr_scene_rect *output_scene_rect;

   /* listeners */
   struct wl_listener output_frame;
   struct wl_listener output_destroy;
   struct wl_listener output_request_state;
};

/* functions declarations */
void ERR(char *where);
void LOG(const char *format, ...);

void arrange_layers(struct hellwm_output *output);
void hellwm_toplevel_resize(struct hellwm_toplevel *toplevel, struct wlr_box geo);
void output_manager_test_or_apply(struct wlr_output_configuration_v1 *config, int test_or_apply);

static void process_cursor_move(uint32_t time);
static void process_cursor_resize(uint32_t time);
static void process_cursor_motion(uint32_t time);
static void hellwm_focus_toplevel(struct hellwm_toplevel *toplevel, struct wlr_surface *surface);
static struct hellwm_toplevel *hellwm_toplevel_at_coords(double lx, double ly, struct wlr_surface **surface, double *sx, double *sy);

static void hellwm_new_pointer(struct wlr_input_device *device);
static void hellwm_new_keyboard(struct wlr_input_device *device);

static void handle_renderer_lost(struct wl_listener *listener, void *data);

static void handle_backend_new_input(struct wl_listener *listener, void *data);
static void handle_backend_new_output(struct wl_listener *listener, void *data);

static void handle_seat_set_request_cursor(struct wl_listener *listener, void *data);
static void handle_seat_request_set_selection(struct wl_listener *listener, void *data);

static void handle_keyboard_key(struct wl_listener *listener, void *data);
static void handle_keyboard_destroy(struct wl_listener *listener, void *data);
static void handle_keyboard_modifiers(struct wl_listener *listener, void *data);

static void handle_cursor_axis(struct wl_listener *listener, void *data);
static void handle_cursor_frame(struct wl_listener *listener, void *data);
static void handle_cursor_button(struct wl_listener *listener, void *data);
static void handle_cursor_motion_relative(struct wl_listener *listener, void *data);
static void handle_cursor_motion_absolute(struct wl_listener *listener, void *data);

static void handle_output_frame(struct wl_listener *listener, void *data);
static void handle_output_destroy(struct wl_listener *listener, void *data);
static void handle_output_manager_test(struct wl_listener *listener, void *data);
static void handle_output_request_state(struct wl_listener *listener, void *data);
static void handle_output_layout_update(struct wl_listener *listener, void *data);
static void handle_output_manager_apply(struct wl_listener *listener, void *data);

static void handle_xdg_toplevel_map(struct wl_listener *listener, void *data);
static void handle_xdg_toplevel_unmap(struct wl_listener *listener, void *data);
static void handle_xdg_shell_new_popup(struct wl_listener *listener, void *data);
static void handle_xdg_toplevel_commit(struct wl_listener *listener, void *data);
static void handle_xdg_toplevel_destroy(struct wl_listener *listener, void *data);
static void handle_xdg_shell_new_toplevel(struct wl_listener *listener, void *data);
static void handle_xdg_toplevel_set_title(struct wl_listener *listener, void *data);
static void handle_xdg_toplevel_set_app_id(struct wl_listener *listener, void *data);
static void handle_xdg_toplevel_set_parent(struct wl_listener *listener, void *data);
static void handle_xdg_toplevel_request_move(struct wl_listener *listener, void *data);
static void handle_xdg_toplevel_request_resize(struct wl_listener *listener, void *data);
static void handle_xdg_toplevel_request_maximize(struct wl_listener *listener, void *data);
static void handle_xdg_toplevel_request_minimize(struct wl_listener *listener, void *data);
static void handle_xdg_toplevel_request_fullscreen(struct wl_listener *listener, void *data);
static void handle_xdg_activation_request_activate(struct wl_listener *listener, void *data);
static void handle_xdg_toplevel_request_show_window_menu(struct wl_listener *listener, void *data);


/* global variables*/
struct hellwm_server server;

/* functions implementations */
void LOG(const char *format, ...)
{
   va_list ap;
   va_start(ap, format);
   vfprintf(stdout, format, ap);
   va_end(ap);
}

void ERR(char *where)
{
   perror(where);
   exit (1);
}

void arrange_layers(struct hellwm_output *output)
{
   // TODO: arrange_layers()
   return;
}

void hellwm_toplevel_resize(struct hellwm_toplevel *toplevel, struct wlr_box geo)
{
   if (!toplevel->output || !toplevel->xdg_toplevel->base->surface->mapped)
      return;

   /* Update scene-graph, including borders */
   wlr_scene_node_set_position(&toplevel->scene->node, toplevel->geometry.x, toplevel->geometry.y);
   wlr_scene_node_set_position(&toplevel->scene_surface->node, toplevel->border_size, toplevel->border_size);

   wlr_scene_rect_set_size(toplevel->border[0], toplevel->geometry.width, toplevel->border_size);
   wlr_scene_rect_set_size(toplevel->border[1], toplevel->geometry.width, toplevel->border_size);
   wlr_scene_rect_set_size(toplevel->border[2], toplevel->border_size, toplevel->geometry.height - 2 * toplevel->border_size);
   wlr_scene_rect_set_size(toplevel->border[3], toplevel->border_size, toplevel->geometry.height - 2 * toplevel->border_size);

   wlr_scene_node_set_position(&toplevel->border[1]->node, 0, toplevel->geometry.height - toplevel->border_size);
   wlr_scene_node_set_position(&toplevel->border[2]->node, 0, toplevel->border_size);
   wlr_scene_node_set_position(&toplevel->border[3]->node, toplevel->geometry.width - toplevel->border_size, toplevel->border_size);
   
   toplevel->resize = wlr_xdg_toplevel_set_size(toplevel->xdg_toplevel, toplevel->geometry.width - 2 * toplevel->border_size, toplevel->geometry.height - 2 * toplevel->border_size);
}

static struct hellwm_toplevel *hellwm_toplevel_at_coords(double lx, double ly, struct wlr_surface **surface, double *sx, double *sy)
{
   /* This returns the topmost node in the scene at the given layout coords. */
   struct wlr_scene_tree *tree;
   struct wlr_scene_buffer *scene_buffer;
   struct wlr_scene_surface *scene_surface;
   struct wlr_scene_node *node = wlr_scene_node_at(&server.scene->tree.node, lx, ly, sx, sy);
   
   if (node == NULL || node->type != WLR_SCENE_NODE_BUFFER)
      return NULL;
   
   scene_buffer = wlr_scene_buffer_from_node(node);
   scene_surface = wlr_scene_surface_try_from_buffer(scene_buffer);
   
   if (!scene_surface)
      return NULL;
   
   *surface = scene_surface->surface;
   tree = node->parent;
   
   while (tree != NULL && tree->node.data == NULL)
   {
      tree = tree->node.parent;
   }
   return tree->node.data;
}

static void hellwm_focus_toplevel(struct hellwm_toplevel *toplevel, struct wlr_surface *surface)
{
   struct wlr_keyboard *keyboard;
struct wlr_seat *seat = server.seat;
struct wlr_surface *prev_surface = seat->keyboard_state.focused_surface;

if (toplevel == NULL)
return;

if (prev_surface == surface)	/* Don't re-focus an already focused surface. */
   return;

if (prev_surface)
{
/*
* Deactivate the previously focused surface. This lets the client know
 * it no longer has focus and the client will repaint accordingly, e.g.
 * stop displaying a caret.
 */
   if (wlr_xdg_toplevel_try_from_wlr_surface(prev_surface) != NULL) 
   {
      wlr_xdg_toplevel_set_activated(wlr_xdg_toplevel_try_from_wlr_surface(prev_surface), false);
   }
}
keyboard = wlr_seat_get_keyboard(seat);

/* Move the toplevel to the front */
wlr_scene_node_raise_to_top(&toplevel->scene_surface->node);
wl_list_remove(&toplevel->link);
wl_list_insert(&server.toplevels, &toplevel->link);
/* Activate the new surface */
wlr_xdg_toplevel_set_activated(toplevel->xdg_toplevel, true);
/*
 * Tell the seat to have the keyboard enter this surface. wlroots will keep
 * track of this and automatically send key events to the appropriate
 * clients without additional work on your part.
 */
if (keyboard != NULL)
{
   wlr_seat_keyboard_notify_enter(seat, toplevel->xdg_toplevel->base->surface, keyboard->keycodes, keyboard->num_keycodes, &keyboard->modifiers);
}
}

static void process_cursor_move(uint32_t time)
{
   /* Move the grabbed toplevel to the new position. */
   struct hellwm_toplevel *toplevel = server.grabbed_toplevel;
   
   wlr_scene_node_set_position(&toplevel->scene->node, server.cursor->x - server.cursor_grab_x, server.cursor->y - server.cursor_grab_y);
}

static void process_cursor_resize(uint32_t time)
{
   /*
    * Resizing the grabbed toplevel can be a little bit complicated, because we
    * could be resizing from any corner or edge. This not only resizes the
    * toplevel on one or two axes, but can also move the toplevel if you resize
    * from the top or left edges (or top-left corner).
    *
    * Note that some shortcuts are taken here. In a more fleshed-out
    * compositor, you'd wait for the client to prepare a buffer at the new
    * size, then commit any movement that was prepared.
    */
      struct wlr_box *geo_box;
   struct hellwm_toplevel *toplevel = server.grabbed_toplevel;
   
   double border_x = server.cursor->x - server.cursor_grab_x;
   double border_y = server.cursor->y - server.cursor_grab_y;
   
   int new_width;
   int new_height;
   int new_left = server.grab_geometry.x;
   int new_right = server.grab_geometry.x + server.grab_geometry.width;
   int new_top = server.grab_geometry.y;
   int new_bottom = server.grab_geometry.y + server.grab_geometry.height;
   
   if (toplevel->resize & WLR_EDGE_TOP)
   {
      new_top = border_y;
      if (new_top >= new_bottom)
      {
         new_top = new_bottom - 1;
      }
   } else
   if(toplevel->resize & WLR_EDGE_BOTTOM)
   {
      new_bottom = border_y;
      if (new_bottom <= new_top)
      {
         new_bottom = new_top + 1;
      }
   }
   
   if (toplevel->resize & WLR_EDGE_LEFT)
   {
      new_left = border_x;
      if (new_left >= new_right)
      {
         new_left = new_right - 1;
      }
   } else
   if (toplevel->resize & WLR_EDGE_RIGHT)
   {
      new_right = border_x;
      if (new_right <= new_left)
      {
         new_right = new_left + 1;
      }
   }
   
   geo_box = &toplevel->geometry;
   wlr_scene_node_set_position(&toplevel->scene_surface->node, new_left - geo_box->x, new_top - geo_box->y);
   
   new_width = new_right - new_left;
   new_height = new_bottom - new_top;
   wlr_xdg_toplevel_set_size(toplevel->xdg_toplevel, new_width, new_height);
}

static void process_cursor_motion(uint32_t time)
{
   double sx, sy;

   struct wlr_seat *seat = server.seat;
   struct wlr_surface *surface = NULL;
   struct hellwm_toplevel *toplevel;
   
   if (server.cursor_mode == CURSOR_MODE_MOVE)
   {
      process_cursor_move(time);
      return;
   } else
   if (server.cursor_mode == CURSOR_MODE_RESIZE)
   {
      process_cursor_resize(time);
      return;
   }
   
   /* Otherwise, find the toplevel under the pointer and send the event along. */
   toplevel = hellwm_toplevel_at_coords(server.cursor->x, server.cursor->y, &surface, &sx, &sy);
   
   if (!toplevel)
   {
      /* If there's no toplevel under the cursor, set the cursor image to a
       * default. This is what makes the cursor image appear when you move it
       * around the screen, not over any toplevels. */
      wlr_cursor_set_xcursor(server.cursor, server.cursor_manager, "default");
   }
   if (surface)
   {
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
   } else
   {
      /* Clear pointer focus so future button events and such are not sent to
       * the last client to have the cursor over it. */
      wlr_seat_pointer_clear_focus(seat);
   }
}

static void handle_seat_set_request_cursor(struct wl_listener *listener, void *data)
{
   /* This event is raised by the seat when a client provides a cursor image */
   struct wlr_seat_client *focused_client = server.seat->pointer_state.focused_client;
   struct wlr_seat_pointer_request_set_cursor_event *event = data;
   
   /* This can be sent by any client, so we check to make sure this one is
    * actually has pointer focus first. */
   if (focused_client == event->seat_client)
   {
      /* Once we've vetted the client, we can tell the cursor to use the
       * provided surface as the cursor image. It will set the hardware cursor
       * on the output that it's currently on and continue to do so as the
       * cursor moves between outputs. */
      wlr_cursor_set_surface(server.cursor, event->surface, event->hotspot_x, event->hotspot_y);
   }
}

static void handle_seat_request_set_selection(struct wl_listener *listener, void *data)
{
   struct wlr_seat_request_set_selection_event *event = data;
   wlr_seat_set_selection(server.seat, event->source, event->serial);
}

static void handle_keyboard_modifiers(struct wl_listener *listener, void *data)
{
   /* This event is raised when a modifier key, such as shift or alt, is
    * pressed. We simply communicate this to the client. */
   struct hellwm_keyboard *keyboard = wl_container_of(listener, keyboard, modifiers);
   /*
    * A seat can only have one keyboard, but this is a limitation of the
    * Wayland protocol - not wlroots. We assign all connected keyboards to the
    * same seat. You can swap out the underlying wlr_keyboard like this and
    * wlr_seat handles this transparently.
    */
   wlr_seat_set_keyboard(server.seat, keyboard->wlr_keyboard);
   /* Send modifiers to the client. */
   wlr_seat_keyboard_notify_modifiers(server.seat, &keyboard->wlr_keyboard->modifiers);
}

static bool handle_keybinding(xkb_keysym_t sym)
{
   /*
    * Here we handle compositor keybindings. This is when the compositor is
    * processing keys, rather than passing them on to the client for its own
    * processing.
    *
    * This function assumes Alt is held down.
    */
   switch (sym)
   {
      case XKB_KEY_Escape:
         wl_display_terminate(server.display);
         break;
   
      case XKB_KEY_Return:
         if (fork() == 0)
         {
            execl("/bin/sh", "/bin/sh", "-c", "foot", (void *)NULL);
         }
         break;
   
      default:
      return false;
   }
   return true;
}

static void handle_keyboard_key(struct wl_listener *listener, void *data)
{
   /* This event is raised when a key is pressed or released. */
   struct wlr_keyboard_key_event *event = data;
   struct hellwm_keyboard *keyboard = wl_container_of(listener, keyboard, key);
   
   /* Translate libinput keycode -> xkbcommon */
   uint32_t keycode = event->keycode + 8;
   
   /* Get a list of keysyms based on the keymap for this keyboard */
   const xkb_keysym_t *syms;
   int nsyms = xkb_state_key_get_syms(keyboard->wlr_keyboard->xkb_state, keycode, &syms);
   
   bool handled = false;
   uint32_t modifiers = wlr_keyboard_get_modifiers(keyboard->wlr_keyboard);
   if ((modifiers & WLR_MODIFIER_ALT) && event->state == WL_KEYBOARD_KEY_STATE_PRESSED)
   {
      /* If alt is held down and this button was _pressed_, we attempt to
       * process it as a compositor keybinding. */
      for (int i = 0; i < nsyms; i++)
      {
         handled = handle_keybinding(syms[i]);
      }
   }
   
   if (!handled)
   {
      wlr_seat_set_keyboard(server.seat, keyboard->wlr_keyboard);
      wlr_seat_keyboard_notify_key(server.seat, event->time_msec, event->keycode, event->state);
   }
}

static void handle_keyboard_destroy(struct wl_listener *listener, void *data)
{
   /* This event is raised by the keyboard base wlr_input_device to signal
    * the destruction of the wlr_keyboard. It will no longer receive events
    * and should be destroyed.
    */
   struct hellwm_keyboard *keyboard = wl_container_of(listener, keyboard, destroy);
   wl_list_remove(&keyboard->modifiers.link);
   wl_list_remove(&keyboard->key.link);
   wl_list_remove(&keyboard->destroy.link);
   wl_list_remove(&keyboard->link);
   free(keyboard);
}

static void hellwm_new_keyboard(struct wlr_input_device *device)
{
   struct xkb_keymap *keymap;
   struct xkb_context *context;
   struct wlr_keyboard *wlr_keyboard = wlr_keyboard_from_input_device(device);
   
   struct hellwm_keyboard *keyboard = calloc(1, sizeof(*keyboard));
   keyboard->wlr_keyboard = wlr_keyboard;
   
   /* We need to prepare an XKB keymap and assign it to the keyboard. This
    * assumes the defaults (e.g. layout = "us"). */
    context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    keymap = xkb_keymap_new_from_names(context, NULL, XKB_KEYMAP_COMPILE_NO_FLAGS);
   
   wlr_keyboard_set_keymap(wlr_keyboard, keymap);
   xkb_keymap_unref(keymap);
   xkb_context_unref(context);
   wlr_keyboard_set_repeat_info(wlr_keyboard, 50, 200);

   /* Here we set up listeners for keyboard events. */
   keyboard->key.notify = handle_keyboard_key;
   wl_signal_add(&wlr_keyboard->events.key, &keyboard->key);
   keyboard->destroy.notify = handle_keyboard_destroy;
   wl_signal_add(&device->events.destroy, &keyboard->destroy);
   keyboard->modifiers.notify = handle_keyboard_modifiers;
   wl_signal_add(&wlr_keyboard->events.modifiers, &keyboard->modifiers);
   
   wlr_seat_set_keyboard(server.seat, keyboard->wlr_keyboard);
   
   /* And add the keyboard to our list of keyboards */
   wl_list_insert(&server.keyboards, &keyboard->link);
}

static void hellwm_new_pointer(struct wlr_input_device *device)
{
   /* We don't do anything special with pointers. All of our pointer handling
    * is proxied through wlr_cursor. On another compositor, you might take this
    * opportunity to do libinput configuration on the device to set
    * acceleration, etc. */
   wlr_cursor_attach_input_device(server.cursor, device);
}

static void handle_backend_new_input(struct wl_listener *listener, void *data)
{
   /* This event is raised by the backend when a new input device becomes
    * available. */
   uint32_t caps;
   struct wlr_input_device *device = data;
   
   switch (device->type)
   {
      case WLR_INPUT_DEVICE_KEYBOARD:
         hellwm_new_keyboard(device);
         break;
   
      case WLR_INPUT_DEVICE_POINTER:
         hellwm_new_pointer(device);
         break;
   
      default:
      break;
   }
   
   caps = WL_SEAT_CAPABILITY_POINTER;
   if (!wl_list_empty(&server.keyboards))
   {
   caps |= WL_SEAT_CAPABILITY_KEYBOARD;
   }
   wlr_seat_set_capabilities(server.seat, caps);
}

static void handle_cursor_axis(struct wl_listener *listener, void *data)
{
   /* This event is forwarded by the cursor when a pointer emits an axis event,
    * for example when you move the scroll wheel. */
   struct wlr_pointer_axis_event *event = data;
   
   wlr_seat_pointer_notify_axis(
         server.seat, event->time_msec, event->orientation, event->delta, event->delta_discrete, event->source, event->relative_direction);
}

static void handle_cursor_frame(struct wl_listener *listener, void *data)
{
   /* This event is forwarded by the cursor when a pointer emits an frame
    * event. Frame events are sent after regular pointer events to group
    * multiple events together. For instance, two axis events may happen at the
    * same time, in which case a frame event won't be sent in between. */
   /* Notify the client with pointer focus of the frame event. */
   wlr_seat_pointer_notify_frame(server.seat);

}

static void handle_cursor_button(struct wl_listener *listener, void *data)
{
   /* This event is forwarded by the cursor when a pointer emits a button
 * event. */
   double sx, sy;
   struct hellwm_toplevel *toplevel;
   struct wlr_surface *surface = NULL;
   struct wlr_pointer_button_event *event = data;
   
   /* Notify the client with pointer focus that a button press has occurred */
   wlr_seat_pointer_notify_button(server.seat, event->time_msec, event->button, event->state);
   
   toplevel = hellwm_toplevel_at_coords(server.cursor->x, server.cursor->y, &surface, &sx, &sy);
   
   if (event->state == WL_POINTER_BUTTON_STATE_RELEASED)
   {
      /* If you released any buttons, we exit interactive move/resize mode. */
      server.cursor_mode = CURSOR_MODE_NORMAL;
      server.grabbed_toplevel = NULL;
   } else
   {
      /* Focus that client if the button was _pressed_ */
      hellwm_focus_toplevel(toplevel, surface);
   }
}

static void handle_cursor_motion_relative(struct wl_listener *listener, void *data)
{
   /* The cursor doesn't move unless we tell it to. The cursor automatically
    * handles constraining the motion to the output layout, as well as any
    * special configuration applied for the specific input device which
    * generated the event. You can pass NULL for the device if you want to move
    * the cursor around without any input. */
   
   struct wlr_pointer_motion_event *event = data;
   
   wlr_cursor_move(server.cursor, &event->pointer->base, event->delta_x, event->delta_y);
   process_cursor_motion(event->time_msec);
}

static void handle_cursor_motion_absolute(struct wl_listener *listener, void *data)
{
   /* This event is forwarded by the cursor when a pointer emits an _absolute_
    * motion event, from 0..1 on each axis. This happens, for example, when
    * wlroots is running under a Wayland window rather than KMS+DRM, and you
    * move the mouse over the window. You could enter the window from any edge,
    * so we have to warp the mouse there. There is also some hardware which
    * emits these events. */
   
   struct wlr_pointer_motion_absolute_event *event = data;
   
   wlr_cursor_warp_absolute(server.cursor, &event->pointer->base, event->x, event->y);
   process_cursor_motion(event->time_msec);
}

static void handle_renderer_lost(struct wl_listener *listener, void *data)
{
   struct hellwm_output *output;
   struct wlr_renderer *old_renderer = server.renderer;
   struct wlr_allocator *old_allocator = server.allocator;
   
   LOG("Renderer lost, retrying...");
   
   if (!(server.renderer = wlr_renderer_autocreate(server.backend)))
      ERR("handle_renderer_lost(): wlr_renderer_autocreate()");
   
   if (!(server.allocator = wlr_allocator_autocreate(server.backend, server.renderer)))
      ERR("handle_renderer_lost(): wlr_allocator_autocreate()");
   
   wl_signal_add(&server.renderer->events.lost, &server.renderer_lost);
   wlr_compositor_set_renderer(server.compositor, server.renderer);
   
   wl_list_for_each(output, &server.outputs, link)
      wlr_output_init_render(output->wlr_output, server.allocator, server.renderer);
   
   wlr_allocator_destroy(old_allocator);
   wlr_renderer_destroy(old_renderer);
}

static void handle_xdg_activation_request_activate(struct wl_listener *listener, void *data)
{
   return;
}

static void handle_xdg_toplevel_map(struct wl_listener *listener, void *data)
{
   struct hellwm_toplevel *toplevel = wl_container_of(listener, toplevel, xdg_toplevel_map);

   wl_list_insert(&server.toplevels, &toplevel->link);
   hellwm_focus_toplevel(toplevel, toplevel->xdg_toplevel->base->surface);

}

static void handle_xdg_toplevel_unmap(struct wl_listener *listener, void *data)
{
   struct hellwm_toplevel *toplevel = wl_container_of(listener, toplevel, xdg_toplevel_unmap);

   if (toplevel == server.grabbed_toplevel)
   {
      server.cursor_mode = CURSOR_MODE_NORMAL;
   }
   wl_list_remove(&toplevel->link);
}

static void handle_xdg_toplevel_commit(struct wl_listener *listener, void *data)
{
   struct hellwm_toplevel *toplevel = wl_container_of(listener, toplevel, xdg_toplevel_commit);

   if (toplevel->xdg_toplevel->base->initial_commit)
   {
      wlr_xdg_toplevel_set_wm_capabilities(toplevel->xdg_toplevel, WLR_XDG_TOPLEVEL_WM_CAPABILITIES_FULLSCREEN);
      wlr_xdg_toplevel_set_size(toplevel->xdg_toplevel, 0, 0);
      return;
   }
   
   if (toplevel->xdg_toplevel->base->surface->mapped && toplevel->output)
      hellwm_toplevel_resize(toplevel, toplevel->geometry);
}

static void handle_xdg_toplevel_destroy(struct wl_listener *listener, void *data) 
{
   struct hellwm_toplevel *toplevel = wl_container_of(listener, toplevel, xdg_toplevel_destroy);

   wl_list_remove(&toplevel->xdg_toplevel_map.link);
   wl_list_remove(&toplevel->xdg_toplevel_unmap.link);
   wl_list_remove(&toplevel->xdg_toplevel_commit.link);
   wl_list_remove(&toplevel->xdg_toplevel_destroy.link);
   wl_list_remove(&toplevel->xdg_toplevel_request_move.link);
   wl_list_remove(&toplevel->xdg_toplevel_request_resize.link);
   wl_list_remove(&toplevel->xdg_toplevel_request_maximize.link);
   wl_list_remove(&toplevel->xdg_toplevel_request_fullscreen.link);
   
   free(toplevel);
}

static void handle_xdg_toplevel_set_title(struct wl_listener *listener, void *data) {}
static void handle_xdg_toplevel_set_app_id(struct wl_listener *listener, void *data) {}
static void handle_xdg_toplevel_set_parent(struct wl_listener *listener, void *data) {}
static void handle_xdg_toplevel_request_move(struct wl_listener *listener, void *data) {}
static void handle_xdg_toplevel_request_resize(struct wl_listener *listener, void *data) {}
static void handle_xdg_toplevel_request_maximize(struct wl_listener *listener, void *data) {}
static void handle_xdg_toplevel_request_minimize(struct wl_listener *listener, void *data) {}
static void handle_xdg_toplevel_request_fullscreen(struct wl_listener *listener, void *data) {}
static void handle_xdg_toplevel_request_show_window_menu(struct wl_listener *listener, void *data) {}

static void handle_xdg_shell_new_toplevel(struct wl_listener *listener, void *data)
{
   struct wlr_xdg_toplevel *xdg_toplevel = data;
   struct hellwm_toplevel *toplevel = calloc(1, sizeof(*toplevel));
   
   toplevel->xdg_toplevel = xdg_toplevel;
   toplevel->scene_surface = wlr_scene_xdg_surface_create(&server.scene->tree, xdg_toplevel->base);
   toplevel->scene_surface->node.data = toplevel;
   xdg_toplevel->base->data = toplevel->scene_surface;
   
   wl_signal_add(&xdg_toplevel->base->surface->events.map, &toplevel->xdg_toplevel_map);
   toplevel->xdg_toplevel_map.notify = handle_xdg_toplevel_map;
   
   wl_signal_add(&xdg_toplevel->base->surface->events.unmap, &toplevel->xdg_toplevel_unmap);
   toplevel->xdg_toplevel_unmap.notify = handle_xdg_toplevel_unmap;
   
   wl_signal_add(&xdg_toplevel->base->surface->events.commit, &toplevel->xdg_toplevel_commit);
   toplevel->xdg_toplevel_commit.notify = handle_xdg_toplevel_commit;
   
   wl_signal_add(&xdg_toplevel->events.destroy, &toplevel->xdg_toplevel_destroy);
   toplevel->xdg_toplevel_destroy.notify = handle_xdg_toplevel_destroy;
   
   wl_signal_add(&xdg_toplevel->events.request_move, &toplevel->xdg_toplevel_request_move);
   toplevel->xdg_toplevel_request_move.notify = handle_xdg_toplevel_request_move;
   
   wl_signal_add(&xdg_toplevel->events.request_resize, &toplevel->xdg_toplevel_request_resize);
   toplevel->xdg_toplevel_request_resize.notify = handle_xdg_toplevel_request_resize;
   
   wl_signal_add(&xdg_toplevel->events.request_maximize, &toplevel->xdg_toplevel_request_maximize);
   toplevel->xdg_toplevel_request_maximize.notify = handle_xdg_toplevel_request_maximize;
   
   wl_signal_add(&xdg_toplevel->events.request_fullscreen, &toplevel->xdg_toplevel_request_fullscreen);
   toplevel->xdg_toplevel_request_fullscreen.notify = handle_xdg_toplevel_request_fullscreen;
}

static void handle_xdg_shell_new_popup(struct wl_listener *listener, void *data)
{

}

static void handle_output_layout_update(struct wl_listener *listener, void *data)
{
   struct wlr_output_configuration_v1 *config = wlr_output_configuration_v1_create();
   struct wlr_output_configuration_head_v1 *output_configuration_head;
   struct hellwm_output *output;

   wl_list_for_each(output, &server.outputs, link) 
   {
      if (output->wlr_output->enabled)
      	continue;
      output_configuration_head = wlr_output_configuration_head_v1_create(config, output->wlr_output);
      output_configuration_head->state.enabled = 0;
      wlr_output_layout_remove(server.output_layout, output->wlr_output);
      // TODO:
      output->output_area = output->window_area = (struct wlr_box){0};
   }
   wl_list_for_each(output, &server.outputs, link)
   {
      if (output->wlr_output->enabled && !wlr_output_layout_get(server.output_layout, output->wlr_output))
         wlr_output_layout_add_auto(server.output_layout, output->wlr_output);
   }
   
   wlr_output_layout_get_box(server.output_layout, NULL, &server.output_layout_geometry);
   
   wlr_scene_node_set_position(&server.scene_rect->node, server.output_layout_geometry.x, server.output_layout_geometry.y);
   wlr_scene_rect_set_size(server.scene_rect, server.output_layout_geometry.width, server.output_layout_geometry.height);
   
   wl_list_for_each(output, &server.outputs, link)
   {
      if (!output->wlr_output->enabled)
         continue;
      
      output_configuration_head = wlr_output_configuration_head_v1_create(config, output->wlr_output);
      
      wlr_output_layout_get_box(server.output_layout, output->wlr_output, &output->output_area);
      output->window_area = output->output_area;
      //wlr_scene_output_set_position(output->scene_output, output->output_area.x, output->output_area.y);
      //wlr_scene_node_set_position(&output->output_scene_rect->node, output->output_area.x, output->output_area.y);
      //wlr_scene_rect_set_size(output->output_scene_rect, output->output_area.width, output->output_area.height);
      
      arrange_layers(output);
      
      output_configuration_head->state.x = output->output_area.x;
      output_configuration_head->state.y = output->output_area.y;
   }
   
   wlr_output_manager_v1_set_configuration(server.output_manager, config);
}

void output_manager_test_or_apply(struct wlr_output_configuration_v1 *config, int test_or_apply)
{
   struct wlr_output_configuration_head_v1 *output_configuration_head;
   int succeed = 0;
   
   wl_list_for_each(output_configuration_head, &config->heads, link)
   {
      struct wlr_output *wlr_output = output_configuration_head->state.output;
      struct hellwm_output *output = wlr_output->data;
      struct wlr_output_state state;
      
      wlr_output_state_init(&state);
      wlr_output_state_set_enabled(&state, output_configuration_head->state.enabled);
      if (!output_configuration_head->state.enabled)
      {
         if (test_or_apply == 0) /* APPLY - server.output_manager->events.apply */
         {
            wlr_output_commit_state(wlr_output, &state);
            
            if (wlr_output->enabled && (output->output_area.x != output_configuration_head->state.x || output->output_area.y != output_configuration_head->state.y))
               wlr_output_layout_add(server.output_layout, wlr_output, output_configuration_head->state.x, output_configuration_head->state.y);
      
         }
         else /* TEST - server.output_manager->events.test */
         {
            wlr_output_test_state(wlr_output, &state);
         }
      
         wlr_output_state_finish(&state);
      }
      
      if (output_configuration_head->state.mode)
         wlr_output_state_set_mode(&state, output_configuration_head->state.mode);
      else
         wlr_output_state_set_custom_mode(&state, output_configuration_head->state.custom_mode.width, output_configuration_head->state.custom_mode.height, output_configuration_head->state.custom_mode.refresh);
      
      wlr_output_state_set_transform(&state, output_configuration_head->state.transform);
      wlr_output_state_set_scale(&state, output_configuration_head->state.scale);
      wlr_output_state_set_adaptive_sync_enabled(&state, output_configuration_head->state.adaptive_sync_enabled);
   }

   if (succeed)
      wlr_output_configuration_v1_send_succeeded(config);
   else
      wlr_output_configuration_v1_send_failed(config);
   wlr_output_configuration_v1_destroy(config);
   
   handle_output_layout_update(NULL, NULL);
}

static void handle_output_manager_apply(struct wl_listener *listener, void *data)
{
   struct wlr_output_configuration_v1 *config = data;
   output_manager_test_or_apply(config, 0);
}

static void handle_output_manager_test(struct wl_listener *listener, void *data)
{
   struct wlr_output_configuration_v1 *config = data;
   output_manager_test_or_apply(config, 1);
}

static void handle_output_frame(struct wl_listener *listener, void *data)
{
   /* This function is called every time an output is ready to display a frame,
    * generally at the output's refresh rate (e.g. 60Hz). */
   struct timespec now;
   struct wlr_scene *scene = server.scene;
   struct hellwm_output *output = wl_container_of(listener, output, output_frame);
   struct wlr_scene_output *scene_output = wlr_scene_get_scene_output(scene, output->wlr_output);
   
   wlr_scene_output_commit(scene_output, NULL);
   clock_gettime(CLOCK_MONOTONIC, &now);
   wlr_scene_output_send_frame_done(scene_output, &now);
}

static void handle_output_destroy(struct wl_listener *listener, void *data)
{
   struct hellwm_output *output = wl_container_of(listener, output, output_destroy);
   
   wl_list_remove(&output->link);
   wl_list_remove(&output->output_frame.link);
   wl_list_remove(&output->output_request_state.link);
   wl_list_remove(&output->output_destroy.link);
}

static void handle_output_request_state(struct wl_listener *listener, void *data)
{
   struct wlr_output_event_request_state *event = data;
   wlr_output_commit_state(event->output, event->state);
   handle_output_layout_update(NULL, NULL);
}

static void handle_backend_new_output(struct wl_listener *listener, void *data)
{
   /* This event is raised by the backend when a new output (aka a display or
    * monitor) becomes available. */
   struct hellwm_output *output;
   struct wlr_output_mode *mode;
   struct wlr_output_state state;
   struct wlr_output *wlr_output = data;
   struct wlr_scene_output *scene_output;
   struct wlr_output_layout_output *l_output;
   
   /* Configures the output created by the backend to use our allocator
    * and our renderer. Must be done once, before commiting the output */
   wlr_output_init_render(wlr_output, server.allocator, server.renderer);
   
   /* The output may be disabled, switch it on. */
   wlr_output_state_init(&state);
   wlr_output_state_set_enabled(&state, true);
   
   /* Some backends don't have modes. DRM+KMS does, and we need to set a mode
    * before we can use the output. The mode is a tuple of (width, height,
    * refresh rate), and each monitor supports only a specific set of modes. We
    * just pick the monitor's preferred mode, a more sophisticated compositor
    * would let the user configure it. */
   mode = wlr_output_preferred_mode(wlr_output);
   if (mode != NULL)
      wlr_output_state_set_mode(&state, mode);
   
   /* Atomically applies the new output state. */
   wlr_output_commit_state(wlr_output, &state);
   wlr_output_state_finish(&state);
   
   /* Allocates and configures our state for this output */
   output = calloc(1, sizeof(*output));
   output->wlr_output = wlr_output;
   
   /* Sets up a listener for the output_frame event. */
   output->output_frame.notify = handle_output_frame;
   wl_signal_add(&wlr_output->events.frame, &output->output_frame);
   
   /* Sets up a listener for the state request event. */
   output->output_request_state.notify = handle_output_request_state;
   wl_signal_add(&wlr_output->events.request_state, &output->output_request_state);
   
   /* Sets up a listener for the destroy event. */
   output->output_destroy.notify = handle_output_destroy;
   wl_signal_add(&wlr_output->events.destroy, &output->output_destroy);
   
   wl_list_insert(&server.outputs, &output->link);
   
   /* Adds this to the output layout. The add_auto function arranges outputs
    * from left-to-right in the order they appear. A more sophisticated
    * compositor would let the user configure the arrangement of outputs in the
    * layout.
    *
    * The output layout utility automatically adds a wl_output global to the
    * display, which Wayland clients can see to find out information about the
    * output (such as DPI, scale factor, manufacturer, etc).
    */
   l_output = wlr_output_layout_add_auto(server.output_layout, wlr_output);
   scene_output = wlr_scene_output_create(server.scene, wlr_output);
   wlr_scene_output_layout_add_output(server.scene_output_layout, l_output, scene_output);
}

int main(int argc, char *argv[])
{
   printf("Hello World...\n"); /* https://www.reddit.com/r/ProgrammerHumor/comments/1euwm7v/helloworldfeaturegotmergedguys/ */
   
   wlr_log_init(WLR_DEBUG, NULL);

   /* The Wayland display is managed by libwayland. It handles accepting  
    * clients from the Unix socket, managing Wayland globals, and so on. */
   server.display = wl_display_create();

   /* The backend is a wlroots feature which abstracts the underlying input and
    * output hardware. The autocreate option will choose the most suitable     
    * backend based on the current environment, such as opening an X11 window  
    * if an X11 server is running. */
   server.backend = wlr_backend_autocreate(wl_display_get_event_loop(server.display), &server.session);
   if (!server.backend)
      ERR("wlr_backend_autocreate()");

   /* Configure a listener to be notified when new outputs are available on the
    * backend. */
   wl_list_init(&server.outputs);
   wl_list_init(&server.toplevels);
   wl_list_init(&server.keyboards);

   server.backend_new_output.notify = handle_backend_new_output; 
   wl_signal_add(&server.backend->events.new_output, &server.backend_new_output);
   server.backend_new_input.notify = handle_backend_new_input;
   wl_signal_add(&server.backend->events.new_input, &server.backend_new_input);
  
   /* Autocreates a renderer, either Pixman, GLES2 or Vulkan for us. The user
    * can also specify a renderer using the WLR_RENDERER env var. 
    * The renderer is responsible for defining the various pixel formats it  
    * supports for shared memory, this configures that for clients. */
   server.renderer = wlr_renderer_autocreate(server.backend);
   if (!server.renderer)
      ERR("wlr_renderer_autocreate()");

   wl_signal_add(&server.renderer->events.lost, &server.renderer_lost);
   server.renderer_lost.notify = handle_renderer_lost;
   wlr_renderer_init_wl_display(server.renderer, server.display);
   
   /* Autocreates an allocator for us.
    * The allocator is the bridge between the renderer and the backend. It
    * handles the buffer creation, allowing wlroots to render onto the    
    * screen */
   server.allocator = wlr_allocator_autocreate(server.backend, server.renderer);
   if (!server.allocator)
      ERR("wlr_allocator_autocreate()");
   
   
   server.subcompositor = wlr_subcompositor_create(server.display);
   server.screencopy_manager= wlr_screencopy_manager_v1_create(server.display);
   server.data_device_manager = wlr_data_device_manager_create(server.display);
   server.data_control_manager = wlr_data_control_manager_v1_create(server.display);

   server.compositor = wlr_compositor_create(server.display, 6, server.renderer);

   /* xdg_activation */
   server.xdg_activation = wlr_xdg_activation_v1_create(server.display);
   server.xdg_activation_request_activate.notify = handle_xdg_activation_request_activate; 
   wl_signal_add(&server.xdg_activation->events.request_activate, &server.xdg_activation_request_activate);


   /* output layout */
   server.output_layout = wlr_output_layout_create(server.display);
   server.output_layout_update.notify = handle_output_layout_update; 
   wl_signal_add(&server.output_layout->events.change, &server.output_layout_update);


   /* output manager */
   server.output_manager = wlr_output_manager_v1_create(server.display);
   server.output_manager_apply.notify = handle_output_manager_apply;
   wl_signal_add(&server.output_manager->events.apply, &server.output_manager_apply);
   
   server.output_manager_test.notify = handle_output_manager_test;
   wl_signal_add(&server.output_manager->events.test, &server.output_manager_test);
   
   server.scene = wlr_scene_create();
   server.scene_output_layout = wlr_scene_attach_output_layout(server.scene, server.output_layout);
   server.scene_rect = wlr_scene_rect_create(&server.scene->tree, 0, 0, (float[]){0.4f, 0.f, 0.f, 1.f});
   for (int i = 0; i < scene_layers_count; i++)
      server.layer_surfaces[i] = wlr_scene_tree_create(&server.scene->tree);
   server.drag_icon = wlr_scene_tree_create(&server.scene->tree);
   wlr_scene_node_place_below(&server.drag_icon->node, &server.layer_surfaces[scene_layer_block]->node);
   
   
   /* seat */
   server.seat = wlr_seat_create(server.display, "seat0");
   wl_signal_add(&server.seat->events.request_set_cursor, &server.seat_request_set_cursor);
   server.seat_request_set_cursor.notify = handle_seat_set_request_cursor;
   
   wl_signal_add(&server.seat->events.request_set_selection, &server.seat_request_set_selection);
   server.seat_request_set_selection.notify = handle_seat_request_set_selection;
   
   
   /* cursor */
   server.cursor = wlr_cursor_create();
   server.cursor_manager = wlr_xcursor_manager_create(NULL, 24);
   wlr_cursor_attach_output_layout(server.cursor, server.output_layout);

   wl_signal_add(&server.cursor->events.motion, &server.cursor_motion_relative);
   server.cursor_motion_relative.notify = handle_cursor_motion_relative;

   wl_signal_add(&server.cursor->events.motion_absolute, &server.cursor_motion_absolute);
   server.cursor_motion_absolute.notify = handle_cursor_motion_absolute;

   wl_signal_add(&server.cursor->events.button, &server.cursor_button);
   server.cursor_button.notify = handle_cursor_button;

   wl_signal_add(&server.cursor->events.axis, &server.cursor_axis);
   server.cursor_axis.notify = handle_cursor_axis;

   wl_signal_add(&server.cursor->events.frame, &server.cursor_frame);
   server.cursor_frame.notify = handle_cursor_frame;


   server.xdg_shell = wlr_xdg_shell_create(server.display, 6);
   wl_signal_add(&server.xdg_shell->events.new_toplevel, &server.xdg_shell_new_toplevel);
   server.xdg_shell_new_toplevel.notify = handle_xdg_shell_new_toplevel;
   
   wl_signal_add(&server.xdg_shell->events.new_popup, &server.xdg_shell_new_popup);
   server.xdg_shell_new_popup.notify = handle_xdg_shell_new_popup;
   
   
   /* Make sure that XClient will connect 
    * to the XWayland (if enabled)
    * instead of any other X server */
   
   /* Add a Unix socket to the Wayland display. */
   server.socket = wl_display_add_socket_auto(server.display);
   if (!server.socket)
   {
      wlr_backend_destroy(server.backend);
      ERR("wl_display_add_socket_auto()");
   }
   
   /* Start the backend. */
   if (!wlr_backend_start(server.backend))
   {
      wlr_backend_destroy(server.backend);
      wl_display_destroy(server.display);
      ERR("wlr_backend_start()");
   }
   
   unsetenv("DISPLAY");
   setenv("WAYLAND_DISPLAY", server.socket, 1);
   setenv("XCURSOR_SIZE", "24", 1);
   
   /* Run the Wayland event loop. This does not return until you exit the
    * compositor. Starting the backend rigged up all of the necessary event
    * loop configuration to listen to libinput events, DRM events, generate
    * frame events at the refresh rate, and so on. */
   wl_display_run(server.display);

   /* ------ */
   printf("...Underworld (I mean Hell)\n");
   wl_display_destroy_clients(server.display);

   return 0;
}
