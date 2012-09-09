#include <iostream>
#include <cassert>
#include "fbo.hpp"

using namespace std;

frame_buffer_t::frame_buffer_t(size_t width, size_t height) {
	__width = width;
	__height = height;
			
	glGenFramebuffersEXT(1, &__handle);
}

frame_buffer_t::~frame_buffer_t() {
	glDeleteFramebuffersEXT(1, &__handle);
}

void frame_buffer_t::bind() {
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, __handle);
}

void frame_buffer_t::release() {
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}

#define WHEN_FBO_STATUS(fbo_status)  case fbo_status: cerr << "glCheckFramebufferStatus failed: " << #fbo_status << endl; break;

bool frame_buffer_t::is_valid() const {
	GLenum fbo_status = glCheckFramebufferStatus(GL_FRAMEBUFFER_EXT);	
	if ( fbo_status == GL_FRAMEBUFFER_COMPLETE)
		return true;
	
	switch (fbo_status) {
		WHEN_FBO_STATUS(GL_FRAMEBUFFER_UNDEFINED);
		WHEN_FBO_STATUS(GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT);
		WHEN_FBO_STATUS(GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT);
		WHEN_FBO_STATUS(GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER);
		WHEN_FBO_STATUS(GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER);
		WHEN_FBO_STATUS(GL_FRAMEBUFFER_UNSUPPORTED);
		WHEN_FBO_STATUS(GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE);
		default:
			assert(false);
	}
	
	return false;
}

void frame_buffer_t::attach_render_buffer(GLenum attachement, GLenum internal_format) {
	GLuint render_buffer_handle;
	glGenRenderbuffers(1, &render_buffer_handle);
	glBindRenderbuffer(GL_RENDERBUFFER_EXT, render_buffer_handle);
	glRenderbufferStorage(GL_RENDERBUFFER_EXT, internal_format, __width, __height);	
	glFramebufferRenderbuffer(GL_FRAMEBUFFER_EXT, attachement, GL_RENDERBUFFER_EXT, render_buffer_handle);	
	glBindRenderbuffer(GL_RENDERBUFFER_EXT, 0);
	
	__render_buffer_handles.insert(pair<GLenum, GLuint>(attachement, render_buffer_handle));
}

void frame_buffer_t::attach_texture(GLenum attachment, const texture_t &texture) {
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, attachment, texture.target, texture.handle, 0);
}

void frame_buffer_t::select_color_buffers(GLenum *draw_buffers, GLenum read_buffer) {
	glReadBuffer(read_buffer);
	
	if (draw_buffers == NULL) {
		glDrawBuffer(GL_NONE);
		return;
	}
	
	size_t n = sizeof(draw_buffers) / sizeof(GLenum);
	glDrawBuffers(n, draw_buffers);
}

