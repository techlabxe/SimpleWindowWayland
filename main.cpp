#include <iostream>
#include "WaylandCore.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <GLES2/gl2.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "glm/gtc/matrix_transform.hpp"

#include "model.h"

#define WIDTH 320
#define HEIGHT 240
#define TITLE  "SimpleWindow"


const GLchar* srcVertexShader[] =
{
  "attribute vec4 position0;\n"
  "attribute vec3 normal0;\n"
  "varying vec4 vsout_color0;\n"
  "uniform vec4 matPVW[4];\n"
  "void main() {\n"
  "vec4 pos;\n"
  "  pos = matPVW[0] * position0.xxxx;\n"
  "  pos += matPVW[1]* position0.yyyy;\n"
  "  pos += matPVW[2]* position0.zzzz;\n"
  "  pos += matPVW[3]* position0.wwww;\n"
  "  gl_Position = pos;\n"
  "  float lmb = clamp( dot( vec3(0.0, 0.5, 0.5), normalize(normal0.xyz)), 0, 1 );\n"
  "  lmb = lmb * 0.5 + 0.5;\n"
  "  vsout_color0.rgb = vec3(lmb,lmb,lmb);\n"
  "  vsout_color0.a = 1.0;\n"
  "}"
 };
 
const GLchar* srcFragmentShader[] =
{
  "precision mediump float; \n"
  "varying vec4 vsout_color0;\n"
  "void main() {\n"
  "  gl_FragColor = vsout_color0;\n"
  "}"
};

GLuint createShaderProgram( const char* srcVS[], const char* srcFS[] );

struct DrawBatch {
  GLuint vb, ib;
  GLuint shader;
  int indexCount;
} drawObj;

GLint locPVW;

template<typename T>
GLuint createBufferObject( std::vector<T>& src, GLuint type ){
 GLuint vbo;
 glGenBuffers( 1, &vbo );
 glBindBuffer( type, vbo );
 glBufferData( type, sizeof(T) * src.size(), src.data(), GL_STATIC_DRAW );
 return vbo;
}
void CreateResource() {
	drawObj.shader = createShaderProgram( srcVertexShader, srcFragmentShader );
	GLint locPos = glGetAttribLocation( drawObj.shader, "position0" );
	GLint locNrm = glGetAttribLocation( drawObj.shader, "normal0" );

	locPVW = glGetUniformLocation( drawObj.shader, "matPVW" );

  std::vector<uint16_t> indicesTorus;
  std::vector<VertexPN> verticesTorus;
  createTorus( indicesTorus, verticesTorus );
  drawObj.vb = createBufferObject( verticesTorus, GL_ARRAY_BUFFER );
  drawObj.ib = createBufferObject( indicesTorus, GL_ELEMENT_ARRAY_BUFFER );
  drawObj.indexCount = indicesTorus.size();
  
	char* offset = NULL;
  int stride = sizeof(VertexPN);
	glVertexAttribPointer( locPos, 3, GL_FLOAT, GL_FALSE, stride, offset );
	offset += sizeof(VertexPosition);
	glVertexAttribPointer( locNrm, 3, GL_FLOAT, GL_FALSE, stride, offset );
	offset += sizeof(VertexNormal);
	
	glEnableVertexAttribArray( locPos );
	glEnableVertexAttribArray( locNrm );
}
void DestroyResource() {
	glDeleteBuffers( 1, &drawObj.vb );
	glDeleteBuffers( 1, &drawObj.ib );
	glDeleteProgram( drawObj.shader );
}


void drawCube()
{
  glEnable( GL_DEPTH_TEST );
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  
  static double angle = 0.0f;
  angle += .01f;
  if( angle > 3600.0 ) {
    angle -= 3600.0;
  }
  
  glm::vec3 cameraPos = glm::vec3( 0.0, 0.0f, 10.0f );
	glm::mat4 proj = glm::perspective<float>( 30.0f, float(WIDTH)/float(HEIGHT), 1.0f, 100.0f );
	glm::mat4 view = glm::lookAt<float>( cameraPos, glm::vec3(0.0f,0.0f,0.0f), glm::vec3(0.0f,1.0f,0.0f) );
	glm::mat4 world= glm::rotate( glm::mat4(1.0f), (float)angle, glm::vec3(0.0f,0.0f,1.0f) );
	world= glm::rotate( world, (float) angle * 0.5f, glm::vec3( 0.0f, 0.0f, 1.0f ) );
  world= glm::rotate( world, (float) angle * 0.5f, glm::vec3( 1.0f, 0.0f, 0.0f ));
  glUseProgram( drawObj.shader );

  glm::mat4 pvw = proj * view * world;
  glUniform4fv( locPVW, 4, glm::value_ptr(pvw) );

  glBindBuffer( GL_ARRAY_BUFFER, drawObj.vb );
  glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, drawObj.ib );

  glDrawElements( GL_TRIANGLES, drawObj.indexCount, GL_UNSIGNED_SHORT, NULL );   
}

int main() {
  WaylandCore* core = new WaylandCore(WIDTH, HEIGHT, TITLE);

  fprintf( stderr, "Renderer: %s\n", (char*)glGetString(GL_RENDERER) );
  fprintf( stderr, "GL_VERSION: %s\n", (char*)glGetString( GL_VERSION ) );
  fprintf( stderr, "GL_EXTENSIONS: %s\n", (char*)glGetString(GL_EXTENSIONS) );

  CreateResource();
  

  
  while( !core->isShouldClose() ) {
    core->pollEvents();
    usleep(16*1000);
    
    glClearColor( 0, 0, 0.25f, 0.75f );
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
   
    drawCube();
    
    core->swapBuffers();
  }

  delete core;core = NULL;
  return 0;
}
