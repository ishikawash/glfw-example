
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

#define BUFFER_OFFSET(STRUCT, MEMBER) ((GLubyte *)NULL + (offsetof(STRUCT, MEMBER)))

float camera_fovy = 30.0f;
int screen_width;
int screen_height;

glm::mat4 projection_matrix;
glm::mat4 view_matrix;
glm::mat3 normal_matrix;
glm::vec3 light_direction;

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
  glfwGetWindowSize(&screen_width, &screen_height);
  glfwSetWindowTitle("Teapot");
  glfwEnable(GLFW_STICKY_KEYS);
  glfwSwapInterval(1);

	shader_program_t shader_program;
	shader_program_t::build(shader_program, "diffuse.vs", "diffuse.fs");

	mesh_t teapot;
	if (! mesh_t::read_from_file("teapot.ctm", teapot)) {
		std::cerr << "Failed to read ctm file" << std::endl;
	  glfwTerminate();
    exit(EXIT_FAILURE);		
	}
	teapot.load_to_buffers();
	
	projection_matrix = glm::perspective(camera_fovy, (float) screen_width / (float) screen_height, 1.0f, 30.0f);

  glm::vec3 camera_position(0.0f, 0.0f, 3.0f);
  glm::vec3 camera_center(0.0f, 0.0f, 0.0f);
  glm::vec3 camera_up(0.0f, 1.0f, 0.0f);
  view_matrix = glm::lookAt(camera_position, camera_center, camera_up); // from world to camera

	normal_matrix = glm::mat3(glm::transpose(glm::inverse(view_matrix)));

	light_direction = glm::vec3(0.0, -1.0, 0.0);

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

  do {
		
		{			
			glCullFace(GL_BACK);
			glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
			glClearDepth(1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glViewport(0, 0, screen_width, screen_height);

			shader_program.bind();
			shader_program.set_uniform_value("light_direction", light_direction);
			shader_program.set_uniform_value("projection_matrix", projection_matrix);
			shader_program.set_uniform_value("model_view_matrix", view_matrix);
			shader_program.set_uniform_value("normal_matrix", normal_matrix);
			teapot.render(shader_program);
			shader_program.release();
		}

    glfwSwapBuffers();

  }
  while (glfwGetKey(GLFW_KEY_ESC) != GLFW_PRESS && glfwGetWindowParam(GLFW_OPENED));

  glfwTerminate();

  return 0;
}

