
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
  std::vector < float >vertices;
   std::vector < float >normals;
   std::vector < unsigned int >indices;
   std::vector < float >tex_coords;
};

struct vertex_attribute_handles_t {
  GLuint position;
  GLuint normal;
  GLuint tex_coord;
};

struct teapot_t {
  GLuint vertex_buffer_handle;
  GLuint normal_buffer_handle;
  GLuint index_buffer_handle;
  GLuint tex_coord_buffer_handle;
  unsigned int vertex_count;
  unsigned int index_count;

  shader_program_t shader_program;
  vertex_attribute_handles_t vertex_attributes;
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

#define ARRAY_BUFFER_SIZE(container, data_type)  ((container).size()*sizeof(data_type))

bool load_teapot(const char *ctm_filepath, teapot_t & teapot)
{
  mesh_t mesh;
  if (!load_mesh(ctm_filepath, mesh))
    return false;

  teapot.vertex_count = mesh.vertices.size() / 3;
  teapot.index_count = mesh.indices.size();

  //--- Buffers
  glGenBuffers(1, &teapot.vertex_buffer_handle);
  glBindBuffer(GL_ARRAY_BUFFER, teapot.vertex_buffer_handle);
  glBufferData(GL_ARRAY_BUFFER, ARRAY_BUFFER_SIZE(mesh.vertices, float), &mesh.vertices[0], GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glGenBuffers(1, &teapot.normal_buffer_handle);
  glBindBuffer(GL_ARRAY_BUFFER, teapot.normal_buffer_handle);
  glBufferData(GL_ARRAY_BUFFER, ARRAY_BUFFER_SIZE(mesh.normals, float), &mesh.normals[0], GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glGenBuffers(1, &teapot.index_buffer_handle);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, teapot.index_buffer_handle);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, ARRAY_BUFFER_SIZE(mesh.indices, unsigned int), &mesh.indices[0], GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

  glGenBuffers(1, &teapot.tex_coord_buffer_handle);
  glBindBuffer(GL_ARRAY_BUFFER, teapot.tex_coord_buffer_handle);
  glBufferData(GL_ARRAY_BUFFER, ARRAY_BUFFER_SIZE(mesh.tex_coords, float), &mesh.tex_coords[0], GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  //--- Shader
  if (!teapot.shader_program.add_shader_from_source_file(GL_VERTEX_SHADER, "simple.vs")) {
    std::cerr << teapot.shader_program.log() << std::endl;
    return false;
  }
  if (!teapot.shader_program.add_shader_from_source_file(GL_FRAGMENT_SHADER, "simple.fs")) {
    std::cerr << teapot.shader_program.log() << std::endl;
    return false;
  }
  if (!teapot.shader_program.link()) {
    std::cerr << teapot.shader_program.log() << std::endl;
    return false;
  }

  teapot.shader_program.bind();
  teapot.vertex_attributes.position = teapot.shader_program.attribute_location("position");
  teapot.vertex_attributes.normal = teapot.shader_program.attribute_location("normal");
  teapot.vertex_attributes.tex_coord = teapot.shader_program.attribute_location("tex_coord");
  teapot.shader_program.release();

  return true;
}

void draw_teapot(const teapot_t & teapot)
{
  const vertex_attribute_handles_t *vertex_attributes = &(teapot.vertex_attributes);

  GLsizei stride = 3 * sizeof(float);
  glBindBuffer(GL_ARRAY_BUFFER, teapot.vertex_buffer_handle);
  glVertexAttribPointer(vertex_attributes->position, 3, GL_FLOAT, GL_FALSE, stride, BUFFER_OFFSET(0));
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glBindBuffer(GL_ARRAY_BUFFER, teapot.normal_buffer_handle);
  glVertexAttribPointer(vertex_attributes->normal, 3, GL_FLOAT, GL_FALSE, stride, BUFFER_OFFSET(0));
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glBindBuffer(GL_ARRAY_BUFFER, teapot.tex_coord_buffer_handle);
  glVertexAttribPointer(vertex_attributes->tex_coord, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), BUFFER_OFFSET(0));
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glEnableVertexAttribArray(vertex_attributes->position);
  glEnableVertexAttribArray(vertex_attributes->normal);
  glEnableVertexAttribArray(vertex_attributes->tex_coord);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, teapot.index_buffer_handle);
  glDrawElements(GL_TRIANGLES, teapot.index_count, GL_UNSIGNED_INT, BUFFER_OFFSET(0));
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  glDisableVertexAttribArray(vertex_attributes->tex_coord);
  glDisableVertexAttribArray(vertex_attributes->normal);
  glDisableVertexAttribArray(vertex_attributes->position);

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

  teapot_t teapot;
  load_teapot(ctm_filepath, teapot);

  //--- Texture
  const char *texture_filepath = "checker.tga";
  GLuint texture_id;
  glGenTextures(1, &texture_id);
  glBindTexture(GL_TEXTURE_2D, texture_id);
  if (!glfwLoadTexture2D(texture_filepath, GLFW_BUILD_MIPMAPS_BIT)) {
    std::cout << "Failed to load texture: " << texture_filepath << std::endl;
    glfwTerminate();
    exit(EXIT_FAILURE);
  }
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glBindTexture(GL_TEXTURE_2D, 0);

  glm::vec3 eye(0.0f, 0.0f, 5.0f);
  glm::vec3 center(0.0f, 0.0f, 0.0f);
  glm::vec3 up(0.0f, 1.0f, 0.0f);
  glm::mat4 view_matrix = glm::lookAt(eye, center, up); // from world to camera
  glm::vec3 light_direction = glm::normalize(glm::vec3(-1.0f, -1.0f, -1.0f)); // light direction in world space

  teapot.shader_program.bind();
  teapot.shader_program.set_uniform_value("light_direction", light_direction);
  teapot.shader_program.set_uniform_value("texture0", 0); // use texture unit 0

  //--- Renering Modes
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);

  do {
    glClearColor(0.5f, 0.5f, 0.5f, 0.0f);
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glViewport(0, 0, screen_width, screen_height);

    glm::mat4 projection_matrix = glm::perspective(camera_fovy, (float) screen_width / (float) screen_height, 1.0f, 30.0f);
    glm::mat4 rotation = glm::mat4_cast(trackball_state.orientation);
    glm::mat4 model_matrix(1.0f);
    glm::mat3 normal_matrix = glm::mat3(model_matrix); // upper 3x3 matrix of model matrix

    teapot.shader_program.set_uniform_value("projection_matrix", projection_matrix);
    teapot.shader_program.set_uniform_value("view_matrix", view_matrix * rotation);
    teapot.shader_program.set_uniform_value("model_matrix", model_matrix);
    teapot.shader_program.set_uniform_value("normal_matrix", normal_matrix);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    draw_teapot(teapot);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(0);

    glfwSwapBuffers();

  }
  while (glfwGetKey(GLFW_KEY_ESC) != GLFW_PRESS && glfwGetWindowParam(GLFW_OPENED));

  glfwTerminate();

  return 0;
}
