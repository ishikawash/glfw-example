#include "texture.hpp"

std::vector<texture_unit_t> texture_unit_t::collection(4);

void texture_unit_t::initialize() {				
	texture_unit_t::collection[0].unit_id = GL_TEXTURE0;
	texture_unit_t::collection[1].unit_id = GL_TEXTURE1;
	texture_unit_t::collection[2].unit_id = GL_TEXTURE2;
	texture_unit_t::collection[3].unit_id = GL_TEXTURE3;
}

void texture_unit_t::attach(int index, const texture_t *texture) {
	texture_unit_t::collection[index].texture = texture;
}

void dettach(int index) {
	texture_unit_t::collection[index].texture = NULL;
}