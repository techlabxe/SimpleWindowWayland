#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <poll.h>
#include <linux/input.h>

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

WaylandCore::WaylandCore( int width, int height, const char* title )
: mDisplay(NULL),mRegistry(NULL),mCompositor(NULL),mShm(NULL),
  mSeat(NULL),mPointer(NULL),
  mShouldClose(false),mWidth(0),mHeight(0)
{
  mDisplay = wl_display_connect(NULL);
  setup_registry_handlers();
  
  if( mDisplay ) {
    mRegistry = wl_display_get_registry( mDisplay );
  }
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

void WaylandCore::seat_handle_capabilities(
  void* data,
  wl_seat* seat, 
  uint32_t caps )
{
  WaylandCore* core = static_cast<WaylandCore*>(data);
  if( caps & WL_SEAT_CAPABILITY_POINTER ) {
    if( core->mPointer == NULL ) {
      core->mPointer = wl_seat_get_pointer( seat );
    }
  }
  if( !(caps & WL_SEAT_CAPABILITY_POINTER) ) {
    if( core->mPointer ) {
      wl_pointer_destroy( core->mPointer );
      core->mPointer = NULL;
    }
  }
  
  if( caps & WL_SEAT_CAPABILITY_KEYBOARD ) {
    
  }
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
}
static void shell_surface_handler_popup_done( void *data, struct wl_shell_surface *shell_surface )
{
}
static void frame_redraw( void* data, wl_callback* callback, uint32_t time )
{
  WaylandCore* core = static_cast<WaylandCore*>(data);
  if(core){
	  wl_callback_destroy( callback );  
    core->redrawWindow();
  }
}
static wl_callback_listener frame_listeners = {
  frame_redraw,
};
  
void WaylandCore::createWindow( int width, int height, const char* title )
{
  if( mDisplay == NULL || mCompositor == NULL ) {
    return;
  }
  int stride = width * sizeof(uint32_t);
  int size = stride * height;
  int fd = os_create_mem( size );
  if( fd < 0 ) {
    return;
  }
  void* image_ptr = mmap( NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0 );
  wl_surface* surface = wl_compositor_create_surface( mCompositor );
  wl_shm_pool* pool = wl_shm_create_pool( mShm, fd, size );
  memset( image_ptr, 0x00, size );
  mWidth = width;
  mHeight = height;
  
  wl_shell_surface* shell_surface = wl_shell_get_shell_surface( mShell, surface );
  wl_shell_surface_set_toplevel( shell_surface );
  wl_buffer* wb = wl_shm_pool_create_buffer( pool, 0, width, height, stride, WL_SHM_FORMAT_XRGB8888 );
  wl_shm_pool_destroy( pool ); pool = NULL;
  wl_shell_surface_set_title( shell_surface, title );

  wl_callback* callback = wl_surface_frame( surface );
  static wl_shell_surface_listener shell_surf_listeners = {
    shell_surface_handler_ping,
    shell_surface_handler_configure,
    shell_surface_handler_popup_done,
  };

  
  wl_shell_surface_add_listener( shell_surface, &shell_surf_listeners, this );
  wl_callback_add_listener( callback, &frame_listeners, this );
  wl_surface_attach( surface, wb, 0, 0 );
  
  mShellSurface = shell_surface;
  mSurface.surface = surface;
  mSurface.buffer = wb;
  mSurface.memory  = image_ptr;

  wl_surface_damage( surface, 0, 0, mWidth, mHeight );  
  wl_surface_commit( surface );
  
  wl_region* region;
  region = wl_compositor_create_region( mCompositor );
  wl_region_subtract( region, 0, 0, 640, 480 );
  wl_surface_set_opaque_region( surface, region );
  wl_region_destroy( region );
  
  //region = wl_compositor_create_region( mCompositor );
  //wl_region_add( region, 0, 0, 320, 240 );
  //wl_surface_set_opaque_region( surface, region );
  //wl_surface_set_input_region( surface, region );
  //wl_region_destroy( region );
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

uint32_t calcColor()
{
  static int hue = 0;
  hue += 10;
  if( hue >= 360 ) { hue = 0; }
  
  float max = 1.0f;
  float min = 0.0f;
  float s = 1.0f;
  float v = 1.0f;
  
  int dh = hue / 60;
  float p = v * (1.0f - s);
  float q = v * (1.0f - s * (hue / 60.0f - dh) );
  float t = v * (1.0f - s * (1.0f - (hue / 60.0f - dh ) ) );
  
  int r,g,b;
  switch( dh ) {
  case 0: r = v*255; g = t*255; b = p*255; break;
  case 1: r = q*255; g = v*255; b = p*255; break;
  case 2: r = p*255; g = v*255; b = t*255; break;
  case 3: r = p*255; g = q*255; b = v*255; break;
  case 4: r = t*255; g = p*255; b = v*255; break;
  case 5: r = v*255; g = p*255; b = q*255; break;
  }
  return (r << 16) | (g << 8) | b;
}

void WaylandCore::redrawWindow()
{
  int width =  mWidth;
  int height = mHeight;
  wl_surface* surface = mSurface.surface;
  static int HEIGHT = height;
  HEIGHT -= 5;
  if( HEIGHT < 0 ) { HEIGHT = height; }
  wl_surface_damage( surface, 0, 0, width, HEIGHT );

  
  uint32_t val = calcColor();
  val |= 0xFF000000;
  for(int y=0;y<height;++y) {
    uint8_t* p = static_cast<uint8_t*>( mSurface.memory ) + width * y * sizeof(uint32_t);
    for(int x=0;x<width;++x) {
      reinterpret_cast<uint32_t*>(p)[x] = val;
    }
  }
  wl_callback* callback = wl_surface_frame( surface );
  wl_surface_attach( surface, mSurface.buffer, 0, 0 );
  wl_callback_add_listener( callback, &frame_listeners, this );
  wl_surface_commit( surface ); 
}
