#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <wayland-util.h>
#include <wayland-server.h>
#include <wayland-server-core.h>

#include <wlroots-0.18/wlr/backend.h>

#include <wlroots-0.18/wlr/types/wlr_drm.h>
#include <wlroots-0.18/wlr/types/wlr_scene.h>
#include <wlroots-0.18/wlr/types/wlr_buffer.h>
#include <wlroots-0.18/wlr/types/wlr_output.h>
#include <wlroots-0.18/wlr/types/wlr_compositor.h>
#include <wlroots-0.18/wlr/types/wlr_screencopy_v1.h>
#include <wlroots-0.18/wlr/types/wlr_data_control_v1.h>
#include <wlroots-0.18/wlr/types/wlr_linux_dmabuf_v1.h>
#include <wlroots-0.18/wlr/types/wlr_linux_drm_syncobj_v1.h>

#include <wlroots-0.18/wlr/render/allocator.h>
#include <wlroots-0.18/wlr/render/wlr_renderer.h>

enum { scene_layer_bg, scene_layer_bottom, scene_layer_tile, scene_layer_float, scene_layer_top, scene_layer_fs, scene_layer_overlay, scene_layer_block, scene_layers_count };

struct hellwm_server 
{
	struct wl_display *display;
   struct wl_event_loop *event_loop;

   struct wlr_scene *scene;
   struct wlr_scene_rect *scene_rect;
   struct wlr_scene_tree *drag_icon;
   struct wlr_scene_tree *layer_surfaces[scene_layers_count]; 

   struct wlr_session *session;
   struct wlr_backend *backend;
   struct wlr_renderer *renderer;
   struct wlr_allocator *allocator;
   struct wlr_compositor *compositor;

	struct wlr_screencopy_manager_v1 *screencopy_manager_v1;
   struct wlr_data_control_manager_v1 *data_control_manager_v1;

   struct wl_list outputs;

   struct wl_listener renderer_lost;
};

struct hellwm_output
{
   struct wl_list link;

   struct wl_listener frame;
   struct wl_listener destroy;

   struct wlr_output *wlr_output;
   struct wlr_scene_output *scene_output;
   struct wlr_scene_rect *output_scene_rect;
};

struct hellwm_server server;

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

int main(int argc, char *argv[])
{
	printf("Hello World...\n"); /* https://www.reddit.com/r/ProgrammerHumor/comments/1euwm7v/helloworldfeaturegotmergedguys/ */

   server.display = wl_display_create();
   server.event_loop = wl_event_loop_create();


   server.backend = wlr_backend_autocreate(server.event_loop, &server.session);
   if (!server.backend)
      ERR("wlr_backend_autocreate()");
   

   server.scene = wlr_scene_create();
   server.scene_rect = wlr_scene_rect_create(&server.scene->tree, 0, 0, (float[]){1.f,.5f,1.f,.7f});
   for (int i = 0; i < scene_layers_count; i++)
   	server.layer_surfaces[i] = wlr_scene_tree_create(&server.scene->tree);
   server.drag_icon = wlr_scene_tree_create(&server.scene->tree);
	wlr_scene_node_place_below(&server.drag_icon->node, &server.layer_surfaces[scene_layer_block]->node);


   server.renderer = wlr_renderer_autocreate(server.backend);
   if (!server.renderer)
      ERR("wlr_renderer_autocreate()");
	
   wl_signal_add(&server.renderer->events.lost, &server.renderer_lost);
   server.renderer_lost.notify = handle_renderer_lost;
   wlr_renderer_init_wl_shm(server.renderer, server.display);

   if (wlr_renderer_get_texture_formats(server.renderer, WLR_BUFFER_CAP_DMABUF))
   {
		wlr_drm_create(server.display, server.renderer);
		wlr_scene_set_linux_dmabuf_v1(server.scene, wlr_linux_dmabuf_v1_create_with_renderer(server.display, 5, server.renderer));
	}
	wlr_linux_drm_syncobj_manager_v1_create(server.display, 1,	wlr_renderer_get_drm_fd(server.renderer));


   server.allocator = wlr_allocator_autocreate(server.backend, server.renderer);
   if (!server.allocator)
      ERR("wlr_allocator_autocreate()");


   server.compositor = wlr_compositor_create(server.display, 6, server.renderer);
	server.screencopy_manager_v1 = wlr_screencopy_manager_v1_create(server.display);
	server.data_control_manager_v1 = wlr_data_control_manager_v1_create(server.display);

   /* ------ */
   sleep(1);
	printf("...Underworld (I mean Hell)\n");
   wl_display_destroy_clients(server.display);

	return 0;
}
