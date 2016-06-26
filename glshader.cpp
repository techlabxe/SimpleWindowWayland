#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <GLES2/gl2.h>

void checkCompiled( GLuint shader ) {
  GLint status;
  glGetShaderiv( shader, GL_COMPILE_STATUS, &status );
  if( status != GL_TRUE ) {
    GLint length;
    glGetShaderiv( shader, GL_INFO_LOG_LENGTH, &length );
    if( length ) {
      char* buf = (char*)malloc( length );
      glGetShaderInfoLog( shader, length, NULL, buf );
      fprintf( stderr, "CompiledLog: %s\n", buf );
      free( buf );
    }
    exit( EXIT_FAILURE );
  }
  fprintf( stdout, "Compile Succeed.\n" );
}

GLuint createShaderProgram( const char* srcVS[], const char* srcFS[] ) {
  GLuint shaderVS = glCreateShader( GL_VERTEX_SHADER );
  GLuint shaderFS = glCreateShader( GL_FRAGMENT_SHADER );
 
  glShaderSource( shaderVS, 1, srcVS, 0 );
  int errCode = glGetError();
  if( errCode != GL_NO_ERROR ) {
    fprintf( stderr, "GLErr.  %X\n", errCode );
    exit(1);
  }

  glCompileShader( shaderVS );
  checkCompiled( shaderVS );
 
  glShaderSource( shaderFS, 1, srcFS, NULL );
  glCompileShader( shaderFS );
  checkCompiled( shaderFS );
 
  GLuint program;
  program = glCreateProgram();
  glAttachShader( program, shaderVS );
  glAttachShader( program, shaderFS );
 
  glLinkProgram( program );
 
  return program;
}
