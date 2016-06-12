#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <poll.h>

#include "WaylandCore.h"


int os_create_mem( int size ) {
  static const char fmt[] = "/weston-shared-XXXXXX";
  const char* path = NULL;
  char* name = NULL;
  int fd = -1;
  path = getenv("XDG_RUNTIME_DIR");
  if( !path ) {
    return -1;
  }
  name = (char*) malloc( strlen(path) + sizeof(fmt) );
  if( !name ) {
    return -1;
  }
  strcpy( name, path );
  strcat( name, fmt );
  
  fd = mkostemp( name, O_CLOEXEC );
  if( fd >= 0 ) {
    unlink(name);
  }
  free(name); name = NULL;
  if( ftruncate( fd, size ) < 0 ) {
    close(fd);
    return -1;
  }
  return fd;
}

WaylandCore::WaylandCore()
: mDisplay(NULL),mRegistry(NULL),mCompositor(NULL),mShm(NULL),
  mShouldClose(false)
{
  mDisplay = wl_display_connect(NULL);

  if( mDisplay ) {
    mRegistry = wl_display_get_registry( mDisplay );
  }
  if( mRegistry ) {
    static wl_registry_listener listeners= {
      handlerGlobal, handlerGlobalRemove,
    };
    wl_registry_add_listener( mRegistry, &listeners, this );
  }
  
  if( mRegistry ) {
    wl_display_dispatch( mDisplay );
    wl_display_roundtrip( mDisplay );
  }

}

WaylandCore::~WaylandCore(){
  if( mShm ) {
    wl_shm_destroy( mShm );
    mShm = NULL;
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


static void handle_ping(
  void *data, 
  struct wl_shell_surface *shell_surface,
  uint32_t serial )
{
	wl_shell_surface_pong( shell_surface, serial );
}
static void handle_configure(
  void *data, 
  struct wl_shell_surface *shell_surface,
  uint32_t edges, 
  int32_t width, 
  int32_t height )
{
}
static void handle_popup_done( void *data, struct wl_shell_surface *shell_surface )
{
}

WaylandWindow* WaylandCore::createWindow( int width, int height, const char* title ) {
  int stride = width * sizeof(uint32_t);
  int size = stride * height;
  int fd = os_create_mem( size );
  if( fd < 0 ) {
    fprintf( stderr, "failed os_create_mem\n" );
    return NULL;
  }
  
  wl_surface* surface = wl_compositor_create_surface( mCompositor );
  if( surface == NULL ) {
    fprintf(stderr, "can't crate surface\n");
    return NULL;
  }
  void* shm_data = mmap( NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0 );
  mShmPool = wl_shm_create_pool( mShm, fd, size );
  
  wl_shell_surface* shell_surface = wl_shell_get_shell_surface( mShell, surface );
  if( shell_surface == NULL ) {
    fprintf(stderr, "can't create shell surface\n");
    return NULL;
  }  
  wl_shell_surface_set_toplevel( shell_surface );

  wl_buffer* wb = wl_shm_pool_create_buffer( mShmPool, 0, width, height, stride, WL_SHM_FORMAT_XRGB8888 );
  wl_shm_pool_destroy( mShmPool ); mShmPool = 0;
  
  wl_shell_surface_set_title( shell_surface, title );
  
  WaylandWindow* pWindow = new WaylandWindow( surface, wb, shell_surface, width, height );
  return pWindow;
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
void WaylandCore::handlerGlobal( 
  void* data,
  wl_registry* reg, 
  uint32_t name, 
  const char* interface, 
  uint32_t version )
{
  WaylandCore* core = static_cast<WaylandCore*>(data);
  void* obj = 0;
  fprintf( stderr, "registry_handler %s %d\n", interface, version );
  if( strcmp( interface, "wl_compositor") == 0 ) {
    obj = wl_registry_bind( reg, name, &wl_compositor_interface, 1 );
    core->mCompositor = static_cast<wl_compositor*>(obj);
    fprintf( stderr, "Compositor : %p\n", core->mCompositor );
    obj = 0;
  }
  if( strcmp( interface, "wl_shm") == 0 ) {
    obj = wl_registry_bind( reg, name, &wl_shm_interface, 1 );
    core->mShm = static_cast<wl_shm*>(obj);
    obj = 0;
  }
  if( strcmp( interface, "wl_shell") == 0 ) {
    obj = wl_registry_bind( reg, name, &wl_shell_interface, 1 );
    core->mShell = static_cast<wl_shell*>(obj);
    obj = 0;
  }
}

void WaylandCore::handlerGlobalRemove(
  void* data,
  wl_registry* reg,
  uint32_t name )
{
   
}


WaylandWindow::WaylandWindow( 
  wl_surface* surface, 
  wl_buffer* buffer, 
  wl_shell_surface* shell_surface, 
  int width, int height  )
: mSurface(surface), mBuffer(buffer),
  mShellSurface(shell_surface),
  mWidth(width), mHeight(height)
{
  static wl_shell_surface_listener listeners = {
    handlerPing,
    handlerConfigure,
    handlerPopupDone,
  };
  wl_shell_surface_set_toplevel( shell_surface );
  wl_shell_surface_add_listener( shell_surface, &listeners, this );
  wl_surface_attach( mSurface, mBuffer, 0, 0 );
}

WaylandWindow::~WaylandWindow()
{
  if( mShellSurface ) {
    wl_shell_surface_destroy( mShellSurface );
  }
  if( mSurface ) {
    wl_surface_destroy( mSurface );
  }
  mShellSurface = 0;
  mSurface = 0;
}

void WaylandWindow::updateWindow()
{
  wl_surface_damage( mSurface, 0, 0, mWidth, mHeight );
  wl_surface_commit( mSurface );
}

void WaylandWindow::handlerPing( void* data, wl_shell_surface* shell_surface, uint32_t serial )
{
	wl_shell_surface_pong( shell_surface, serial );
}
void WaylandWindow::handlerConfigure( 
  void* data, 
  wl_shell_surface* shell_surface, 
  uint32_t edge, 
  int32_t width, 
  int32_t height )
{
 
}
void WaylandWindow::handlerPopupDone(
  void* data,
  wl_shell_surface* shell_surface )
{
 
}
