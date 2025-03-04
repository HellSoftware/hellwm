#include <time.h>
#include <ctype.h>
#include <stdio.h>
#include <assert.h>
#include <getopt.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/un.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/socket.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-keysyms.h>

#include <wayland-util.h>
#include <wayland-server-core.h>

#include <wlr/backend.h>
#include <wlr/util/box.h>
#include <wlr/util/log.h>
#include <wlr/types/wlr_drm.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/backend/session.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/render/allocator.h>
#include <wlr/backend/libinput.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_pointer.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_viewporter.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_subcompositor.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_server_decoration.h>

#include <wlr/types/wlr_screencopy_v1.h>
#include <wlr/types/wlr_xdg_output_v1.h>
#include <wlr/types/wlr_layer_shell_v1.h>
#include <wlr/types/wlr_data_control_v1.h>
#include <wlr/types/wlr_export_dmabuf_v1.h>
#include <wlr/types/wlr_xdg_decoration_v1.h>
#include <wlr/types/wlr_xdg_activation_v1.h>
#include <wlr/types/wlr_alpha_modifier_v1.h>
#include <wlr/types/wlr_presentation_time.h>
#include <wlr/types/wlr_fractional_scale_v1.h>
#include <wlr/types/wlr_linux_drm_syncobj_v1.h>
#include <wlr/types/wlr_primary_selection_v1.h>
#include <wlr/types/wlr_single_pixel_buffer_v1.h>
#include <wlr/types/wlr_foreign_toplevel_management_v1.h>

#include "wlr-layer-shell-unstable-v1-protocol.h"

#define IPC_SOCKET_PATH "/tmp/hellwm.sock"
#define IPC_BUFFER_SIZE 256


/* structures */
enum hellwm_cursor_mode
{
    HELLWM_CURSOR_MOVE,
    HELLWM_CURSOR_RESIZE,
    HELLWM_CURSOR_PASSTHROUGH,
};

enum hellwm_keybind_type
{
    HELLWM_KEYBIND_COMMAND,
    HELLWM_KEYBIND_FUNCTION,
    HELLWM_KEYBIND_WORKSPACE,
};

enum HELLWM_LAYER
{
    HELLWM_BACKGROUND,
    HELLWM_TOP,
    HELLWM_BOTTOM,
    HELLWM_OVERLAY,
    HELLWM_LAYER_COUNT,
};

enum HELLWM_VIEW
{
    HELLWM_VIEW_TOPLEVEL,
    HELLWM_VIEW_POPUP,
    HELLWM_VIEW_LAYER
};

enum hellwm_border_state
{
    HELLWM_BORDER_INVISIBLE,
    HELLWM_BORDER_INACTIVE,
    HELLWM_BORDER_ACTIVE
};

enum animation_direction
{
    HELLWM_ANIMATION_LEFT,
    HELLWM_ANIMATION_RIGHT,
    HELLWM_ANIMATION_UP,
    HELLWM_ANIMATION_DOWN,
    HELLWM_ANIMATION_GROW,
    HELLWM_ANIMATION_SHRINK,
    HELLWM_ANIMATION_SOLID,
    HELLWM_ANIMATION_COUNT
};

enum hellwm_direction
{
    HELLWM_UP,
    HELLWM_DOWN,
    HELLWM_LEFT,
    HELLWM_RIGHT
};

struct hellwm_server
{
    char **exec_args;
    size_t exec_args_count;
    uint32_t resize_edges;

    int layout_reapply;
    unsigned cursor_mode;
    unsigned mapped_functions;
    double cursor_grab_x, cursor_grab_y;

    /* hellwm */
    pthread_t ipc_thread;
    struct hellwm_output *active_output;
    struct hellwm_toplevel *grabbed_toplevel;
    struct hellwm_workspace *active_workspace;
    struct hellwm_config_manager *config_manager;
    struct hellwm_function_map_entry *hellwm_function_map;

    /* wayland */
    struct wl_list outputs;
    struct wl_list keyboards;
    struct wl_list workspaces;

    struct wl_display *wl_display;
    struct wl_event_loop *event_loop;

    /* wlroots */
    struct wlr_seat *seat;
    struct wlr_scene *scene;
    struct wlr_cursor *cursor;
    struct wlr_backend *backend;
    struct wlr_box grab_geometry;
    struct wlr_session * session;
    struct wlr_renderer *renderer;
    struct wlr_allocator *allocator;
    struct wlr_xdg_shell *xdg_shell;
    struct wlr_compositor *compositor;
    struct wlr_xcursor_manager *cursor_mgr;
    struct wlr_output_layout *output_layout;
    struct wlr_subcompositor *subcompositor;
    struct wlr_scene_output_layout *scene_layout;
    struct wlr_server_decoration_manager *wlr_server_decoration_manager;

    struct hellwm_layer_surface *layer_exclusive_keyboard;

    struct wlr_scene_tree *background_tree; 
    struct wlr_scene_tree *tile_tree; 
    struct wlr_scene_tree *floating_tree; 
    struct wlr_scene_tree *bottom_tree; 
    struct wlr_scene_tree *top_tree; 
    struct wlr_scene_tree *overlay_tree; 
    struct wlr_scene_tree *fullscreen_tree; 

    struct wlr_layer_shell_v1 *layer_shell;
    struct wlr_xdg_decoration_manager_v1 *xdg_decoration_manager;
    struct wlr_foreign_toplevel_manager_v1 *foreign_toplevel_manager;

    /* listeners */
    struct wl_listener renderer_lost;

    struct wl_listener cursor_axis;
    struct wl_listener cursor_frame;
    struct wl_listener cursor_button;
    struct wl_listener cursor_motion;
    struct wl_listener cursor_motion_absolute;
    struct wl_listener cursor_motion_relative;

    struct wl_listener backend_new_input;
    struct wl_listener backend_new_output;

    struct wl_listener request_cursor;
    struct wl_listener request_set_selection;

    struct wl_listener new_xdg_popup;
    struct wl_listener new_xdg_toplevel;

    struct wl_listener new_layer_surface;
    struct wl_listener xdg_decoration_new_toplevel_decoration;
};

struct hellwm_output
{
    struct wl_list link;
    struct wlr_box usable_area;
    struct wlr_box total_area;
    struct wlr_output *wlr_output;

    struct hellwm_server *server;
    struct hellwm_workspace *active_workspace;

    struct {
        struct wl_list background;
        struct wl_list bottom;
        struct wl_list top;
        struct wl_list overlay;
    } layers;

    /* listeners */
    struct wl_listener frame;
    struct wl_listener request_state;
    struct wl_listener destroy;
};

struct hellwm_node
{
    bool is_split;

    union {
        struct {
            float split_ratio;
            bool vertical_split;
            struct hellwm_node *left;
            struct hellwm_node *right;
        } split;

        struct hellwm_toplevel *toplevel;
    };

    struct hellwm_node *parent;
};

struct hellwm_workspace
{
    int id;
    int layout;
    bool fullscreened;

    struct wl_list link;
    struct wl_list toplevels;

    struct hellwm_server *server;
    struct hellwm_output *output;
    struct hellwm_toplevel *now_focused;
    struct hellwm_toplevel *prev_focused;

    struct hellwm_node *layout_tree;
    struct hellwm_node *focused_node;
    struct hellwm_node *next_to_focus_node;
};

struct hellwm_toplevel
{
    bool floating;
    bool fullscreen;
    bool in_animation;
    bool first_layout;
    bool set_fade_clock;
    bool set_animation_clock;

    float previous_bezier;

    enum hellwm_border_state border_state; // type of border state

    struct wl_list link;
    struct timespec fade_start; // for fade animation
    struct timespec animation_start; // for fade animation

    struct hellwm_server *server;
    struct hellwm_workspace *workspace;

    struct wlr_box last_state; // used for example for fullscreen
    struct wlr_box current_geom;          // current
    struct wlr_box pending_geom;          // pending -> currnet
    struct wlr_box desired_geom;          // desired_geom, last one
    struct wlr_box slide_geom;            // anim_end_geom
    struct wlr_box animation_start_geom;  // current_geom, when it started

    struct wlr_scene_rect *borders[4];
    struct wlr_scene_tree *scene_tree;
    struct wlr_xdg_toplevel *xdg_toplevel;
    struct wlr_foreign_toplevel_handle_v1 *foreign_toplevel_handle;

    /* listeners */
    struct wl_listener map;
    struct wl_listener unmap;
    struct wl_listener commit;
    struct wl_listener destroy;
    struct wl_listener set_title;
    struct wl_listener set_app_id;
    struct wl_listener request_move;
    struct wl_listener request_resize;
    struct wl_listener request_maximize;
    struct wl_listener request_fullscreen;
};

struct hellwm_layer_surface
{
    struct wl_list link;

    struct hellwm_server *server;

    struct wlr_scene_layer_surface_v1 *scene;
    struct wlr_layer_surface_v1 *wlr_layer_surface;

    struct wl_listener map;
    struct wl_listener unmap;
    struct wl_listener commit;
    struct wl_listener destroy;
    struct wl_listener new_popup;
};

struct hellwm_view
{
    enum HELLWM_VIEW view_type;
    union {
        struct hellwm_toplevel *toplevel;
        struct hellwm_popup *popup;
        struct hellwm_layer_surface *layer_surface;
    };
};

struct hellwm_popup
{
    struct hellwm_server *server;
    struct wlr_xdg_popup *xdg_popup;
    struct wlr_scene_tree *scene_tree;

    /* listeners */
    struct wl_listener commit;
    struct wl_listener destroy;
};

struct hellwm_keyboard
{
    struct wl_list link;
    struct hellwm_server *server;
    struct wlr_keyboard *wlr_keyboard;

    /* listeners */
    struct wl_listener key;
    struct wl_listener destroy;
    struct wl_listener modifiers;
};

struct hellwm_config_keybind_workspace
{
    int workspace_id;
    int binary_workspace_val;

    bool pressed;
    bool move_with_active;
    bool binary_workspaces_enabled;
};

union hellwm_function
{
    char *as_void;                                     /* If keybind is just executing some command */
    void (*as_func)(struct hellwm_server*);            /* If keybind is running mapped function */
    struct hellwm_config_keybind_workspace workspace;  /* If keybind is meant for changing workspaces */
};

struct hellwm_function_map_entry
{
    const char* name;
    union hellwm_function func;
};

/* keybinding */
typedef struct
{
    int count;
    xkb_keysym_t *keysyms;
    enum hellwm_keybind_type type;
    union hellwm_function *content;
} hellwm_config_keybind;

/* keybindings config */
typedef struct
{
    unsigned count;
    hellwm_config_keybind **keybindings;
} hellwm_config_manager_keybindings;

typedef struct
{
    char *  name;

    char*   rules;
    char*   model;
    char*   layout;
    char*   variant;
    char*   options;

    int32_t delay;
    int32_t rate;

} hellwm_config_keyboard;

typedef struct
{
    int count;
    hellwm_config_keyboard **keyboards; 
    hellwm_config_keyboard *default_keyboard;
} hellwm_config_manager_keyboard;

typedef struct
{
    char *name;

    bool vrr;
    bool enabled;

    float scale;
    int32_t hz;
    int32_t width;
    int32_t height;
    int32_t transfrom;
} hellwm_config_monitor;

typedef struct
{
    int count;
    hellwm_config_monitor **monitors;
} hellwm_config_manager_monitor;

typedef struct 
{
    float master_ratio;

    uint32_t inner_gap;
    uint32_t outer_gap;

    float fade_duration;
    bool fade_decoration;

    float animation_duration;
    float animation_bezier[4];
    bool animations_decoration;
    enum animation_direction default_animation;

    uint32_t border_width;
    float active_border_color[4];
    float inactive_border_color[4];

} hellwm_config_manager_decoration;

typedef struct
{
    bool tap_click;
    bool natural_scroll;
    bool cursor_follow_toplevels;
} hellwm_config_manager_input;

struct hellwm_binary_workspaces_manager
{
    int workspace_sum;
    bool binary_started;

    int keybinds_count;
    hellwm_config_keybind **keybinds;
};

struct hellwm_config_manager
{
    /* Todo : config manger for this */
    unsigned tiling_layout;

    /* appearance */
    hellwm_config_manager_decoration *decoration;

    /* functionality */
    hellwm_config_manager_input *input;
    hellwm_config_manager_keybindings *keybindings;
    hellwm_config_manager_monitor *monitor_manager;
    hellwm_config_manager_keyboard *keyboard_manager;
    struct hellwm_binary_workspaces_manager *binary_workspaces_manager;
};

/* functions declarations */
void ERR(const char *format, ...);
void RUN_EXEC(const char *commnad);
void LOG(const char *format, ...);
void check_usage(int argc, char**argv);
void hellwm_lua_error (lua_State *L, const char *fmt, ...);

int is_digit_str(const char* str);
int hex_to_float(const char* hex, float* color);
int get_color(const char* color_str, float* color);

struct hellwm_config_manager *hellwm_config_manager_create();

int hellwm_lua_env(lua_State *L);
int hellwm_lua_exec(lua_State *L);
int hellwm_lua_add_keybind(lua_State *L);
int hellwm_lua_add_monitor(lua_State *L);
int hellwm_lua_add_keyboard(lua_State *L);
int hellwm_lua_tiling_layout(lua_State *L);
int hellwm_lua_decoration_outer_gap(lua_State *L);
int hellwm_lua_decoration_inner_gap(lua_State *L);
int hellwm_lua_decoration_border_width(lua_State *L);
int hellwm_lua_decoration_border_active(lua_State *L);
int hellwm_lua_decoration_border_inactive(lua_State *L);
int hellwm_lua_decoration_animation_bezier(lua_State *L);
int hellwm_lua_decoration_animation_duration(lua_State *L);
int hellwm_lua_decoration_fade_duration(lua_State *L);

void hellwm_config_print(struct hellwm_config_manager *config);
bool hellwm_function_find(const char* name, struct hellwm_server* server, union hellwm_function *func);
bool hellwm_convert_string_to_xkb_keys(xkb_keysym_t **keysyms_arr, const char *keys_str, int *num_keys);
bool hellwm_config_manager_keyboard_find_and_apply(hellwm_config_manager_keyboard *keyboard_manager, struct hellwm_keyboard *keyboard);

void hellwm_function_expose(struct hellwm_server *server);
void hellwm_config_manager_load_from_file(char * filename);
void hellwm_config_manager_monitor_find_and_apply(char *name, struct wlr_output_state *state, hellwm_config_manager_monitor *monitor_manager);

/* returns number of active workspaces + 1 */
static int hellwm_workspace_get_next_id(struct hellwm_server *server);

/* destroy workspace */
static void hellwm_workspace_destroy(struct hellwm_workspace *workspace);

/* changes workspace to provided id, if not exist create from function hellwm_workspace_create() */
static void hellwm_workspace_change(struct hellwm_server *server, int workspace);

/* creates workspace from provided id */
static void hellwm_workspace_create(struct hellwm_server *server, int workspace_id);
struct hellwm_workspace *hellwm_workspace_create_begin(struct hellwm_server *server, int workspace_id);
static void hellwm_workspace_create_for_output(struct hellwm_server *server, int workspace_id, struct hellwm_output *output);

/* find workspace by id, if function cannot find workspace returns NULL */
static struct hellwm_workspace *hellwm_workspace_find(struct hellwm_server *server, int id);

static void hellwm_binary_workspaces_manager_add_keybind(struct hellwm_binary_workspaces_manager *manager, hellwm_config_keybind *keybind);

/* add toplevel to specific workspace */
static void hellwm_workspace_add_toplevel(struct hellwm_workspace *workspace, struct hellwm_toplevel *toplevel);

/* move toplevel from workspace to new active workspace */
void hellwm_toplevel_move_to_workspace(struct hellwm_server *server, int workspace_id);

void hellwm_config_manager_free(struct hellwm_config_manager *config);
void hellwm_config_manager_monitor_free(hellwm_config_manager_monitor *monitor);
void hellwm_config_manager_keyboard_free(hellwm_config_manager_keyboard *keyboard);
void hellwm_config_keybindings_free(hellwm_config_manager_keybindings * keybindings);
void hellwm_config_manager_binary_workspaces_free(struct hellwm_binary_workspaces_manager *binary_workspaces);

hellwm_config_manager_input *hellwm_config_manager_input_create();
hellwm_config_manager_monitor *hellwm_config_manager_monitor_create();
hellwm_config_manager_keyboard *hellwm_config_manager_keyboard_create();
hellwm_config_manager_decoration *hellwm_config_manager_decoration_create();
hellwm_config_manager_keybindings *hellwm_config_manager_keybindings_create();
struct hellwm_binary_workspaces_manager *hellwm_config_binary_workspace_manager_create();

void hellwm_config_manager_reload(struct hellwm_server *server);
void hellwm_config_manager_monitor_reload(struct hellwm_server *server);
void hellwm_config_manager_keyboard_reload(struct hellwm_server *server);

void hellwm_config_manager_keyboard_set(struct hellwm_keyboard *keyboard, hellwm_config_keyboard *config);
void hellwm_config_manager_monitor_set(hellwm_config_manager_monitor *config, struct hellwm_output *output);

void hellwm_config_keybind_add_allocate(hellwm_config_manager_keybindings *config_keybindings);
void hellwm_config_keybind_add_function_or_cmd_to_config(struct hellwm_server *server, char *keys, void *content);
void hellwm_config_keybind_add_workspace_to_config(struct hellwm_server *server, char *keys, struct hellwm_config_keybind_workspace workspace);
void hellwm_function_add_to_map(struct hellwm_server *server, const char* name, void (*func)(struct hellwm_server*));
void hellwm_config_manager_monitor_add(hellwm_config_manager_monitor *monitor_manager, hellwm_config_monitor *monitor);
void hellwm_config_manager_keyboard_add(hellwm_config_manager_keyboard *keyboard_manager, hellwm_config_keyboard *keyboard);

static uint32_t hellwm_xkb_keysym_to_wlr_modifier(xkb_keysym_t sym);
static bool toplevel_should_float(struct hellwm_toplevel *toplevel);
static bool handle_keybinding_binary_workspaces(struct hellwm_server *server, xkb_keysym_t sym);
static bool handle_keybinding(struct hellwm_server *server, uint32_t modifiers, xkb_keysym_t sym);

struct hellwm_view *root_parent_surface(struct wlr_surface *wlr_surface);
struct hellwm_view *desktop_view_at(struct hellwm_server *server, double lx, double ly, struct wlr_surface **surface, double *sx, double *sy);

static void hellwm_server_kill(struct hellwm_server *server);
static void hellwm_focus_next_in_list(struct hellwm_server *server);
static void hellwm_toplevel_fullscreen(struct hellwm_server *server);
static void hellwm_focus_next_potential(struct hellwm_server *server);
static void hellwm_toplevel_kill_active(struct hellwm_server *server);

static void hellwm_focus_toplevel(struct hellwm_toplevel *toplevel);
static void hellwm_cursor_follow_toplevel(struct hellwm_server *s, struct hellwm_toplevel *t);

static void keyboard_handle_key(struct wl_listener *listener, void *data);
static void keyboard_handle_destroy(struct wl_listener *listener, void *data);
static void keyboard_handle_modifiers(struct wl_listener *listener, void *data);
static void server_new_pointer(struct hellwm_server *server, struct wlr_input_device *device);
static void server_new_keyboard(struct hellwm_server *server, struct wlr_input_device *device);

static void handle_renderer_lost(struct wl_listener *listener, void *data);
static void server_backend_new_input(struct wl_listener *listener, void *data);
static void server_backend_new_output(struct wl_listener *listener, void *data);

static void seat_request_cursor(struct wl_listener *listener, void *data);
static void seat_request_set_selection(struct wl_listener *listener, void *data);

static void reset_cursor_mode(struct hellwm_server *server);
static void server_cursor_axis(struct wl_listener *listener, void *data) ;
static void server_cursor_button(struct wl_listener *listener, void *data);
static void server_cursor_frame(struct wl_listener *listener, void *data) ;
static void server_cursor_motion(struct wl_listener *listener, void *data);
static void process_cursor_move(struct hellwm_server *server, uint32_t time);
static void process_cursor_resize(struct hellwm_server *server, uint32_t time);
static void process_cursor_motion(struct hellwm_server *server, uint32_t time);
static void server_cursor_motion_absolute(struct wl_listener *listener, void *data);
static void begin_interactive(struct hellwm_toplevel *toplevel, enum hellwm_cursor_mode mode, uint32_t edges);

static void output_frame(struct wl_listener *listener, void *data);
static void output_destroy(struct wl_listener *listener, void *data);
static void output_request_state(struct wl_listener *listener, void *data);

static void focus_layer_surface(struct hellwm_layer_surface *layer_surface);
static void handle_new_layer_surface(struct wl_listener *listener, void *data);
static void layer_surface_handle_map(struct wl_listener *listener, void *data);
static void layer_surface_map_func(struct hellwm_layer_surface *layer_surface);
static void layer_surface_handle_unmap(struct wl_listener *listener, void *data);
static void layer_surface_unmap_func(struct hellwm_layer_surface *layer_surface);
static void layer_surface_handle_commit(struct wl_listener *listener, void *data);
static void layer_surface_handle_destroy(struct wl_listener *listener, void *data);

static void xdg_toplevel_map(struct wl_listener *listener, void *data);
static void xdg_toplevel_unmap(struct wl_listener *listener, void *data);
static void xdg_toplevel_commit(struct wl_listener *listener, void *data);
static void xdg_toplevel_destroy(struct wl_listener *listener, void *data);

static void xdg_toplevel_set_title(struct wl_listener *listener, void *data);
static void xdg_toplevel_set_app_id(struct wl_listener *listener, void *data);

static void xdg_toplevel_request_move(struct wl_listener *listener, void *data);
static void xdg_toplevel_request_resize(struct wl_listener *listener, void *data);
static void xdg_toplevel_request_maximize(struct wl_listener *listener, void *data);
static void xdg_toplevel_request_fullscreen(struct wl_listener *listener, void *data);

static void toplevel_center(struct hellwm_toplevel *toplevel);
static void toplevel_set_fullscreen(struct hellwm_toplevel *toplevel);
static void toplevel_unset_fullscreen(struct hellwm_toplevel *toplevel);
static void toplevel_get_clip(struct hellwm_toplevel *toplevel, struct wlr_box *clip);
static void toplevel_set_start_geometry(struct hellwm_toplevel *toplevel, struct wlr_box box);

static void toplevel_fade_start(struct hellwm_toplevel *toplevel);
static void toplevel_animation_start(struct hellwm_toplevel *toplevel);

float *border_get_color(enum hellwm_border_state state);
static void borders_toplevel_update(struct hellwm_toplevel *toplevel);
static void borders_toplevel_set_all(struct hellwm_server *server);
static void borders_toplevel_create(struct hellwm_toplevel *toplevel);
static void toplevel_borders_set_state(struct hellwm_toplevel *toplevel, enum hellwm_border_state state);

static void xdg_decoration_new_toplevel_decoration(struct wl_listener *listener, void *data);

static void xdg_popup_commit(struct wl_listener *listener, void *data);
static void xdg_popup_destroy(struct wl_listener *listener, void *data);
static void server_new_xdg_popup(struct wl_listener *listener, void *data);
static void server_new_xdg_toplevel(struct wl_listener *listener, void *data);

void layout_dwindle(struct hellwm_workspace *workspace);
void calculate_layout(struct hellwm_node *node, struct wlr_box *area, int inner_gap, int outer_gap, int border_width);

/* Global Variables */
struct hellwm_server *GLOBAL_SERVER = NULL;

struct hellwm_node *find_node_by_toplevel(struct hellwm_node *node, struct hellwm_toplevel *toplevel)
{
    if (!node) return NULL;

    if (!node->is_split && node->toplevel == toplevel)
    {
        return node;
    }

    struct hellwm_node *found = find_node_by_toplevel(node->split.left, toplevel);
    if (found) return found;

    return find_node_by_toplevel(node->split.right, toplevel);
}

bool should_split_vertically(struct hellwm_node *node)
{
    if (node == NULL) return false;

    int depth = 0;
    while (node->parent != NULL)
    {
        node = node->parent;
        depth++;
    }
    return depth % 2 == 0;
}

void add_toplevel_to_tree(struct hellwm_workspace *workspace, struct hellwm_toplevel *new_toplevel)
{
    struct hellwm_node *root = workspace->layout_tree;

    if (!root)
    {
        workspace->layout_tree = calloc(1, sizeof(struct hellwm_node));
        workspace->layout_tree->is_split = false;
        workspace->layout_tree->toplevel = new_toplevel;
        workspace->layout_tree->parent = NULL;
        workspace->focused_node = workspace->layout_tree;
        return;
    }

    struct hellwm_node *focus = NULL;

    if (workspace->fullscreened == true && workspace->next_to_focus_node != NULL)
        focus = workspace->next_to_focus_node;
    else
        focus = workspace->focused_node;

    if (!focus) focus = root;

    struct hellwm_node *new_split = calloc(1, sizeof(struct hellwm_node));

    new_split->is_split = true;
    new_split->split.split_ratio = 0.5f;
    new_split->split.vertical_split = should_split_vertically(focus);
    new_split->split.left = focus;
    new_split->split.right = calloc(1, sizeof(struct hellwm_node));
    new_split->split.right->is_split = false;
    new_split->split.right->toplevel = new_toplevel;
    new_split->parent = focus->parent;

    if (focus->parent)
    {
        if (focus->parent->split.left == focus)
        {
            focus->parent->split.left = new_split;
        }
        else
        {
            focus->parent->split.right = new_split;
        }
    } 
    else
    {
        workspace->layout_tree = new_split;
    }

    focus->parent = new_split;
    new_split->split.right->parent = new_split;

    workspace->focused_node = new_split->split.right;

    if (workspace->focused_node == NULL)
    {
        LOG("Error: Focused node is NULL after adding toplevel to tree.\n");
    }
}

void remove_toplevel_from_tree(struct hellwm_workspace *workspace, struct hellwm_toplevel *toplevel)
{
    struct hellwm_node *node = find_node_by_toplevel(workspace->layout_tree, toplevel);
    if (!node) return;

    struct hellwm_node *parent = node->parent;
    if (!parent)
    {
        free(node);
        workspace->layout_tree = NULL;
        return;
    }

    struct hellwm_node *sibling = (parent->split.left == node) ? parent->split.right : parent->split.left;

    if (parent->parent)
    {
        struct hellwm_node *grandparent = parent->parent;
        if (grandparent->split.left == parent) {
            grandparent->split.left = sibling;
        }
        else
        {
            grandparent->split.right = sibling;
        }
        sibling->parent = grandparent;
    } 
    else
    {
        workspace->layout_tree = sibling;
        sibling->parent = NULL;
    }

    free(node);
    free(parent);
}

struct hellwm_node *node_get_sibling(struct hellwm_node *node)
{
    if (!node || !node->parent) return NULL;

    struct hellwm_node *parent = node->parent;

    if (parent->is_split)
    {
        return (parent->split.left == node) ? parent->split.right : parent->split.left;
    }
    return NULL;
}

/* TODO */
void server_focus_left(struct hellwm_server *server)
{
}
void server_focus_right(struct hellwm_server *server)
{
}
void server_focus_up(struct hellwm_server *server)
{
}
void server_focus_down(struct hellwm_server *server)
{
}

void calculate_layout(struct hellwm_node *node, struct wlr_box *area, int inner_gap, int outer_gap, int border_width)
{
    if (!node) return;

    if (!node->is_split)
    {
        // TODO: instead of skipping toplevel, remove it from layout_tree
        if (!node->toplevel->fullscreen)
        {
            node->toplevel->desired_geom.x = area->x + border_width + outer_gap;
            node->toplevel->desired_geom.y = area->y + border_width + outer_gap;
            node->toplevel->desired_geom.width = area->width - 2 * (border_width + outer_gap);
            node->toplevel->desired_geom.height = area->height - 2 * (border_width + outer_gap);

            if (node->toplevel->desired_geom.width < 0) node->toplevel->desired_geom.width = 0;
            if (node->toplevel->desired_geom.height < 0) node->toplevel->desired_geom.height = 0;

            if (node->toplevel->first_layout)
            {
                toplevel_set_start_geometry(node->toplevel, node->toplevel->desired_geom);
                node->toplevel->first_layout = false;
            }
        }
        toplevel_animation_start(node->toplevel);
        return;
    }

    struct wlr_box left_area = *area;
    struct wlr_box right_area = *area;

    if (node->split.vertical_split)
    {
        left_area.width = (area->width - inner_gap) * node->split.split_ratio;
        right_area.x += left_area.width + inner_gap;
        right_area.width -= left_area.width + inner_gap;
    }
    else
    {
        left_area.height = (area->height - inner_gap) * node->split.split_ratio;
        right_area.y += left_area.height + inner_gap;
        right_area.height -= left_area.height + inner_gap;
    }

    calculate_layout(node->split.left, &left_area, inner_gap, outer_gap, border_width);
    calculate_layout(node->split.right, &right_area, inner_gap, outer_gap, border_width);
}

void layout_dwindle(struct hellwm_workspace *workspace)
{
    if (!workspace->layout_tree) return;

    int inner_gap = workspace->server->config_manager->decoration->inner_gap;
    int outer_gap = workspace->server->config_manager->decoration->outer_gap;
    int border_width = workspace->server->config_manager->decoration->border_width;

    struct wlr_box area = {
        .x = workspace->output->usable_area.x + outer_gap,
        .y = workspace->output->usable_area.y + outer_gap,
        .width = workspace->output->usable_area.width - 2 * outer_gap,
        .height = workspace->output->usable_area.height - 2 * outer_gap,
    };

    calculate_layout(workspace->layout_tree, &area, inner_gap, outer_gap, border_width);
    hellwm_cursor_follow_toplevel(workspace->server, workspace->now_focused);
}

void apply_layout(struct hellwm_workspace *workspace)
{
    if (workspace == NULL)
        return;

    layout_dwindle(workspace);
}

/* functions implementations */
void LOG(const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    vfprintf(stdout, format, ap);
    va_end(ap);

    FILE *file = fopen("./logfile.log", "a");

    if (file != NULL)
    {
        va_start(ap, format);
        vfprintf(file, format, ap);
        va_end(ap);
        fclose(file);
    }
}

void ipc_handle(const char *cmd)
{
    if (strncmp(cmd, "execute ", 8) == 0)
    {
        RUN_EXEC(cmd + 8);
    } 
    else if (strcmp(cmd, "reload") == 0)
    {
        hellwm_config_manager_reload(GLOBAL_SERVER);
    }
    else if (strncmp(cmd, "workspace ", 10) == 0)
    {
        int ws = atoi(cmd + 10);
        hellwm_workspace_change(GLOBAL_SERVER, ws);
    }
    else
    {
        LOG("Unknown command: %s\n", cmd);
    }
}

void *ipc_listener(void *arg)
{
    int server_fd, client_fd;
    struct sockaddr_un addr;
    char buffer[IPC_BUFFER_SIZE];

    if ((server_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        return NULL;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, IPC_SOCKET_PATH, sizeof(addr.sun_path) - 1);
    unlink(IPC_SOCKET_PATH);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("bind");
        close(server_fd);
        return NULL;
    }

    if (listen(server_fd, 5) == -1) {
        perror("listen");
        close(server_fd);
        return NULL;
    }

    printf("hellwm: Listening for commands...\n");

    while (1) {
        client_fd = accept(server_fd, NULL, NULL);
        if (client_fd == -1) {
            perror("accept");
            continue;
        }

        memset(buffer, 0, IPC_BUFFER_SIZE);
        read(client_fd, buffer, IPC_BUFFER_SIZE - 1);
        ipc_handle(buffer);
        close(client_fd);
    }

    close(server_fd);
    unlink(IPC_SOCKET_PATH);
    return NULL;
}

pthread_t ipc_init()
{
    pthread_t thread;
    pthread_create(&thread, NULL, ipc_listener, NULL);
    pthread_detach(thread);
    return thread;
}

int is_digit_str(const char* str)
{
    while (*str)
    {
        if (!isdigit(*str))
        {
            return 0;
        }
        str++;
    }
    return 1;
}

int hex_to_float(const char* hex, float* color)
{
    unsigned int r, g, b;
    
    if (sscanf(hex, "#%2x%2x%2x", &r, &g, &b) != 3) {
        fprintf(stderr, "Error: Invalid hex color format. Expected #RRGGBB.\n");
        return 0;
    }

    color[0] = (float)r / 255.0f; // Red
    color[1] = (float)g / 255.0f; // Green
    color[2] = (float)b / 255.0f; // Blue
    color[3] = 1.0f;              // Alpha set to 1.0 for hex colors

    return 1;
}

int get_color(const char* color_str, float* color)
{
    if (color_str[0] == '#')
    {
        if (hex_to_float(color_str, color))
            return 1;
        else
            return 0;
    }

    int r, g, b, a;

    if (sscanf(color_str, "%d,%d,%d,%d", &r, &g, &b, &a) == 4)
    {
        // RGBA
        color[0] = (float)r / 255.0f;
        color[1] = (float)g / 255.0f;
        color[2] = (float)b / 255.0f;
        color[3] = (float)a / 255.0f;
        return 1;
    }
    else
    {
        // RGB
        color[0] = (float)r / 255.0f;
        color[1] = (float)g / 255.0f;
        color[2] = (float)b / 255.0f;
        color[3] = 1.0f;
        return 1;
    }
    // err
    return 0;
}

void ERR(const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    vfprintf(stdout, format, ap);
    va_end(ap);

    FILE *file = fopen("./logfile.log", "a");

    if (file != NULL)
    {
        va_start(ap, format);
        vfprintf(file, format, ap);
        va_end(ap);
        fclose(file);
    }
    exit(EXIT_FAILURE);
}

void hellwm_lua_error (lua_State *L, const char *fmt, ...)
{
    va_list argp;
    va_start(argp, fmt);
    vfprintf(stderr, fmt, argp);
    va_end(argp);
    lua_close(L);
}

void RUN_EXEC(const char *commnad)
{
    if (fork() == 0)
    {
        execl("/bin/sh", "/bin/sh", "-c", commnad, (void *)NULL);
    }
}


void remove_spaces(char *str)
{
    char *dest = str;
    while (*str)
    {
        if (*str != ' ')
        {
            *dest++ = *str;
        }
        str++;
    }
    *dest = '\0';
}

void check_usage(int argc, char**argv)
{
    int c;
    while ((c = getopt(argc, argv, "s:h")) != -1)
    {
        switch (c)
        {
            case 's':
                //startup_cmd = optarg;
                break;

            default:
                printf("Usage: %s [-s startup command]\n", argv[0]);
                exit(0);
        }
    }
    if (optind < argc)
    {
        printf("Usage: %s [-s startup command]\n", argv[0]);
        exit(0);
    }
}

static void toplevel_animation_start(struct hellwm_toplevel *toplevel)
{
    if (toplevel == NULL) return;
    toplevel->set_animation_clock = true;
    toplevel->in_animation = true;

    toplevel->animation_start_geom = toplevel->current_geom;
}

static void toplevel_fade_start(struct hellwm_toplevel *toplevel)
{
    if (toplevel == NULL) return;
    toplevel->set_fade_clock = true;
}

static void toplevel_get_clip(struct hellwm_toplevel *toplevel, struct wlr_box *clip)
{
    struct wlr_box xdg_geom = {0};
    *clip = (struct wlr_box){
        .x = 0,
        .y = 0,
        .width = toplevel->current_geom.width,
        .height = toplevel->current_geom.height,
    };


    wlr_xdg_surface_get_geometry(toplevel->xdg_toplevel->base, &xdg_geom);
    clip->x = xdg_geom.x;
    clip->y = xdg_geom.y;
}

static void toplevel_set_start_geometry(struct hellwm_toplevel *toplevel, struct wlr_box box)
{
    int x = box.x;
    int y = box.y;

    int mw = box.x + box.width / 2;
    int mh = box.y + box.height / 2;

    int tw = box.width;
    int th = box.height;

    switch (toplevel->server->config_manager->decoration->default_animation)
    {
        case HELLWM_ANIMATION_LEFT:
            toplevel->current_geom.width = tw;
            toplevel->current_geom.height = th;
            toplevel->current_geom.x = x - tw / 2;
            toplevel->current_geom.y = mh - th / 2;
            break;

        case HELLWM_ANIMATION_RIGHT:
            toplevel->current_geom.width = tw;
            toplevel->current_geom.height = th;
            toplevel->current_geom.x = x + tw / 2;
            toplevel->current_geom.y = mh - th / 2;
            break;

        case HELLWM_ANIMATION_UP:
            toplevel->current_geom.width = tw;
            toplevel->current_geom.height = th;
            toplevel->current_geom.x = mw - tw / 2;
            toplevel->current_geom.y = y - th / 2;
            break;

        case HELLWM_ANIMATION_DOWN:
            toplevel->current_geom.width = tw;
            toplevel->current_geom.height = th;
            toplevel->current_geom.x = mw - tw / 2;
            toplevel->current_geom.y = y + th / 2;
            break;

        case HELLWM_ANIMATION_GROW:
            toplevel->current_geom.width = 0;
            toplevel->current_geom.height = 0;
            toplevel->current_geom.x = mw;
            toplevel->current_geom.y = mh;
            break;

        case HELLWM_ANIMATION_SHRINK:
            tw = toplevel->workspace->output->wlr_output->width;
            th = toplevel->workspace->output->wlr_output->height;

            toplevel->current_geom.width = tw;
            toplevel->current_geom.height = th;
            toplevel->current_geom.x = mw - tw / 2;
            toplevel->current_geom.y = mh - th / 2;
            break;

        case HELLWM_ANIMATION_SOLID:
            toplevel->current_geom.width = tw;
            toplevel->current_geom.height = th;
            toplevel->current_geom.x = mw - tw / 2;
            toplevel->current_geom.y = mh - th / 2;
            break;

        default:
            toplevel->current_geom.width = 0;
            toplevel->current_geom.height = 0;
            toplevel->current_geom.x = mw;
            toplevel->current_geom.y = mh;
            break;
    }
}

hellwm_config_manager_monitor *hellwm_config_manager_monitor_create()
{
    hellwm_config_manager_monitor *monitor_manager = calloc(1, sizeof(hellwm_config_manager_monitor));
    if (!monitor_manager)
    {
        LOG("hellwm_config_manager_monitor_create()%s:%d: failed to allocate memory for hellwm_config_manager_monitor\n",__func__, __LINE__);
        return NULL;
    }

    monitor_manager->count = 0;
    monitor_manager->monitors = NULL;

    return monitor_manager;
}

hellwm_config_manager_keyboard *hellwm_config_manager_keyboard_create()
{
    hellwm_config_manager_keyboard *keyboard_manager = calloc(1, sizeof(hellwm_config_manager_keyboard));

    if (!keyboard_manager)
    {
        LOG("%s:%d failed to allocate memory for hellwm_config_manager_keyboard\n",__func__, __LINE__);
        return NULL;
    }

    keyboard_manager->count = 0;
    keyboard_manager->keyboards = NULL;

    keyboard_manager->default_keyboard = calloc(1, sizeof(hellwm_config_keyboard));
    keyboard_manager->default_keyboard->name = "default";
    keyboard_manager->default_keyboard->layout = "us";
    keyboard_manager->default_keyboard->rate = 25;
    keyboard_manager->default_keyboard->delay = 600;
    keyboard_manager->default_keyboard->rules = NULL;
    keyboard_manager->default_keyboard->model = NULL;
    keyboard_manager->default_keyboard->variant = NULL;
    keyboard_manager->default_keyboard->options = NULL;

    return keyboard_manager;
}

hellwm_config_manager_keybindings *hellwm_config_manager_keybindings_create()
{
    hellwm_config_manager_keybindings *keybindings = calloc(1, sizeof(hellwm_config_manager_keybindings));
    keybindings->count = 0;

    return keybindings;
}

/* TODO: load from config */
hellwm_config_manager_input *hellwm_config_manager_input_create()
{
    hellwm_config_manager_input *input = calloc(1, sizeof(hellwm_config_manager_input));

    input->tap_click = true;
    input->natural_scroll = false; /* inverted scroll */
    input->cursor_follow_toplevels = true;

    return input;
}

hellwm_config_manager_decoration *hellwm_config_manager_decoration_create()
{
    hellwm_config_manager_decoration *decoration = calloc(1, sizeof(hellwm_config_manager_decoration));

    /* tiling */
    decoration->master_ratio = 0.7;

    /* gaps */
    decoration->inner_gap = 20;
    decoration->outer_gap = 30;

    /* fade animation duration */
    decoration->fade_duration = 0.4f;
    decoration->fade_decoration = true;

    decoration->animation_duration = 0.4f;
    decoration->animation_bezier[0] = 0.0f;
    decoration->animation_bezier[1] = 0.9f;
    decoration->animation_bezier[2] = 0.1f;
    decoration->animation_bezier[3] = 1.0f;
    decoration->animations_decoration = true;
    decoration->default_animation = HELLWM_ANIMATION_GROW;

    /* border */
    decoration->border_width = 2;

    /* border active color */
    decoration->active_border_color[0] = 0.9f; 
    decoration->active_border_color[1] = 0.9f; 
    decoration->active_border_color[2] = 0.9f; 
    decoration->active_border_color[3] = 1.0f; 

    /* border inactive color */
    decoration->inactive_border_color[0] = 0.7f; 
    decoration->inactive_border_color[1] = 0.1f; 
    decoration->inactive_border_color[2] = 0.1f; 
    decoration->inactive_border_color[3] = 1.0f; 

    return decoration;
}

struct hellwm_binary_workspaces_manager *hellwm_config_binary_workspace_manager_create()
{
    /* binary workspaces manager */
    struct hellwm_binary_workspaces_manager *binary_workspaces = calloc(1, sizeof(struct hellwm_binary_workspaces_manager));
    binary_workspaces->binary_started = false;
    binary_workspaces->keybinds_count = 0;
    binary_workspaces->workspace_sum = 0;
    binary_workspaces->keybinds = NULL;

    return binary_workspaces;
}

struct hellwm_config_manager *hellwm_config_manager_create()
{
    struct hellwm_config_manager *config_manager = calloc(1, sizeof(struct hellwm_config_manager));

    /* remove this and create it as sub config_manager */
    config_manager->tiling_layout = 1;

    /* decoration */
    config_manager->decoration = hellwm_config_manager_decoration_create();

    /* input */
    config_manager->input = hellwm_config_manager_input_create();

    /* keybindings */
    config_manager->keybindings = hellwm_config_manager_keybindings_create();

    /* keyboard */
    config_manager->keyboard_manager = hellwm_config_manager_keyboard_create();

    /* monitor */
    config_manager->monitor_manager = hellwm_config_manager_monitor_create();

    /* binary workspaces */
    config_manager->binary_workspaces_manager = hellwm_config_binary_workspace_manager_create();

    return config_manager;
}

static void hellwm_cursor_follow_toplevel(struct hellwm_server* s, struct hellwm_toplevel *t)
{
    if (!s->config_manager->input->cursor_follow_toplevels) return;
    if (t == NULL) return;

    int x = t->desired_geom.x + t->desired_geom.width / 2;
    int y = t->desired_geom.y + t->desired_geom.height / 2;
    wlr_cursor_warp(s->cursor, NULL, x, y);
}

static void hellwm_focus_toplevel(struct hellwm_toplevel *toplevel)
{
    if (!toplevel || !toplevel->xdg_toplevel || !toplevel->xdg_toplevel->base)
    {
        LOG("Invalid toplevel structure or uninitialized xdg_toplevel.\n");
        return;
    }

    struct wlr_surface *surface = toplevel->xdg_toplevel->base->surface;
    struct hellwm_server *server = toplevel->server;
    struct wlr_seat *seat = server->seat;
    struct wlr_surface *prev_surface = seat->keyboard_state.focused_surface;

    if (prev_surface == surface)
    {
        /* Don't re-focus an already focused surface. */
        return;
    }

    if (prev_surface)
    {
        struct wlr_xdg_toplevel *prev_toplevel = wlr_xdg_toplevel_try_from_wlr_surface(prev_surface);
        if (prev_toplevel != NULL)
        {
            if (server->active_workspace->now_focused != NULL)
            {
                server->active_workspace->prev_focused = server->active_workspace->now_focused;
                toplevel_borders_set_state(server->active_workspace->prev_focused, HELLWM_BORDER_INACTIVE);
            }

            wlr_xdg_toplevel_set_activated(prev_toplevel, false);
        }
    }

    toplevel->workspace->now_focused = toplevel;
    toplevel->workspace->focused_node = find_node_by_toplevel(toplevel->workspace->layout_tree, toplevel);

    struct wlr_keyboard *keyboard = wlr_seat_get_keyboard(seat);

    /* Move the toplevel to the front */
    wlr_scene_node_raise_to_top(&toplevel->scene_tree->node);

    wl_list_remove(&toplevel->link);
    wl_list_insert(&server->active_workspace->toplevels, &toplevel->link);

    /* Activate the new surface */
    wlr_xdg_toplevel_set_activated(toplevel->xdg_toplevel, true);

    if (keyboard != NULL)
    {
        wlr_seat_keyboard_notify_enter(seat, toplevel->xdg_toplevel->base->surface, keyboard->keycodes, keyboard->num_keycodes, &keyboard->modifiers);
    }

    toplevel_borders_set_state(toplevel, HELLWM_BORDER_ACTIVE);
}

static void unfocus_focused(struct hellwm_server *server)
{
    if (server == NULL)
        return;

    struct wlr_seat *seat = server->seat;
    struct wlr_surface *focused_surface = seat->keyboard_state.focused_surface;

    if (focused_surface == NULL) {
        return;
    }

    struct hellwm_toplevel *toplevel = NULL;
    wl_list_for_each(toplevel, &server->active_workspace->toplevels, link)
    {
        if (toplevel->xdg_toplevel->base->surface == focused_surface)
        {
            wlr_xdg_toplevel_set_activated(toplevel->xdg_toplevel, false);

            wlr_seat_keyboard_clear_focus(seat);

            server->active_workspace->now_focused = NULL;
            return;
        }
    }

    wlr_seat_keyboard_clear_focus(seat);
}

static void focus_layer_surface(struct hellwm_layer_surface *layer_surface)
{
    struct hellwm_server *server = layer_surface->server;
    enum zwlr_layer_surface_v1_keyboard_interactivity keyboard_interactive = layer_surface->wlr_layer_surface->current.keyboard_interactive;

    switch(keyboard_interactive) 
    {
        case ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_NONE: {
            return;
        }
        case ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_EXCLUSIVE: {
            unfocus_focused(server);
            server->layer_exclusive_keyboard = layer_surface;
            struct wlr_keyboard *keyboard = wlr_seat_get_keyboard(server->seat);

            wlr_scene_node_raise_to_top(&layer_surface->scene->tree->node);

            if(keyboard != NULL) {
              wlr_seat_keyboard_notify_enter(server->seat, layer_surface->wlr_layer_surface->surface, keyboard->keycodes, keyboard->num_keycodes, &keyboard->modifiers);
            }
            return;
        }
        case ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_ON_DEMAND: {
            if(server->layer_exclusive_keyboard != NULL) return;
            struct wlr_keyboard *keyboard = wlr_seat_get_keyboard(server->seat);

            wlr_scene_node_raise_to_top(&layer_surface->scene->tree->node);
            if(keyboard != NULL) 
            {
              wlr_seat_keyboard_notify_enter(server->seat, layer_surface->wlr_layer_surface->surface, keyboard->keycodes, keyboard->num_keycodes, &keyboard->modifiers);
            }
            return;
        }
    }
}

static void hellwm_focus_next_in_list(struct hellwm_server *server)
{
    if (wl_list_length(&server->active_workspace->toplevels) < 1)
        return;

    struct hellwm_toplevel *next_toplevel = wl_container_of(server->active_workspace->toplevels.prev, next_toplevel, link);
    if (next_toplevel)
    {
        hellwm_focus_toplevel(next_toplevel);
    }
    else
    {
        struct hellwm_toplevel *toplevel;
        wl_list_for_each(toplevel, &server->active_workspace->toplevels, link)
        {

            hellwm_focus_toplevel(toplevel);
            break;
        }
    }

    hellwm_cursor_follow_toplevel(server, server->active_workspace->now_focused);
}

static void hellwm_focus_next_potential(struct hellwm_server *server)
{
    /* check if the next to focus node is valid. */
    if (server->active_workspace->next_to_focus_node != NULL)
    {
        if (server->active_workspace->next_to_focus_node->toplevel)
        {
            hellwm_focus_toplevel(server->active_workspace->next_to_focus_node->toplevel);
        }
    }
    else
    {
        /* if next_to_focus_node is NULL, focus another toplevel */
        if (!wl_list_empty(&server->active_workspace->toplevels))
        {
            struct hellwm_toplevel *fallback_toplevel = wl_container_of(server->active_workspace->toplevels.next, fallback_toplevel, link);
            hellwm_focus_toplevel(fallback_toplevel);
        }
    }
}

static void hellwm_toplevel_kill_active(struct hellwm_server *server)
{
    if (!server->active_workspace) return;
    if (wl_list_length(&server->active_workspace->toplevels) < 1) return;
    if (server->seat->keyboard_state.focused_surface == NULL) return;

    struct wlr_xdg_toplevel *toplevel = wlr_xdg_toplevel_try_from_wlr_surface(server->seat->keyboard_state.focused_surface);
    if (!toplevel)
        return;

    wlr_xdg_toplevel_send_close(toplevel);
}

static void hellwm_toplevel_fullscreen(struct hellwm_server *server)
{
    if (server->active_workspace->now_focused == NULL)
        return;

    if (server->active_workspace->fullscreened)
        toplevel_unset_fullscreen(server->active_workspace->now_focused);
    else
        toplevel_set_fullscreen(server->active_workspace->now_focused);

    toplevel_animation_start(server->active_workspace->now_focused);
}

static void hellwm_server_kill(struct hellwm_server *server)
{
    hellwm_config_manager_free(server->config_manager);
    wl_display_destroy_clients(server->wl_display);
    wlr_scene_node_destroy(&server->scene->tree.node);
    wlr_xcursor_manager_destroy(server->cursor_mgr);
    wlr_cursor_destroy(server->cursor);
    wlr_allocator_destroy(server->allocator);
    wlr_renderer_destroy(server->renderer);
    wlr_backend_destroy(server->backend);
    wl_display_destroy(server->wl_display);
    exit(EXIT_SUCCESS);
}

static void keyboard_handle_modifiers(struct wl_listener *listener, void *data)
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
    wlr_seat_set_keyboard(keyboard->server->seat, keyboard->wlr_keyboard);
    /* Send modifiers to the client. */
    wlr_seat_keyboard_notify_modifiers(keyboard->server->seat, &keyboard->wlr_keyboard->modifiers);
}

static bool handle_keybinding(struct hellwm_server *server, uint32_t modifiers, xkb_keysym_t sym) 
{
    /*
     * Here we handle compositor keybindings. This is when the compositor is
     * processing keys, rather than passing them on to the client for its own
     * processing.
     */

    hellwm_config_manager_keybindings *keybindings = server->config_manager->keybindings;

    for (int j = 0; j < keybindings->count; j++)
    {
        int check = 0;
        for (int k = 0; k < keybindings->keybindings[j]->count-1; k++)
        {
            if (hellwm_xkb_keysym_to_wlr_modifier(keybindings->keybindings[j]->keysyms[k]) & modifiers)
            {
                check++;
            }
        }
        if (keybindings->keybindings[j]->keysyms[keybindings->keybindings[j]->count-1] == sym)
        {
            check++;
        }
        if (keybindings->keybindings[j]->content && check == keybindings->keybindings[j]->count)
        {
            switch (keybindings->keybindings[j]->type)
            {
                case HELLWM_KEYBIND_FUNCTION:
                    if (keybindings->keybindings[j]->content->as_func)
                        keybindings->keybindings[j]->content->as_func(server);
                    break;

                case HELLWM_KEYBIND_WORKSPACE:
                    if (keybindings->keybindings[j]->content->workspace.binary_workspaces_enabled)
                    {
                        if (!server->config_manager->binary_workspaces_manager->binary_started)
                            server->config_manager->binary_workspaces_manager->binary_started = true;

                        keybindings->keybindings[j]->content->workspace.pressed = true;
                        server->config_manager->binary_workspaces_manager->workspace_sum += keybindings->keybindings[j]->content->workspace.binary_workspace_val;
                    }
                    else
                    {
                        if (keybindings->keybindings[j]->content->workspace.move_with_active)
                        {
                            // TODO: make move_to_workspace work with binary workspaces
                            //hellwm_toplevel_move_to_workspace(server, keybindings->keybindings[j]->content->workspace.workspace_id);

                            hellwm_workspace_change(server, keybindings->keybindings[j]->content->workspace.workspace_id);
                        }
                        else
                            hellwm_workspace_change(server, keybindings->keybindings[j]->content->workspace.workspace_id);
                    }
                    break;

                case HELLWM_KEYBIND_COMMAND:
                    if (keybindings->keybindings[j]->content->as_void)
                        RUN_EXEC(keybindings->keybindings[j]->content->as_void);
                    break;

                default:
                    LOG("%s:%s Wrong keybind type: %d", __func__, __LINE__, server->config_manager->keybindings->keybindings[j]->type);

            }
            return true;
        }
    }
    return false;
}

static bool handle_keybinding_binary_workspaces(struct hellwm_server *server, xkb_keysym_t sym) 
{
    hellwm_config_keybind **keybinds = server->config_manager->binary_workspaces_manager->keybinds;
    bool change_workspace = false;

    for (int i = 0; i < server->config_manager->binary_workspaces_manager->keybinds_count; i++)
    {
        if (keybinds[i]->content->workspace.pressed && keybinds[i]->type == HELLWM_KEYBIND_WORKSPACE)
        {
            for (int j = 0; j < keybinds[i]->count; j++)
            {
                if (keybinds[i]->keysyms[j] == sym)
                {
                    if (hellwm_xkb_keysym_to_wlr_modifier(sym) == 0)
                    {
                        keybinds[i]->content->workspace.pressed = false;
                        change_workspace = true;
                    }
                }
            }
        } 
    }

    if (change_workspace)
    {
        server->config_manager->binary_workspaces_manager->binary_started = false;
        hellwm_workspace_change(server, server->config_manager->binary_workspaces_manager->workspace_sum);
        server->config_manager->binary_workspaces_manager->workspace_sum = 0;
        true;
    }

    return false;
}

static void keyboard_handle_key(struct wl_listener *listener, void *data)
{
    /* This event is raised when a key is pressed or released. */
    struct hellwm_keyboard *keyboard = wl_container_of(listener, keyboard, key);
    struct hellwm_server *server = keyboard->server;
    struct wlr_keyboard_key_event *event = data;
    struct wlr_seat *seat = server->seat;

    /* Translate libinput keycode -> xkbcommon */
    uint32_t keycode = event->keycode + 8;

    /* Get a list of keysyms based on the keymap for this keyboard */
    const xkb_keysym_t *syms;
    int nsyms = xkb_state_key_get_syms(keyboard->wlr_keyboard->xkb_state, keycode, &syms);

    bool handled = false;
    uint32_t modifiers = wlr_keyboard_get_modifiers(keyboard->wlr_keyboard);

    /* MODIFIERS 
     *
     *  WLR_MODIFIER_SHIFT
     *  WLR_MODIFIER_CAPS
     *  WLR_MODIFIER_CTRL
     *  WLR_MODIFIER_ALT
     *  WLR_MODIFIER_MOD2
     *  WLR_MODIFIER_MOD3
     *  WLR_MODIFIER_LOGO
     *  WLR_MODIFIER_MOD5
     */

    if (event->state == WL_KEYBOARD_KEY_STATE_PRESSED)
    {
        /* If alt is held down and this button was _pressed_, we attempt to process it as a compositor keybinding. */
        for (int i = 0; i < nsyms; i++)
        {
            handled = handle_keybinding(server, modifiers, syms[i]);
        }
    }

    if (event->state == WL_KEYBOARD_KEY_STATE_RELEASED && server->config_manager->binary_workspaces_manager->binary_started)
    {
        for (int i = 0; i < nsyms; i++)
        {
            if (handle_keybinding_binary_workspaces(server, syms[i]))
                break;
        }
    }

    if (!handled)
    {
        /* Otherwise, we pass it along to the client. */
        wlr_seat_set_keyboard(seat, keyboard->wlr_keyboard);
        wlr_seat_keyboard_notify_key(seat, event->time_msec, event->keycode, event->state);
    }
}

static void keyboard_handle_destroy(struct wl_listener *listener, void *data)
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

static void server_new_keyboard(struct hellwm_server *server, struct wlr_input_device *device)
{
    struct wlr_keyboard *wlr_keyboard = wlr_keyboard_from_input_device(device);

    struct hellwm_keyboard *keyboard = calloc(1, sizeof(*keyboard));
    keyboard->server = server;
    keyboard->wlr_keyboard = wlr_keyboard;

    /* Set everything according to config */
    if (!hellwm_config_manager_keyboard_find_and_apply(server->config_manager->keyboard_manager, keyboard))
    {
        hellwm_config_manager_keyboard_set(keyboard, server->config_manager->keyboard_manager->default_keyboard);
    }

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

static void server_new_pointer(struct hellwm_server *server, struct wlr_input_device *device)
{
    /* enable natural scrolling and tap to click*/

    if(wlr_input_device_is_libinput(device))
    {
        struct libinput_device *libinput_device = wlr_libinput_get_device_handle(device);

        if (libinput_device_config_scroll_has_natural_scroll(libinput_device) && server->config_manager->input->natural_scroll)
        {
            libinput_device_config_scroll_set_natural_scroll_enabled(libinput_device, true);
        }

        if (libinput_device_config_tap_get_finger_count(libinput_device) && server->config_manager->input->tap_click)
        {
            libinput_device_config_tap_set_enabled(libinput_device, true);
        }
    }

    wlr_cursor_attach_input_device(server->cursor, device);
}

static void server_backend_new_input(struct wl_listener *listener, void *data)
{
    /* This event is raised by the backend when a new input device becomes
     * available. */
    struct hellwm_server *server = wl_container_of(listener, server, backend_new_input);
    struct wlr_input_device *device = data;
    switch (device->type)
    {
        case WLR_INPUT_DEVICE_KEYBOARD:
            server_new_keyboard(server, device);
            break;
        case WLR_INPUT_DEVICE_POINTER:
            server_new_pointer(server, device);
            break;
        default:
            break;
    }
    /* We need to let the wlr_seat know what our capabilities are, which is
     * communiciated to the client. In hellwm we always have a cursor, even if
     * there are no pointer devices, so we always include that capability. */
    uint32_t caps = WL_SEAT_CAPABILITY_POINTER;
    if (!wl_list_empty(&server->keyboards))
    {
        caps |= WL_SEAT_CAPABILITY_KEYBOARD;
    }
    wlr_seat_set_capabilities(server->seat, caps);
}

static void seat_request_cursor(struct wl_listener *listener, void *data)
{
    struct hellwm_server *server = wl_container_of(listener, server, request_cursor);
    /* This event is raised by the seat when a client provides a cursor image */
    struct wlr_seat_pointer_request_set_cursor_event *event = data;
    struct wlr_seat_client *focused_client = server->seat->pointer_state.focused_client;
    /* This can be sent by any client, so we check to make sure this one is
     * actually has pointer focus first. */
    if (focused_client == event->seat_client)
    {
        /* Once we've vetted the client, we can tell the cursor to use the
         * provided surface as the cursor image. It will set the hardware cursor
         * on the output that it's currently on and continue to do so as the
         * cursor moves between outputs. */
        wlr_cursor_set_surface(server->cursor, event->surface, event->hotspot_x, event->hotspot_y);
    }
}

static void seat_request_set_selection(struct wl_listener *listener, void *data)
{
    /* This event is raised by the seat when a client wants to set the selection,
     * usually when the user copies something. wlroots allows compositors to
     * ignore such requests if they so choose, but in hellwm we always honor
     */
    struct hellwm_server *server = wl_container_of(listener, server, request_set_selection);
    struct wlr_seat_request_set_selection_event *event = data;
    wlr_seat_set_selection(server->seat, event->source, event->serial);
}

static void reset_cursor_mode(struct hellwm_server *server)
{
    /* Reset the cursor mode to passthrough. */
    server->cursor_mode = HELLWM_CURSOR_PASSTHROUGH;
    server->grabbed_toplevel = NULL;
    server->cursor_mode = HELLWM_CURSOR_PASSTHROUGH;
    server->resize_edges = false;
    server->grabbed_toplevel = NULL;

    struct wlr_xdg_toplevel *toplevel = wlr_xdg_toplevel_try_from_wlr_surface(server->seat->keyboard_state.focused_surface);

    if(toplevel != NULL)
    {
    }
    else
    {
        wlr_cursor_set_xcursor(server->cursor, server->cursor_mgr, "default");
    }
}

static void process_cursor_move(struct hellwm_server *server, uint32_t time)
{
    /* Move the grabbed toplevel to the new position. */
    struct hellwm_toplevel *toplevel = server->grabbed_toplevel;
    wlr_scene_node_set_position(&toplevel->scene_tree->node, server->cursor->x - server->cursor_grab_x, server->cursor->y - server->cursor_grab_y);
}

static void process_cursor_resize(struct hellwm_server *server, uint32_t time)
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
    struct hellwm_toplevel *toplevel = server->grabbed_toplevel;
    double border_x = server->cursor->x - server->cursor_grab_x;
    double border_y = server->cursor->y - server->cursor_grab_y;
    int new_left = server->grab_geometry.x;
    int new_right = server->grab_geometry.x + server->grab_geometry.width;
    int new_top = server->grab_geometry.y;
    int new_bottom = server->grab_geometry.y + server->grab_geometry.height;

    if (server->resize_edges & WLR_EDGE_TOP)
    {
        new_top = border_y;
        if (new_top >= new_bottom)
        {
            new_top = new_bottom - 1;
        }
    } else if (server->resize_edges & WLR_EDGE_BOTTOM)
    {
        new_bottom = border_y;
        if (new_bottom <= new_top)
        {
            new_bottom = new_top + 1;
        }
    }
    if (server->resize_edges & WLR_EDGE_LEFT) 
    {
        new_left = border_x;
        if (new_left >= new_right)
        {
            new_left = new_right - 1;
        }
    } else if (server->resize_edges & WLR_EDGE_RIGHT) 
    {
        new_right = border_x;
        if (new_right <= new_left)
        {
            new_right = new_left + 1;
        }
    }

    struct wlr_box geo_box;
    wlr_xdg_surface_get_geometry(toplevel->xdg_toplevel->base, &geo_box);
    wlr_scene_node_set_position(&toplevel->scene_tree->node, new_left - geo_box.x, new_top - geo_box.y);

    int new_width = new_right - new_left;
    int new_height = new_bottom - new_top;
    wlr_xdg_toplevel_set_size(toplevel->xdg_toplevel, new_width, new_height);
}

static void process_cursor_motion(struct hellwm_server *server, uint32_t time)
{
    if(server->cursor_mode == HELLWM_CURSOR_MOVE) {
        process_cursor_move(server, time);
        return;
    } else if (server->cursor_mode == HELLWM_CURSOR_RESIZE) {
        process_cursor_resize(server, time);
        return;
    }

    double sx, sy;
    struct wlr_seat *seat = server->seat;
    struct wlr_surface *surface = NULL;
    struct hellwm_view *view = desktop_view_at(server, server->cursor->x, server->cursor->y, &surface, &sx, &sy);

    if (view == NULL)
    {
        wlr_cursor_set_xcursor(server->cursor, server->cursor_mgr, "default");

        /* clear pointer focus so future button events and such are not sent to
         * the last client to have the cursor over it */
        wlr_seat_pointer_clear_focus(seat);
        return;
    }

    if (view->view_type == HELLWM_VIEW_TOPLEVEL)
        hellwm_focus_toplevel(view->toplevel);
    if (view->view_type == HELLWM_VIEW_LAYER)
        focus_layer_surface(view->layer_surface);

    wlr_seat_pointer_notify_enter(seat, surface, sx, sy);
    wlr_seat_pointer_notify_motion(seat, time, sx, sy);
}

static void server_cursor_motion(struct wl_listener *listener, void *data)
{
    /* This event is forwarded by the cursor when a pointer emits a _relative_
     * pointer motion event (i.e. a delta) */
    struct hellwm_server *server = wl_container_of(listener, server, cursor_motion);
    struct wlr_pointer_motion_event *event = data;
    /* The cursor doesn't move unless we tell it to. The cursor automatically
     * handles constraining the motion to the output layout, as well as any
     * special configuration applied for the specific input device which
     * generated the event. You can pass NULL for the device if you want to move
     * the cursor around without any input. */
    wlr_cursor_move(server->cursor, &event->pointer->base, event->delta_x, event->delta_y);
    process_cursor_motion(server, event->time_msec);
}

static void server_cursor_motion_absolute(struct wl_listener *listener, void *data) 
{
    /* This event is forwarded by the cursor when a pointer emits an _absolute_
     * motion event, from 0..1 on each axis. This happens, for example, when
     * wlroots is running under a Wayland window rather than KMS+DRM, and you
     * move the mouse over the window. You could enter the window from any edge,
     * so we have to warp the mouse there. There is also some hardware which
     * emits these events. */
    struct hellwm_server *server = wl_container_of(listener, server, cursor_motion_absolute);
    struct wlr_pointer_motion_absolute_event *event = data;
    wlr_cursor_warp_absolute(server->cursor, &event->pointer->base, event->x, event->y);
    process_cursor_motion(server, event->time_msec);
}

static int box_area(struct wlr_box *box)
{
  return box->width * box->height;
}

struct hellwm_output *toplevel_get_primary_output(struct hellwm_toplevel *toplevel)
{
    uint32_t toplevel_x =
        toplevel->scene_tree->node.x +
        toplevel->xdg_toplevel->base->current.geometry.x;
    uint32_t toplevel_y =
        toplevel->scene_tree->node.y +
        toplevel->xdg_toplevel->base->current.geometry.y;

    struct wlr_box toplevel_box = {
        .x = toplevel_x,
        .y = toplevel_y,
        .width = toplevel->xdg_toplevel->base->current.geometry.width,
        .height = toplevel->xdg_toplevel->base->current.geometry.height
    };

    struct wlr_box intersection_box;
    struct wlr_box output_box;
    uint32_t max_area = 0;
    struct hellwm_output *max_area_output = NULL;

    struct hellwm_output *o;
    wl_list_for_each(o, &GLOBAL_SERVER->outputs, link) {
        wlr_output_layout_get_box(GLOBAL_SERVER->output_layout, o->wlr_output, &output_box);
        bool intersects = wlr_box_intersection(&intersection_box, &toplevel_box, &output_box);

        if(intersects && box_area(&intersection_box) > max_area)
        {
            max_area = box_area(&intersection_box);
            max_area_output = o;
        }
    }

    return max_area_output;
}

static void server_cursor_button(struct wl_listener *listener, void *data)
{
    struct wlr_pointer_button_event *event = data;

    /* notify the client with pointer focus that a button press has occurred */
    wlr_seat_pointer_notify_button(GLOBAL_SERVER->seat, event->time_msec,
            event->button, event->state);

    if(event->state == WL_POINTER_BUTTON_STATE_RELEASED && GLOBAL_SERVER->cursor_mode != HELLWM_CURSOR_PASSTHROUGH)
    {
        //struct hellwm_output *primary_output = toplevel_get_primary_output(GLOBAL_SERVER->grabbed_toplevel);

        //if(primary_output != GLOBAL_SERVER->grabbed_toplevel->workspace->output)
        //{
        //    GLOBAL_SERVER->grabbed_toplevel->workspace = primary_output->active_workspace;
        //    wl_list_remove(&GLOBAL_SERVER->grabbed_toplevel->link);
        //    wl_list_insert(&primary_output->active_workspace->toplevels, &GLOBAL_SERVER->grabbed_toplevel->link);
        //}

        reset_cursor_mode(GLOBAL_SERVER);
    }
}


static void server_cursor_axis(struct wl_listener *listener, void *data) 
{
    /* This event is forwarded by the cursor when a pointer emits an axis event,
     * for example when you move the scroll wheel. */
    struct hellwm_server *server = wl_container_of(listener, server, cursor_axis);
    struct wlr_pointer_axis_event *event = data;
    /* Notify the client with pointer focus of the axis event. */
    wlr_seat_pointer_notify_axis(server->seat, event->time_msec, event->orientation, event->delta, event->delta_discrete, event->source, event->relative_direction);
}

static void server_cursor_frame(struct wl_listener *listener, void *data) 
{
    /* This event is forwarded by the cursor when a pointer emits an frame
     * event. Frame events are sent after regular pointer events to group
     * multiple events together. For instance, two axis events may happen at the
     * same time, in which case a frame event won't be sent in between. */
    struct hellwm_server *server = wl_container_of(listener, server, cursor_frame);
    /* Notify the client with pointer focus of the frame event. */
    wlr_seat_pointer_notify_frame(server->seat);
}

struct wlr_scene_buffer *surface_find_buffer(struct wlr_scene_node *node, struct wlr_surface *surface)
{
    if(node->type == WLR_SCENE_NODE_BUFFER)
    {
        struct wlr_scene_buffer *scene_buffer = wlr_scene_buffer_from_node(node);

        struct wlr_scene_surface *scene_surface = wlr_scene_surface_try_from_buffer(scene_buffer);
        if(!scene_surface)
            return NULL;

        struct wlr_surface *s = scene_surface->surface;

        if(s && s == surface)
            return scene_buffer;
    }

    if(node->type == WLR_SCENE_NODE_TREE)
    {
        struct wlr_scene_tree *scene_tree = wlr_scene_tree_from_node(node);

        struct wlr_scene_node *child;
        wl_list_for_each(child, &scene_tree->children, link)
        {
            struct wlr_scene_buffer *found_buffer = surface_find_buffer(child, surface);
            if(found_buffer)
                return found_buffer;
        }
    }

    return NULL;
}

float smooth(float current, float previous, float alpha)
{
    return previous + alpha * (current - previous);
}

float ease_out_cubic(float t)
{
    return 1 - powf(1 - t, 3);
}

float cubic_bezier(float t, float p0, float p1, float p2, float p3)
{
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    float u = 1.0f - t;
    return (u * u * u * p0) + (3 * u * u * t * p1) + (3 * u * t * t * p2) + (t * t * t * p3);
}

void animate_toplevel(struct hellwm_toplevel *toplevel, float bezier_value)
{
    struct wlr_box start = toplevel->animation_start_geom; // TODO: add current_geom as an option too
    struct wlr_box desired = toplevel->desired_geom;

    int initial_width = start.width;
    int initial_height = start.height;
    int final_width = desired.width;
    int final_height = desired.height;

    int current_width = initial_width + bezier_value * (final_width - initial_width);
    int current_height = initial_height + bezier_value * (final_height - initial_height);

    int current_x = start.x + bezier_value * (desired.x - start.x);
    int current_y = start.y + bezier_value * (desired.y - start.y);

    toplevel->pending_geom.width = current_width;
    toplevel->pending_geom.height = current_height;
    toplevel->pending_geom.x = current_x;
    toplevel->pending_geom.y = current_y;
}

static void output_frame(struct wl_listener *listener, void *data)
{
    struct hellwm_output *output = wl_container_of(listener, output, frame);
    struct wlr_scene *scene = output->server->scene;

    struct wlr_scene_output *scene_output = wlr_scene_get_scene_output(scene, output->wlr_output);

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    float fade_duration = output->server->config_manager->decoration->fade_duration;
    float anim_duration = output->server->config_manager->decoration->animation_duration;

    bool fade = output->server->config_manager->decoration->fade_decoration;
    bool animations = output->server->config_manager->decoration->animations_decoration;

    if (output->server->layout_reapply == 1) 
    {
        apply_layout(output->server->active_workspace);
        output->server->layout_reapply = 0;
    }

    struct hellwm_toplevel *toplevel;
    wl_list_for_each(toplevel, &GLOBAL_SERVER->active_workspace->toplevels, link)
    {
        struct wlr_scene_buffer *scene_buffer = surface_find_buffer(&toplevel->scene_tree->node, toplevel->xdg_toplevel->base->surface);

        if (fade == true && fade_duration > 0.0f)
        {
            if (toplevel->set_fade_clock)
            {
                toplevel->fade_start = now;
                toplevel->set_fade_clock = false;
            }

            double elapsed = (now.tv_sec - toplevel->fade_start.tv_sec) +
                (now.tv_nsec - toplevel->fade_start.tv_nsec) / 1e9;

            if (elapsed >= fade_duration)
            {
                wlr_scene_buffer_set_opacity(scene_buffer, 1.0f);
            }
            else
            {
                float t = elapsed / fade_duration;
                wlr_scene_buffer_set_opacity(scene_buffer, t);
            }
        }
        else {
            wlr_scene_buffer_set_opacity(scene_buffer, 1.0f);
        }

        if (animations == true && anim_duration > 0.0f)
        {
            if (toplevel->set_animation_clock)
            {
                toplevel->animation_start = now;
                toplevel->set_animation_clock = false;
            }

            double elapsed = (now.tv_sec - toplevel->animation_start.tv_sec) + (now.tv_nsec - toplevel->animation_start.tv_nsec) / 1e9;

            if (elapsed < anim_duration)
            {
                float t = elapsed / anim_duration;
                float *f = GLOBAL_SERVER->config_manager->decoration->animation_bezier;
                float bezier_value = cubic_bezier(t, f[0], f[1], f[2], f[3]);
                bezier_value = smooth(ease_out_cubic(bezier_value), bezier_value, 0.5);
                animate_toplevel(toplevel, bezier_value);
            }
            else if (toplevel->in_animation == true)
            {
                toplevel->pending_geom = toplevel->desired_geom;
                toplevel->in_animation = false;
            }
        }

        if ((animations == false || anim_duration <= 0.0f) || (animations == true && toplevel->in_animation == false))
        {
            if (toplevel->current_geom.width != toplevel->desired_geom.width ||
                    toplevel->current_geom.height != toplevel->desired_geom.height ||
                    toplevel->current_geom.x != toplevel->desired_geom.x ||
                    toplevel->current_geom.y != toplevel->desired_geom.y)
            {
                toplevel->pending_geom = toplevel->desired_geom;
            }
        }

        if (toplevel->pending_geom.width > -1)
        {
            toplevel->current_geom = toplevel->pending_geom;

            wlr_scene_node_set_position(&toplevel->scene_tree->node, toplevel->current_geom.x, toplevel->current_geom.y);
            wlr_xdg_toplevel_set_size(toplevel->xdg_toplevel, toplevel->current_geom.width, toplevel->current_geom.height);
            wlr_scene_node_set_enabled(&toplevel->scene_tree->node, true);

            toplevel->pending_geom.width = -1;
        }

        /* crop the node, to keep tiling right
         *    - some apps does not allow us to change size,
         *    like discord, so we crop them
         *
         *    For some reason it has to be there, because it's not working with fullscreen etc.
         *    TODO: create some checks*/
        if (toplevel->desired_geom.width != toplevel->xdg_toplevel->current.width || toplevel->desired_geom.height != toplevel->xdg_toplevel->current.height) 
        {
            struct wlr_box clip;
            toplevel_get_clip(toplevel, &clip);
            wlr_scene_subsurface_tree_set_clip(&toplevel->scene_tree->node, &clip);
        }
        else
            wlr_scene_subsurface_tree_set_clip(&toplevel->scene_tree->node, NULL);

        borders_toplevel_update(toplevel);
    }

    wlr_scene_output_commit(scene_output, NULL);
    wlr_scene_output_send_frame_done(scene_output, &now);
}

static void output_request_state(struct wl_listener *listener, void *data) 
{
    /* This function is called when the backend requests a new state for
     * the output. For example, Wayland and X11 backends request a new mode
     * when the output window is resized. */
    struct hellwm_output *output = wl_container_of(listener, output, request_state);
    const struct wlr_output_event_request_state *event = data;
    wlr_output_commit_state(output->wlr_output, event->state);
}

static void output_destroy(struct wl_listener *listener, void *data) 
{
    struct hellwm_output *output = wl_container_of(listener, output, destroy);

    wl_list_remove(&output->frame.link);
    wl_list_remove(&output->request_state.link);
    wl_list_remove(&output->destroy.link);
    wl_list_remove(&output->link);
    free(output);
}

static void handle_renderer_lost(struct wl_listener *listener, void *data)
{
    struct hellwm_server *server = wl_container_of(listener, server, renderer_lost);

    struct hellwm_output *output;
    struct wlr_renderer *old_renderer = server->renderer;
    struct wlr_allocator *old_allocator = server->allocator;

    LOG("Renderer lost, retrying...\n");

    if (!(server->renderer = wlr_renderer_autocreate(server->backend)))
        ERR("%s:%d\n",__func__, __LINE__);

    if (!(server->allocator = wlr_allocator_autocreate(server->backend, server->renderer)))
        ERR("%s:%d\n",__func__, __LINE__);

    wl_signal_add(&server->renderer->events.lost, &server->renderer_lost);
    wlr_compositor_set_renderer(server->compositor, server->renderer);

    wl_list_for_each(output, &server->outputs, link)
        wlr_output_init_render(output->wlr_output, server->allocator, server->renderer);

    wlr_allocator_destroy(old_allocator);
    wlr_renderer_destroy(old_renderer);
}

static void server_backend_new_output(struct wl_listener *listener, void *data)
{
    /* This event is raised by the backend when a new output (aka a display or
     * monitor) becomes available. */
    struct hellwm_server *server = wl_container_of(listener, server, backend_new_output);
    struct wlr_output *wlr_output = data;

    /* Allocates and configures our state for this output */
    struct hellwm_output *output = calloc(1, sizeof(*output));
    output->wlr_output = wlr_output;
    output->server = server;

    /* set up layers */
    wl_list_init(&output->layers.background);
    wl_list_init(&output->layers.top);
    wl_list_init(&output->layers.bottom);
    wl_list_init(&output->layers.overlay);

    /* Set everything according to config */
    hellwm_config_manager_monitor_set(server->config_manager->monitor_manager, output);
    hellwm_workspace_create_for_output(server, hellwm_workspace_get_next_id(server), output);

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

    /* Adds this to the output layout. The add_auto function arranges outputs
     * from left-to-right in the order they appear. A more sophisticated
     * compositor would let the user configure the arrangement of outputs in the
     * layout.
     *
     * The output layout utility automatically adds a wl_output global to the
     * display, which Wayland clients can see to find out information about the
     * output (such as DPI, scale factor, manufacturer, etc).
     */
    struct wlr_output_layout_output *l_output = wlr_output_layout_add_auto(server->output_layout, wlr_output);
    struct wlr_scene_output *scene_output = wlr_scene_output_create(server->scene, wlr_output);
    wlr_scene_output_layout_add_output(server->scene_layout, l_output, scene_output);

    server->active_output = output;

    struct wlr_box output_box;
    wlr_output_layout_get_box(server->output_layout, wlr_output, &output_box);

    output->usable_area = output_box;

    output->total_area.width = wlr_output->width;
    output->total_area.height = wlr_output->height;
    output->total_area.x = 0;
    output->total_area.y = 0;
}

float *border_get_color(enum hellwm_border_state state)
{
    static float invisible[] = {0, 0, 0, 0};
    switch(state)
    {
        case HELLWM_BORDER_INVISIBLE:
            return invisible;
        case HELLWM_BORDER_ACTIVE:
            return GLOBAL_SERVER->config_manager->decoration->active_border_color;
        case HELLWM_BORDER_INACTIVE:
            return GLOBAL_SERVER->config_manager->decoration->inactive_border_color;
    }

    return invisible;
}

static void borders_toplevel_create(struct hellwm_toplevel *toplevel)
{
    if (toplevel == NULL)
        return;

    uint32_t width, height;
    width = toplevel->current_geom.width;
    height = toplevel->current_geom.height;

    uint32_t border_width = GLOBAL_SERVER->config_manager->decoration->border_width;

    const float *border_color = border_get_color(HELLWM_BORDER_INVISIBLE);

    toplevel->borders[0] = wlr_scene_rect_create(toplevel->scene_tree, width, border_width, border_color);
    wlr_scene_node_set_position(&toplevel->borders[0]->node, -border_width, -border_width);

    toplevel->borders[1] = wlr_scene_rect_create(toplevel->scene_tree, border_width, height, border_color);
    wlr_scene_node_set_position(&toplevel->borders[1]->node, width, 0);

    toplevel->borders[2] = wlr_scene_rect_create(toplevel->scene_tree, width, border_width, border_color);
    wlr_scene_node_set_position(&toplevel->borders[2]->node, -border_width, height);

    toplevel->borders[3] = wlr_scene_rect_create(toplevel->scene_tree, border_width, height, border_color);
    wlr_scene_node_set_position(&toplevel->borders[3]->node, -border_width, 0);
}

static void toplevel_borders_set_state(struct hellwm_toplevel *toplevel, enum hellwm_border_state state)
{
    if (toplevel == NULL || toplevel->xdg_toplevel == NULL || toplevel->xdg_toplevel->base == NULL) return;
    toplevel->border_state = state;
}

static void border_toplevel_set_color(struct hellwm_toplevel *toplevel, enum hellwm_border_state state)
{
    if (toplevel == NULL || toplevel->xdg_toplevel == NULL || toplevel->xdg_toplevel->base == NULL) return;
    const float *border_color = border_get_color(state);

    for(size_t i = 0; i < 4; i++)
    {
        wlr_scene_rect_set_color(toplevel->borders[i], border_color);
    }
}

static void borders_toplevel_update(struct hellwm_toplevel *toplevel)
{
    if (toplevel == NULL || toplevel->xdg_toplevel == NULL || toplevel->xdg_toplevel->base == NULL) return;
    if (toplevel->borders[0] == NULL) return;

    uint32_t width, height;
    uint32_t border_width = GLOBAL_SERVER->config_manager->decoration->border_width;

    struct wlr_box box = toplevel->xdg_toplevel->base->current.geometry;

    width = box.width;
    height = box.height;

    border_toplevel_set_color(toplevel, toplevel->border_state);

    wlr_scene_node_set_position(&toplevel->borders[0]->node, -border_width, -border_width);
    wlr_scene_node_set_position(&toplevel->borders[1]->node, width, 0);
    wlr_scene_node_set_position(&toplevel->borders[2]->node, -border_width, height);
    wlr_scene_node_set_position(&toplevel->borders[3]->node, -border_width, 0);

    wlr_scene_rect_set_size(toplevel->borders[0], width + 2 * border_width, border_width);
    wlr_scene_rect_set_size(toplevel->borders[1], border_width, height);
    wlr_scene_rect_set_size(toplevel->borders[2], width + 2 * border_width, border_width);
    wlr_scene_rect_set_size(toplevel->borders[3], border_width, height);
}

static void borders_toplevel_set_all(struct hellwm_server *server)
{
    if (server->active_workspace == NULL)
        return;

    struct hellwm_toplevel *toplevel;
    wl_list_for_each(toplevel, &server->active_workspace->toplevels, link)
    {
        if (toplevel == server->active_workspace->now_focused)
            toplevel_borders_set_state(toplevel, HELLWM_BORDER_ACTIVE);
        else
            toplevel_borders_set_state(toplevel, HELLWM_BORDER_INACTIVE);
    }
}

static void toplevel_center(struct hellwm_toplevel *toplevel)
{
    struct wlr_box ob;
    wlr_output_layout_get_box(toplevel->server->output_layout, toplevel->server->active_workspace->output->wlr_output, &ob);

    int x = ob.x + (toplevel->server->active_workspace->output->total_area.width) / 2;
    int y = ob.y + (toplevel->server->active_workspace->output->total_area.height) / 2;

    toplevel->desired_geom.x = x - toplevel->xdg_toplevel->base->current.geometry.width/2;
    toplevel->desired_geom.y = y - toplevel->xdg_toplevel->base->current.geometry.height/2;
}

static void xdg_toplevel_map(struct wl_listener *listener, void *data) 
{
    /* Called when the surface is mapped, or ready to display on-screen. */
    struct hellwm_toplevel *toplevel = wl_container_of(listener, toplevel, map);

    toplevel->floating = toplevel_should_float(toplevel);
    if (toplevel->floating)
    {
        toplevel->scene_tree = wlr_scene_xdg_surface_create(toplevel->server->floating_tree, toplevel->xdg_toplevel->base);
        toplevel->xdg_toplevel->base->data = toplevel->scene_tree;
        toplevel_center(toplevel);
    }
    else
    {
        toplevel->scene_tree = wlr_scene_xdg_surface_create(toplevel->server->tile_tree, toplevel->xdg_toplevel->base);
        toplevel->xdg_toplevel->base->data = toplevel->scene_tree;
        add_toplevel_to_tree(toplevel->server->active_workspace, toplevel);
    }
    wlr_scene_node_set_enabled(&toplevel->scene_tree->node, false);
    hellwm_workspace_add_toplevel(toplevel->server->active_workspace, toplevel);

    toplevel->fullscreen = false;
    toplevel->first_layout = true;
    toplevel->previous_bezier = 0.0f;
    toplevel->set_animation_clock = false;
    toplevel->border_state = HELLWM_BORDER_INVISIBLE;
    toplevel->foreign_toplevel_handle = wlr_foreign_toplevel_handle_v1_create(toplevel->server->foreign_toplevel_manager);

    /* add this toplevel to the scene tree */
    struct hellwm_view *view = calloc(1, sizeof(*view));
    view->view_type = HELLWM_VIEW_TOPLEVEL;
    view->toplevel = toplevel;
    toplevel->scene_tree->node.data = view;

    GLOBAL_SERVER->layout_reapply = 1;

    toplevel_fade_start(toplevel);
    borders_toplevel_create(toplevel);

    hellwm_focus_toplevel(toplevel);
}

static void xdg_toplevel_unmap(struct wl_listener *listener, void *data)
{
    /* Called when the surface is unmapped, and should no longer be shown. */
    struct hellwm_toplevel *toplevel = wl_container_of(listener, toplevel, unmap);

    /* Reset the cursor mode if the grabbed toplevel was unmapped. */
    if (toplevel == toplevel->server->grabbed_toplevel) 
    {
        reset_cursor_mode(toplevel->server);
    }

    wl_list_remove(&toplevel->link);
    GLOBAL_SERVER->layout_reapply = 1;
}

static bool toplevel_should_float(struct hellwm_toplevel *toplevel)
{
    /* if fixed size toplevel is floating */
    bool b = (toplevel->xdg_toplevel->current.max_height && toplevel->xdg_toplevel->current.max_height == toplevel->xdg_toplevel->current.min_height)
        || (toplevel->xdg_toplevel->current.max_width && toplevel->xdg_toplevel->current.max_width == toplevel->xdg_toplevel->current.min_width)
        || toplevel->xdg_toplevel->parent != NULL;

    return b;
}

static void xdg_toplevel_commit(struct wl_listener *listener, void *data) 
{
    /* Called when a new surface state is committed. */
    struct hellwm_toplevel *toplevel = wl_container_of(listener, toplevel, commit);

    if (toplevel->xdg_toplevel->base->initial_commit) 
    {
          wlr_xdg_toplevel_set_wm_capabilities(toplevel->xdg_toplevel, WLR_XDG_TOPLEVEL_WM_CAPABILITIES_FULLSCREEN);
          wlr_xdg_toplevel_set_size(toplevel->xdg_toplevel, 0, 0);
          return;
    }
}

static void xdg_toplevel_destroy(struct wl_listener *listener, void *data)
{
    /* Called when the xdg_toplevel is destroyed. */
    struct hellwm_toplevel *toplevel = wl_container_of(listener, toplevel, destroy);
    struct hellwm_server *server = toplevel->server;

    hellwm_focus_next_potential(server);
    hellwm_cursor_follow_toplevel(server, toplevel->workspace->now_focused);

    toplevel_unset_fullscreen(toplevel);
    remove_toplevel_from_tree(toplevel->workspace, toplevel);

    wlr_foreign_toplevel_handle_v1_destroy(toplevel->foreign_toplevel_handle);

    wl_list_remove(&toplevel->map.link);
    wl_list_remove(&toplevel->unmap.link);
    wl_list_remove(&toplevel->commit.link);
    wl_list_remove(&toplevel->destroy.link);
    wl_list_remove(&toplevel->request_move.link);
    wl_list_remove(&toplevel->request_resize.link);
    wl_list_remove(&toplevel->request_maximize.link);
    wl_list_remove(&toplevel->request_fullscreen.link);

    free(toplevel);

    server->layout_reapply = 1;
}

static void begin_interactive(struct hellwm_toplevel *toplevel, enum hellwm_cursor_mode mode, uint32_t edges) 
{
    /* This function sets up an interactive move or resize operation, where the
     * compositor stops propegating pointer events to clients and instead
     * consumes them itself, to move or resize windows. */
    struct hellwm_server *server = toplevel->server; struct wlr_surface *focused_surface =
        server->seat->pointer_state.focused_surface;
    if (toplevel->xdg_toplevel->base->surface != wlr_surface_get_root_surface(focused_surface)) 
    {
        /* Deny move/resize requests from unfocused clients. */
        return;
    }
    server->grabbed_toplevel = toplevel;
    server->cursor_mode = mode;

    if (mode == HELLWM_CURSOR_MOVE) 
    {
        server->cursor_grab_x = server->cursor->x - toplevel->scene_tree->node.x;
        server->cursor_grab_y = server->cursor->y - toplevel->scene_tree->node.y;
    } else {
        struct wlr_box geo_box;
        wlr_xdg_surface_get_geometry(toplevel->xdg_toplevel->base, &geo_box);

        double border_x = (toplevel->scene_tree->node.x + geo_box.x) + ((edges & WLR_EDGE_RIGHT) ? geo_box.width : 0);
        double border_y = (toplevel->scene_tree->node.y + geo_box.y) + ((edges & WLR_EDGE_BOTTOM) ? geo_box.height : 0);
        server->cursor_grab_x = server->cursor->x - border_x;
        server->cursor_grab_y = server->cursor->y - border_y;

        server->grab_geometry = geo_box;
        server->grab_geometry.x += toplevel->scene_tree->node.x;
        server->grab_geometry.y += toplevel->scene_tree->node.y;

        server->resize_edges = edges;
    }
}

static void xdg_toplevel_request_move(struct wl_listener *listener, void *data) 
{
    /* This event is raised when a client would like to begin an interactive
     * move, typically because the user clicked on their client-side
     * decorations. Note that a more sophisticated compositor should check the
     * provided serial against a list of button press serials sent to this
     * client, to prevent the client from requesting this whenever they want. */
    struct hellwm_toplevel *toplevel = wl_container_of(listener, toplevel, request_move);
    begin_interactive(toplevel, HELLWM_CURSOR_MOVE, 0);
}

static void xdg_toplevel_request_resize(struct wl_listener *listener, void *data) 
{
    /* This event is raised when a client would like to begin an interactive
     * resize, typically because the user clicked on their client-side
     * decorations. Note that a more sophisticated compositor should check the
     * provided serial against a list of button press serials sent to this
     * client, to prevent the client from requesting this whenever they want. */
    struct wlr_xdg_toplevel_resize_event *event = data;
    struct hellwm_toplevel *toplevel = wl_container_of(listener, toplevel, request_resize);
    begin_interactive(toplevel, HELLWM_CURSOR_RESIZE, event->edges);
}

static void xdg_toplevel_request_maximize(struct wl_listener *listener, void *data)
{
    /* This event is raised when a client would like to maximize itself,
     * typically because the user clicked on the maximize button on client-side
     * decorations. hellwm doesn't support maximization, but to conform to
     * xdg-shell protocol we still must send a configure.
     * wlr_xdg_surface_schedule_configure() is used to send an empty reply.
     * However, if the request was sent before an initial commit, we don't do
     * anything and let the client finish the initial surface setup. */

    struct hellwm_toplevel *toplevel = wl_container_of(listener, toplevel, request_maximize);

    if(toplevel->xdg_toplevel->base->initialized)
        wlr_xdg_surface_schedule_configure(toplevel->xdg_toplevel->base);
}

static void toplevel_unset_fullscreen(struct hellwm_toplevel *toplevel)
{
    if (toplevel == NULL || toplevel->xdg_toplevel == NULL || toplevel->scene_tree == NULL || toplevel->fullscreen == false)
        return;

    toplevel_borders_set_state(toplevel, HELLWM_BORDER_ACTIVE);

    toplevel->desired_geom.width = toplevel->last_state.width;
    toplevel->desired_geom.height = toplevel->last_state.height;
    toplevel->desired_geom.x = toplevel->last_state.x;
    toplevel->desired_geom.y = toplevel->last_state.y;

    wlr_scene_node_reparent(&toplevel->scene_tree->node, toplevel->server->tile_tree);
    wlr_xdg_toplevel_set_fullscreen(toplevel->xdg_toplevel, false);
    GLOBAL_SERVER->active_workspace->fullscreened = false;
    GLOBAL_SERVER->layout_reapply = 1;
    toplevel->fullscreen = false;

}

static void toplevel_set_fullscreen(struct hellwm_toplevel *toplevel)
{
    if (toplevel == NULL || toplevel->xdg_toplevel == NULL || toplevel->scene_tree == NULL)
        return;

    struct hellwm_output *output = toplevel->server->active_workspace->output;
    if (output == NULL)
        return;

    struct wlr_box output_box;
    wlr_output_layout_get_box(toplevel->server->output_layout, output->wlr_output, &output_box);

    toplevel->last_state.x = toplevel->current_geom.x;
    toplevel->last_state.y = toplevel->current_geom.y;
    toplevel->last_state.width = toplevel->current_geom.width;
    toplevel->last_state.height = toplevel->current_geom.height;

    toplevel->desired_geom.width = output_box.width;
    toplevel->desired_geom.height = output_box.height;
    toplevel->desired_geom.x = output_box.x;
    toplevel->desired_geom.y = output_box.y;

    toplevel_borders_set_state(toplevel, HELLWM_BORDER_INVISIBLE);
    wlr_scene_node_reparent(&toplevel->scene_tree->node, toplevel->server->fullscreen_tree);
    wlr_xdg_toplevel_set_fullscreen(toplevel->xdg_toplevel, true);

    GLOBAL_SERVER->active_workspace->fullscreened = true;
    GLOBAL_SERVER->layout_reapply = 1;
    toplevel->fullscreen = true;
}

static void xdg_toplevel_request_fullscreen(struct wl_listener *listener, void *data)
{
    struct hellwm_toplevel *toplevel = wl_container_of(listener, toplevel, request_fullscreen);

    if (toplevel == NULL || toplevel->xdg_toplevel == NULL)
        return;

    if (GLOBAL_SERVER->active_workspace->fullscreened)
    {
        toplevel_unset_fullscreen(toplevel);
    }
    else
    {
        toplevel_set_fullscreen(toplevel);
    }

    toplevel_animation_start(toplevel);

    if (toplevel->xdg_toplevel->base->initialized)
        wlr_xdg_surface_schedule_configure(toplevel->xdg_toplevel->base);
}

static void xdg_toplevel_set_app_id(struct wl_listener *listener, void *data)
{
    struct hellwm_toplevel *toplevel = wl_container_of(listener, toplevel, set_title);
}

static void xdg_toplevel_set_title(struct wl_listener *listener, void *data)
{
    struct hellwm_toplevel *toplevel = wl_container_of(listener, toplevel, set_title);
}

static void server_new_xdg_toplevel(struct wl_listener *listener, void *data) 
{
    /* This event is raised when a client creates a new toplevel (application window). */
    struct hellwm_server *server = wl_container_of(listener, server, new_xdg_toplevel);
    struct wlr_xdg_toplevel *xdg_toplevel = data;

    /* Allocate a hellwm_toplevel for this surface */
    struct hellwm_toplevel *toplevel = calloc(1, sizeof(*toplevel));
    toplevel->xdg_toplevel = xdg_toplevel;
    toplevel->server = server;

    /* Listen to the various events it can emit */
    toplevel->map.notify = xdg_toplevel_map;
    wl_signal_add(&xdg_toplevel->base->surface->events.map, &toplevel->map);

    toplevel->unmap.notify = xdg_toplevel_unmap;
    wl_signal_add(&xdg_toplevel->base->surface->events.unmap, &toplevel->unmap);

    toplevel->commit.notify = xdg_toplevel_commit;
    wl_signal_add(&xdg_toplevel->base->surface->events.commit, &toplevel->commit);

    toplevel->destroy.notify = xdg_toplevel_destroy;
    wl_signal_add(&xdg_toplevel->events.destroy, &toplevel->destroy);

    toplevel->request_move.notify = xdg_toplevel_request_move;
    wl_signal_add(&xdg_toplevel->events.request_move, &toplevel->request_move);

    toplevel->request_resize.notify = xdg_toplevel_request_resize;
    wl_signal_add(&xdg_toplevel->events.request_resize, &toplevel->request_resize);

    toplevel->request_maximize.notify = xdg_toplevel_request_maximize;
    wl_signal_add(&xdg_toplevel->events.request_maximize, &toplevel->request_maximize);

    toplevel->request_fullscreen.notify = xdg_toplevel_request_fullscreen;
    wl_signal_add(&xdg_toplevel->events.request_fullscreen, &toplevel->request_fullscreen);

    toplevel->set_app_id.notify = xdg_toplevel_set_app_id;
    wl_signal_add(&xdg_toplevel->events.set_app_id, &toplevel->set_app_id);

    toplevel->set_title.notify = xdg_toplevel_set_title;
    wl_signal_add(&xdg_toplevel->events.set_title, &toplevel->set_title);
}

static void xdg_decoration_new_toplevel_decoration(struct wl_listener *listener, void *data)
{
    struct wlr_xdg_toplevel_decoration_v1 *decoration = data;
    wlr_xdg_toplevel_decoration_v1_set_mode(decoration, WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);
}

struct hellwm_view *root_parent_surface(struct wlr_surface *wlr_surface)
{
    struct wlr_surface *root_wlr_surface = wlr_surface_get_root_surface(wlr_surface);
    struct wlr_xdg_surface *xdg_surface = wlr_xdg_surface_try_from_wlr_surface(root_wlr_surface);

    struct wlr_scene_tree *tree;

    if(xdg_surface != NULL)
        tree = xdg_surface->data;
    else
    {
        struct wlr_layer_surface_v1 *layer_surface = wlr_layer_surface_v1_try_from_wlr_surface(root_wlr_surface);
        if(layer_surface == NULL)
            return NULL;

        tree = xdg_surface->data;
    }

    struct hellwm_view *view = tree->node.data;
    while(view == NULL || view->view_type == HELLWM_VIEW_POPUP)
    {
        tree = tree->node.parent;
        view = tree->node.data;
    }

    return view;
}

/* topmost node at given coords */
struct hellwm_view *desktop_view_at(struct hellwm_server *server, double lx, double ly, struct wlr_surface **surface, double *sx, double *sy)
{
    struct wlr_scene_node *node = wlr_scene_node_at(&server->scene->tree.node, lx, ly, sx, sy);
    if (node == NULL || node->type != WLR_SCENE_NODE_BUFFER)
        return NULL;

    struct wlr_scene_buffer *scene_buffer = wlr_scene_buffer_from_node(node);
    struct wlr_scene_surface *scene_surface = wlr_scene_surface_try_from_buffer(scene_buffer);
    if (!scene_surface)
        return NULL;

    *surface = scene_surface->surface;

    struct wlr_scene_tree *tree = node->parent;
    struct hellwm_view *view = tree->node.data;

    while(view == NULL || view->view_type == HELLWM_VIEW_POPUP)
    {
        tree = tree->node.parent;
        view = tree->node.data;
    }

    return view;
}

void xdg_popup_commit(struct wl_listener *listener, void *data)
{
    struct hellwm_popup *popup = wl_container_of(listener, popup, commit);

    if (popup->xdg_popup->base->initial_commit) 
    {
        /* When an xdg_surface performs an initial commit, the compositor must
         * reply with a configure so the client can map the surface.
         * hellwm sends an empty configure. A more sophisticated compositor
         * might change an xdg_popup's geometry to ensure it's not positioned
         * off-screen, for example. */
        wlr_xdg_surface_schedule_configure(popup->xdg_popup->base);
    }
}

void xdg_popup_destroy(struct wl_listener *listener, void *data)
{
    struct hellwm_popup *popup = wl_container_of(listener, popup, destroy);

    wl_list_remove(&popup->commit.link);
    wl_list_remove(&popup->destroy.link);

    free(popup);
}

static void server_new_xdg_popup(struct wl_listener *listener, void *data)
{
    /* this event is raised when a client creates a new popup */
    struct wlr_xdg_popup *xdg_popup = data;

    struct hellwm_popup *popup = calloc(1, sizeof(*popup));
    popup->xdg_popup = xdg_popup;

    if(xdg_popup->parent != NULL)
    {
        struct wlr_xdg_surface *parent = wlr_xdg_surface_try_from_wlr_surface(xdg_popup->parent);
        struct wlr_scene_tree *parent_tree = parent->data;
        popup->scene_tree = wlr_scene_xdg_surface_create(parent_tree, xdg_popup->base);

        xdg_popup->base->data = popup->scene_tree;
        struct hellwm_view *view = calloc(1, sizeof(*view));

        view->view_type = HELLWM_VIEW_POPUP;
        view->popup = popup;
        popup->scene_tree->node.data = view;

    }
    else
    {
        /* if there is no parent, than we keep the reference to our hellwm_popup state in this */
        /* user data pointer, in order to later reparent this popup (see server_new_xdg_popup) */
        xdg_popup->base->data = popup;
    }

    popup->commit.notify = xdg_popup_commit;
    wl_signal_add(&xdg_popup->base->surface->events.commit, &popup->commit);

    popup->destroy.notify = xdg_popup_destroy;
    wl_signal_add(&xdg_popup->events.destroy, &popup->destroy);
}

static void layer_surface_handle_commit(struct wl_listener *listener, void *data)
{
    struct hellwm_layer_surface *layer_surface = wl_container_of(listener, layer_surface, commit);

    if(!layer_surface->wlr_layer_surface->initialized)
        return;

    if(layer_surface->wlr_layer_surface->initial_commit)
    {
        struct hellwm_output *output = layer_surface->wlr_layer_surface->output->data;
        if (output == NULL)
            output = layer_surface->server->active_output;

        struct wlr_box output_box;
        wlr_output_layout_get_box(layer_surface->server->output_layout, output->wlr_output, &output_box);
        wlr_scene_layer_surface_v1_configure(layer_surface->scene, &output_box, &output->usable_area);
    }
}

static void layer_surface_handle_map(struct wl_listener *listener, void *data)
{
    struct hellwm_layer_surface *layer_surface = wl_container_of(listener, layer_surface, map);
    layer_surface_map_func(layer_surface);
}

static void layer_surface_map_func(struct hellwm_layer_surface *layer_surface)
{
    struct hellwm_server *server = layer_surface->server;

    struct wlr_layer_surface_v1 *wlr_layer_surface = layer_surface->wlr_layer_surface;
    struct hellwm_output *output = wlr_layer_surface->output->data;

    if (output == NULL)
        output = server->active_output;

    uint32_t layer = wlr_layer_surface->pending.layer;

    wlr_scene_node_raise_to_top(&layer_surface->scene->tree->node);

    switch(layer)
    {
        case ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND:
            wl_list_insert(&output->layers.background, &layer_surface->link);
            break;
        case ZWLR_LAYER_SHELL_V1_LAYER_TOP:
            wl_list_insert(&output->layers.top, &layer_surface->link);
            break;
        case ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM:
            wl_list_insert(&output->layers.bottom, &layer_surface->link);
            break;
        case ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY:
            wl_list_insert(&output->layers.overlay, &layer_surface->link);
            break;
    }

    struct wlr_box output_box;
    wlr_output_layout_get_box(server->output_layout, output->wlr_output, &output_box);

    wlr_scene_layer_surface_v1_configure(layer_surface->scene, &output_box, &output->usable_area);

    focus_layer_surface(layer_surface);

    if(output_box.width != output->usable_area.width || output_box.height != output->usable_area.height)
        server->layout_reapply = 1;

    //* TODO - LOOK HERE
    LOG("USABLE_AREA: %d,%d - (%d, %d)\n\n",
            output->usable_area.x,
            output->usable_area.y,
            output->usable_area.width,
            output->usable_area.height);
}

static void layer_surface_handle_unmap(struct wl_listener *listener, void *data)
{
    struct hellwm_layer_surface *layer_surface = wl_container_of(listener, layer_surface, unmap);
    layer_surface_unmap_func(layer_surface);
}

static void layer_surface_unmap_func(struct hellwm_layer_surface *layer_surface)
{
    struct hellwm_server *server = layer_surface->server;

    if (!layer_surface || !layer_surface->wlr_layer_surface || !layer_surface->wlr_layer_surface->output) {
        LOG("layer_surface_handle_unmap(): NULL \n");
        return;
    }

    struct wlr_layer_surface_v1_state state = layer_surface->wlr_layer_surface->current;
    struct hellwm_output *output = layer_surface->wlr_layer_surface->output->data;

    if (!output) {
        LOG("Error: output->data is NULL\n");
        return;
    }

    server->layer_exclusive_keyboard = NULL;

    // Check and modify usable_area only if exclusive_zone is set
    if (state.exclusive_zone > 0) {
        switch (state.anchor) {
            case ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP:
            case (ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP |
                  ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT |
                  ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT):
                // Anchor top
                output->usable_area.y -= (state.exclusive_zone + state.margin.top);
                output->usable_area.height += (state.exclusive_zone + state.margin.top);
                break;

            case ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM:
            case (ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
                  ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT |
                  ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT):
                // Anchor bottom
                output->usable_area.height += (state.exclusive_zone + state.margin.bottom);
                break;

            case ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT:
            case (ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP |
                  ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
                  ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT):
                // Anchor left
                output->usable_area.x -= (state.exclusive_zone + state.margin.left);
                output->usable_area.width += (state.exclusive_zone + state.margin.left);
                break;

            case ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT:
            case (ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP |
                  ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
                  ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT):
                // Anchor right
                output->usable_area.width += (state.exclusive_zone + state.margin.right);
                break;

            default:
                LOG("Warning: Unhandled anchor value %d\n", state.anchor);
                break;
        }
    }

    // Safely remove the layer_surface from the list
    if (!wl_list_empty(&layer_surface->link))
        wl_list_remove(&layer_surface->link);

    // Apply layout
    if (server->active_workspace)
        server->layout_reapply = 1;
}


static void layer_surface_handle_destroy(struct wl_listener *listener, void *data)
{
    struct hellwm_layer_surface *layer_surface = wl_container_of(listener, layer_surface, destroy);

    wl_list_remove(&layer_surface->map.link);
    wl_list_remove(&layer_surface->unmap.link);
    wl_list_remove(&layer_surface->destroy.link);

    free(layer_surface);
}


void layer_surface_handle_new_popup(struct wl_listener *listener, void *data)
{
    struct hellwm_layer_surface *layer_surface = wl_container_of(listener, layer_surface, new_popup);
    struct wlr_xdg_popup *xdg_popup = data;

    struct hellwm_popup *popup = xdg_popup->base->data;

    struct wlr_scene_tree *parent_tree = layer_surface->scene->tree;
    popup->scene_tree = wlr_scene_xdg_surface_create(parent_tree, xdg_popup->base);

    struct hellwm_view *view = calloc(1, sizeof(*view));
    view->view_type = HELLWM_VIEW_POPUP;
    view->popup = popup;
    popup->scene_tree->node.data = view;

    popup->xdg_popup->base->data = popup->scene_tree;
}

static void handle_new_layer_surface(struct wl_listener *listener, void *data)
{
    struct hellwm_server *server = wl_container_of(listener, server, new_layer_surface);
    struct wlr_layer_surface_v1 *wlr_layer_surface = data;

    struct hellwm_layer_surface *layer_surface = calloc(1, sizeof(*layer_surface));
    layer_surface->wlr_layer_surface = wlr_layer_surface;
    layer_surface->wlr_layer_surface->data = layer_surface;
    layer_surface->server = server;

    switch(wlr_layer_surface->pending.layer)
    {
        case ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND:
            layer_surface->scene = wlr_scene_layer_surface_v1_create(server->background_tree, wlr_layer_surface);
            break;
        case ZWLR_LAYER_SHELL_V1_LAYER_TOP:
            layer_surface->scene = wlr_scene_layer_surface_v1_create(server->top_tree, wlr_layer_surface);
            break;
        case ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM:
            layer_surface->scene = wlr_scene_layer_surface_v1_create(server->bottom_tree, wlr_layer_surface);
            break;
        case ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY:
            layer_surface->scene = wlr_scene_layer_surface_v1_create(server->overlay_tree, wlr_layer_surface);
            break;
    }

    struct hellwm_view *view = calloc(1, sizeof(*view));
    view->view_type = HELLWM_VIEW_LAYER;
    view->layer_surface = layer_surface;

    layer_surface->scene->tree->node.data = view;

    if(layer_surface->wlr_layer_surface->output == NULL)
        layer_surface->wlr_layer_surface->output = server->active_workspace->output->wlr_output;
    layer_surface->wlr_layer_surface->output->data = server->active_workspace->output;

    layer_surface->commit.notify = layer_surface_handle_commit;
    wl_signal_add(&wlr_layer_surface->surface->events.commit, &layer_surface->commit);

    layer_surface->map.notify = layer_surface_handle_map;
    wl_signal_add(&wlr_layer_surface->surface->events.map, &layer_surface->map);

    layer_surface->unmap.notify = layer_surface_handle_unmap;
    wl_signal_add(&wlr_layer_surface->surface->events.unmap, &layer_surface->unmap);

    layer_surface->new_popup.notify = layer_surface_handle_new_popup;
    wl_signal_add(&wlr_layer_surface->events.new_popup, &layer_surface->new_popup);

    layer_surface->destroy.notify = layer_surface_handle_destroy;
    wl_signal_add(&wlr_layer_surface->surface->events.destroy, &layer_surface->destroy);
}

bool hellwm_config_manager_keyboard_find_and_apply(hellwm_config_manager_keyboard *keyboard_manager, struct hellwm_keyboard *keyboard)
{
    for (int i = 0; i < keyboard_manager->count; i++)
    {
        if (!strcmp(keyboard_manager->keyboards[i]->name, keyboard->wlr_keyboard->base.name))
        {
            hellwm_config_manager_keyboard_set(keyboard, keyboard_manager->keyboards[i]);
            return true;
        }
    }
    return false;
}

void hellwm_config_manager_keyboard_add(hellwm_config_manager_keyboard *keyboard_manager, hellwm_config_keyboard *keyboard)
{
    if (!strcmp(keyboard->name, "default"))
    {
        keyboard_manager->default_keyboard = keyboard;
        return;
    }

    keyboard_manager->keyboards = realloc(keyboard_manager->keyboards, (keyboard_manager->count + 1) * sizeof(hellwm_config_keyboard));
    if (!keyboard_manager->keyboards)
    {
        LOG("%s:%d failed to reallocate hellwm_config_manager_keyboard\n",__func__, __LINE__);
        return;
    }

    keyboard_manager->keyboards[keyboard_manager->count] = keyboard;
    keyboard_manager->count++;
}

void hellwm_config_manager_keyboard_set(struct hellwm_keyboard *keyboard, hellwm_config_keyboard *config)
{
    struct xkb_context *context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);

    struct xkb_rule_names rule_names = 
    {
        .rules   = config->rules,
        .model   = config->model,
        .layout  = config->layout,
        .variant = config->variant,
        .options = config->options
    };

    struct xkb_keymap *keymap = xkb_keymap_new_from_names(context, &rule_names, XKB_KEYMAP_COMPILE_NO_FLAGS);
    wlr_keyboard_set_keymap(keyboard->wlr_keyboard, keymap);
    wlr_keyboard_set_repeat_info(keyboard->wlr_keyboard, config->rate, config->delay);

    LOG("Keyboard %s - layout: %s, rate: %d, delay: %d\n", keyboard->wlr_keyboard->base.name, config->layout, config->rate, config->delay);

    xkb_keymap_unref(keymap);
    xkb_context_unref(context);
}

void hellwm_config_manager_monitor_find_and_apply(char *name, struct wlr_output_state *state, hellwm_config_manager_monitor *monitor_manager)
{
    for (int i = 0; i < monitor_manager->count; i++)
    {
        if (!strcmp(monitor_manager->monitors[i]->name, name))
        {
            if (monitor_manager->monitors[i]->enabled)
            {
                wlr_output_state_set_custom_mode(state, monitor_manager->monitors[i]->width, monitor_manager->monitors[i]->height, (monitor_manager->monitors[i]->hz * 1000)); /* Hz -> MHz */
                wlr_output_state_set_adaptive_sync_enabled(state, monitor_manager->monitors[i]->vrr);
                wlr_output_state_set_transform(state, monitor_manager->monitors[i]->transfrom);
                wlr_output_state_set_scale(state, monitor_manager->monitors[i]->scale);
            }
            else
            {
                wlr_output_state_set_enabled(state, monitor_manager->monitors[i]->enabled);
            }
        }
    }
}

void hellwm_config_manager_monitor_add(hellwm_config_manager_monitor *monitor_manager, hellwm_config_monitor *monitor)
{
    monitor_manager->monitors = realloc(monitor_manager->monitors, (monitor_manager->count + 1) * sizeof(hellwm_config_monitor));
    if (!monitor_manager->monitors)
    {
        LOG("%s:%d failed to reallocate hellwm_config_manager_monitor\n",__func__, __LINE__);
        return;
    }

    monitor_manager->monitors[monitor_manager->count] = monitor;
    monitor_manager->count++;
}

void hellwm_config_manager_monitor_set(hellwm_config_manager_monitor *config, struct hellwm_output *output)
{
    /* Configures the output created by the backend to use our allocator
     * and our renderer. Must be done once, before commiting the output */
    wlr_output_init_render(output->wlr_output, output->server->allocator, output->server->renderer);

    /* The output may be disabled, switch it on. */
    struct wlr_output_state state;
    wlr_output_state_init(&state);
    wlr_output_state_set_enabled(&state, true);

    /* Some backends don't have modes. DRM+KMS does, and we need to set a mode
     * before we can use the output. The mode is a tuple of (width, height,
     * refresh rate), and each monitor supports only a specific set of modes. We
     * just pick the monitor's preferred mode, a more sophisticated compositor
     * would let the user configure it. */
    struct wlr_output_mode *mode = wlr_output_preferred_mode(output->wlr_output);
    if (mode != NULL) 
    {
        wlr_output_state_set_mode(&state, mode);
    }

    hellwm_config_manager_monitor_find_and_apply(output->wlr_output->name, &state, config);

    /* Atomically applies the new output state. */
    wlr_output_commit_state(output->wlr_output, &state);
    wlr_output_state_finish(&state);

    LOG("Monitor %s: enabled: %d | %dx%d@%.3f vrr: %d scale %.2f transform %d\n",
            output->wlr_output->name,
            output->wlr_output->enabled,
            output->wlr_output->width,
            output->wlr_output->height,
            (output->wlr_output->refresh / 1000.0), /* MHz -> Hz */
            output->wlr_output->adaptive_sync_supported,
            output->wlr_output->scale,
            output->wlr_output->transform
       ); 
}

void hellwm_config_manager_keyboard_reload(struct hellwm_server *server)
{
    LOG("Reloading all keyboards\n");

    struct hellwm_keyboard *keyboard;
    wl_list_for_each(keyboard, &server->keyboards, link)
    {
        if (!hellwm_config_manager_keyboard_find_and_apply(server->config_manager->keyboard_manager, keyboard))
        {
            hellwm_config_manager_keyboard_set(keyboard, server->config_manager->keyboard_manager->default_keyboard);
        }
    }
}

void hellwm_config_manager_monitor_reload(struct hellwm_server *server)
{
    LOG("Reloading all outputs\n");

    struct hellwm_output *output;
    wl_list_for_each(output, &server->outputs, link)
    {
        hellwm_config_manager_monitor_set(server->config_manager->monitor_manager, output);
    }
}

void hellwm_config_manager_reload(struct hellwm_server *server)
{
    hellwm_config_keybindings_free(server->config_manager->keybindings);
    hellwm_config_manager_monitor_free(server->config_manager->monitor_manager);
    hellwm_config_manager_keyboard_free(server->config_manager->keyboard_manager);
    hellwm_config_manager_binary_workspaces_free(server->config_manager->binary_workspaces_manager);

    server->config_manager = hellwm_config_manager_create();
    hellwm_config_manager_load_from_file("./config.lua");

    //TODO: uncomment, for some reason it leads to crash sometimes
    //hellwm_config_manager_monitor_reload(server);
    //hellwm_config_manager_keyboard_reload(server);

    server->layout_reapply = 1;
    borders_toplevel_set_all(server);
}

void hellwm_config_keybindings_free(hellwm_config_manager_keybindings *keybindings)
{
    if (keybindings == NULL) return;

    for (size_t i = 0; i < keybindings->count; i++)
    {
        free(keybindings->keybindings[i]);
        free(keybindings->keybindings[i]->content);
    }

    free(keybindings->keybindings);
    free(keybindings);
}

void hellwm_config_manager_keyboard_free(hellwm_config_manager_keyboard *keyboard)
{
    if (keyboard)
    {
        free(keyboard->default_keyboard);
        free(keyboard->keyboards);
        free(keyboard);
    }
}

void hellwm_config_manager_monitor_free(hellwm_config_manager_monitor *monitor)
{
    if (monitor)
    {
        free(monitor);
    }
}

void hellwm_config_manager_binary_workspaces_free(struct hellwm_binary_workspaces_manager *binary_workspaces)
{
    if (binary_workspaces)
    {
        for (size_t i = 0; i < binary_workspaces->keybinds_count; i++)
        {
            free(binary_workspaces->keybinds[i]->keysyms);
        }
        free(binary_workspaces->keybinds);
    }
}

void hellwm_config_manager_free(struct hellwm_config_manager *config)
{
    if (config)
    {
        hellwm_config_keybindings_free(config->keybindings);
        hellwm_config_manager_monitor_free(config->monitor_manager);
        hellwm_config_manager_keyboard_free(config->keyboard_manager);
        hellwm_config_manager_binary_workspaces_free(config->binary_workspaces_manager);
        free(config);
    }
}

int hellwm_lua_add_keyboard(lua_State *L)
{
    hellwm_config_keyboard *keyboard = calloc(1, sizeof(hellwm_config_keyboard));
    if (keyboard == NULL) {
        LOG("Memory allocation failed for keyboard.\n");
        return 0;
    }

    keyboard->name = NULL;
    keyboard->layout = NULL;
    keyboard->rate = 25;
    keyboard->delay = 600;
    keyboard->options = NULL;
    keyboard->rules = NULL;
    keyboard->variant = NULL;
    keyboard->model = NULL;

    int nargs = lua_gettop(L);

    for (int i = 1; i <= nargs; i++)
    {
        switch (i)
        {
            case 1:
                if (lua_isstring(L, i)) {
                    keyboard->name = strdup(lua_tostring(L, i));
                } else {
                    LOG("Invalid argument for 'name', expected string.\n");
                }
                break;
            case 2:
                if (lua_isstring(L, i)) {
                    keyboard->layout = strdup(lua_tostring(L, i));
                } else {
                    LOG("Invalid argument for 'layout', expected string.\n");
                }
                break;
            case 3:
                if (lua_isnumber(L, i)) {
                    keyboard->rate = (int32_t)lua_tonumber(L, i);
                } else {
                    LOG("Invalid argument for 'rate', expected number.\n");
                }
                break;
            case 4:
                if (lua_isnumber(L, i)) {
                    keyboard->delay = (int32_t)lua_tonumber(L, i);
                } else {
                    LOG("Invalid argument for 'delay', expected number.\n");
                }
                break;
            case 5:
                if (lua_isstring(L, i)) {
                    keyboard->options = strdup(lua_tostring(L, i));
                } else {
                    LOG("Invalid argument for 'options', expected string.\n");
                }
                break;
            case 6:
                if (lua_isstring(L, i)) {
                    keyboard->rules = strdup(lua_tostring(L, i));
                } else {
                    LOG("Invalid argument for 'rules', expected string.\n");
                }
                break;
            case 7:
                if (lua_isstring(L, i)) {
                    keyboard->variant = strdup(lua_tostring(L, i));
                } else {
                    LOG("Invalid argument for 'variant', expected string.\n");
                }
                break;
            case 8:
                if (lua_isstring(L, i)) {
                    keyboard->model = strdup(lua_tostring(L, i));
                } else {
                    LOG("Invalid argument for 'model', expected string.\n");
                }
                break;
            default:
                LOG("Too many arguments provided to keyboard() function!\n");
                break;
        }
    }

    if (keyboard->name == NULL) {
        LOG("Keyboard name is mandatory!\n");
        free(keyboard);
        return 0;
    }

    hellwm_config_manager_keyboard_add(GLOBAL_SERVER->config_manager->keyboard_manager, keyboard);
    return 0;
}

int hellwm_lua_add_monitor(lua_State *L)
{
    hellwm_config_monitor *monitor = calloc(1, sizeof(hellwm_config_monitor));
    int nargs = lua_gettop(L);

    for (int i = 1; i<=nargs; i++)
    {
        switch (i)
        {
            case 1:
                monitor->name = strdup(lua_tostring(L, i));
                break;
            case 2:
                monitor->enabled = (bool)lua_toboolean(L, i);
                break;
            case 3:
                monitor->width = (int32_t)lua_tonumber(L, i);
                break;
            case 4:
                monitor->height = (int32_t)lua_tonumber(L, i);
                break;
            case 5:
                monitor->hz = (int32_t)lua_tonumber(L, i);
                break;
            case 6:
                monitor->vrr = (bool)lua_tonumber(L, i);
                break;
            case 7:
                monitor->scale = (float)lua_tonumber(L, i);
                break;
            case 8:
                monitor->transfrom = (int32_t)lua_tonumber(L, i);
                break;
            default:
                LOG("Provided to much arguments to monitor() function!\n");
        }
    }

    hellwm_config_manager_monitor_add(GLOBAL_SERVER->config_manager->monitor_manager, monitor);

    return 0;
}

int hellwm_lua_add_keybind(lua_State *L)
{
    char *keys = NULL; 
    char *action = NULL; 
    int nargs = lua_gettop(L);
    struct hellwm_config_keybind_workspace workspace;
    workspace.workspace_id = -1;
    workspace.binary_workspace_val = -1;
    workspace.binary_workspaces_enabled = false;

    for (int i = 1; i<=nargs; i++)
    {
        switch (i)
        {
            case 1:
                keys = strdup(lua_tostring(L, i));
                break;
            case 2:
                action = strdup(lua_tostring(L, i));
                break;
            case 3:
                if (!strcmp(action, "workspace") || !strcmp(action, "move_active"))
                    workspace.workspace_id = (int)lua_tonumber(L, i);
                else
                    return 0;
                break;
            case 4:
                workspace.binary_workspaces_enabled = (bool)lua_toboolean(L, i);
                break;
            case 5:
                workspace.binary_workspace_val = (int)lua_tonumber(L, i);
                break;
            case 6:
                workspace.move_with_active = (bool)lua_toboolean(L, i);
                break;
            default:
                LOG("Provided to much arguments to keybind() function!\n");
        }
    }

    if (workspace.workspace_id != -1 && keys != NULL)
    {
        if (!strcmp(action, "move_active"))
            hellwm_config_keybind_add_workspace_to_config(GLOBAL_SERVER, keys, workspace);
        else
            hellwm_config_keybind_add_workspace_to_config(GLOBAL_SERVER, keys, workspace);
        return 0;
    }
    if (keys != NULL && action != NULL)
    {
        hellwm_config_keybind_add_function_or_cmd_to_config(GLOBAL_SERVER, keys, action);
        return 0;
    }
    return 1;
}

int hellwm_lua_env(lua_State *L)
{
    int nargs = lua_gettop(L);

    char *name;
    char *value;

    for (int i = 1; i<=nargs; i++)
    {
        switch (i)
        {
            case 1:
                name = strdup(lua_tostring(L, i));
                break;
            case 2:
                value = strdup(lua_tostring(L, i));
                LOG("Env: %s = %s\n", name, value);
                setenv(name, value, 1);
                return 0;
            default:
                LOG("Provided to much arguments to env() function!\n");
        }
    }
    return 0;
}

int hellwm_lua_exec(lua_State *L)
{
    int nargs = lua_gettop(L);
    char **temp = NULL;

    for (int i = 1; i <= nargs; i++)
    {
        switch (i)
        {
            case 1:
                temp = realloc(GLOBAL_SERVER->exec_args, (GLOBAL_SERVER->exec_args_count + 1) * sizeof(char *));
                if (temp == NULL) {
                    LOG("Memory allocation failed during realloc.\n");
                    return 0;
                }

                GLOBAL_SERVER->exec_args = temp;
                GLOBAL_SERVER->exec_args[GLOBAL_SERVER->exec_args_count] = strdup(lua_tostring(L, i));
                if (GLOBAL_SERVER->exec_args[GLOBAL_SERVER->exec_args_count] == NULL) {
                    LOG("Failed to allocate memory for new argument.\n");
                    return 0;
                }
                GLOBAL_SERVER->exec_args_count++;
                break;
            default:
                LOG("Too many arguments provided to exec() function!\n");
                break;
        }
    }
    return 0;
}

int hellwm_lua_tiling_layout(lua_State *L)
{
    int nargs = lua_gettop(L);

    for (int i = 1; i<=nargs; i++)
    {
        switch (i)
        {
            case 1:
                GLOBAL_SERVER->config_manager->tiling_layout = lua_tointeger(L, i);
                break;
            default:
                LOG("Provided to much arguments to exec() function!\n");
        }
    }
    return 0;
}

int hellwm_lua_decoration_border_width(lua_State *L)
{
    int nargs = lua_gettop(L);

    for (int i = 1; i<=nargs; i++)
    {
        switch (i)
        {
            case 1:
                GLOBAL_SERVER->config_manager->decoration->border_width = lua_tointeger(L, i);
                break;
            default:
                LOG("Provided to much arguments to border_width() function!\n");
        }
    }
    return 0;
}

int hellwm_lua_decoration_inner_gap(lua_State *L)
{
    int nargs = lua_gettop(L);

    for (int i = 1; i<=nargs; i++)
    {
        switch (i)
        {
            case 1:
                GLOBAL_SERVER->config_manager->decoration->inner_gap = lua_tointeger(L, i);
                break;
            default:
                LOG("Provided to much arguments to inner_gap() function!\n");
        }
    }
    return 0;
}

int hellwm_lua_decoration_outer_gap(lua_State *L)
{
    int nargs = lua_gettop(L);

    for (int i = 1; i<=nargs; i++)
    {
        switch (i)
        {
            case 1:
                GLOBAL_SERVER->config_manager->decoration->outer_gap = lua_tointeger(L, i);
                break;
            default:
                LOG("Provided to much arguments to outer_gap() function!\n");
        }
    }
    return 0;
}


int hellwm_lua_decoration_border_active(lua_State *L)
{
    float color[4];
    char *string_color = NULL;
    int nargs = lua_gettop(L);

    for (int i = 1; i<=nargs; i++)
    {
        switch (i)
        {
            case 1:
                 string_color = strdup(lua_tostring(L, i));

                if (get_color(string_color, color))
                {
                    LOG("BorderColor (ACTIVE):  (%f, %f, %f), Alpha: %f\n", color[0], color[1], color[2], color[3]);
                }
                else {
                    LOG("Invalid color format: border_active_color(\"%s\")\n", string_color);
                    return 1;
                }

                GLOBAL_SERVER->config_manager->decoration->active_border_color[0] = color[0];
                GLOBAL_SERVER->config_manager->decoration->active_border_color[1] = color[1];
                GLOBAL_SERVER->config_manager->decoration->active_border_color[2] = color[2];
                GLOBAL_SERVER->config_manager->decoration->active_border_color[3] = color[3];
                break;
            default:
                LOG("Provided to much arguments to gaps() function!\n");
        }
    }
    return 0;
}

int hellwm_lua_decoration_fade_duration(lua_State *L)
{
    int nargs = lua_gettop(L);

    for (int i = 1; i<=nargs; i++)
    {
        switch (i)
        {
            case 1:
                GLOBAL_SERVER->config_manager->decoration->fade_duration = lua_tonumber(L, i);
                break;
            default:
                LOG("Provided to much arguments to fade_duration() function!\n");
        }
    }
    return 0;
}

int hellwm_lua_decoration_animation_duration(lua_State *L)
{
    int nargs = lua_gettop(L);

    for (int i = 1; i<=nargs; i++)
    {
        switch (i)
        {
            case 1:
                GLOBAL_SERVER->config_manager->decoration->animation_duration = lua_tonumber(L, i);
                break;
            default:
                LOG("Provided to much arguments to fade_duration() function!\n");
        }
    }
    return 0;
}

int hellwm_lua_decoration_animation_bezier(lua_State *L)
{
    int nargs = lua_gettop(L);

    for (int i = 1; i<=nargs; i++)
    {
        switch (i)
        {
            case 1:
                GLOBAL_SERVER->config_manager->decoration->animation_bezier[0] = lua_tonumber(L, i);
                break;
            case 2:
                GLOBAL_SERVER->config_manager->decoration->animation_bezier[1] = lua_tonumber(L, i);
                break;
            case 3:
                GLOBAL_SERVER->config_manager->decoration->animation_bezier[2] = lua_tonumber(L, i);
                break;
            case 4:
                GLOBAL_SERVER->config_manager->decoration->animation_bezier[3] = lua_tonumber(L, i);
                break;
            default:
                LOG("Provided to much arguments to fade_duration() function!\n");
        }
    }
    return 0;
}

int hellwm_lua_decoration_border_inactive(lua_State *L)
{
    float color[4];
    char *string_color = NULL;
    int nargs = lua_gettop(L);

    for (int i = 1; i<=nargs; i++)
    {
        switch (i)
        {
            case 1:
                 string_color = strdup(lua_tostring(L, i));

                if (get_color(string_color, color))
                {
                    LOG("BorderColor(INACTIVE): (%f, %f, %f), Alpha: %f\n", color[0], color[1], color[2], color[3]);
                }
                else {
                    LOG("Invalid color format: border_inactive_color(\"%s\")\n", string_color);
                    return 1;
                }

                GLOBAL_SERVER->config_manager->decoration->inactive_border_color[0] = color[0];
                GLOBAL_SERVER->config_manager->decoration->inactive_border_color[1] = color[1];
                GLOBAL_SERVER->config_manager->decoration->inactive_border_color[2] = color[2];
                GLOBAL_SERVER->config_manager->decoration->inactive_border_color[3] = color[3];
                break;
            default:
                LOG("Provided to much arguments to gaps() function!\n");
        }
    }
    return 0;
}

int hellwm_lua_decoration_animation_direction(lua_State *L)
{
    char *animation_string = NULL;
    int nargs = lua_gettop(L);
    enum animation_direction animation_state;

    for (int i = 1; i<=nargs; i++)
    {
        switch (i)
        {
            case 1:
                if (lua_isstring(L, i)) {
                    animation_string  = strdup(lua_tostring(L, i));
                } else {
                    LOG("Invalid argument for 'animation_direction', expected string.\n");
                    return 1;
                }
                break;
            default:
                LOG("Provided to much arguments to animation_direction() function!\n");
                return 1;
        }
    }
    
    if (animation_string == NULL)
        return 1;

     if (strcmp(animation_string, "grow") == 0)
        animation_state = HELLWM_ANIMATION_GROW;
    else if (strcmp(animation_string, "shrink") == 0)
        animation_state = HELLWM_ANIMATION_SHRINK;
    else if (strcmp(animation_string, "solid") == 0)
        animation_state = HELLWM_ANIMATION_SOLID;
    else if (strcmp(animation_string, "left") == 0)
        animation_state = HELLWM_ANIMATION_LEFT;
    else if (strcmp(animation_string, "right") == 0)
        animation_state = HELLWM_ANIMATION_RIGHT;
    else if (strcmp(animation_string, "up") == 0)
        animation_state = HELLWM_ANIMATION_UP;
    else if (strcmp(animation_string, "down") == 0)
        animation_state = HELLWM_ANIMATION_DOWN;
    else
        return 1;

    GLOBAL_SERVER->config_manager->decoration->default_animation = animation_state;
    return 0;
}

void hellwm_config_manager_load_from_file(char * filename)
{
    LOG("Loading config from: %s\n", filename);

    lua_State *L = luaL_newstate();
    luaL_openlibs(L);

    lua_pushcfunction(L, hellwm_lua_add_monitor);
    lua_setglobal(L, "monitor");

    lua_pushcfunction(L, hellwm_lua_add_keyboard);
    lua_setglobal(L, "keyboard");

    lua_pushcfunction(L, hellwm_lua_add_keybind);
    lua_setglobal(L, "bind");

    lua_pushcfunction(L, hellwm_lua_env);
    lua_setglobal(L, "env");

    lua_pushcfunction(L, hellwm_lua_exec);
    lua_setglobal(L, "exec");

    lua_pushcfunction(L, hellwm_lua_tiling_layout);
    lua_setglobal(L, "layout");

    lua_pushcfunction(L, hellwm_lua_decoration_border_width);
    lua_setglobal(L, "border_width");

    lua_pushcfunction(L, hellwm_lua_decoration_inner_gap);
    lua_setglobal(L, "inner_gap");

    lua_pushcfunction(L, hellwm_lua_decoration_outer_gap);
    lua_setglobal(L, "outer_gap");

    lua_pushcfunction(L, hellwm_lua_decoration_border_active);
    lua_setglobal(L, "border_active_color");

    lua_pushcfunction(L, hellwm_lua_decoration_border_inactive);
    lua_setglobal(L, "border_inactive_color");

    lua_pushcfunction(L, hellwm_lua_decoration_fade_duration);
    lua_setglobal(L, "fade_duration");

    lua_pushcfunction(L, hellwm_lua_decoration_animation_duration);
    lua_setglobal(L, "animation_duration");

    lua_pushcfunction(L, hellwm_lua_decoration_animation_bezier);
    lua_setglobal(L, "animation_bezier");

    lua_pushcfunction(L, hellwm_lua_decoration_animation_direction);
    lua_setglobal(L, "animation_direction");

    if (luaL_loadfile(L, filename) || lua_pcall(L, 0, 0, 0))
        ;
        // TODO > Create dummy config manager to store all vars from config, so in case of an error it instead of crash it will just not apply.
        //hellwm_lua_error(L, "Cannot load configuration file: %s\n", lua_tostring(L, -1));
    else
    {
        // TODO > if not failed GLOBAL_SERVER->config_manager = dummy config
    }

    lua_close(L);

    hellwm_config_print(GLOBAL_SERVER->config_manager);
}

bool hellwm_convert_string_to_xkb_keys(xkb_keysym_t **keysyms_arr, const char *keys_str, int *num_keys)
{
    char *keys_copy = strdup(keys_str);
    if (!keys_copy)
    {
        LOG("hellwm_convert_string_to_xkb_keys(%s:%d): Failed to allocate memory\n",__func__, __LINE__);
        *keysyms_arr = NULL;
        return false;
    }

    char *token = strtok(keys_copy, ",");
    int count = 0;

    // Initial allocation for maximum possible keys
    xkb_keysym_t *keysyms = malloc(sizeof(xkb_keysym_t) * 10);
    if (!keysyms) {
        LOG("%s:%d Failed to allocate memory for keysyms\n",__func__, __LINE__);
        free(keys_copy);
        *keysyms_arr = NULL;
        return false;
    }

    struct xkb_context *ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    if (!ctx)
    {
        LOG("%s:%d Failed to create XKB context\n",__func__, __LINE__);
        free(keys_copy);
        free(keysyms);
        *keysyms_arr = NULL;
        return false;
    }

    while (token)
    {
        remove_spaces(token);
        xkb_keysym_t keysym = xkb_keysym_from_name(token, XKB_KEYSYM_NO_FLAGS);
        if (keysym == XKB_KEY_NoSymbol)
        {
            LOG("%s:%d Invalid keysym: %s, keys_str: \"%s\"\n",__func__, __LINE__, token, keys_str);
            free(keys_copy);
            free(keysyms);
            *keysyms_arr = NULL;
            return false;
        } 

        if (count % 10 == 0)
        {
            keysyms = realloc(keysyms, sizeof(xkb_keysym_t) * (count + 10));
            if (!keysyms) {
                LOG("%s:%d Failed to reallocate memory for keysyms\n",__func__, __LINE__);
                free(keys_copy);
                *keysyms_arr = NULL;
                return false;
            }
        }

        keysyms[count++] = keysym;
        token = strtok(NULL, ",");
    }

    free(keys_copy);
    xkb_context_unref(ctx);

    *num_keys = count;
    *keysyms_arr = keysyms;
    return true;
}

static uint32_t hellwm_xkb_keysym_to_wlr_modifier(xkb_keysym_t sym)
{
    switch (sym)
    {
        case XKB_KEY_Shift_L:
        case XKB_KEY_Shift_R:
            return WLR_MODIFIER_SHIFT;
        case XKB_KEY_Control_L:
        case XKB_KEY_Control_R:
            return WLR_MODIFIER_CTRL;
        case XKB_KEY_Alt_L:
        case XKB_KEY_Alt_R:
            return WLR_MODIFIER_ALT;
        case XKB_KEY_Super_L:
        case XKB_KEY_Super_R:
            return WLR_MODIFIER_LOGO;
        default:
            return 0;
    }
}

void hellwm_config_keybind_add_allocate(hellwm_config_manager_keybindings *config_keybindings)
{
    hellwm_config_keybind **temp_keybindings = realloc(config_keybindings->keybindings, (config_keybindings->count + 1) * sizeof(hellwm_config_keybind *));
    if (temp_keybindings == NULL)
    {
        LOG("%s:%d Failed to allocate memory for keybindings\n",__func__, __LINE__);
        return;
    }

    config_keybindings->keybindings = temp_keybindings;

    config_keybindings->keybindings[config_keybindings->count] = malloc(sizeof(hellwm_config_keybind));
    if (config_keybindings->keybindings[config_keybindings->count] == NULL)
    {
        LOG("%s:%d Failed to allocate memory for new keybind\n",__func__, __LINE__);
        return;
    }
}

void hellwm_config_keybind_add_function_or_cmd_to_config(struct hellwm_server *server, char *keys, void *content)
{
    hellwm_config_manager_keybindings *config_keybindings = server->config_manager->keybindings;
    hellwm_config_keybind_add_allocate(server->config_manager->keybindings); 

    if (hellwm_convert_string_to_xkb_keys(&config_keybindings->keybindings[config_keybindings->count]->keysyms, keys, &config_keybindings->keybindings[config_keybindings->count]->count))
    {
        config_keybindings->keybindings[config_keybindings->count]->content = malloc(sizeof(union hellwm_function));

        if (hellwm_function_find(content, server, config_keybindings->keybindings[config_keybindings->count]->content))
        {
            config_keybindings->keybindings[config_keybindings->count]->type = HELLWM_KEYBIND_FUNCTION;
        }
        else
        {
            config_keybindings->keybindings[config_keybindings->count]->content->as_void = content;
            config_keybindings->keybindings[config_keybindings->count]->type = HELLWM_KEYBIND_COMMAND;
        }
    }
    LOG("Keybind: [%s] = %s\n", keys, content);
    config_keybindings->count++; 
}

void hellwm_config_keybind_add_workspace_to_config(struct hellwm_server *server, char *keys, struct hellwm_config_keybind_workspace workspace)
{
    hellwm_config_manager_keybindings *config_keybindings = server->config_manager->keybindings;
    hellwm_config_keybind_add_allocate(server->config_manager->keybindings); 

    if (hellwm_convert_string_to_xkb_keys(&config_keybindings->keybindings[config_keybindings->count]->keysyms, keys, &config_keybindings->keybindings[config_keybindings->count]->count))
    {
        config_keybindings->keybindings[config_keybindings->count]->content = malloc(sizeof(union hellwm_function));
        config_keybindings->keybindings[config_keybindings->count]->type = HELLWM_KEYBIND_WORKSPACE;

        config_keybindings->keybindings[config_keybindings->count]->content->workspace.pressed = false;
        config_keybindings->keybindings[config_keybindings->count]->content->workspace.workspace_id = workspace.workspace_id;
        config_keybindings->keybindings[config_keybindings->count]->content->workspace.move_with_active = workspace.move_with_active;
        config_keybindings->keybindings[config_keybindings->count]->content->workspace.binary_workspace_val = workspace.binary_workspace_val;
        config_keybindings->keybindings[config_keybindings->count]->content->workspace.binary_workspaces_enabled = workspace.binary_workspaces_enabled;

        hellwm_binary_workspaces_manager_add_keybind(server->config_manager->binary_workspaces_manager, config_keybindings->keybindings[config_keybindings->count]);
    }
    LOG("Keybind: [%s] = workspace: %d \t - bw: %d bw_val: %d mv with active: %d\n", keys, 
            workspace.workspace_id,
            workspace.binary_workspaces_enabled,
            workspace.binary_workspace_val,
            workspace.move_with_active);

    config_keybindings->count++; 
}

void hellwm_config_print(struct hellwm_config_manager *config)
{
    if (!config)
        return;

    LOG("\n\nstruct hellwm_config_manager* config\n{\n");

    // Printing monitor_manager details
    if (config->monitor_manager)
    {
        hellwm_config_manager_monitor *mon = config->monitor_manager;

        LOG("\tstruct hellwm_config_manager_monitor* monitor_manager\n\t{\n");
        LOG("\t\tcount = %d\n\t\thellwm_config_monitor **monitors[%d] = \n\t\t{\n", mon->count, mon->count);

        for (size_t i = 0; i < mon->count; i++)
        {
            LOG("\t\t\t[%d] = \n\t\t\t{\n", i);
            LOG("\t\t\t\tname      = %s\n", mon->monitors[i]->name);
            LOG("\t\t\t\tenabled   = %d\n", mon->monitors[i]->enabled);
            LOG("\t\t\t\twidth     = %d\n", mon->monitors[i]->width);
            LOG("\t\t\t\theight    = %d\n", mon->monitors[i]->height);
            LOG("\t\t\t\thz        = %d\n", mon->monitors[i]->hz);
            LOG("\t\t\t\tvrr       = %d\n", mon->monitors[i]->vrr);
            LOG("\t\t\t\tscale     = %f\n", mon->monitors[i]->scale);
            LOG("\t\t\t\ttransfrom = %d\n", mon->monitors[i]->transfrom);
            LOG("\t\t\t}\n");
        }
        LOG("\t\t}\n\t}\n");
    }

    // Printing keyboard_manager details
    if (config->keyboard_manager)
    {
        hellwm_config_manager_keyboard *kbd = config->keyboard_manager;

        LOG("\tstruct hellwm_config_manager_keyboard* keyboard_manager\n\t{\n");
        LOG("\t\tcount = %d\n\t\thellwm_config_keyboard **keyboards[%d] = \n\t\t{\n", kbd->count, kbd->count);

        for (size_t i = 0; i < kbd->count; i++)
        {
            LOG("\t\t\t[%d] = \n\t\t\t{\n", i);
            LOG("\t\t\t\tname   = %s\n", kbd->keyboards[i]->name);
            LOG("\t\t\t\tlayout = %s\n", kbd->keyboards[i]->layout);
            LOG("\t\t\t\tdelay  = %d\n", kbd->keyboards[i]->delay);
            LOG("\t\t\t\trate   = %d\n", kbd->keyboards[i]->rate);
            LOG("\t\t\t\toption = %s\n", kbd->keyboards[i]->options);
            LOG("\t\t\t\trules  = %s\n", kbd->keyboards[i]->rules);
            LOG("\t\t\t\tvariant = %s\n", kbd->keyboards[i]->variant);
            LOG("\t\t\t\tmodel  = %s\n", kbd->keyboards[i]->model);
            LOG("\t\t\t}\n");
        }
        LOG("\t\t}\n\t}\n");
    }

    // Printing keybindings details
    if (config->keybindings)
    {
        hellwm_config_manager_keybindings *kbd = config->keybindings;

        LOG("\tstruct hellwm_config_manager_keybindings* keybindings_manager\n\t{\n");
        LOG("\t\tcount = %d\n\t\thellwm_config_keybindings **keybindings[%d] = \n\t\t{\n", kbd->count, kbd->count);

        for (size_t i = 0; i < kbd->count; i++)
        {
            LOG("\t\t\t[%d] = \n\t\t\t{\n", i);
            LOG("\t\t\t\tcount        = %d\n", kbd->keybindings[i]->count);
            LOG("\t\t\t\ttype         = %d\n", kbd->keybindings[i]->type);

            switch (kbd->keybindings[i]->type)
            {
                case HELLWM_KEYBIND_FUNCTION:
                    LOG("\t\t\t\tfunction  = %p\n", kbd->keybindings[i]->content->as_func);
                    break;

                case HELLWM_KEYBIND_WORKSPACE:
                    LOG("\t\t\t\tworkspace    = \n\t\t\t\t{\n");
                    LOG("\t\t\t\t\tworkspace_id              = %d\n", kbd->keybindings[i]->content->workspace.workspace_id);
                    LOG("\t\t\t\t\tmove_with_active          = %d\n", kbd->keybindings[i]->content->workspace.move_with_active);
                    LOG("\t\t\t\t\tbinary_workspace_val      = %d\n", kbd->keybindings[i]->content->workspace.binary_workspace_val);
                    LOG("\t\t\t\t\tbinary_workspaces_enabled = %d\n", kbd->keybindings[i]->content->workspace.binary_workspaces_enabled);
                    LOG("\t\t\t\t}\n");
                    break;

                case HELLWM_KEYBIND_COMMAND:
                    LOG("\t\t\t\tcontent      = %s\n", (char *)kbd->keybindings[i]->content->as_void);
                    break;

                default:
                    LOG("%s:%s Wrong keybind type: %d", __func__, __LINE__, kbd->keybindings[i]->type);
            }
            LOG("\t\t\t\tkeysyms[%d] = \n\t\t\t\t{\n", kbd->keybindings[i]->count);

            for (size_t j = 0; j < kbd->keybindings[i]->count; j++)
            {
                char keysym[16];
                if (xkb_keysym_get_name(kbd->keybindings[i]->keysyms[j], keysym, 16) != -1)
                    LOG("\t\t\t\t\tkey[%d] = %s\n", j, keysym);
                else
                    LOG("\t\t\t\t\tkey[%d] = %s\n", j, NULL);
            }
            LOG("\t\t\t\t}\n\t\t\t}\n");
        }
        LOG("\t\t}\n\t}\n");
    }

    // Printing decoration details
    if (config->decoration)
    {
        hellwm_config_manager_decoration *dec = config->decoration;

        LOG("\tstruct hellwm_config_manager_decoration* decoration\n\t{\n");
        LOG("\t\tinner_gap             = %d\n", dec->inner_gap);
        LOG("\t\touter_gap             = %d\n", dec->outer_gap);
        LOG("\t\tborder_width     = %d\n", dec->border_width);
        LOG("\t\tactive_border_color = {%f, %f, %f, %f}\n", 
            dec->active_border_color[0], dec->active_border_color[1], 
            dec->active_border_color[2], dec->active_border_color[3]);
        LOG("\t\tinactive_border_color = {%f, %f, %f, %f}\n", 
            dec->inactive_border_color[0], dec->inactive_border_color[1], 
            dec->inactive_border_color[2], dec->inactive_border_color[3]);
        LOG("\t}\n");
    }

    LOG("}\n");
}

void hellwm_function_add_to_map(struct hellwm_server *server, const char* name, void (*func)(struct hellwm_server*))
{
    if (!server->mapped_functions) server->mapped_functions = 0;

    server->hellwm_function_map = realloc(server->hellwm_function_map, (server->mapped_functions + 1) * sizeof(struct hellwm_function_map_entry));
    if (!server->hellwm_function_map)
    {
        LOG("%s:%d Failed to allocate memory for function map\n",__func__, __LINE__);
    }

    server->hellwm_function_map[server->mapped_functions].name = name;
    server->hellwm_function_map[server->mapped_functions].func.as_func= func;
    server->mapped_functions++;
}

bool hellwm_function_find(const char* name, struct hellwm_server* server, union hellwm_function *func)
{
    for (size_t i = 0; i < server->mapped_functions; i++)
    {
        if (strcmp(name, server->hellwm_function_map[i].name) == 0)
        {
            if (func)
                if (func->as_func)
                    func->as_func = server->hellwm_function_map[i].func.as_func;
            return true;
        }
    }
    return false;
}

void hellwm_function_expose(struct hellwm_server *server)
{
    hellwm_function_add_to_map(server, "kill_server",   hellwm_server_kill);

    hellwm_function_add_to_map(server, "focus_next",    hellwm_focus_next_in_list);

    hellwm_function_add_to_map(server, "focus_left",    server_focus_left);
    hellwm_function_add_to_map(server, "focus_right",   server_focus_right);
    hellwm_function_add_to_map(server, "focus_up",      server_focus_up);
    hellwm_function_add_to_map(server, "focus_down",    server_focus_down);

    hellwm_function_add_to_map(server, "kill_active",   hellwm_toplevel_kill_active);
    hellwm_function_add_to_map(server, "reload_config", hellwm_config_manager_reload);

    hellwm_function_add_to_map(server, "set_fullscreen", hellwm_toplevel_fullscreen);
}

struct hellwm_workspace *hellwm_workspace_create_begin(struct hellwm_server *server, int workspace_id)
{
    struct hellwm_workspace *workspace = calloc(1, sizeof(struct hellwm_workspace));
    if (!workspace)
    {
        LOG("%s:%s Cannot allocate memory for new workspace\n", __func__, __LINE__);
        return NULL;
    }

    wl_list_insert(&server->workspaces, &workspace->link);
    wl_list_init(&workspace->toplevels);
    workspace->now_focused = NULL;
    workspace->prev_focused = NULL;
    workspace->id = workspace_id;
    workspace->layout = 1;

    workspace->fullscreened = false;
    workspace->server = server;

    return workspace;
}

static void hellwm_workspace_create(struct hellwm_server *server, int workspace_id)
{
    struct hellwm_workspace *workspace = hellwm_workspace_create_begin(server, workspace_id);
    if (workspace == NULL)
        return;

    workspace->output = server->active_workspace->output;;

    LOG("Creating new workspace: %d\n", workspace_id);
    hellwm_workspace_change(server, workspace_id);
}

static void hellwm_workspace_create_for_output(struct hellwm_server *server, int workspace_id, struct hellwm_output *output)
{
    struct hellwm_workspace *workspace = hellwm_workspace_create_begin(server, workspace_id);
    if (workspace == NULL)
        return;

    workspace->output = output;

    /* This is importand, without this line HellWM will not boot. */
    if (server->active_workspace == NULL)
        server->active_workspace = workspace;

    LOG("Creating new workspace: %d\n", workspace_id);
    hellwm_workspace_change(server, workspace_id);
}

static void hellwm_workspace_destroy(struct hellwm_workspace *workspace)
{
    wl_list_remove(&workspace->link);
    free(workspace);
}

static void hellwm_workspace_add_toplevel(struct hellwm_workspace *workspace, struct hellwm_toplevel *toplevel)
{
    wl_list_insert(&workspace->toplevels, &toplevel->link);
    toplevel->workspace = workspace;

    LOG("%s added to workspace: %d\n", toplevel->xdg_toplevel->title, workspace->id);
}

/* Note: this function also automatically changes workspace to given id */
void hellwm_toplevel_move_to_workspace(struct hellwm_server *server, int workspace_id)
{
    if (server->active_workspace->now_focused == NULL) return;
    struct hellwm_toplevel *toplevel = server->active_workspace->now_focused;

    if (toplevel)
    {
        LOG("Moving toplevel from %d, to %d\n", server->active_workspace->id, workspace_id);
        wl_list_remove(&toplevel->link);

        hellwm_workspace_change(server, workspace_id);
        wl_list_insert(&server->active_workspace->toplevels, &toplevel->link);
        add_toplevel_to_tree(server->active_workspace, toplevel);
        hellwm_focus_toplevel(toplevel);
        server->layout_reapply = 1;
    }
    else
        hellwm_workspace_change(server, workspace_id);
}

/* Change workspace by it's id */
static void hellwm_workspace_change(struct hellwm_server *server, int workspace_id)
{
    struct hellwm_workspace *workspace = hellwm_workspace_find(server, workspace_id);
    struct hellwm_workspace *prev_workspace = server->active_workspace;

    if (workspace == NULL) hellwm_workspace_create(server, workspace_id);
    if (server->active_workspace->id == workspace_id) return;

    LOG("Changed Workspace from %d to %d\n", server->active_workspace->id, workspace_id);

    /* Enable toplevels */
    if (workspace)
    {
        if (wl_list_length(&workspace->toplevels) != 0)
        {
            //LOG("TOPLEVEL_LIST: %d\n", wl_list_length(&workspace->toplevels));
            struct hellwm_toplevel *toplevel;
            wl_list_for_each(toplevel, &workspace->toplevels, link)
            {
                // TODO: expose it to config
                if (prev_workspace->id > workspace_id)
                {
                    toplevel->desired_geom.x -= workspace->output->wlr_output->width;
                }
                else
                {
                    toplevel->desired_geom.x += workspace->output->wlr_output->width;
                }
                toplevel_animation_start(toplevel);

                wlr_scene_node_set_enabled(&toplevel->scene_tree->node, true);
            }
        }
    }

    /* Disable toplevels */
    if (prev_workspace)
    {
        if (wl_list_length(&prev_workspace->toplevels) == 0)
        {
            hellwm_workspace_destroy(prev_workspace);
        }
        else
        {
            //LOG("PREV_TOPLEVEL_LIST: %d\n", wl_list_length(&prev_workspace->toplevels));
            struct hellwm_toplevel *toplevel;
            wl_list_for_each(toplevel, &prev_workspace->toplevels, link)
            {
                // TODO: expose it to config
                if (prev_workspace->id > workspace_id)
                {
                    toplevel->desired_geom.x += workspace->output->wlr_output->width;
                }
                else
                {
                    toplevel->desired_geom.x -= workspace->output->wlr_output->width;
                }
                toplevel_animation_start(toplevel);
                wlr_scene_node_set_enabled(&toplevel->scene_tree->node, false);
            }
        }
    }

    server->active_workspace = workspace;
    if (server->active_workspace->now_focused != NULL)
        hellwm_focus_toplevel(server->active_workspace->now_focused);
    else
        unfocus_focused(server);

    server->layout_reapply = 1;
}

/* Return next id based on count of workspaces */
static int hellwm_workspace_get_next_id(struct hellwm_server *server)
{
    return wl_list_length(&server->workspaces) + 1;
}

/* Returns workspace with provided id, if it's not exist return NULL */
static struct hellwm_workspace *hellwm_workspace_find(struct hellwm_server *server, int id)
{
    struct hellwm_workspace *workspace;
    wl_list_for_each(workspace, &server->workspaces, link)
    {
        if (workspace->id == id)
        {
            return workspace;
        }
    }
    return NULL;
}

static void hellwm_binary_workspaces_manager_add_keybind(struct hellwm_binary_workspaces_manager *manager, hellwm_config_keybind *keybind)
{
    manager->keybinds = realloc(manager->keybinds, (manager->keybinds_count + 1) * sizeof(hellwm_config_keybind*));
    if (!manager->keybinds)
        LOG("%s:%s failed to reallocate keybinds for binary_workspaces_manager\n", __func__, __LINE__);

    manager->keybinds[manager->keybinds_count] = keybind;
    manager->keybinds_count++;
}

int main(int argc, char *argv[])
{
    printf("Hello World...\n"); /* https://www.reddit.com/r/ProgrammerHumor/comments/1euwm7v/helloworldfeaturegotmergedguys/ */

    wlr_log_init(WLR_DEBUG, NULL);
    LOG("~HELLWM LOG~\n");

    /* server */
    struct hellwm_server *server = calloc(1, sizeof(struct hellwm_server));
    GLOBAL_SERVER = server;

    /* ipc */
    server->ipc_thread = ipc_init();

    /* functions */
    hellwm_function_expose(server);

    /* config */
    server->config_manager = hellwm_config_manager_create();
    hellwm_config_manager_load_from_file("./config.lua");

    /* display */
    server->wl_display = wl_display_create();
    server->event_loop = wl_display_get_event_loop(server->wl_display);

    /* lists */
    wl_list_init(&server->outputs);
    wl_list_init(&server->keyboards);
    wl_list_init(&server->workspaces);

    /* backend */
    server->backend = wlr_backend_autocreate(server->event_loop, &server->session);

    server->backend_new_output.notify = server_backend_new_output;
    wl_signal_add(&server->backend->events.new_output, &server->backend_new_output);

    server->backend_new_input.notify = server_backend_new_input;
    wl_signal_add(&server->backend->events.new_input, &server->backend_new_input);

    if (server->backend == NULL)
    {
        wlr_log(WLR_ERROR, "failed to create wlr_backend");
        ERR("%s:%d failed to create wlr_backend\n",__func__, __LINE__);
    }


    /* renderer */
    server->renderer = wlr_renderer_autocreate(server->backend);
    wl_signal_add(&server->renderer->events.lost, &server->renderer_lost);
    server->renderer_lost.notify = handle_renderer_lost;

    if (server->renderer == NULL)
    {
        wlr_log(WLR_ERROR, "failed to create wlr_renderer");
        ERR("%s:%d failed to create wlr_renderer\n",__func__, __LINE__);
    }
    wlr_renderer_init_wl_display(server->renderer, server->wl_display);


    /* allocator */ 
    server->allocator = wlr_allocator_autocreate(server->backend, server->renderer);
    if (server->allocator == NULL)
    {
        wlr_log(WLR_ERROR, "failed to create wlr_allocator");
        ERR("%s:%d failed to create wlr_allocator\n",__func__, __LINE__);
    }


    server->compositor = wlr_compositor_create(server->wl_display, 6, server->renderer);
    server->subcompositor = wlr_subcompositor_create(server->wl_display);
    wlr_screencopy_manager_v1_create(server->wl_display);
    wlr_data_device_manager_create(server->wl_display);
    wlr_data_control_manager_v1_create(server->wl_display);
    wlr_export_dmabuf_manager_v1_create(server->wl_display);
    wlr_primary_selection_v1_device_manager_create(server->wl_display);
    wlr_viewporter_create(server->wl_display);
    wlr_single_pixel_buffer_manager_v1_create(server->wl_display);
    wlr_fractional_scale_manager_v1_create(server->wl_display, 1);
    wlr_presentation_create(server->wl_display, server->backend);
    wlr_alpha_modifier_v1_create(server->wl_display);

    /* foreign_toplevel_manager_v1 */
    server->foreign_toplevel_manager = wlr_foreign_toplevel_manager_v1_create(server->wl_display);

    /* output layout */
    server->output_layout = wlr_output_layout_create(server->wl_display);
    wlr_xdg_output_manager_v1_create(server->wl_display, server->output_layout);


    /* scene */
    server->scene = wlr_scene_create();
    server->scene_layout = wlr_scene_attach_output_layout(server->scene, server->output_layout);

    server->background_tree = wlr_scene_tree_create(&server->scene->tree); 
    server->tile_tree = wlr_scene_tree_create(&server->scene->tree); 
    server->floating_tree = wlr_scene_tree_create(&server->scene->tree); 
    server->bottom_tree = wlr_scene_tree_create(&server->scene->tree); 
    server->top_tree = wlr_scene_tree_create(&server->scene->tree); 
    server->overlay_tree = wlr_scene_tree_create(&server->scene->tree); 
    server->fullscreen_tree = wlr_scene_tree_create(&server->scene->tree); 

    /* xdg_shell */
    server->xdg_shell = wlr_xdg_shell_create(server->wl_display, 6);
    server->new_xdg_toplevel.notify = server_new_xdg_toplevel;
    wl_signal_add(&server->xdg_shell->events.new_toplevel, &server->new_xdg_toplevel);
    server->new_xdg_popup.notify = server_new_xdg_popup;
    wl_signal_add(&server->xdg_shell->events.new_popup, &server->new_xdg_popup);

    /* wlr_server_decoration */
    server->wlr_server_decoration_manager = wlr_server_decoration_manager_create(server->wl_display);
    wlr_server_decoration_manager_set_default_mode(server->wlr_server_decoration_manager, WLR_SERVER_DECORATION_MANAGER_MODE_SERVER);


    /* wlr_xdg_decoration_v1 */
    server->xdg_decoration_manager = wlr_xdg_decoration_manager_v1_create(server->wl_display);
    server->xdg_decoration_new_toplevel_decoration.notify = xdg_decoration_new_toplevel_decoration;
    wl_signal_add(&server->xdg_decoration_manager->events.new_toplevel_decoration, &server->xdg_decoration_new_toplevel_decoration);

    if (!wlr_renderer_init_wl_shm(server->renderer, server->wl_display))
    {
        ERR("Failed to initialize shared memory pool!\n");
    }
    wlr_linux_dmabuf_v1_create_with_renderer(server->wl_display, 5, server->renderer);

    /* wlr_xdg_activation_v1 */
    wlr_xdg_activation_v1_create(server->wl_display);


    /* TODO: idle_notifier */
    /* TODO: session lock */


    /* wlr_layer_shell_v1 */
    server->layer_shell = wlr_layer_shell_v1_create(server->wl_display, 4);
    server->new_layer_surface.notify = handle_new_layer_surface;
    wl_signal_add(&server->layer_shell->events.new_surface, &server->new_layer_surface);


    /* cursor */
    server->cursor = wlr_cursor_create();
    wlr_cursor_attach_output_layout(server->cursor, server->output_layout);
    server->cursor_mgr = wlr_xcursor_manager_create(NULL, 24);
    server->cursor_mode = HELLWM_CURSOR_PASSTHROUGH;

    server->cursor_motion.notify = server_cursor_motion;
    wl_signal_add(&server->cursor->events.motion, &server->cursor_motion);

    server->cursor_motion_absolute.notify = server_cursor_motion_absolute;
    wl_signal_add(&server->cursor->events.motion_absolute, &server->cursor_motion_absolute);

    server->cursor_button.notify = server_cursor_button;
    wl_signal_add(&server->cursor->events.button, &server->cursor_button);

    server->cursor_axis.notify = server_cursor_axis;
    wl_signal_add(&server->cursor->events.axis, &server->cursor_axis);

    server->cursor_frame.notify = server_cursor_frame;
    wl_signal_add(&server->cursor->events.frame, &server->cursor_frame);


    /* seat */
    server->seat = wlr_seat_create(server->wl_display, "seat0");

    server->request_cursor.notify = seat_request_cursor;
    wl_signal_add(&server->seat->events.request_set_cursor, &server->request_cursor);

    server->request_set_selection.notify = seat_request_set_selection;
    wl_signal_add(&server->seat->events.request_set_selection, &server->request_set_selection);


    /* Add a Unix socket to the Wayland display. */
    const char *socket = wl_display_add_socket_auto(server->wl_display);
    if (!socket)
    {
        wlr_backend_destroy(server->backend);
        ERR("%s:%d failed to create socket\n",__func__, __LINE__);
    }

    /* Start the backend. This will enumerate outputs and inputs, become the DRM master, etc */
    if (!wlr_backend_start(server->backend))
    {
        wlr_backend_destroy(server->backend);
        wl_display_destroy(server->wl_display);
        ERR("%s:%d failed to start backend\n",__func__, __LINE__);
    }


    /* Set the WAYLAND_DISPLAY environment variable to our socket and run the startup command if requested. */
    setenv("WAYLAND_DISPLAY", socket, true);

    /* Execute programs specified in config */
    for (int c = 0; c < server->exec_args_count; c++) {
        RUN_EXEC(server->exec_args[c]);
    }

    /* Run the Wayland event loop. This does not return until you exit the
     * compositor. Starting the backend rigged up all of the necessary event
     * loop configuration to listen to libinput events, DRM events, generate
     * frame events at the refresh rate, and so on. */
    wlr_log(WLR_INFO, "Running Wayland compositor on WAYLAND_DISPLAY=%s", socket);
    wl_display_run(server->wl_display);


    /* Once wl_display_run returns, we destroy all clients then shut down the server-> */
    hellwm_server_kill(server);

    return 0;
}
