
#include <iostream>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <fstream>
#include <string>

#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#include <GL/glfw.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <openctmpp.h>

#define PNG_DEBUG 3
#include <png.h>

#include "shader.hpp"
#include "mesh.hpp"
#include "fbo.hpp"
#include "texture.hpp"
#include "trackball.hpp"

struct image_t {
	GLenum format;
	size_t width;
	size_t height;
	size_t byte_depth;
	unsigned char *data;
};

struct model_t {
	mesh_t *mesh;
	glm::vec3 position;
	glm::vec3 scale;
	glm::quat orientation;
};

struct camera_t {
	float fovy;
	float aspect_ratio;
	glm::quat orientation;
	
	glm::mat4 projection_matrix;
	glm::mat4 view_inverse_matrix;
};
 
glm::vec4 light_position;
glm::vec2 parallax_scale_bias;

model_t teapot;
model_t board;
shader_program_t normal_map_shader;
frame_buffer_t *fbo;

glm::ivec2 viewport;
camera_t camera;

texture_t image_texture;
texture_t normal_texture;
texture_t height_texture;
texture_t color_texture;
texture_t depth_texture;

trackball_t trackball(200.0f);

bool camera_zoom = false;
bool parallax_mapping_enabled = true;

void log(const char *format, ...) {
  va_list args;
  va_start(args, format);
  std::vfprintf(stderr, format, args);
  std::fprintf(stderr, "\n");
  va_end(args);
}

// ref: http://zarb.org/~gc/html/libpng.html
bool read_image_from_png_file(const char *filepath, image_t &image) {
  FILE *fp = fopen(filepath, "rb");
  if (!fp) {
    log("[read_png_file] File %s could not be opened for reading", filepath);
		return false;
	}

  unsigned char header[8]; 
  fread(header, 1, 8, fp);
  if (png_sig_cmp(header, 0, 8)) {
    log("[read_png_file] File %s is not recognized as a PNG file", filepath);
		return false;
	}

  png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png_ptr) {
    log("[read_png_file] png_create_read_struct failed");
		return false;
	}

  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr) {
    log("[read_png_file] png_create_info_struct failed");
		return false;
	}

  if (setjmp(png_jmpbuf(png_ptr))) {
    log("[read_png_file] Error during init_io");
		return false;
	}

  png_init_io(png_ptr, fp);
  png_set_sig_bytes(png_ptr, 8);

  png_read_info(png_ptr, info_ptr);

  int width = png_get_image_width(png_ptr, info_ptr);
  int height = png_get_image_height(png_ptr, info_ptr);
  png_byte color_type = png_get_color_type(png_ptr, info_ptr);
  png_byte bit_depth = png_get_bit_depth(png_ptr, info_ptr);

  png_set_interlace_handling(png_ptr);
  png_read_update_info(png_ptr, info_ptr);

  if (setjmp(png_jmpbuf(png_ptr))) {
    log("[read_png_file] Error during read_image");
		return false;
	}

	png_byte byte_depth = png_get_channels(png_ptr, info_ptr);
	if (bit_depth == 16)
		byte_depth *= 2;

	png_byte *buf = new png_byte[width * height * byte_depth];

	int offset = 0;
  for (int y = 0; y < height; y++) {
		png_read_row(png_ptr, buf + offset, NULL);
		offset += width * byte_depth;
	}

	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	
  fclose(fp);

	image.width = width;
	image.height = height;
	image.byte_depth = byte_depth;
	image.data = (unsigned char *)buf;

  switch (color_type) {
  case PNG_COLOR_TYPE_GRAY:
		image.format = GL_LUMINANCE;
    break;
  case PNG_COLOR_TYPE_RGB:
		image.format = GL_RGB;
    break;
  case PNG_COLOR_TYPE_RGBA:
		image.format = GL_RGBA;
    break;
  case PNG_COLOR_TYPE_GA:
		image.format = GL_LUMINANCE_ALPHA;
    break;
  default:
    log("Non-support color type");
		return false;
  }

	return true;
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
	image_t image;
	if (! read_image_from_png_file(filepath, image) )
		return false;

	GLuint texture_handle;
	glGenTextures(1, &texture_handle);
	glBindTexture(GL_TEXTURE_2D, texture_handle);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image.width, image.height, 0, GL_RGB, GL_UNSIGNED_BYTE, (GLvoid *)image.data);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glBindTexture(GL_TEXTURE_2D, 0);

	texture.handle = texture_handle;
	texture.target = GL_TEXTURE_2D;

	return true;
}

void debug_draw_texture(GLuint texture_handle) {
	glEnable(GL_TEXTURE_2D);
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

void model_setup() {
	teapot.mesh = new mesh_t();
	mesh_t::read_from_file("assets/mesh/teapot.ctm", *(teapot.mesh));
	teapot.mesh->load_to_buffers();
	teapot.position.y = 0.5f;
	teapot.scale.x = teapot.scale.y = teapot.scale.z = 1.0f;
	
	board.mesh = new mesh_t();
	mesh_t::read_from_file("assets/mesh/quad.ctm", *(board.mesh));
	board.mesh->load_to_buffers();
	board.scale.x = board.scale.z = 1.5f;
	board.scale.y = 1.0f;
}

void camera_setup() {	
	camera.fovy = 30.0f;
	camera.aspect_ratio = (float) viewport.x / (float) viewport.y;
}

void shader_setup() {		
	light_position = glm::vec4(10.0f, 10.0f, 0.0f, 1.0f); // in world space
	
	parallax_scale_bias.r = 0.04f;
	parallax_scale_bias.g = 0.02f;
	
	shader_program_t::build(normal_map_shader, "assets/shader/normal_map.vs", "assets/shader/normal_map.fs");	
	normal_map_shader.bind();
	normal_map_shader.set_uniform_value("texcoord_scale", 2.0f);
	normal_map_shader.set_uniform_value("texture1", 1);
	normal_map_shader.set_uniform_value("texture2", 2);
	normal_map_shader.set_uniform_value("texture3", 3);
	normal_map_shader.set_uniform_value("specular_color", glm::vec3(0.3f));
	normal_map_shader.set_uniform_value("specular_power", 100.0f);
	normal_map_shader.release();
}

void texture_setup() {	
	build_image_texutre(image_texture, "assets/image/polkadots.png");
	build_image_texutre(normal_texture, "assets/image/polkadots_normal.png");
	build_image_texutre(height_texture, "assets/image/polkadots_height.png");
	
	build_color_texture(color_texture, viewport.x, viewport.y);
	build_depth_texture(depth_texture, viewport.x, viewport.y);
	
	texture_unit_t::initialize();
	texture_unit_t::attach(0, &color_texture);
	texture_unit_t::attach(1, &image_texture);
	texture_unit_t::attach(2, &normal_texture);
	texture_unit_t::attach(3, &height_texture);
}

void frame_buffer_setup() {
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
}

void render_model(const model_t &model, const camera_t &camera, const shader_program_t &shader_program) {
	glm::mat4 rotation_matrix = glm::mat4_cast(model.orientation);
	glm::mat4 scale_matrix = glm::scale(glm::mat4(1.0), model.scale);
	glm::mat4 translation_matrix = glm::translate(glm::mat4(1.0), model.position); 
	
	glm::mat4 model_matrix = translation_matrix * scale_matrix * rotation_matrix;
	glm::mat4 model_view_matrix = camera.view_inverse_matrix * model_matrix; 
	glm::mat3 normal_matrix = glm::mat3(glm::transpose(glm::inverse(model_view_matrix)));
	
	shader_program.set_uniform_value("projection_matrix", camera.projection_matrix);
	shader_program.set_uniform_value("model_view_matrix", model_view_matrix);	
	shader_program.set_uniform_value("normal_matrix", normal_matrix);
	
	model.mesh->render(shader_program);
}

void setup() {
	model_setup();	
	camera_setup();
	texture_setup();
	frame_buffer_setup();
	shader_setup();
	
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);	
	glCullFace(GL_BACK);	
}

void cleanup() {
	
}

void update() {
	camera.projection_matrix = glm::perspective(camera.fovy, camera.aspect_ratio, 1.0f, 30.0f);
	camera.view_inverse_matrix = glm::lookAt(glm::vec3(0.0f, 1.5f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::mat4_cast(camera.orientation);	
}

void render() {
	for (size_t i = 0; i < TEXTURE_UNITS.size(); i++) {
		TEXTURE_UNITS[i].activate();
	}
	
	glViewport(0, 0, viewport.x, viewport.y);
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	

	normal_map_shader.bind();
	if (parallax_mapping_enabled)
		normal_map_shader.set_uniform_value("scale_bias", parallax_scale_bias);
	else
		normal_map_shader.set_uniform_value("scale_bias", glm::vec2(0.0f));
	normal_map_shader.set_uniform_value("light_position", camera.view_inverse_matrix * light_position);
	render_model(board, camera, normal_map_shader);
	normal_map_shader.release();

	// debug_draw_texture(color_texture.handle);

	for (size_t i = 0; i < TEXTURE_UNITS.size(); i++) {
		TEXTURE_UNITS[i].deactivate();
	}

}

void mouse_button(int button, int action) {
	if (button != GLFW_MOUSE_BUTTON_LEFT)
		return;
	
	switch (action) {
		case GLFW_PRESS:
			int x, y;
     	glfwGetMousePos(&x, &y);
			trackball.drag_start(x, y);
			break;
		case GLFW_RELEASE:
			trackball.drag_end();
			break;
   }
}

void mouse_motion(int x, int y) {
	if (! trackball.dragged()) 
		return;
	
	if (camera_zoom) {
		float theta = glm::clamp(0.5f * trackball.direction(x, y).y, -0.5f, 0.5f);
		camera.fovy += theta;
	} else {
		trackball.rotate(camera.orientation, x, y);
	}
		
	trackball.drag_update(x, y);	
}

void keyboard(int key, int action) {
  switch (action) {
  case GLFW_PRESS:
    if (key == GLFW_KEY_LSHIFT) {
      camera_zoom = true;
		}
		if (key == GLFW_KEY_SPACE) {
			parallax_mapping_enabled = !parallax_mapping_enabled;
		}
    break;
  case GLFW_RELEASE:
		if (key == GLFW_KEY_LSHIFT) {
    	camera_zoom = false;
		}
    break;
  }
}

void resize(int width, int height) {
	viewport.x = width;
	viewport.y = height;
	
	trackball.center(0.5 * width, 0.5 * height);
}

int main(int argc, char **args)
{
  if (!glfwInit()) {
		log("Failed to initialize GLFW");
    exit(EXIT_FAILURE);
  }

  int depth_bits = 16;
  if (!glfwOpenWindow(640, 480, 0, 0, 0, 0, depth_bits, 0, GLFW_WINDOW)) {
		log("Failed to open GLFW window");
    glfwTerminate();
    exit(EXIT_FAILURE);
  }
  glfwSetWindowTitle("Teapot");
  glfwEnable(GLFW_STICKY_KEYS);
  glfwSwapInterval(1);

	glfwSetKeyCallback(keyboard);
  glfwSetMouseButtonCallback(mouse_button);
  glfwSetMousePosCallback(mouse_motion);
	glfwSetWindowSizeCallback(resize);

	setup();

  do {
		update();
		render();						
    glfwSwapBuffers();
  }
  while (glfwGetKey(GLFW_KEY_ESC) != GLFW_PRESS && glfwGetWindowParam(GLFW_OPENED));

	cleanup();

  glfwTerminate();

  return 0;
}

