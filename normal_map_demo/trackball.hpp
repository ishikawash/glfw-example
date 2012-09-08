#ifndef TRACKBALL_HPP
#define TRACKBALL_HPP

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class trackball_t {

private:
	
	glm::ivec2 __center_position;
	glm::ivec2 __drag_start_position;
	float __radius;
	bool __dragged;

	glm::vec3 map_to_sphere(const glm::ivec2 &point);

public:
	
	trackball_t(float radius) : __radius(radius) { }
	
	void drag_start(int x, int y);
	void drag_update(int x, int y);
	void drag_end();
	bool dragged() const { return __dragged; }
	
	glm::quat& rotate(glm::quat &orientation, int x, int y);
	
	glm::vec2 direction(int x, int y);
	
	void center(float x, float y) {
		__center_position.x = x;
		__center_position.y = y;
	}
		
};

#endif
