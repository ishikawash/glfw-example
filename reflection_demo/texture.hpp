#ifndef TEXTURE_HPP
#define TEXTURE_HPP

#include <OpenGL/gl.h>

struct texture_t {
	GLenum target;
	GLuint handle;
};

struct texture_unit_t {
	GLenum unit_id;
	int index;
	texture_t *texture;
};

#endif
