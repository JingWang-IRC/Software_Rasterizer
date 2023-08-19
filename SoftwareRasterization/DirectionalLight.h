#pragma once
#include <glm.hpp>

class DirectionalLight
{
public:

	DirectionalLight(glm::vec3 _pos, glm::vec3 _direction, glm::vec3 _color) :
		pos(_pos),
		direction(_direction),
		color(_color)
	{}

	glm::vec3 pos;
	glm::vec3 direction;
	glm::vec3 color;

};

