#pragma once
#include <glm.hpp>

class Camera
{
public:

	Camera(glm::vec3 _pos, float _Yaw, float _Pitch);

	void updateCameraVector();
	void processMouseMovement(float xoffset, float yoffset);

	glm::vec3 pos;
	glm::vec3 right;
	glm::vec3 front;
	glm::vec3 up;
	glm::vec3 worldUp = glm::vec3(0.0, 1.0, 0.0);

	float Yaw;
	float Pitch;
	float MovementSpeed = 0.3f;
	float MouseSensitivity = 0.5f;

};

