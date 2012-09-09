#ifndef TEXTURE_HPP
#define TEXTURE_HPP

#include <vector>
#include <OpenGL/gl.h>

struct texture_t {
	GLenum target;
	GLuint handle;
};

struct texture_unit_t {

	GLenum unit_id;
	const texture_t *texture;
	
	texture_unit_t() : unit_id(0), texture(NULL) { }
	
	int index() const {
		return unit_id - GL_TEXTURE0;
	}
	
	void activate() const {
		if (texture != NULL) {
			glActiveTexture(unit_id);
			glBindTexture(texture->target, texture->handle);	
		}
	}
	
	void deactivate() const {
		if (texture != NULL) {
			glBindTexture(texture->target, 0);
			glActiveTexture(0);
		}
	}
	
	static std::vector<texture_unit_t> collection;
	
	static void initialize();
	
	static void attach(int index, const texture_t *texture);
	static void dettach(int index);
	
};

#endif
