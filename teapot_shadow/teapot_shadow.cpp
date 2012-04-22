
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

#include "shader.hpp"

#define BUFFER_OFFSET(bytes) ((GLubyte *)NULL + (bytes))

struct mesh_t {
  std::vector<float> vertices;
  std::vector<float> normals;
  std::vector<unsigned int> indices;
  std::vector<float> tex_coords;
};

struct texture_t {
	GLuint handle;
	int unit_id;
};

struct array_buffer_t {
	GLuint handle;
	unsigned int count;
};

struct material_t {
	glm::vec3 diffuse;
	glm::vec3 specular;
	float shininess;
};

struct mesh_object_t {
	array_buffer_t vertex_buffer;
	array_buffer_t normal_buffer;
	array_buffer_t index_buffer;
	array_buffer_t tex_coord_buffer;
  shader_program_t shader_program;
	bool has_tex_coords;
	std::vector<texture_t> textures;
	material_t material;
};

struct trackball_state_t {
  glm::ivec2 center_position;
  glm::ivec2 prev_position;
  glm::quat orientation;
  float radius;
  bool dragged;
};

float camera_fovy = 30.0f;
bool camera_zoom = false;
int screen_width, screen_height;
trackball_state_t trackball_state;

GLenum texture_unit_names[] = {
	GL_TEXTURE0,
	GL_TEXTURE1,
	GL_TEXTURE2,
	GL_TEXTURE3
};

glm::vec3 map_to_sphere(const trackball_state_t & tb_state, const glm::ivec2 & point)
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

  float length_squared = p.x * p.x + p.y * p.y;
  float z = std::sqrt(radius * radius - length_squared);
  glm::vec3 q = glm::vec3(p.x, p.y, z);
  return glm::normalize(q / radius);
}

void mouse(int button, int action)
{
  if (button == GLFW_MOUSE_BUTTON_LEFT) {
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

void motion(int x, int y)
{
  if (!trackball_state.dragged)
    return;

  glm::ivec2 current_position(x, y);

  if (camera_zoom) {
    glm::vec2 v(trackball_state.prev_position.x - current_position.x, trackball_state.prev_position.y - current_position.y);
    float delta = glm::length(v);
    if (delta > 0.0f) {
      float direction = glm::sign(glm::dot(v, glm::vec2(0.0f, 1.0f)));
      float theta = direction * glm::clamp(delta, 0.1f, 0.5f);
      camera_fovy = glm::clamp(camera_fovy + theta, 5.0f, 60.0f);
    }
  } else {
    glm::vec3 v0 = map_to_sphere(trackball_state, trackball_state.prev_position);
    glm::vec3 v1 = map_to_sphere(trackball_state, current_position);
    glm::vec3 v2 = glm::cross(v0, v1); // calculate rotation axis

    float d = glm::dot(v0, v1);
    float s = std::sqrt((1.0f + d) * 2.0f);
    glm::quat q(0.5f * s, v2 / s);
    trackball_state.orientation = q * trackball_state.orientation;
    trackball_state.orientation /= glm::length(trackball_state.orientation);
  }

  trackball_state.prev_position.x = x;
  trackball_state.prev_position.y = y;
}

void resize(int width, int height)
{
  height = height > 0 ? height : 1;
  screen_width = width;
  screen_height = height;
  trackball_state.center_position.x = 0.5 * width;
  trackball_state.center_position.y = 0.5 * height;
}

void keyboard(int key, int action)
{
  switch (action) {
  case GLFW_PRESS:
    if (key == GLFW_KEY_LSHIFT) {
      camera_zoom = true;
    }
    break;
  case GLFW_RELEASE:
    camera_zoom = false;
    break;
  }
}

bool load_mesh_plane(mesh_t &mesh) {
	float vertices[][3] = {
		{ -0.50, 0.00, -0.50 },
		{ 0.50, 0.00, -0.50 },
		{ -0.50, 0.00, 0.50 },
		{ 0.50, 0.00, 0.50 }
	};

	float normals[][3] = {
		{ 0.00, 1.00, 0.00 },
		{ 0.00, 1.00, 0.00 },
		{ 0.00, 1.00, 0.00 },
		{ 0.00, 1.00, 0.00 }
	};

	unsigned int indices[][3] = {
		{ 0, 2, 1 },
		{ 1, 2, 3 }
	};
	
	unsigned int vertex_element_count = 3 * 4;
	unsigned int normal_element_count = vertex_element_count;
	unsigned int indice_count = 2 * 3;
	
	mesh.vertices.resize(vertex_element_count);
	mesh.normals.resize(normal_element_count);
	mesh.indices.resize(indice_count);
	
	std::memcpy(&mesh.vertices[0], vertices, vertex_element_count * sizeof(float));
	std::memcpy(&mesh.indices[0], indices, indice_count * sizeof(unsigned int));
	std::memcpy(&mesh.normals[0], normals, normal_element_count * sizeof(float));
	
	return true;
}

bool load_mesh_cube(mesh_t &mesh) {
	float vertices[][3] = {
		{ -0.50, -0.50, -0.50 },
		{ 0.50, -0.50, -0.50 },
		{ -0.50, 0.50, -0.50 },
		{ 0.50, 0.50, -0.50 },
		{ -0.50, -0.50, 0.50 },
		{ 0.50, -0.50, 0.50 },
		{ -0.50, 0.50, 0.50 },
		{ 0.50, 0.50, 0.50 },
	};

	float normals[][3] = {
		{ -0.58, -0.58, -0.58 },
		{ 0.82, -0.41, -0.41 },
		{ -0.41, 0.82, -0.41 },
		{ 0.41, 0.41, -0.82 },
		{ -0.41, -0.41, 0.82 },
		{ 0.41, -0.82, 0.41 },
		{ -0.82, 0.41, 0.41 },
		{ 0.58, 0.58, 0.58 }
	};

	unsigned int indices[][3] = {
		{ 0, 1, 5 },
		{ 0, 2, 3 },
		{ 0, 3, 1 },
		{ 0, 4, 6 },
		{ 0, 5, 4 },
		{ 0, 6, 2 },
		{ 1, 3, 7 },
		{ 1, 7, 5 },
		{ 2, 6, 7 },
		{ 2, 7, 3 },
		{ 4, 5, 7 },
		{ 4, 7, 6 }
	};
	
	unsigned int vertex_element_count = 3 * 8;
	unsigned int normal_element_count = vertex_element_count;
	unsigned int indice_count = 12 * 3;
	
	mesh.vertices.resize(vertex_element_count);
	mesh.normals.resize(normal_element_count);
	mesh.indices.resize(indice_count);
	
	std::memcpy(&mesh.vertices[0], vertices, vertex_element_count * sizeof(float));
	std::memcpy(&mesh.indices[0], indices, indice_count * sizeof(unsigned int));
	std::memcpy(&mesh.normals[0], normals, normal_element_count * sizeof(float));
	
	return true;
}

bool load_mesh_from_file(const char *ctm_filepath, mesh_t & mesh)
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

    unsigned int uv_map_count = ctm.GetInteger(CTM_UV_MAP_COUNT);
    if (uv_map_count > 0) {
      const CTMfloat *tex_coords = ctm.GetFloatArray(CTM_UV_MAP_1);
      unsigned int tex_coord_element_count = 2 * vertex_count;
      mesh.tex_coords.resize(tex_coord_element_count);
      std::memcpy(&mesh.tex_coords[0], tex_coords, tex_coord_element_count * sizeof(float));
    } else {
      std::cerr << "*** UV map not found" << std::endl;
    }

    return true;
  }
  catch(ctm_error & e) {
    std::cerr << "*** Loading CTM file failed: " << e.what() << std::endl;
    return false;
  }
}

bool build_mesh_object(const mesh_t &mesh, mesh_object_t &object) {

  object.vertex_buffer.count = mesh.vertices.size();
	object.normal_buffer.count = mesh.normals.size();
	object.index_buffer.count = mesh.indices.size();
	object.tex_coord_buffer.count = mesh.tex_coords.size();
	object.has_tex_coords = object.tex_coord_buffer.count > 0;

  //--- Buffers
  glGenBuffers(1, &object.vertex_buffer.handle);
  glBindBuffer(GL_ARRAY_BUFFER, object.vertex_buffer.handle);
  glBufferData(GL_ARRAY_BUFFER, object.vertex_buffer.count * sizeof(float), &mesh.vertices[0], GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glGenBuffers(1, &object.normal_buffer.handle);
  glBindBuffer(GL_ARRAY_BUFFER, object.normal_buffer.handle);
  glBufferData(GL_ARRAY_BUFFER, object.normal_buffer.count * sizeof(float), &mesh.normals[0], GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glGenBuffers(1, &object.index_buffer.handle);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, object.index_buffer.handle);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, object.index_buffer.count * sizeof(unsigned int), &mesh.indices[0], GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	if (object.has_tex_coords) {
  	glGenBuffers(1, &object.tex_coord_buffer.handle);
  	glBindBuffer(GL_ARRAY_BUFFER, object.tex_coord_buffer.handle);
  	glBufferData(GL_ARRAY_BUFFER, object.tex_coord_buffer.count * sizeof(float), &mesh.tex_coords[0], GL_STATIC_DRAW);
  	glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

  //--- Shader
  if (!object.shader_program.add_shader_from_source_file(GL_VERTEX_SHADER, "simple.vs")) {
    std::cerr << object.shader_program.log() << std::endl;
    return false;
  }
  if (!object.shader_program.add_shader_from_source_file(GL_FRAGMENT_SHADER, "simple.fs")) {
    std::cerr << object.shader_program.log() << std::endl;
    return false;
  }
  if (!object.shader_program.link()) {
    std::cerr << object.shader_program.log() << std::endl;
    return false;
  }

	return true;
}

void draw_mesh_object(const mesh_object_t & object)
{
	GLuint position_location = object.shader_program.attribute_location("vertex_position");
	GLuint normal_location = object.shader_program.attribute_location("vertex_normal");
	GLuint tex_coord_location = object.has_tex_coords ? object.shader_program.attribute_location("vertex_tex_coord") : 0;

	object.shader_program.set_uniform_value("material.diffuse", object.material.diffuse);
	object.shader_program.set_uniform_value("material.specular", object.material.specular);
	object.shader_program.set_uniform_value("material.shininess", object.material.shininess);

  GLsizei stride = 3 * sizeof(float);
  glBindBuffer(GL_ARRAY_BUFFER, object.vertex_buffer.handle);
  glVertexAttribPointer(position_location, 3, GL_FLOAT, GL_FALSE, stride, BUFFER_OFFSET(0));
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glBindBuffer(GL_ARRAY_BUFFER, object.normal_buffer.handle);
  glVertexAttribPointer(normal_location, 3, GL_FLOAT, GL_FALSE, stride, BUFFER_OFFSET(0));
  glBindBuffer(GL_ARRAY_BUFFER, 0);

	if (object.has_tex_coords) {
  	glBindBuffer(GL_ARRAY_BUFFER, object.tex_coord_buffer.handle);
  	glVertexAttribPointer(tex_coord_location, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), BUFFER_OFFSET(0));
  	glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	for (size_t i = 0; i < object.textures.size(); i++) {
		const texture_t &tex = object.textures[i];
		glActiveTexture(texture_unit_names[tex.unit_id]);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, tex.handle);
	}

  glEnableVertexAttribArray(position_location);
  glEnableVertexAttribArray(normal_location);
	if (object.has_tex_coords) glEnableVertexAttribArray(tex_coord_location);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, object.index_buffer.handle);
  glDrawElements(GL_TRIANGLES, object.index_buffer.count, GL_UNSIGNED_INT, BUFFER_OFFSET(0));
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	if (object.has_tex_coords) glDisableVertexAttribArray(tex_coord_location);
  glDisableVertexAttribArray(normal_location);
  glDisableVertexAttribArray(position_location);

	for (size_t i = 0; i < object.textures.size(); i++) {
		const texture_t &tex = object.textures[i];
		glActiveTexture(texture_unit_names[tex.unit_id]);
		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);
	}

}

void draw_teapot(const mesh_object_t &teapot) {
	glm::mat4 model_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.5f, 0.0f));
	glm::mat3 normal_matrix = glm::mat3(glm::transpose(glm::inverse(model_matrix)));
	
	teapot.shader_program.bind();	
  teapot.shader_program.set_uniform_value("model_matrix", model_matrix);
  teapot.shader_program.set_uniform_value("normal_matrix", normal_matrix);
  draw_mesh_object(teapot);
	teapot.shader_program.release();
}

void draw_floor(const mesh_object_t &floor) {
	glm::mat4 model_matrix = glm::scale(glm::mat4(1.0f), glm::vec3(2.0f, 0.05f, 2.0f));
	glm::mat3 normal_matrix = glm::mat3(glm::transpose(glm::inverse(model_matrix)));
	
	floor.shader_program.bind();
  floor.shader_program.set_uniform_value("model_matrix", model_matrix);
  floor.shader_program.set_uniform_value("normal_matrix", normal_matrix);
  draw_mesh_object(floor);
	floor.shader_program.release();
}

bool build_texutre_from_file(const char *filepath, texture_t &tex) {
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

	tex.handle = texture_handle;

	return true;
}

int main(int argc, char **args)
{
  const char *ctm_filepath = (argc > 1) ? args[1] : "teapot.ctm";

  trackball_state.radius = 150.0f;
  trackball_state.dragged = false;
  trackball_state.orientation.w = 1.0f;
  trackball_state.orientation.x = 0.0f;
  trackball_state.orientation.y = 0.0f;
  trackball_state.orientation.z = 0.0f;

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
  glfwSetWindowSizeCallback(resize);
  glfwSetKeyCallback(keyboard);
  glfwSetMouseButtonCallback(mouse);
  glfwSetMousePosCallback(motion);
  glfwSetWindowTitle("Spinning Teapot");
  glfwEnable(GLFW_STICKY_KEYS);
  glfwSwapInterval(1);

	//--- Mesh Objects
	mesh_t mesh_floor;
	load_mesh_cube(mesh_floor);
  mesh_object_t floor;
  if (! build_mesh_object(mesh_floor, floor) ) {
    glfwTerminate();
    exit(EXIT_FAILURE);	
	}

	mesh_t mesh_teapot;
	if (! load_mesh_from_file(ctm_filepath, mesh_teapot) ) {
	  glfwTerminate();
    exit(EXIT_FAILURE);		
	}
  mesh_object_t teapot;
  if (! build_mesh_object(mesh_teapot, teapot) ) {
	  glfwTerminate();
    exit(EXIT_FAILURE);
	}

  //--- Texture
	const char *texture_filepath = "checker.tga";
	texture_t tex;
	if (! build_texutre_from_file(texture_filepath, tex)) {
		std::cout << "Failed to load texture: " << texture_filepath << std::endl;
	  glfwTerminate();
	  exit(EXIT_FAILURE);		
	}
	tex.unit_id = 1;

	// Scene settings
  glm::vec3 eye(0.0f, 0.0f, 5.0f);
  glm::vec3 center(0.0f, 0.0f, 0.0f);
  glm::vec3 up(0.0f, 1.0f, 0.0f);
  glm::mat4 view_matrix = glm::lookAt(eye, center, up); // from world to camera
  glm::vec3 light_position = glm::vec3(0.0f, 3.0f, 3.0f); // light position in world space

	teapot.material.diffuse = glm::vec3(0.0f, 1.0f, 1.0f);
	teapot.material.specular = glm::vec3(0.8f);
	teapot.material.shininess = 128.0f;
	teapot.shader_program.set_uniform_value("texture0", tex.unit_id); 
	teapot.textures.push_back(tex);
	
	floor.material.diffuse = glm::vec3(1.0f, 1.0f, 1.0f);
	floor.material.specular = glm::vec3(0.8f);
	floor.material.shininess = 2.0f;

  //--- Renering Modes
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);

  do {
    glClearColor(0.5f, 0.5f, 0.5f, 0.0f);
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glViewport(0, 0, screen_width, screen_height);

		glm::mat4 rotation = glm::mat4_cast(trackball_state.orientation);
		
    glm::mat4 projection_matrix = glm::perspective(camera_fovy, (float) screen_width / (float) screen_height, 1.0f, 30.0f);
		glm::mat4 _view_matrix = view_matrix * rotation;
		glm::vec3 _light_position = light_position;
		// glm::mat4 _view_matrix = view_matrix;
		// glm::vec3 _light_position = light_position * glm::transpose(glm::mat3(rotation));

		teapot.shader_program.bind();		
		teapot.shader_program.set_uniform_value("light_position", _light_position);
    teapot.shader_program.set_uniform_value("projection_matrix", projection_matrix);
    teapot.shader_program.set_uniform_value("view_matrix", _view_matrix);
		draw_teapot(teapot);
		teapot.shader_program.release();

		floor.shader_program.bind();		
		floor.shader_program.set_uniform_value("light_position", _light_position);
    floor.shader_program.set_uniform_value("projection_matrix", projection_matrix);
    floor.shader_program.set_uniform_value("view_matrix", _view_matrix);
    draw_floor(floor);
		floor.shader_program.release();

    glfwSwapBuffers();

  }
  while (glfwGetKey(GLFW_KEY_ESC) != GLFW_PRESS && glfwGetWindowParam(GLFW_OPENED));

  glfwTerminate();

  return 0;
}
