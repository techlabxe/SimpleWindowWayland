#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <poll.h>


#include "WaylandCore.h"


static void handler_global ( 
              void* data,
              wl_registry* reg, 
              uint32_t name, 
              const char* interface, 
              uint32_t version )
{
  WaylandCore* core = static_cast<WaylandCore*>(data);
  if( core ) {
    core->registry_listener_global( reg, name, interface, version );
  }
}
static void handler_global_remove(
              void* data,
              wl_registry* reg,
              uint32_t name )
{
  WaylandCore* core = static_cast<WaylandCore*>(data);
  if( core ) {
    core->registry_listener_global_remove( reg, name );
  }
}
static void seat_handler_capabilities(
  void* data,
  wl_seat* seat,
  uint32_t caps )
{
  WaylandCore* core = static_cast<WaylandCore*>(data);
  if( core ){
    core->seat_listener_capabilities( seat, caps );
  }
}
  
static wl_registry_listener global_handlers = {
  handler_global, handler_global_remove
};
static wl_seat_listener seat_listeners = {
  seat_handler_capabilities,
};

void WaylandCore::setup_registry_handlers()
{
  if( mDisplay == NULL ) {
    return;
  }
  
  mRegistry = wl_display_get_registry( mDisplay );
  if( mRegistry ) {
    wl_registry_add_listener( mRegistry, &global_handlers, this );
    wl_display_dispatch( mDisplay );
    wl_display_roundtrip( mDisplay );
  }
}

void WaylandCore::registry_listener_global(
  wl_registry* reg, 
  uint32_t name, 
  const char* interface, 
  uint32_t version )
{
  void* obj = 0;

  if( strcmp( interface, "wl_compositor" ) == 0 ) {
    obj = wl_registry_bind( reg, name, &wl_compositor_interface, 1 );
    mCompositor = static_cast<wl_compositor*>(obj);
    obj = NULL;
  }
  if( strcmp( interface, "wl_shm") == 0 ) {
    obj = wl_registry_bind( reg, name, &wl_shm_interface, 1 );
    mShm = static_cast<wl_shm*>(obj);
    obj = 0;
    loadImageCursor();
  }
  if( strcmp( interface, "wl_shell") == 0 ) {
    obj = wl_registry_bind( reg, name, &wl_shell_interface, 1 );
    mShell = static_cast<wl_shell*>(obj);
    obj = 0;
  }
  if( strcmp( interface, "wl_seat") == 0 ) {
    obj = wl_registry_bind( reg, name, &wl_seat_interface, 1 );
    mSeat = static_cast<wl_seat*>(obj);
    wl_seat_add_listener( mSeat, &seat_listeners, this );
    obj = 0;
  }
}

void WaylandCore::loadImageCursor() {
  mCursorTheme = wl_cursor_theme_load( "default", 32, mShm );
  const char* name[] = {
    "left_ptr", "top_side", "bottom_side",
    "left_side", "top_left_corner", "bottom_left_corner",
    "right_side","top_right_corner","bottom_right_corner"
  };
  int count = sizeof(name)/sizeof(name[0]);
  
  for(int i=0;i<count;++i) {
    mCursors[i] = wl_cursor_theme_get_cursor( mCursorTheme, name[i] );
  }
  mCursorSurface = wl_compositor_create_surface( mCompositor );
  mCursorIndex = 0;
}
void WaylandCore::updateMouseCursor()
{
  int resize_type = cursorHitTest();
  
  switch(resize_type) {
  default:
  case WL_SHELL_SURFACE_RESIZE_NONE:
    mCursorIndex = 0;
    break;
  case WL_SHELL_SURFACE_RESIZE_TOP:
    mCursorIndex = 1; break;
  case WL_SHELL_SURFACE_RESIZE_BOTTOM:
    mCursorIndex = 2; break;
  case WL_SHELL_SURFACE_RESIZE_LEFT:
    mCursorIndex = 3; break;
  case WL_SHELL_SURFACE_RESIZE_TOP_LEFT:
    mCursorIndex = 4; break;
  case WL_SHELL_SURFACE_RESIZE_BOTTOM_LEFT:
    mCursorIndex = 5; break;
    
  case WL_SHELL_SURFACE_RESIZE_RIGHT:
    mCursorIndex = 6; break;
  case WL_SHELL_SURFACE_RESIZE_TOP_RIGHT:
    mCursorIndex = 7; break;
  case WL_SHELL_SURFACE_RESIZE_BOTTOM_RIGHT:
    mCursorIndex = 8; break;
  }

  wl_cursor_image* image = mCursors[mCursorIndex]->images[0];
  wl_buffer* cursor_buf = wl_cursor_image_get_buffer(image);
  wl_surface_attach(mCursorSurface, cursor_buf, 0, 0 );
  wl_surface_damage(mCursorSurface, 0, 0, image->width, image->height );
  wl_surface_commit(mCursorSurface);
  
  wl_pointer_set_cursor( mPointer, mCursor.serial, mCursorSurface, image->hotspot_x, image->hotspot_y );
}

int WaylandCore::cursorHitTest() const {
  int resize_type = 0;
  const int margin = 5;
  int top = abs( mCursor.y ) < margin ? 1 : 0;
  int bottom = abs( mCursor.y-mHeight ) < margin ? 1 : 0;
  int left = abs( mCursor.x ) < margin ? 1 : 0;
  int right = abs( mCursor.x-mWidth ) < margin ? 1 : 0;
  
  resize_type |= WL_SHELL_SURFACE_RESIZE_TOP * top;
  resize_type |= WL_SHELL_SURFACE_RESIZE_BOTTOM * bottom;
  resize_type |= WL_SHELL_SURFACE_RESIZE_LEFT * left;
  resize_type |= WL_SHELL_SURFACE_RESIZE_RIGHT * right;
  return resize_type;
}
 
void WaylandCore::registry_listener_global_remove(
  wl_registry* reg, 
  uint32_t name )
{
}


