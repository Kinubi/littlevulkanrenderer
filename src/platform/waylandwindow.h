#pragma once

#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>

namespace lvr {

class WaylandWindow {

public:
  WaylandWindow();
  ~WaylandWindow() {};

  static void registry_handle_global(void *data, struct wl_registry *registry,
                                     uint32_t name, const char *interface,
                                     uint32_t version);

  void createDisplay();
  wl_display *getDisplay() { return display; }
  wl_surface *getSurface() { return surface; }

private:
  wl_surface *surface = nullptr;
  wl_display *display;

  struct wl_registry_listener registry_listener = {registry_handle_global,
                                                   NULL};
  struct wl_registry *registry;
};

} // namespace lvr