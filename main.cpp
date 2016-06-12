#include <iostream>
#include "WaylandCore.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WIDTH 640
#define HEIGHT 480
#define TITLE  "SimpleWindow"

int main() {
  WaylandCore* core = new WaylandCore();
  
  WaylandWindow* window = core->createWindow( WIDTH, HEIGHT, TITLE );
  if( window ) {
    window->updateWindow();
  }
  while( core->msgloop() ) {
    
  }

  delete window; window = NULL;
  delete core;core = NULL;
  return 0;
}
