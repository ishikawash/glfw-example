#ifndef SHADER_HPP
#define SHADER_HPP

#include <vector>
#include <string>
#include <OpenGL/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>


class shader_t {

private:	
	GLenum __type_id;
	GLuint __handle;
	std::string __log;
	
	bool is_allocated() const;
	
public:
	shader_t(GLenum shader_type_id);
	shader_t(const shader_t &shader);
	~shader_t();
	
	bool compile(const char *source);
	bool is_compiled() const;
	
	GLenum type_id() const { return __type_id; }
	const std::string& log() const { return __log; }
	GLuint handle() const { return __handle; }
	
};


typedef std::vector<const shader_t *> shader_container;

class shader_program_t {

private:
	GLuint __handle;
	std::string __log;
	shader_container __shaders;

	bool is_allocated() const;

public:
	shader_program_t();
	~shader_program_t();
	
	bool add_shader(const shader_t &shader);
	bool add_shader_from_source_code(GLenum shader_type_id, const char *source_code);
	bool add_shader_from_source_file(GLenum shader_type_id, const char *source_filepath);
	
	bool link();
	bool is_linked() const;
	
	void bind() const;
	void release() const;
	
	GLuint attribute_location(const char *name) const;
	GLuint uniform_location(const char *name) const;
	void set_uniform_value(GLuint location, int value) const;
	void set_uniform_value(const char *name, int value) const;
	void set_uniform_value(const char *name, float value) const;
	void set_uniform_value(const char *name, const glm::vec3 &v) const;
	void set_uniform_value(const char *name, const glm::mat3 &mat) const;
	void set_uniform_value(const char *name, const glm::mat4 &mat) const;
	
	const std::string& log() const { return __log; }
	GLuint handle() const { return __handle; }

};

#endif
