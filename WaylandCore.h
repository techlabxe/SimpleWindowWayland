#pragma once

#include <stdint.h>
#include <wayland-client.h>

class WaylandWindow;

class WaylandCore {
public:
  WaylandCore();
  ~WaylandCore();
  
  bool msgloop();

  WaylandWindow* createWindow( int width, int height, const char* title );  
private:
  static void handlerGlobal( 
                void* data,
                wl_registry* reg, 
                uint32_t name, 
                const char* interface, 
                uint32_t version );
  static void handlerGlobalRemove(
                void* data,
                wl_registry* reg,
                uint32_t name );
                
private:
  wl_display* mDisplay;
  wl_registry* mRegistry;
  wl_compositor* mCompositor;
  wl_shell*   mShell;
  wl_shm* mShm;
  wl_shm_pool* mShmPool;
};

class WaylandWindow {
public:
  WaylandWindow( wl_surface* surf, wl_buffer* buf, wl_shell_surface* shell_surface, int width, int height );
  ~WaylandWindow();
  
  void updateWindow();
  
private:
  static void handlerPing( void* data, wl_shell_surface* shell_surface, uint32_t serial );
  static void handlerConfigure( 
                void* data, 
                wl_shell_surface* shell_surface, 
                uint32_t edge, 
                int32_t width, 
                int32_t height );
  static void handlerPopupDone(
                void* data,
                wl_shell_surface* shell_surface );
private:

  wl_surface* mSurface;
  wl_buffer*  mBuffer;
  wl_shell_surface* mShellSurface;
  int mWidth, mHeight;
};