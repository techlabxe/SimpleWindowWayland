#pragma once

#include <stdint.h>
#include <wayland-client.h>
#include <wayland-cursor.h>
#include <wayland-egl.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>

class WaylandCore {
public:
  WaylandCore( int width, int height, const char* title );
  WaylandCore();
  ~WaylandCore();
  
  void waitEvents();
  void pollEvents();
  
  bool isShouldClose();

  int  getWidth() const   { return mWidth; }
  int  getHeight() const  { return mHeight; }

  void swapBuffers();
private:
  void createWindow( int width, int height, const char* title );  
  void setup_registry_handlers();
  void initEGL();
  
  void loadImageCursor();
  void updateMouseCursor();
  int  cursorHitTest() const;

public:
  void registry_listener_global( wl_registry* reg, uint32_t name, const char* interface, uint32_t version );
  void registry_listener_global_remove( wl_registry* reg, uint32_t name );

  // 入力に関するイベントハンドラ群.
  void seat_listener_capabilities( wl_seat* seat, uint32_t caps );
  void pointer_handler_enter_leave( bool is_enter, wl_surface* surface, double cx, double cy, uint32_t serial );
  void pointer_handler_move( double cx, double cy );
  void pointer_handler_button( uint32_t button, uint32_t status, uint32_t serial );
  void pointer_handler_wheel( uint32_t axis, double val );

  void surface_handler_configure(uint32_t edges, int width, int height );
private:
  wl_display* mDisplay;
  wl_registry* mRegistry;
  wl_compositor* mCompositor;
  wl_shell*   mShell;
  wl_shm* mShm;
  wl_seat* mSeat;
  wl_pointer* mPointer;
  wl_keyboard* mKeyboard;
  
  EGLDisplay mEglDisplay;
  EGLContext mEglContext;
  EGLConfig  mEglConfig;
  
  wl_egl_window* mEglWindow;
  
  struct Surface {
    wl_surface* surface;
    EGLSurface  eglSurface;
  } mSurface;
  wl_shell_surface* mShellSurface;

  struct MouseCursor {
    int x, y;
    uint32_t serial;
  } mCursor;
  
  wl_cursor_theme*  mCursorTheme;
  int mCursorIndex;
  wl_cursor* mCursors[9];
  wl_surface* mCursorSurface;
  
  bool mShouldClose;
  int  mWidth, mHeight;
};
