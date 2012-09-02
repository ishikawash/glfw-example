#include "trackball.hpp"

using namespace std;
using namespace glm;

void trackball_t::drag_start(int x, int y) {
	__dragged = true;
	drag_update(x, y);
}

void trackball_t::drag_update(int x, int y) {
	if (__dragged) {
		__drag_start_position.x = x - __center_position.x;
		__drag_start_position.y = y - __center_position.y;
	}
}

void trackball_t::drag_end() {
	if (__dragged) {
		drag_update(0.0f, 0.0f);
		__dragged = false;
	}
}

glm::vec2 trackball_t::direction(int x, int y) {
	ivec2 drag_end_position(x - __center_position.x, y - __center_position.y);	
	vec2 v(drag_end_position.x - __drag_start_position.x, drag_end_position.y - __drag_start_position.y);
	v.y = -v.y;	
	return normalize(v);
}

glm::quat& trackball_t::rotate(glm::quat &orientation, int x, int y) {
	if (! __dragged)
		return orientation;
	
  glm::vec3 v0 = map_to_sphere(__drag_start_position);
  glm::vec3 v1 = map_to_sphere(ivec2(x - __center_position.x, y - __center_position.y));
  glm::vec3 v2 = cross(v0, v1); // calculate rotation axis

  float d = dot(v0, v1);
  float s = sqrtf((1.0f + d) * 2.0f);
  glm::quat q(0.5f * s, v2 / s);

  orientation = q * orientation;
  orientation /= length(orientation);
	return orientation;
}

glm::vec3 trackball_t::map_to_sphere(const glm::ivec2 &point) {
	vec2 p(point.x, point.y);

  p.y = -p.y;

  float safe_radius = __radius - 1.0f;
  if (glm::length(p) > safe_radius) {
    float theta = std::atan2(p.y, p.x);
    p.x = safe_radius * cos(theta);
    p.y = safe_radius * sin(theta);
  }

  float length_squared = p.x * p.x + p.y * p.y;
  float z = sqrtf(__radius * __radius - length_squared);
  glm::vec3 q(p.x, p.y, z);
  return normalize(q / __radius);
}
