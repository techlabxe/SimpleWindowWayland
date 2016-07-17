#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <poll.h>
#include <linux/input.h>

#include "WaylandCore.h"

static 
void pointer_handler_enter(
  void* data, 
  wl_pointer* pointer,
  uint32_t serial, 
  wl_surface* surface, 
  wl_fixed_t sx, wl_fixed_t sy )
{
  WaylandCore* core = static_cast<WaylandCore*>(data);
  if( core ) {
    double cx = wl_fixed_to_double(sx);
    double cy = wl_fixed_to_double(sy);
    core->pointer_handler_enter_leave( true, surface, cx, cy, serial );
  }
}  
static 
void pointer_handler_leave(
  void* data,
  wl_pointer* pointer,
  uint32_t serial,
  wl_surface* surface
)
{
  WaylandCore* core = static_cast<WaylandCore*>(data);
  if( core ) {
    core->pointer_handler_enter_leave( false, surface, 0, 0, serial );
  }  
}
static
void pointer_handler_motion(
  void* data,
  wl_pointer* pointer,
  uint32_t time,
  wl_fixed_t sx, wl_fixed_t sy )
{
  WaylandCore* core = static_cast<WaylandCore*>(data);
  if( core ) {
    double cx = wl_fixed_to_double(sx);
    double cy = wl_fixed_to_double(sy);
    core->pointer_handler_move( cx, cy );
  }
}
static
void pointer_handler_button(
  void* data,
  wl_pointer* pointer,
  uint32_t serial,
  uint32_t time,
  uint32_t button,
  uint32_t status )
{
  WaylandCore* core = static_cast<WaylandCore*>(data);
  if( core ) {
    core->pointer_handler_button( button, status, serial );
  }
}

static
void pointer_handler_axis(
  void* data,
  wl_pointer* pointer,
  uint32_t time,
  uint32_t axis,
  wl_fixed_t value )
{
  WaylandCore* core = static_cast<WaylandCore*>(data);
  if( core ) {
    double val = wl_fixed_to_double(value);
    core->pointer_handler_wheel( axis, val );
  } 
}

static wl_pointer_listener pointer_listeners = {
  pointer_handler_enter, pointer_handler_leave,
  pointer_handler_motion,pointer_handler_button, pointer_handler_axis,
};

static 
void keyboard_handler_keymap( 
  void* data, 
  wl_keyboard* keyboard, 
  uint32_t format, 
  int fd, 
  uint32_t size )
{
  
}

static
void keyboard_handler_enter( 
  void* data,
  wl_keyboard* keyboard,
  uint32_t serial, 
  wl_surface* surface,
  wl_array* key )
{
  printf( "Keyboard focused.\n" );
}

static
void keyboard_handler_leave(
  void* data,
  wl_keyboard* keyboard,
  uint32_t serial,
  wl_surface* surface )
{
  printf( "Keyboard lost focus.\n");
}

static 
void keyboard_handler_key(
  void* data,
  wl_keyboard* keyboard,
  uint32_t serial,
  uint32_t time,
  uint32_t key,
  uint32_t state )
{
  printf( "Key %d state : %d\n", key, state );
}

static
void  keyboard_handler_modifiers(
  void* data,
  wl_keyboard* keyboard,
  uint32_t serial,
  uint32_t mods_depressed,
  uint32_t mods_latched,
  uint32_t mods_locked,
  uint32_t group )
{
  printf( "Modifiers depressed %d, latched %d, locked %d, group = %d\n",
    mods_depressed, mods_latched, mods_locked, group
  );
}

static wl_keyboard_listener keyboard_listeners = {
  keyboard_handler_keymap,
  keyboard_handler_enter, keyboard_handler_leave,
  keyboard_handler_key, keyboard_handler_modifiers
};
  
  
void WaylandCore::seat_listener_capabilities( wl_seat* seat, uint32_t caps )
{
  if( caps & WL_SEAT_CAPABILITY_POINTER ) {
    wl_pointer* pointer = wl_seat_get_pointer( seat );
	  wl_pointer_add_listener( pointer, &pointer_listeners, this );
    mPointer = pointer;
  }
  if( !(caps & WL_SEAT_CAPABILITY_POINTER) ) {
    wl_pointer_destroy( mPointer );
    mPointer = NULL;
  }
  if( caps & WL_SEAT_CAPABILITY_KEYBOARD ) {
    wl_keyboard* kbd = wl_seat_get_keyboard( seat );
    wl_keyboard_add_listener( kbd, &keyboard_listeners, this );
    mKeyboard = kbd;
  }
  if( !(caps & WL_SEAT_CAPABILITY_KEYBOARD) ) {
    wl_keyboard_destroy( mKeyboard );
    mKeyboard = NULL;
  }
}


void WaylandCore::pointer_handler_enter_leave(
  bool is_enter, 
  wl_surface* surface, 
  double cx, double cy,
  uint32_t serial  )
{
  mCursor.x = cx;
  mCursor.y = cy;
  mCursor.serial = serial;
  
  if( is_enter ) {
    updateMouseCursor();
  }
}
void WaylandCore::pointer_handler_move( double cx, double cy )
{
  mCursor.x = cx; mCursor.y = cy;
  printf( "Mouse (%d, %d)\n", (int)cx, (int)cy );
  updateMouseCursor();
}
void WaylandCore::pointer_handler_button( uint32_t button, uint32_t status, uint32_t serial )
{
  if( button == BTN_LEFT ) {
    if( status == WL_POINTER_BUTTON_STATE_PRESSED ) {
      printf( "Pointer LeftButton Pressed.\n");

      int resize_type = cursorHitTest();
      if( resize_type != WL_SHELL_SURFACE_RESIZE_NONE ) {
        wl_shell_surface_resize( mShellSurface, mSeat, serial, resize_type );
      } else {
        wl_shell_surface_move( mShellSurface, mSeat, serial );
      }
    }
    if( status == 0 ) {
      printf( "Pointer LeftButton Released.\n");
    }
  }
  if( button == BTN_RIGHT ) {
    if( status == WL_POINTER_BUTTON_STATE_PRESSED ) {
      printf( "Pointer RightButton Pressed.\n");
    }
    if( status == 0 ) {
      printf( "Pointer RightButton Released.\n");
    }
  }
}
void WaylandCore::pointer_handler_wheel( uint32_t axis, double val )
{
  if( val > 0 ) {
    printf( "Wheel down.\n" );
  } else if( val < 0 ) {
    printf( "Wheel up.\n" );
  }
}
