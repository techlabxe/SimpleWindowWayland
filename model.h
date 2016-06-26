#include <cstdint>
#include <vector>

struct VertexPosition {
	float x, y, z;
};
struct VertexNormal {
	float nx, ny, nz;
};

struct VertexPN {
	VertexPosition Position;
	VertexNormal   Normal;
};

void createTorus( std::vector<uint16_t>& indices, std::vector<VertexPN>& vertices );
