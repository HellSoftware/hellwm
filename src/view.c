#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <wlr/types/wlr_output_layout.h>

#include "../include/server.h"

void hellwm_view_init
	(
		struct hellwm_view *view, const struct hellwm_view_interface *impl,
		enum hellwm_view_type type, struct hellwm_server *server
	)
{
	assert(impl->destroy);
	view->impl = impl;
	view->type = type;
	view->server = server;
	view->alpha = 1.0f;
	view->title = NULL;
	view->app_id = NULL;
	wl_signal_init(&view->events.unmap);
	wl_signal_init(&view->events.destroy);
	wl_list_init(&view->children);
}

void view_destroy(struct hellwm_view *view)
{
	if (view == NULL)
	{
		return;
	}

	wl_signal_emit(&view->events.destroy, view);

	if (view->wlr_surface != NULL)
	{
		view_unmap(view);
	}

	if (view->fullscreen_output != NULL)
	{
		view->fullscreen_output->= NULL;
	}

	view->impl->destroy(view);
}

void view_damage_whole(struct hellwm_view *view)
{
	struct hellwm_output *output;
	wl_list_for_each(output, &view->server->outputs, link)
	{
		output_damage_whole_view(output, view);
	}
}

void view_unmap(struct hellwm_view *view)
{
	assert(view->wlr_surface != NULL);

	wl_signal_emit(&view->events.unmap, view);

	view_damage_whole(view);
	wl_list_remove(&view->link);

	wl_list_remove(&view->new_subsurface.link);

	struct hellwm_view_child *child, *tmp;
	wl_list_for_each_safe(child, tmp, &view->children, link)
	{
		view_child_destroy(child);
	}

	if (view->fullscreen_output != NULL)
	{
		output_damage_whole(view->fullscreen_output);
		view->fullscreen_output->fullscreen_view = NULL;
		view->fullscreen_output = NULL;
	}

	view->wlr_surface = NULL;
	view->box.width = view->box.height = 0;

	if (view->toplevel_handle)
	{
		wlr_foreign_toplevel_handle_v1_destroy(view->toplevel_handle);
		view->toplevel_handle = NULL;
	}
}
