#include <iostream>
#include <openctmpp.h>
#include "mesh.hpp"

using namespace std;

void mesh_t::load_to_buffers() {
  glGenBuffers(1, &vertex_buffer_handle);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_handle);
  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertex_t), &vertices[0], GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glGenBuffers(1, &index_buffer_handle);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer_handle);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void mesh_t::render(const shader_program_t &shader_program) {
	GLuint position_location = shader_program.attribute_location("vertex_position");
	GLuint normal_location = shader_program.attribute_location("vertex_normal");
	GLuint tex_coord_location = shader_program.attribute_location("vertex_tex_coord");

  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_handle);
  glVertexAttribPointer(position_location, 4, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (GLvoid *)(offsetof(vertex_t, position)));
	glVertexAttribPointer(normal_location, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (GLvoid *)(offsetof(vertex_t, normal)));
	glVertexAttribPointer(tex_coord_location, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (GLvoid *)(offsetof(vertex_t, tex_coord)));
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glEnableVertexAttribArray(position_location);
  glEnableVertexAttribArray(normal_location);
	glEnableVertexAttribArray(tex_coord_location);
	
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer_handle);
  glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, (GLvoid *)0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glDisableVertexAttribArray(tex_coord_location);
  glDisableVertexAttribArray(normal_location);
  glDisableVertexAttribArray(position_location);

}

bool mesh_t::read_from_file(const char *ctm_filepath, mesh_t &mesh) {
  CTMimporter ctm;
  try {
    ctm.Load(ctm_filepath);
	} catch(ctm_error & e) {
		cerr << "*** Loading CTM file failed: " << e.what() << endl;
		return false;
	}
	
	const CTMfloat *vertices;
	const CTMuint *indices;
	const CTMfloat *normals;
	const CTMfloat *tex_coords;
	
  unsigned int vertex_count = ctm.GetInteger(CTM_VERTEX_COUNT);
 	vertices = ctm.GetFloatArray(CTM_VERTICES);

  unsigned int face_count = ctm.GetInteger(CTM_TRIANGLE_COUNT);
  unsigned int index_count = face_count * 3;
  indices = ctm.GetIntegerArray(CTM_INDICES);
	
	if (ctm.GetInteger(CTM_HAS_NORMALS) != CTM_TRUE) {
		cerr << "*** normals not found" << endl;
	}
	normals = ctm.GetFloatArray(CTM_NORMALS);

  unsigned int uv_map_count = ctm.GetInteger(CTM_UV_MAP_COUNT);
  if (uv_map_count > 0) {
    tex_coords = ctm.GetFloatArray(CTM_UV_MAP_1);
  } else {
    std::cerr << "*** uv map not found" << std::endl;
  }

	mesh.vertices.resize(vertex_count);
	for (unsigned int i = 0; i < vertex_count; i++) {
		vertex_t &v = mesh.vertices[i];
		
		unsigned int j = 3*i;
		v.position.x = vertices[j];
		v.position.y = vertices[j + 1];
		v.position.z = vertices[j + 2];
		v.position.w = 1.0f;
				
		v.normal.x = normals[j];
		v.normal.y = normals[j + 1];
		v.normal.z = normals[j + 2];

		if (uv_map_count > 0) {
			unsigned int k = 2*i;
			v.tex_coord.x = tex_coords[k];
			v.tex_coord.y = tex_coords[k + 1];			
		}	
	}

  mesh.indices.resize(index_count);
  std::memcpy(&mesh.indices[0], indices, index_count * sizeof(unsigned int));
	
	return true;
}