#ifndef FRAMEBUFFER_OBJECT_HPP
#define FRAMEBUFFER_OBJECT_HPP

#include <map>
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#include "texture.hpp"

class frame_buffer_t {

public:

	frame_buffer_t(size_t width, size_t height);
	~frame_buffer_t();

	GLuint handle() const { return __handle; };	
	void bind();
	void release();	
	bool is_valid() const;	
	void attach_render_buffer(GLenum attachement, GLenum internal_format);
	void attach_texture(GLenum attachment, const texture_t &texture);	
	void select_color_buffers(GLenum *draw_buffers, GLenum read_buffer = GL_NONE);

private:

	size_t __width;
	size_t __height;
	GLuint __handle;	
	std::map<GLenum, GLuint> __render_buffer_handles;

};

#endif
