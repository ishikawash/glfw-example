
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <GL/glfw.h>

#define BUFFER_OFFSET(bytes) ((GLubyte *)NULL + (bytes))


struct drawable_t {
  GLuint vertex_buffer_handle;
  GLuint index_buffer_handle;
};

struct attribute_handle_t {
  GLuint position;
  GLuint color;
};

struct uniform_handle_t {
  GLuint projection_matrix;
  GLuint view_matrix;
  GLuint model_matrix;
};

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

int main(void)
{
  int width, height, x;
  double t;

  if (!glfwInit()) {
    std::cerr << "Failed to initialize GLFW" << std::endl;
    exit(EXIT_FAILURE);
  }

  if (!glfwOpenWindow(640, 480, 0, 0, 0, 0, 0, 0, GLFW_WINDOW)) {
    std::cerr << "Failed to open GLFW window" << std::endl;

    glfwTerminate();
    exit(EXIT_FAILURE);
  }

  glfwSetWindowTitle("Spinning Triangle");

  glfwEnable(GLFW_STICKY_KEYS);

  glfwSwapInterval(1);

  unsigned int vertex_count = 3;
  float vertices[][3] = {
    {-5.0f, 0.0f, -4.0f},
    {5.0f, 0.0f, -4.0f},
    {0.0f, 0.0f, 6.0f}
  };
  float colors[][3] = {
    {1.0f, 0.0f, 0.0f},
    {0.0f, 1.0f, 0.0f},
    {0.0f, 0.0f, 1.0f}
  };
  unsigned int face_count = 1;
  unsigned int index_count = vertex_count * face_count;
  unsigned int faces[][3] = {
    {0, 1, 2}
  };

  drawable_t drawable;
  glGenBuffers(1, &drawable.vertex_buffer_handle);
  glBindBuffer(GL_ARRAY_BUFFER, drawable.vertex_buffer_handle);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices) + sizeof(colors), NULL, GL_STATIC_DRAW); // pointer to data must be NULL
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
  glBufferSubData(GL_ARRAY_BUFFER, sizeof(vertices), sizeof(colors), colors);

  glGenBuffers(1, &drawable.index_buffer_handle);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, drawable.index_buffer_handle);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_count * sizeof(unsigned int), faces, GL_STATIC_DRAW);

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
  attribute.color = glGetAttribLocation(program_handle, "color");
  glEnableVertexAttribArray(attribute.position);
  glEnableVertexAttribArray(attribute.color);

  uniform_handle_t uniform;
  uniform.projection_matrix = glGetUniformLocation(program_handle, "projection_matrix");
  uniform.view_matrix = glGetUniformLocation(program_handle, "view_matrix");
  uniform.model_matrix = glGetUniformLocation(program_handle, "model_matrix");

  glm::vec3 eye(0.0f, 1.0f, 0.0f);
  glm::vec3 center(0.0f, 20.0f, 0.0f);
  glm::vec3 up(0.0f, 0.0f, 1.0f);
  glm::mat4 view_matrix = glm::lookAt(eye, center, up); // from world to camera
  glUniformMatrix4fv(uniform.view_matrix, 1, 0, glm::value_ptr(view_matrix));

  do {
    t = glfwGetTime();
    glfwGetMousePos(&x, NULL);

    glfwGetWindowSize(&width, &height);

    height = height > 0 ? height : 1;

    glViewport(0, 0, width, height);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glm::mat4 projection_matrix = glm::perspective(65.0f, (float) width / (float) height, 1.0f, 100.f);
    glUniformMatrix4fv(uniform.projection_matrix, 1, 0, glm::value_ptr(projection_matrix));

    float theta = 0.3f * (float) x + (float) t * 100.0f;
    glm::mat4 model_translation = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 14.0f, 0.0f));
    glm::mat4 model_rotation = glm::rotate(glm::mat4(1.0f), theta, glm::vec3(0.0f, 0.0f, 1.0f));
    glm::mat4 model_matrix = model_translation * model_rotation; // from local to world
    glUniformMatrix4fv(uniform.model_matrix, 1, 0, glm::value_ptr(model_matrix));

    glBindBuffer(GL_ARRAY_BUFFER, drawable.vertex_buffer_handle);
    glVertexAttribPointer(attribute.position, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
    glVertexAttribPointer(attribute.color, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), BUFFER_OFFSET(sizeof(vertices)));

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, drawable.index_buffer_handle);
    glDrawElements(GL_TRIANGLES, face_count * 3, GL_UNSIGNED_INT, BUFFER_OFFSET(0));

    glfwSwapBuffers();

  }
  while (glfwGetKey(GLFW_KEY_ESC) != GLFW_PRESS && glfwGetWindowParam(GLFW_OPENED));

  glfwTerminate();

  return 0;
}
