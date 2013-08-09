#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <linux/input.h>
#include <assert.h>
#include <signal.h>
#include <math.h>
#include <sys/types.h>

#include <wayland-server.h>
#include <weston/compositor.h>

// class nuttyshell
static void activate(struct desktop_shell *shell, struct weston_surface *es,
						  struct weston_seat *seat);

static struct workspace* get_current_workspace(struct desktop_shell *shell);

enum shell_surface_type {
	SHELL_SURFACE_NONE,
	SHELL_SURFACE_FULLSCREEN,
	SHELL_SURFACE_MAXIMIZED,
	SHELL_SURFACE_XWAYLAND
};

struct nuttyshell {
  struct weston_compositor* _compositor;
  struct workspaces* _workspaces;
  
  // listeners
  struct wl_listener _some_listener;
  
  // layers
  struct weston_layer _fullscreen_layer;
//   struct weston_layer _panel_layer;
//   struct weston_layer _background_layer;
//   struct weston_layer lock_layer;
  
  struct wl_listener pointer_focus_listener;
  struct weston_surface *grab_surface;
  
  bool _locked;
};

WL_EXPORT int
module_init(struct weston_compositor* ec,
            int* argc, char* argv[]) {
  struct weston_seat* seat;
  struct desktop_shell* shell;
  struct workspace** pws;
  unsigned int i;
  struct wl_event_loop* loop;

  shell = malloc(sizeof * shell);
  if(shell == NULL)
    return -1;

  memset(shell, 0, sizeof * shell);
  shell->compositor = ec;

  shell->destroy_listener.notify = shell_destroy;
  wl_signal_add(&ec->destroy_signal, &shell->destroy_listener);
  shell->idle_listener.notify = idle_handler;
  wl_signal_add(&ec->idle_signal, &shell->idle_listener);
  shell->wake_listener.notify = wake_handler;
  wl_signal_add(&ec->wake_signal, &shell->wake_listener);
  shell->show_input_panel_listener.notify = show_input_panels;
  wl_signal_add(&ec->show_input_panel_signal, &shell->show_input_panel_listener);
  shell->hide_input_panel_listener.notify = hide_input_panels;
  wl_signal_add(&ec->hide_input_panel_signal, &shell->hide_input_panel_listener);
  shell->update_input_panel_listener.notify = update_input_panels;
  wl_signal_add(&ec->update_input_panel_signal, &shell->update_input_panel_listener);
  ec->ping_handler = ping_handler;
  ec->shell_interface.shell = shell;
  ec->shell_interface.create_shell_surface = create_shell_surface;
  ec->shell_interface.set_toplevel = set_toplevel;
  ec->shell_interface.set_transient = set_transient;
  ec->shell_interface.set_fullscreen = set_fullscreen;
  ec->shell_interface.set_xwayland = set_xwayland;
  ec->shell_interface.move = surface_move;
  ec->shell_interface.resize = surface_resize;

  wl_list_init(&shell->input_panel.surfaces);

  weston_layer_init(&shell->fullscreen_layer, &ec->cursor_layer.link);
  weston_layer_init(&shell->panel_layer, &shell->fullscreen_layer.link);
  weston_layer_init(&shell->background_layer, &shell->panel_layer.link);
  weston_layer_init(&shell->lock_layer, NULL);
  weston_layer_init(&shell->input_panel_layer, NULL);

  wl_array_init(&shell->workspaces.array);
  wl_list_init(&shell->workspaces.client_list);

  shell_configuration(shell);

  for(i = 0; i < shell->workspaces.num; i++) {
    pws = wl_array_add(&shell->workspaces.array, sizeof * pws);
    if(pws == NULL)
      return -1;

    *pws = workspace_create();
    if(*pws == NULL)
      return -1;
  }
  activate_workspace(shell, 0);

  wl_list_init(&shell->workspaces.anim_sticky_list);
  wl_list_init(&shell->workspaces.animation.link);
  shell->workspaces.animation.frame = animate_workspace_change_frame;

  if(wl_global_create(ec->wl_display, &wl_shell_interface, 1,
                      shell, bind_shell) == NULL)
    return -1;

  if(wl_global_create(ec->wl_display,
                      &desktop_shell_interface, 2,
                      shell, bind_desktop_shell) == NULL)
    return -1;

  if(wl_global_create(ec->wl_display, &screensaver_interface, 1,
                      shell, bind_screensaver) == NULL)
    return -1;

  if(wl_global_create(ec->wl_display, &wl_input_panel_interface, 1,
                      shell, bind_input_panel) == NULL)
    return -1;

  if(wl_global_create(ec->wl_display, &workspace_manager_interface, 1,
                      shell, bind_workspace_manager) == NULL)
    return -1;

  shell->child.deathstamp = weston_compositor_get_time();

  loop = wl_display_get_event_loop(ec->wl_display);
  wl_event_loop_add_idle(loop, launch_desktop_shell_process, shell);

  shell->screensaver.timer =
    wl_event_loop_add_timer(loop, screensaver_timeout, shell);

  wl_list_for_each(seat, &ec->seat_list, link)
  create_pointer_focus_listener(seat);

  shell_add_bindings(ec, shell);

  shell_fade_init(shell);

  return 0;
}
