#include "waylandwindow.h"
#include <cstddef>
#include <cstring>
#include <stdint.h>
#include <stdio.h>
#include <wayland-client-protocol.h>

namespace lvr {

struct wl_compositor *compositor = NULL;

WaylandWindow::WaylandWindow() { createDisplay(); }

void WaylandWindow::registry_handle_global(void *data,
                                           struct wl_registry *registry,
                                           uint32_t name, const char *interface,
                                           uint32_t version) {
  printf("interface: '%s', version: %d, name: %d\n", interface, version, name);

  if (strcmp(interface, wl_compositor_interface.name) == 0) {
    compositor = (wl_compositor *)wl_registry_bind(registry, name,
                                                   &wl_compositor_interface, 4);
  }
}

void WaylandWindow::createDisplay() {

  display = wl_display_connect(NULL);
  registry = wl_display_get_registry(display);
  wl_registry_add_listener(registry, &registry_listener, NULL);
  wl_display_roundtrip(display);

  surface = wl_compositor_create_surface(compositor);
}
} // namespace lvr