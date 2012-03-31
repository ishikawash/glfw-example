
#include <iostream>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <sstream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <openctmpp.h>
#include <GL/glfw.h>

#define BUFFER_OFFSET(bytes) ((GLubyte *)NULL + (bytes))

struct mesh_t {
  std::vector<float> vertices;
  std::vector<float> normals;
  std::vector<unsigned int> indices;
};

struct drawable_t {
  GLuint vertex_buffer_handle;
  GLuint normal_buffer_handle;
  GLuint index_buffer_handle;

  unsigned int vertex_count;
  unsigned int index_count;
};

struct attribute_handle_t {
  GLuint position;
  GLuint normal;
};

struct uniform_handle_t {
  GLuint projection_matrix;
  GLuint view_matrix;
  GLuint model_matrix;
  GLuint normal_matrix;
  GLuint light_direction;
  GLuint material_color;
};

struct trackball_state_t {
	glm::ivec2 center_position;
	glm::ivec2 prev_position;
	glm::quat orientation;
	float radius;
	bool dragged;
};

trackball_state_t trackball_state;

GLuint build_shader(const char *source, GLenum shader_type)
{
  GLuint shader_handle = glCreateShader(shader_type);
  glShaderSource(shader_handle, 1, &source, 0);
  glCompileShader(shader_handle);

  GLint compile_success;
  glGetShaderiv(shader_handle, GL_COMPILE_STATUS, &compile_success);

  if (compile_success == GL_FALSE) {
    GLchar log[256];
    glGetShaderInfoLog(shader_handle, sizeof(log), 0, &log[0]);
    std::cerr << log << std::endl;
    return 0;
  }

  return shader_handle;
}

GLuint build_program(const std::string & vertex_shader_source, const std::string & fragment_shader_source)
{
  GLuint vertex_shader_handle = build_shader(vertex_shader_source.c_str(), GL_VERTEX_SHADER);
  if (!vertex_shader_handle) {
    std::cerr << "*** compile vertex shader failed" << std::endl;
    return 0;
  }
  GLuint fragment_shader_handle = build_shader(fragment_shader_source.c_str(), GL_FRAGMENT_SHADER);
  if (!fragment_shader_handle) {
    std::cerr << "*** compile fragment shader failed" << std::endl;
    return 0;
  }

  GLuint program_handle = glCreateProgram();
  glAttachShader(program_handle, vertex_shader_handle);
  glAttachShader(program_handle, fragment_shader_handle);
  glLinkProgram(program_handle);

  GLint link_success;
  glGetProgramiv(program_handle, GL_LINK_STATUS, &link_success);
  if (link_success == GL_FALSE) {
    GLchar log[256];
    glGetProgramInfoLog(program_handle, sizeof(log), 0, &log[0]);
    std::cerr << log << std::endl;
    return 0;
  }

  return program_handle;
}

void read_shader_source(const char *filename, std::string & source)
{
  std::ifstream file(filename);
  std::stringstream ss;
  std::string buf;
  while (std::getline(file, buf)) {
    ss << buf << std::endl;
  }
  source = ss.str();
  file.close();
}

bool load_mesh(const char *ctm_filepath, mesh_t & mesh)
{
  CTMimporter ctm;

  try {
    ctm.Load(ctm_filepath);

    unsigned int vertex_count = ctm.GetInteger(CTM_VERTEX_COUNT);
    unsigned int vertex_element_count = 3 * vertex_count;
    const CTMfloat *vertices = ctm.GetFloatArray(CTM_VERTICES);
    mesh.vertices.resize(vertex_element_count);
    std::memcpy(&mesh.vertices[0], vertices, vertex_element_count * sizeof(float));

    unsigned int face_count = ctm.GetInteger(CTM_TRIANGLE_COUNT);
    unsigned int indice_count = face_count * 3;
    const CTMuint *indices = ctm.GetIntegerArray(CTM_INDICES);
    mesh.indices.resize(indice_count);
    std::memcpy(&mesh.indices[0], indices, indice_count * sizeof(unsigned int));

    if (ctm.GetInteger(CTM_HAS_NORMALS) == CTM_TRUE) {
      const CTMfloat *normals = ctm.GetFloatArray(CTM_NORMALS);
      mesh.normals.resize(vertex_element_count);
      std::memcpy(&mesh.normals[0], normals, vertex_element_count * sizeof(float));
    } else {
      std::cerr << "*** CTM_HAS_NORMALS == false" << std::endl;
    }

    return true;
  }
  catch(ctm_error & e) {
    std::cerr << "*** Loading CTM file failed: " << e.what() << std::endl;
    return false;
  }
}

glm::vec3 map_to_sphere(const trackball_state_t &tb_state, const glm::ivec2 &point) 
{
    glm::vec2 p(point.x - tb_state.center_position.x, point.y - tb_state.center_position.y);
    
    p.y = -p.y;
    
    const float radius = tb_state.radius;
    const float safe_radius = radius - 1.0f;
    
    if (glm::length(p) > safe_radius) {
        float theta = std::atan2(p.y, p.x);
        p.x = safe_radius * std::cos(theta);
        p.y = safe_radius * std::sin(theta);
    }
    
		float length_squared = p.x*p.x + p.y*p.y;
    float z = std::sqrt(radius * radius - length_squared);
    glm::vec3 q = glm::vec3(p.x, p.y, z);
		return glm::normalize(q / radius);
}

void mouse(int button, int action) {
	if (( button == GLFW_MOUSE_BUTTON_LEFT )) {
		switch (action) {
			case GLFW_PRESS:
				glfwGetMousePos(&trackball_state.prev_position.x, &trackball_state.prev_position.y);
				trackball_state.dragged = true;
				break;
			case GLFW_RELEASE:
				trackball_state.dragged = false;
				break;
		}
	}
}

void motion(int x, int y) {
	if (! trackball_state.dragged)
		return;
	
	glm::ivec2 current_position(x, y);
	glm::vec3 v0 = map_to_sphere(trackball_state, trackball_state.prev_position);
	glm::vec3 v1 = map_to_sphere(trackball_state, current_position);
	glm::vec3 v2 = glm::cross(v0, v1); // calculate rotation axis
	
	float d = glm::dot(v0, v1);
	float s = std::sqrt((1.0f + d) * 2.0f);
	glm::quat q(0.5f * s, v2 / s);
	trackball_state.orientation = q * trackball_state.orientation;
	trackball_state.orientation /= glm::length(trackball_state.orientation);
		
	trackball_state.prev_position = current_position;
}

int main(int argc, char **args)
{
  const char *ctm_filepath = (argc > 1) ? args[1] : "teapot.ctm";

	trackball_state.radius = 300.0f;
	trackball_state.dragged = false;
	trackball_state.orientation.w = 1.0f;
	trackball_state.orientation.x = 0.0f;
	trackball_state.orientation.y = 0.0f;
	trackball_state.orientation.z = 0.0f;

  int width, height;

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
	glfwSetMouseButtonCallback(mouse);
	glfwSetMousePosCallback(motion);
  glfwSetWindowTitle("Spinning Teapot");
  glfwEnable(GLFW_STICKY_KEYS);
  glfwSwapInterval(1);

  mesh_t mesh;
  if (!load_mesh(ctm_filepath, mesh)) {
    exit(EXIT_FAILURE);
  }
  size_t vertex_buffer_size = mesh.vertices.size() * sizeof(float);
  size_t normal_buffer_size = mesh.normals.size() * sizeof(float);
  size_t indice_buffer_size = mesh.indices.size() * sizeof(unsigned int);

  drawable_t drawable;
	drawable.vertex_count = mesh.vertices.size() / 3;
	drawable.index_count = mesh.indices.size();
	
  glGenBuffers(1, &drawable.vertex_buffer_handle);
  glBindBuffer(GL_ARRAY_BUFFER, drawable.vertex_buffer_handle);
  glBufferData(GL_ARRAY_BUFFER, vertex_buffer_size, &mesh.vertices[0], GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glGenBuffers(1, &drawable.normal_buffer_handle);
  glBindBuffer(GL_ARRAY_BUFFER, drawable.normal_buffer_handle);
  glBufferData(GL_ARRAY_BUFFER, normal_buffer_size, &mesh.normals[0], GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glGenBuffers(1, &drawable.index_buffer_handle);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, drawable.index_buffer_handle);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indice_buffer_size, &mesh.indices[0], GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

  std::string vertex_shader_source;
  std::string fragment_shader_source;
  read_shader_source("simple.vs", vertex_shader_source);
  read_shader_source("simple.fs", fragment_shader_source);
  GLuint program_handle = build_program(vertex_shader_source, fragment_shader_source);
  if (!program_handle) {
    exit(EXIT_FAILURE);
  }
  glUseProgram(program_handle);

  attribute_handle_t attribute;
  attribute.position = glGetAttribLocation(program_handle, "position");
  attribute.normal = glGetAttribLocation(program_handle, "normal");

  uniform_handle_t uniform;
  uniform.projection_matrix = glGetUniformLocation(program_handle, "projection_matrix");
  uniform.view_matrix = glGetUniformLocation(program_handle, "view_matrix");
  uniform.model_matrix = glGetUniformLocation(program_handle, "model_matrix");
  uniform.normal_matrix = glGetUniformLocation(program_handle, "normal_matrix");
  uniform.light_direction = glGetUniformLocation(program_handle, "light_direction");
  uniform.material_color = glGetUniformLocation(program_handle, "material_color");

  glm::vec3 eye(0.0f, 0.0f, 5.0f);
  glm::vec3 center(0.0f, 0.0f, 0.0f);
  glm::vec3 up(0.0f, 1.0f, 0.0f);
  glm::mat4 view_matrix = glm::lookAt(eye, center, up); // from world to camera
  glUniformMatrix4fv(uniform.view_matrix, 1, 0, glm::value_ptr(view_matrix));

  glm::vec3 light_direction = glm::normalize(glm::vec3(-1.0f, -1.0f, -1.0f)); // light direction in world space
  glUniform3fv(uniform.light_direction, 1, glm::value_ptr(light_direction));

  glm::vec3 material_color(0.0f, 0.0f, 1.0f);
  glUniform3fv(uniform.material_color, 1, glm::value_ptr(material_color));

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);

  do {
    glfwGetWindowSize(&width, &height);

    height = height > 0 ? height : 1;
		
		trackball_state.center_position.x = 0.5 * width;
		trackball_state.center_position.y = 0.5 * height;


    glClearColor(0.5f, 0.5f, 0.5f, 0.0f);
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
    glViewport(0, 0, width, height);

    glm::mat4 projection_matrix = glm::perspective(60.0f, (float) width / (float) height, 1.0f, 30.0f);
    glUniformMatrix4fv(uniform.projection_matrix, 1, 0, glm::value_ptr(projection_matrix));

		glm::mat4 model_rotation = glm::mat4_cast(trackball_state.orientation);
    glm::mat4 model_matrix = model_rotation; // from local to world
    glUniformMatrix4fv(uniform.model_matrix, 1, 0, glm::value_ptr(model_matrix));

    glm::mat3 normal_matrix = glm::mat3(model_matrix); // upper 3x3 matrix of model matrix
    glUniformMatrix3fv(uniform.normal_matrix, 1, 0, glm::value_ptr(normal_matrix));

    GLsizei stride = 3 * sizeof(float);
    glBindBuffer(GL_ARRAY_BUFFER, drawable.vertex_buffer_handle);
    glVertexAttribPointer(attribute.position, 3, GL_FLOAT, GL_FALSE, stride, BUFFER_OFFSET(0));
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindBuffer(GL_ARRAY_BUFFER, drawable.normal_buffer_handle);
    glVertexAttribPointer(attribute.normal, 3, GL_FLOAT, GL_FALSE, stride, BUFFER_OFFSET(0));
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glEnableVertexAttribArray(attribute.position);
    glEnableVertexAttribArray(attribute.normal);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, drawable.index_buffer_handle);
    glDrawElements(GL_TRIANGLES, drawable.index_count, GL_UNSIGNED_INT, BUFFER_OFFSET(0));
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glDisableVertexAttribArray(attribute.normal);
    glDisableVertexAttribArray(attribute.position);

    glfwSwapBuffers();

  }
  while (glfwGetKey(GLFW_KEY_ESC) != GLFW_PRESS && glfwGetWindowParam(GLFW_OPENED));

  glfwTerminate();

  return 0;
}
