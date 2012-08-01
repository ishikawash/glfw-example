
#include <iostream>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <fstream>
#include <string>
#include <sstream>

#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#include <GL/glfw.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <openctmpp.h>

#include "shader.hpp"
#include "mesh.hpp"
#include "fbo.hpp"
#include "texture.hpp"


struct model_t {
	mesh_t *mesh;
	glm::vec3 position;
	glm::vec3 scale;
	float angle;
};

struct camera_t {
	glm::mat4 projection_matrix;
	glm::mat4 view_inverse_matrix;	
};

glm::ivec2 viewport; 
glm::vec3 light_direction;

model_t teapot;
model_t board;
shader_program_t diffuse_shader;
shader_program_t reflection_shader;
frame_buffer_t *fbo;

camera_t default_camera;
camera_t reflection_camera;

texture_t image_texture;
texture_t color_texture;
texture_t depth_texture;
texture_unit_t texture_unit_0 = { GL_TEXTURE0, 0, &color_texture };
texture_unit_t texture_unit_1 = { GL_TEXTURE1, 1, &image_texture };


glm::mat4 mirror_matrix(const glm::vec3 &n, float d) {
	glm::vec4 P = glm::vec4(n, d);
	return glm::mat4(
			-2.0f*P.x*P.x + 1.0f, -2.0f*P.y*P.x,        -2.0f*P.z*P.x,        0.0f,
			-2.0f*P.x*P.y,        -2.0f*P.y*P.y + 1.0f, -2.0f*P.z*P.y,        0.0f,
			-2.0f*P.x*P.z,        -2.0f*P.y*P.z,        -2.0f*P.z*P.z + 1.0f, 0.0f,
			-2.0f*P.x*P.w,        -2.0f*P.y*P.w,        -2.0f*P.z*P.w,        1.0f
			);	
}

bool build_color_texture(texture_t &texture, size_t width, size_t height) {
	GLuint texture_handle;
	
	glGenTextures(1, &texture_handle);
  glBindTexture(GL_TEXTURE_2D, texture_handle);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glBindTexture(GL_TEXTURE_2D, 0);

	texture.handle = texture_handle;
	texture.target = GL_TEXTURE_2D;
	
	return true;
}

bool build_depth_texture(texture_t &texture, size_t width, size_t height) {
	GLuint texture_handle;
	
  glGenTextures(1, &texture_handle);
  glBindTexture(GL_TEXTURE_2D, texture_handle);   
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, width, height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, 0);  
	glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);  
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);  
  glBindTexture(GL_TEXTURE_2D, 0);

	texture.handle = texture_handle;
	texture.target = GL_TEXTURE_2D;
	
	return true;
}

bool build_image_texutre(texture_t &texture, const char *filepath) {
	GLuint texture_handle;
	
	glGenTextures(1, &texture_handle);
  glBindTexture(GL_TEXTURE_2D, texture_handle);
  if (!glfwLoadTexture2D(filepath, GLFW_BUILD_MIPMAPS_BIT))
		return false;
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glBindTexture(GL_TEXTURE_2D, 0);

	texture.handle = texture_handle;
	texture.target = GL_TEXTURE_2D;

	return true;
}


void debug_draw_texture(GLuint texture_handle) {
	glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glBindTexture(GL_TEXTURE_2D, texture_handle);
  glBegin(GL_TRIANGLE_FAN);
  glTexCoord2d(0.0, 0.0);
  glVertex2d(-1.0, -1.0);
  glTexCoord2d(1.0, 0.0);
  glVertex2d( 1.0, -1.0);
  glTexCoord2d(1.0, 1.0);
  glVertex2d( 1.0,  1.0);
  glTexCoord2d(0.0, 1.0);
  glVertex2d(-1.0,  1.0);
  glEnd();
  glBindTexture(GL_TEXTURE_2D, 0);	
}

void setup_models() {
	teapot.mesh = new mesh_t();
	mesh_t::read_from_file("mesh/teapot.ctm", *(teapot.mesh));
	teapot.mesh->load_to_buffers();
	teapot.position.y = 0.3f;
	teapot.scale.x = teapot.scale.y = teapot.scale.z = 1.0f;
	teapot.angle = 0.0f;
	
	board.mesh = new mesh_t();
	mesh_t::read_from_file("mesh/quad.ctm", *(board.mesh));
	board.mesh->load_to_buffers();
	board.scale.x = board.scale.z = 1.5f;
	board.scale.y = 1.0f;
	board.angle = 0.0f;
}

void setup_cameras() {
	float aspect_ratio = (float) viewport.x / (float) viewport.y;
	glm::mat4 projection_matrix = glm::perspective(30.0f, aspect_ratio, 1.0f, 30.0f);
	glm::mat4 view_inverse_matrix	= glm::lookAt(glm::vec3(0.0f, 1.5f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // from world to camera
		
	default_camera.projection_matrix = projection_matrix;
	default_camera.view_inverse_matrix = view_inverse_matrix;

	reflection_camera.projection_matrix = projection_matrix;
	reflection_camera.view_inverse_matrix = view_inverse_matrix * mirror_matrix(glm::vec3(0.0f, 1.0f, 0.0f), 0.0f);
}

void render_model(const model_t &model, const camera_t &camera, const shader_program_t &shader_program) {
	glm::mat4 rotation_matrix = glm::rotate(glm::mat4(1.0), model.angle, glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 scale_matrix = glm::scale(glm::mat4(1.0), model.scale);
	glm::mat4 translation_matrix = glm::translate(glm::mat4(1.0), model.position); // from model to world	
	
	glm::mat4 model_matrix = translation_matrix * scale_matrix * rotation_matrix;
	glm::mat4 model_view_matrix = camera.view_inverse_matrix * model_matrix;
	glm::mat3 normal_matrix = glm::mat3(glm::transpose(glm::inverse(model_view_matrix)));
	
	shader_program.set_uniform_value("projection_matrix", camera.projection_matrix);
	shader_program.set_uniform_value("model_view_matrix", model_view_matrix);	
	shader_program.set_uniform_value("normal_matrix", normal_matrix);
	
	model.mesh->render(shader_program);
}

void setup() {
	setup_models();
	
	setup_cameras();
	
	shader_program_t::build(diffuse_shader, "shader/diffuse.vs", "shader/diffuse.fs");
	shader_program_t::build(reflection_shader, "shader/reflection.vs", "shader/reflection.fs");

	light_direction = glm::vec3(0.0f, -1.0f, 0.0f);
	
	build_image_texutre(image_texture, "wood.tga");
	build_color_texture(color_texture, viewport.x, viewport.y);
	build_depth_texture(depth_texture, viewport.x, viewport.y);

	fbo = new frame_buffer_t(viewport.x, viewport.y);
	fbo->bind();
	fbo->attach_texture(GL_COLOR_ATTACHMENT0_EXT, color_texture);
	fbo->attach_render_buffer(GL_DEPTH_ATTACHMENT_EXT, GL_DEPTH_COMPONENT);
	GLenum draw_buffers[] = { GL_COLOR_ATTACHMENT0_EXT };
	fbo->select_color_buffers(draw_buffers);
	if (! fbo->is_valid()) {
	  glfwTerminate();
    exit(EXIT_FAILURE);		
	}
	fbo->release();
	
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);	
	glCullFace(GL_BACK);	
}

void cleanup() {
	
}

void render() {	
	fbo->bind();
	glFrontFace(GL_CW);
	glViewport(0, 0, viewport.x, viewport.y);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	diffuse_shader.bind();
	diffuse_shader.set_uniform_value("light_direction", light_direction);
	render_model(teapot, reflection_camera, diffuse_shader);
	diffuse_shader.release();
	fbo->release();

	glFrontFace(GL_CCW);
	glViewport(0, 0, viewport.x, viewport.y);
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	
	
	diffuse_shader.bind();
	render_model(teapot, default_camera, diffuse_shader);
	diffuse_shader.release();

	glActiveTexture(texture_unit_0.unit_id);
	glBindTexture(texture_unit_0.texture->target, texture_unit_0.texture->handle);
	glActiveTexture(texture_unit_1.unit_id);
	glBindTexture(texture_unit_1.texture->target, texture_unit_1.texture->handle);
	reflection_shader.bind();
	reflection_shader.set_uniform_value("blend_factor", 0.6f);
	reflection_shader.set_uniform_value("viewport", viewport);
	reflection_shader.set_uniform_value("texture0", texture_unit_0.index);
	reflection_shader.set_uniform_value("texture1", texture_unit_1.index);
	render_model(board, default_camera, reflection_shader);
	reflection_shader.release();
	glBindTexture(texture_unit_0.texture->target, 0);
	glActiveTexture(0);

	// debug_draw_texture(color_texture.handle);
	
	teapot.angle += 1.0f;
}

int main(int argc, char **args)
{
  if (!glfwInit()) {
    std::cerr << "Failed to initialize GLFW" << std::endl;
    exit(EXIT_FAILURE);
  }

  int depth_bits = 16;
  if (!glfwOpenWindow(640, 480, 0, 0, 0, 0, depth_bits, 0, GLFW_WINDOW)) {
    std::cerr << "Failed to open GLFW window" << std::endl;
    glfwTerminate();
    exit(EXIT_FAILURE);
  }
  glfwGetWindowSize(&viewport.x, &viewport.y);
  glfwSetWindowTitle("Teapot");
  glfwEnable(GLFW_STICKY_KEYS);
  glfwSwapInterval(1);

	setup();

  do {
		render();						
    glfwSwapBuffers();
  }
  while (glfwGetKey(GLFW_KEY_ESC) != GLFW_PRESS && glfwGetWindowParam(GLFW_OPENED));

	cleanup();

  glfwTerminate();

  return 0;
}

