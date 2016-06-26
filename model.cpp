#include <stdio.h>
#include <cmath>

#include "model.h"

static const double PI = 3.14159265;

void createTorus( std::vector<uint16_t>& indices, std::vector<VertexPN>& vertices )
{
	float radius = 3.0f;
	float minorRadius = 1.0f;
	int n = 20;

	int vertexCount = (n+1) * (n+1);
	for( int i = 0; i <= n; ++i ) {
		double ph = PI * 2.0 * i / n;
		double r = cos( ph ) * minorRadius;
		double y = sin( ph ) * minorRadius;

		for( int j = 0; j <= n; ++j ) {
			double th = 2.0 * PI * j / n;
			float x = (float)( (r+radius) * cos(th) );
			float z = (float)( (r+radius) * sin(th) );

			VertexPN v;
			v.Position.x = x; v.Position.y = (float)y; v.Position.z = z;

			float nx = (float)( r * cos(th) );
			float ny = (float) y;
			float nz = (float)( r * sin(th) );
			v.Normal.nx = nx; v.Normal.ny = ny; v.Normal.nz = nz;

			vertices.push_back( v );
		}
	}
	for( int i = 0; i < n; ++i ) {
		for( int j = 0; j < n; ++j ) {
			int index = (n+1) * j + i;
			indices.push_back( index );
			indices.push_back( index+n+2 );
			indices.push_back( index+1 );

			indices.push_back( index );
			indices.push_back( index+n+1 );
			indices.push_back( index+n+2 );
		}
	}
}
