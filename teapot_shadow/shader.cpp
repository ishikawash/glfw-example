#include <iostream>
#include <fstream>
#include <sstream>
#include "shader.hpp"

using namespace std;

static inline bool compile_ok(GLuint shader_handle) {
	GLint compile_success;
	glGetShaderiv(shader_handle, GL_COMPILE_STATUS, &compile_success);
	return ( compile_success == GL_TRUE );	
}

static inline bool link_ok(GLuint program_handle) {
	GLint link_success;
  glLinkProgram(program_handle);  
  glGetProgramiv(program_handle, GL_LINK_STATUS, &link_success);
	return ( link_success == GL_TRUE );
}

static void read_source_code(const char *filepath, std::string & source_code) {
  ifstream file(filepath);
  stringstream ss;
  string buf;
  while (getline(file, buf)) {
    ss << buf << endl;
  }
  source_code = ss.str();
  file.close();
}


shader_t::shader_t(GLenum shader_type_id) {
	__type_id = shader_type_id;
	__handle = 0;
}

shader_t::shader_t(const shader_t &shader) {
	__type_id = shader.__type_id;
	__handle = shader.__handle;
}

shader_t::~shader_t() {
	
}

bool shader_t::is_allocated() const {
	return ( glIsShader(__handle) == GL_TRUE );
}

bool shader_t::is_compiled() const {
	return compile_ok(__handle);
}

bool shader_t::compile(const char *source_code) {	
  GLuint shader_handle = glCreateShader(__type_id);

  glShaderSource(shader_handle, 1, &source_code, 0);
  glCompileShader(shader_handle);

  if (compile_ok(shader_handle)) {
		if (is_allocated())
			glDeleteShader(__handle);
			
		__handle = shader_handle;
		return true;
	} else {
		GLint log_length = 0;
		glGetShaderiv(shader_handle, GL_INFO_LOG_LENGTH, &log_length);
		
		std::string log(log_length + 1, '\0');
    glGetShaderInfoLog(shader_handle, log_length, 0, &log[0]);
		__log = log;
		
		glDeleteShader(shader_handle);

		return false;
  }
}

shader_program_t::shader_program_t() {
	__handle = 0;
}

shader_program_t::~shader_program_t() {
	glDeleteProgram(__handle);
	
	for (shader_container::iterator it = __shaders.begin(); it != __shaders.end(); it++) 
		delete (*it);
}

bool shader_program_t::add_shader(const shader_t &shader) {
	if (! shader.is_compiled() )
		return false;		
	shader_t *__shader = new shader_t(shader.type_id());
	*__shader = shader;
	__shaders.push_back(__shader);
	return true;
}

bool shader_program_t::add_shader_from_source_code(GLenum shader_type_id, const char *source_code) {
	shader_t shader(shader_type_id);
	if (shader.compile(source_code)) {
		return add_shader(shader);
	} else {
		__log = shader.log();
		return false;
	}
}

bool shader_program_t::add_shader_from_source_file(GLenum shader_type_id, const char *source_filepath) {
	string source_code;
	read_source_code(source_filepath, source_code);
	return add_shader_from_source_code(shader_type_id, source_code.c_str());
}

bool shader_program_t::is_linked() const {
	return link_ok(__handle);
}

bool shader_program_t::is_allocated() const {
	return ( glIsProgram(__handle) == GL_TRUE );
}

bool shader_program_t::link() {		
	GLuint program_handle = glCreateProgram();
	
	for (shader_container::iterator it = __shaders.begin(); it != __shaders.end(); it++) {
		glAttachShader(program_handle, (*it)->handle());
	}

  if (link_ok(program_handle)) {
		if (is_allocated())
			glDeleteProgram(__handle);
			
		__handle = program_handle;
		return true;
	} else {
		GLint log_length = 0;
		glGetProgramiv(program_handle, GL_INFO_LOG_LENGTH, &log_length);
		
		std::string log(log_length + 1, '\0');
    glGetProgramInfoLog(program_handle, log_length, 0, &log[0]);
		__log = log;

		glDeleteProgram(program_handle);

		return false;
  }	
}

void shader_program_t::bind() const {
	glUseProgram(__handle);
}

void shader_program_t::release() const {
	glUseProgram(0);
}

GLuint shader_program_t::attribute_location(const char *name) const {
	return glGetAttribLocation(__handle, name);
}

GLuint shader_program_t::uniform_location(const char *name) const {
	return glGetUniformLocation(__handle, name);
}

void shader_program_t::set_uniform_value(const char *name, const glm::mat4 &mat) const {
	glUniformMatrix4fv(uniform_location(name), 1, 0, glm::value_ptr(mat));
}

void shader_program_t::set_uniform_value(const char *name, const glm::mat3 &mat) const {
	glUniformMatrix3fv(uniform_location(name), 1, 0, glm::value_ptr(mat));
}

void shader_program_t::set_uniform_value(const char *name, const glm::vec3 &v) const {
	glUniform3fv(uniform_location(name), 1, glm::value_ptr(v));
}

void shader_program_t::set_uniform_value(const char *name, int value) const {
	set_uniform_value(uniform_location(name), value);
}

void shader_program_t::set_uniform_value(const char *name, float value) const {
	glUniform1f(uniform_location(name), value);
}

void shader_program_t::set_uniform_value(GLuint location, int value) const {
	glUniform1i(location, value);
}
