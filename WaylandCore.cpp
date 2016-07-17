#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <poll.h>
#include <linux/input.h>

#include "WaylandCore.h"

WaylandCore::WaylandCore( int width, int height, const char* title )
: mDisplay(NULL),mRegistry(NULL),mCompositor(NULL),mShm(NULL),
  mSeat(NULL),mPointer(NULL),
  mEglDisplay(EGL_NO_DISPLAY), mEglContext(EGL_NO_CONTEXT),mEglWindow(0),mShellSurface(NULL),
  mShouldClose(false),mWidth(0),mHeight(0)
{
  mDisplay = wl_display_connect(NULL);
  setup_registry_handlers();
  
  if( mDisplay ) {
    mRegistry = wl_display_get_registry( mDisplay );
  }
  
  initEGL();
  
  createWindow( width, height, title );
}

WaylandCore::~WaylandCore(){
  if( mSeat ) {
    wl_seat_destroy( mSeat );
    mSeat = NULL;
  }
  if( mShm ) {
    wl_shm_destroy( mShm );
    mShm = NULL;
  }
  if( mSurface.eglSurface || mEglWindow ) {
    eglMakeCurrent( mEglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT );
    eglDestroySurface( mEglContext, mSurface.eglSurface );
    wl_egl_window_destroy( mEglWindow );
    mEglWindow = NULL;
    mSurface.eglSurface = 0;
  }
  
  if( mEglContext ) {
    eglDestroyContext( mEglDisplay, mEglContext );
    mEglContext = 0;
  }
  if( mEglDisplay ) {
    eglTerminate( mEglDisplay );
    mEglDisplay = 0;
  }
  
  if( mCompositor ) {
    wl_compositor_destroy( mCompositor );
    mCompositor = NULL;
  }
  if( mRegistry ) {
    wl_registry_destroy( mRegistry );
    mRegistry = NULL;
  }
  if( mDisplay ) {
    wl_display_flush( mDisplay );
    wl_display_disconnect( mDisplay );
    mDisplay = NULL;
  }
}

void WaylandCore::initEGL() {
  EGLint major,minor;
  EGLConfig* configs;
  EGLint attrib[] = {
    EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
    EGL_RED_SIZE, 8,
    EGL_GREEN_SIZE, 8,
    EGL_BLUE_SIZE, 8,
    EGL_ALPHA_SIZE, 8,
    EGL_DEPTH_SIZE, 16,
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
    EGL_NONE,
  };
  EGLint context_attrib[] = {
    EGL_CONTEXT_CLIENT_VERSION, 2,
    EGL_NONE,
  };
  mEglDisplay = eglGetDisplay( (EGLNativeDisplayType)mDisplay );
  if( mEglDisplay == EGL_NO_DISPLAY ) {
    fprintf( stderr, "Cannot create egl display.\n" );
  }
  if( eglInitialize(mEglDisplay, &major, &minor ) != EGL_TRUE ) {
    fprintf( stderr, "Cannot initialize egl display.\n" );
  }
  printf( "EGL major: %d, minor : %d\n", major, minor );
  
  EGLint count = 0;
  int n = 0;
  eglGetConfigs( mEglDisplay, NULL, 0, &count );
  configs = (EGLConfig*) malloc( count * sizeof(EGLConfig) );
  eglChooseConfig( mEglDisplay, attrib, configs, count, &n );
  
  for( int i=0;i<n;++i ) {
    int size = 0;
    eglGetConfigAttrib( mEglDisplay, configs[i], EGL_BUFFER_SIZE, &size );
    printf( "[%d] Buffer size  is %d\n", i, size );
    if( size != 32 ) {
      continue;
    }
    eglGetConfigAttrib( mEglDisplay, configs[i], EGL_DEPTH_SIZE, &size );
    printf( "[%d] Depth size is %d\n" , i, size );
    // とりあえず RGBA8888,デプスありなので最初のものを使用する.
    mEglConfig = configs[i];
    break;
  }
  mEglContext = eglCreateContext( mEglDisplay, mEglConfig, EGL_NO_CONTEXT, context_attrib );
}

static void shell_surface_handler_ping(
  void *data, 
  struct wl_shell_surface *shell_surface,
  uint32_t serial )
{
	wl_shell_surface_pong( shell_surface, serial );
}
static void shell_surface_handler_configure(
  void *data, 
  struct wl_shell_surface *shell_surface,
  uint32_t edges, 
  int32_t width, 
  int32_t height )
{
  WaylandCore* core = static_cast<WaylandCore*>(data);
  if( core ) {
    //fprintf( stderr, "Configure(%d,%d)\n", width, height ;
    core->surface_handler_configure( edges, width, height );
  }
}
static void shell_surface_handler_popup_done( void *data, struct wl_shell_surface *shell_surface )
{
}
  
void WaylandCore::createWindow( int width, int height, const char* title )
{
  if( mDisplay == NULL || mCompositor == NULL ) {
    return;
  }
  mWidth = width;
  mHeight = height;

  static wl_shell_surface_listener shell_surf_listeners = {
    shell_surface_handler_ping,
    shell_surface_handler_configure,
    shell_surface_handler_popup_done,
  };

  wl_surface* surface = wl_compositor_create_surface( mCompositor );
  wl_shell_surface* shell_surface = wl_shell_get_shell_surface( mShell, surface );
  wl_shell_surface_set_toplevel( shell_surface );
  wl_shell_surface_set_title( shell_surface, title );

  wl_shell_surface_add_listener( shell_surface, &shell_surf_listeners, this );

  mShellSurface = shell_surface;
  mSurface.surface = surface;
  
  mEglWindow = wl_egl_window_create( surface, mWidth, mHeight );
  mSurface.eglSurface = eglCreateWindowSurface( mEglDisplay, mEglConfig, mEglWindow, NULL );
  
  if( !eglMakeCurrent( mEglDisplay, mSurface.eglSurface, mSurface.eglSurface, mEglContext ) ) {
    fprintf( stderr, "MakeCurrent failed.\n" );
  }
}
void WaylandCore::surface_handler_configure( uint32_t edges, int width, int height ) {
  if( edges != 0 ) {
    wl_egl_window_resize( mEglWindow, width, height, 0, 0);
    mWidth = width;
    mHeight = height;
  }
}


void WaylandCore::waitEvents() {
  if( mDisplay == NULL || mRegistry == NULL ) {
    mShouldClose = true;
    return;
  }
  int ret = wl_display_dispatch(mDisplay);
  if( ret == -1 ) {
    mShouldClose = true;
    return;
  }
  
  return;
}
void WaylandCore::pollEvents() {
  if( mDisplay == NULL || mRegistry == NULL ) {
    mShouldClose = true;
    return;
  }
  pollfd fds[] = {
    { wl_display_get_fd(mDisplay), POLLIN },
  };
  while( wl_display_prepare_read(mDisplay) != 0 )
  {
    wl_display_dispatch_pending(mDisplay);
  }
  wl_display_flush(mDisplay);
  if( poll(fds,1,0) > 0 )
  {
    wl_display_read_events(mDisplay);
    wl_display_dispatch_pending(mDisplay);
  } else {
    wl_display_cancel_read(mDisplay);
  }
}
bool WaylandCore::isShouldClose() {
  return mShouldClose;
}


void WaylandCore::swapBuffers()
{
  eglSwapBuffers( mEglDisplay, mSurface.eglSurface );
}
