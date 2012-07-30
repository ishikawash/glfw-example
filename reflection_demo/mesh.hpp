#ifndef MESH_HPP
#define MESH_HPP

#include <vector>
#include <OpenGL/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader.hpp"

struct vertex_t {
	glm::vec4 position;
	glm::vec3 normal;
	glm::vec2 tex_coord;
};

struct mesh_t {
	
	std::vector<vertex_t> vertices;
	std::vector<unsigned int> indices;
	GLuint vertex_buffer_handle;
	GLuint index_buffer_handle;
	
	void load_to_buffers();	
	void render(const shader_program_t &shader_program);
	
	static bool read_from_file(const char *ctm_filepath, mesh_t &mesh);
	
};

#endif
