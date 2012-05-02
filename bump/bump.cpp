
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

#define BUFFER_OFFSET(bytes) ((GLubyte *)NULL + (bytes))

struct mesh_t {
  std::vector<float> vertices;
  std::vector<float> normals;
  std::vector<unsigned int> indices;
  std::vector<float> tex_coords;
	std::vector<float> tangents;
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
	array_buffer_t tangent_buffer;
	bool has_tex_coords;
	bool has_tangents;
	std::vector<texture_t> textures;
	shader_program_t *shader_program;
	material_t material;
	glm::mat4 transform_matrix;
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
int screen_width;
int screen_height;
trackball_state_t *current_trackball_state;
trackball_state_t camera_rotation;
trackball_state_t light_rotation;
bool light_rotation_enabled = false;

glm::mat4 __projection_matrix;
glm::mat4 __view_matrix;

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
			int x, y;
      glfwGetMousePos(&x, &y);
			current_trackball_state->prev_position.x = x;
			current_trackball_state->prev_position.y = y;
      current_trackball_state->dragged = true;
      break;
    case GLFW_RELEASE:
      current_trackball_state->dragged = false;
      break;
    }
  }

}

void motion(int x, int y)
{
  if (!current_trackball_state->dragged)
    return;

  glm::ivec2 current_position(x, y);

  if (camera_zoom) {
    glm::vec2 v(current_trackball_state->prev_position.x - current_position.x, current_trackball_state->prev_position.y - current_position.y);
    float delta = glm::length(v);
    if (delta > 0.0f) {
      float direction = glm::sign(glm::dot(v, glm::vec2(0.0f, 1.0f)));
      float theta = direction * glm::clamp(delta, 0.1f, 0.5f);
      camera_fovy = glm::clamp(camera_fovy + theta, 5.0f, 60.0f);
    }
  } else {
    glm::vec3 v0 = map_to_sphere(*current_trackball_state, current_trackball_state->prev_position);
    glm::vec3 v1 = map_to_sphere(*current_trackball_state, current_position);
    glm::vec3 v2 = glm::cross(v0, v1); // calculate rotation axis

    float d = glm::dot(v0, v1);
    float s = std::sqrt((1.0f + d) * 2.0f);
    glm::quat q(0.5f * s, v2 / s);
    current_trackball_state->orientation = q * current_trackball_state->orientation;
    current_trackball_state->orientation /= glm::length(current_trackball_state->orientation);
  }

  current_trackball_state->prev_position.x = x;
  current_trackball_state->prev_position.y = y;
}

void resize(int width, int height)
{
  height = height > 0 ? height : 1;
  screen_width = width;
  screen_height = height;

  camera_rotation.center_position.x = light_rotation.center_position.x = 0.5 * width;
  camera_rotation.center_position.y = light_rotation.center_position.y = 0.5 * height;
}

void keyboard(int key, int action)
{
  switch (action) {
  case GLFW_PRESS:
    if (key == GLFW_KEY_LSHIFT) {
      camera_zoom = true;
    } else if (key == GLFW_KEY_SPACE) {
			light_rotation_enabled = !light_rotation_enabled;
			current_trackball_state = light_rotation_enabled ? &light_rotation : &camera_rotation;
		}
    break;
  case GLFW_RELEASE:
    camera_zoom = false;
    break;
  }
}

void compute_tangent_vectors(mesh_t &mesh) {	
	int vertex_count = mesh.vertices.size() / 3;
	int face_count = mesh.indices.size() / 3;
	
	std::vector<glm::vec3> tangents;
	tangents.resize(vertex_count);
	
	int *tangent_counts = new int[vertex_count];
	for (int i = 0; i < vertex_count; i++) tangent_counts[i] = 0;
	
	std::vector<float> &vertices = mesh.vertices;
	std::vector<float> &normals = mesh.normals;
	std::vector<float> &tex_coords = mesh.tex_coords;
	std::vector<unsigned int> &indices = mesh.indices;
	
	for (int i = 0; i < face_count; i++) {
		int i0 = indices[3*i];
		int i1 = indices[3*i + 1];
		int i2 = indices[3*i + 2];
			
		int j0 = 2*i0;
		int j1 = 2*i1;
		int j2 = 2*i2;
		glm::vec2 uv0(tex_coords[j0], tex_coords[j0 + 1]);
		glm::vec2 uv1(tex_coords[j1], tex_coords[j1 + 1]);
		glm::vec2 uv2(tex_coords[j2], tex_coords[j2 + 1]);
		glm::vec2 st1 = uv1 - uv0;
		glm::vec2 st2 = uv2 - uv0;
		
		float det = st1.x*st2.y - st2.x*st1.y;
		if (det == 0.0f) continue; // pass if inverse matrix does not exist
		// assert(det != 0.0f);

		int k0 = 3*i0;
		int k1 = 3*i1;
		int k2 = 3*i2;		
		glm::vec3 p0(vertices[k0], vertices[k0 + 1], vertices[k0 + 2]);
		glm::vec3 p1(vertices[k1], vertices[k1 + 1], vertices[k1 + 2]);
		glm::vec3 p2(vertices[k2], vertices[k2 + 1], vertices[k2 + 2]);		
		glm::vec3 q1 = p1 - p0;
		glm::vec3 q2 = p2 - p0;
		
		float coef = 1.0f / det;
		glm::mat3 m(
			q1.x, q2.x, 0.0f,
			q1.y, q2.y, 0.0f,
			q1.z, q2.z, 0.0f
		);
		glm::vec3 t = coef * glm::vec3(st2.y, -st1.y, 0.0f) * m;

		tangents[i0] += t;
		tangents[i1] += t;
		tangents[i2] += t;
		
		tangent_counts[i0]++;
		tangent_counts[i1]++;
		tangent_counts[i2]++;
	}
	
	mesh.tangents.resize(3*vertex_count);
	
	for (int i = 0; i < vertex_count; i++) {
		const glm::vec3 n(normals[3*i], normals[3*i + 1], normals[3*i + 2]);
		const glm::vec3 t = tangent_counts[i] > 0 ? tangents[i] / (float)tangent_counts[i] : tangents[i];
		tangents[i] = glm::normalize(t - glm::dot(n, t) * n);

		mesh.tangents[3*i] = tangents[i].x;
		mesh.tangents[3*i + 1] = tangents[i].y;
		mesh.tangents[3*i + 2] = tangents[i].z;

	}
	
	delete [] tangent_counts;
	
}

bool load_mesh_plane(mesh_t &mesh) {
	float vertices[][3] = {
		{ 0.00, 0.00, 0.00 },
		{ 1.00, 0.00, 0.00 },
		{ 0.00, 1.00, 0.00 },
		{ 1.00, 1.00, 0.00 }
	};

	float normals[][3] = {
		{ 0.00, 0.00, 1.00 },
		{ 0.00, 0.00, 1.00 },
		{ 0.00, 0.00, 1.00 },
		{ 0.00, 0.00, 1.00 }
	};

	unsigned int indices[][3] = {
		{ 0, 1, 2 },
		{ 1, 3, 2 }
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

bool build_shader_program(shader_program_t &shader_program, const char *vertex_shader_filepath, const char *fragment_shader_filepath) {
  if (!shader_program.add_shader_from_source_file(GL_VERTEX_SHADER, vertex_shader_filepath)) {
		std::cerr << "*** " << vertex_shader_filepath << std::endl;
    std::cerr << shader_program.log() << std::endl;
    return false;
  }
  if (!shader_program.add_shader_from_source_file(GL_FRAGMENT_SHADER, fragment_shader_filepath)) {
		std::cerr << "*** " << fragment_shader_filepath << std::endl;
    std::cerr << shader_program.log() << std::endl;
    return false;
  }
  if (!shader_program.link()) {
		std::cerr << "*** " << vertex_shader_filepath << " and " << fragment_shader_filepath << std::endl;
    std::cerr << shader_program.log() << std::endl;
    return false;
  }	

	return true;
}

bool build_mesh_object(const mesh_t &mesh, mesh_object_t &object) {

  object.vertex_buffer.count = mesh.vertices.size();
	object.normal_buffer.count = mesh.normals.size();
	object.index_buffer.count = mesh.indices.size();
	object.tex_coord_buffer.count = mesh.tex_coords.size();
	object.tangent_buffer.count = mesh.tangents.size();
	
	object.has_tex_coords = object.tex_coord_buffer.count > 0;
	object.has_tangents = object.tangent_buffer.count > 0;

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
	
	if (object.has_tangents) {
  	glGenBuffers(1, &object.tangent_buffer.handle);
  	glBindBuffer(GL_ARRAY_BUFFER, object.tangent_buffer.handle);
  	glBufferData(GL_ARRAY_BUFFER, object.tangent_buffer.count * sizeof(float), &mesh.tangents[0], GL_STATIC_DRAW);
  	glBindBuffer(GL_ARRAY_BUFFER, 0);		
	}
	
	object.shader_program = NULL;

	return true;
}

void render_object(const mesh_object_t & object)
{
	assert(object.shader_program != NULL);
	
	GLuint position_location = object.shader_program->attribute_location("vertex_position");
	GLuint normal_location = object.shader_program->attribute_location("vertex_normal");
	GLuint tex_coord_location = object.has_tex_coords ? object.shader_program->attribute_location("vertex_tex_coord") : 0;
	GLuint tangent_location = object.has_tangents ? object.shader_program->attribute_location("vertex_tangent") : 0; 

	object.shader_program->set_uniform_value("projection_matrix", __projection_matrix);
	
	glm::mat4 view_inverse_matrix = glm::inverse(__view_matrix);
	glm::mat4 model_view_matrix = __view_matrix * object.transform_matrix;
	glm::mat3 normal_matrix = glm::mat3(glm::transpose(glm::inverse(model_view_matrix)));
	object.shader_program->set_uniform_value("model_view_matrix", model_view_matrix);
	object.shader_program->set_uniform_value("view_matrix", __view_matrix);
	object.shader_program->set_uniform_value("view_inverse_matrix", view_inverse_matrix);
	object.shader_program->set_uniform_value("normal_matrix", normal_matrix);

	object.shader_program->set_uniform_value("material.diffuse", object.material.diffuse);
	object.shader_program->set_uniform_value("material.specular", object.material.specular);
	object.shader_program->set_uniform_value("material.shininess", object.material.shininess);

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
	
	if (object.has_tangents) {
  	glBindBuffer(GL_ARRAY_BUFFER, object.tangent_buffer.handle);
  	glVertexAttribPointer(tangent_location, 3, GL_FLOAT, GL_FALSE, stride, BUFFER_OFFSET(0));
  	glBindBuffer(GL_ARRAY_BUFFER, 0);		
	}

	for (size_t i = 0; i < object.textures.size(); i++) {
		const texture_t &tex = object.textures[i];
		glActiveTexture(texture_unit_names[tex.unit_id]);
		glBindTexture(GL_TEXTURE_2D, tex.handle);
	}

  glEnableVertexAttribArray(position_location);
  glEnableVertexAttribArray(normal_location);
	if (object.has_tex_coords) glEnableVertexAttribArray(tex_coord_location);
	if (object.has_tangents) glEnableVertexAttribArray(tangent_location);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, object.index_buffer.handle);
  glDrawElements(GL_TRIANGLES, object.index_buffer.count, GL_UNSIGNED_INT, BUFFER_OFFSET(0));
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	if (object.has_tangents) glDisableVertexAttribArray(tangent_location);
	if (object.has_tex_coords) glDisableVertexAttribArray(tex_coord_location);
  glDisableVertexAttribArray(normal_location);
  glDisableVertexAttribArray(position_location);

	for (size_t i = 0; i < object.textures.size(); i++) {
		const texture_t &tex = object.textures[i];
		glActiveTexture(texture_unit_names[tex.unit_id]);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

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

bool create_framebuffer_and_depth_texture(size_t width, size_t height, texture_t &tex, GLuint *fb_handle) {
  glGenTextures(1, &tex.handle);
  glBindTexture(GL_TEXTURE_2D, tex.handle);   
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, width, height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, 0);  
	// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
	// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
	glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);  
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);  
  glBindTexture(GL_TEXTURE_2D, 0);

	GLuint handle;
	glGenFramebuffersEXT(1, &handle);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, handle);
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, tex.handle, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);		
	*fb_handle = handle;
	
	return true;
}

void trackback_state_initialize(trackball_state_t &trackball_state) {
  trackball_state.radius = 150.0f;
  trackball_state.dragged = false;
  trackball_state.orientation.w = 1.0f;
  trackball_state.orientation.x = 0.0f;
  trackball_state.orientation.y = 0.0f;
  trackball_state.orientation.z = 0.0f;	
}

// #define DEPTH_BUFFER_DEBUG
#define USE_BUMP_MAPPING

int main(int argc, char **args)
{
  const char *ctm_filepath = (argc > 1) ? args[1] : "teapot.ctm";

	trackback_state_initialize(camera_rotation);
	trackback_state_initialize(light_rotation);
	current_trackball_state = &camera_rotation;

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

	// Shaders
	shader_program_t phong_shader;
	build_shader_program(phong_shader, "phong.vs", "phong.fs");
	shader_program_t rect_shader;
	build_shader_program(rect_shader, "rect.vs", "rect.fs");
	shader_program_t render_buffer_shader;
	build_shader_program(render_buffer_shader, "render_buffer.vs", "render_buffer.fs");
	shader_program_t bump_shader;
	build_shader_program(bump_shader, "bump.vs", "bump.fs");

	//--- Mesh Objects
	mesh_t mesh_floor;
	load_mesh_cube(mesh_floor);
  mesh_object_t floor;
  if (! build_mesh_object(mesh_floor, floor) ) {
    glfwTerminate();
    exit(EXIT_FAILURE);	
	}

	mesh_t mesh_plane;
	load_mesh_plane(mesh_plane);
  mesh_object_t plane;
  if (! build_mesh_object(mesh_plane, plane) ) {
    glfwTerminate();
    exit(EXIT_FAILURE);	
	}

	mesh_t mesh_teapot;
	if (! load_mesh_from_file(ctm_filepath, mesh_teapot) ) {
	  glfwTerminate();
    exit(EXIT_FAILURE);		
	}
	compute_tangent_vectors(mesh_teapot);
  mesh_object_t teapot;
  if (! build_mesh_object(mesh_teapot, teapot) ) {
	  glfwTerminate();
    exit(EXIT_FAILURE);
	}

  //--- Texture
	const char *texture_filepath = "checker.tga";
	texture_t tex;
	tex.unit_id = 1;
	if (! build_texutre_from_file(texture_filepath, tex)) {
		std::cout << "Failed to load texture: " << texture_filepath << std::endl;
	  glfwTerminate();
	  exit(EXIT_FAILURE);		
	}

	//--- FBO
	texture_t depth_tex_buffer;
	int depth_tex_width = 2 * screen_width;
	int depth_tex_height = 2 * screen_height;
	depth_tex_buffer.unit_id = 2;
	GLuint fb_handle;
	create_framebuffer_and_depth_texture(depth_tex_width, depth_tex_height, depth_tex_buffer, &fb_handle);

	// Scene settings
  glm::vec3 camera_position(0.0f, 0.0f, 5.0f);
  glm::vec3 camera_center(0.0f, 0.0f, 0.0f);
  glm::vec3 camera_up(0.0f, 1.0f, 0.0f);
  glm::mat4 view_matrix = glm::lookAt(camera_position, camera_center, camera_up); // from world to camera

	glm::mat4 light_pov_matrix;	
	glm::mat4 bias(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.5f, 0.0f,
		0.5f, 0.5f, 0.5f, 1.0f
		);

	teapot.material.diffuse = glm::vec3(0.0f, 1.0f, 1.0f);
	teapot.material.specular = glm::vec3(0.8f);
	teapot.material.shininess = 128.0f;	
	teapot.textures.push_back(tex);	
	teapot.textures.push_back(depth_tex_buffer);
	
	floor.material.diffuse = glm::vec3(1.0f, 1.0f, 1.0f);
	floor.material.specular = glm::vec3(0.8f);
	floor.material.shininess = 2.0f;
	floor.textures.push_back(depth_tex_buffer);
	
	plane.textures.push_back(depth_tex_buffer);

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

  do {
		//--- Transform
		glm::vec3 light_position = glm::mat3_cast(light_rotation.orientation) * glm::vec3(0.0f, 5.0f, 0.0f);
		glm::vec3 light_center(0.0f, 0.0f, 0.0f);
		glm::vec3 light_up(0.0f, 0.0f, 1.0f);
		glm::mat4 light_view_matrix = glm::lookAt(light_position, light_center, light_up);
		
		teapot.transform_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.5f, 0.0f));
		floor.transform_matrix = glm::scale(glm::mat4(1.0f), glm::vec3(2.0f, 0.05f, 2.0f));
		plane.transform_matrix = glm::scale(glm::mat4(1.0f), glm::vec3(screen_width, screen_height, 1.0f));
		
		//--- Render
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fb_handle);
		{
	    __projection_matrix = glm::perspective(30.0f, (float) screen_width / (float) screen_height, 0.5f, 30.0f);
			__view_matrix = light_view_matrix;
			light_pov_matrix = bias * __projection_matrix * light_view_matrix;
			
			glClear(GL_DEPTH_BUFFER_BIT);
			glClearDepth(1.0f);
			glViewport(0, 0, depth_tex_width, depth_tex_height);
			teapot.shader_program = &render_buffer_shader;
			teapot.shader_program->bind();
			render_object(teapot);
			teapot.shader_program->release();
			floor.shader_program = &render_buffer_shader;
			floor.shader_program->bind();
    	render_object(floor);
			floor.shader_program->release();		
		}
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);		
	
#ifdef DEPTH_BUFFER_DEBUG
		{
			__projection_matrix = glm::ortho(0.0f, (float)screen_width, 0.0f, (float)screen_height, 0.5f, 1.0f);
			__view_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -1.0f));
			
			glDisable(GL_DEPTH_TEST);
			glClear(GL_COLOR_BUFFER_BIT);		
    	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    	glViewport(0, 0, screen_width, screen_height);	
			plane.shader_program = &rect_shader;
			plane.shader_program->bind();	
			plane.shader_program->set_uniform_value("texture2", depth_tex_buffer.unit_id); 
	  	render_object(plane);
			plane.shader_program->release();
			glEnable(GL_DEPTH_TEST);
		}
#endif

#ifndef DEPTH_BUFFER_DEBUG
		{			
			glCullFace(GL_BACK);
			glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
			glClearDepth(1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glViewport(0, 0, screen_width, screen_height);

	    __projection_matrix = glm::perspective(camera_fovy, (float) screen_width / (float) screen_height, 1.0f, 30.0f);
			__view_matrix = view_matrix * glm::mat4_cast(camera_rotation.orientation);

#ifdef USE_BUMP_MAPPING
			teapot.shader_program = &bump_shader;
			teapot.shader_program->bind();		
			teapot.shader_program->set_uniform_value("light_world_position", light_position);
			teapot.shader_program->set_uniform_value("surface_color", glm::vec3(0.7f, 0.6f, 0.18f));
			teapot.shader_program->set_uniform_value("bump_density", 16.0f);
			teapot.shader_program->set_uniform_value("bump_size", 0.15f);
			teapot.shader_program->set_uniform_value("specular_factor", 0.5f);
			render_object(teapot);
			teapot.shader_program->release();
#else
			teapot.shader_program = &phong_shader;
			teapot.shader_program->bind();		
			teapot.shader_program->set_uniform_value("light_position", light_position);
			teapot.shader_program->set_uniform_value("light_pov_matrix", light_pov_matrix);
			teapot.shader_program->set_uniform_value("texture1", tex.unit_id); 
			teapot.shader_program->set_uniform_value("texture2", depth_tex_buffer.unit_id); 
			render_object(teapot);
			teapot.shader_program->release();
#endif

			floor.shader_program = &phong_shader;
			floor.shader_program->bind();	
			floor.shader_program->set_uniform_value("light_world_position", light_position);
			floor.shader_program->set_uniform_value("light_pov_matrix", light_pov_matrix);
			floor.shader_program->set_uniform_value("texture2", depth_tex_buffer.unit_id); 
	    render_object(floor);
			floor.shader_program->release();	
		}
#endif

    glfwSwapBuffers();

  }
  while (glfwGetKey(GLFW_KEY_ESC) != GLFW_PRESS && glfwGetWindowParam(GLFW_OPENED));

  glfwTerminate();

  return 0;
}
