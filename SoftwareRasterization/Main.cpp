#include <iostream>
#include <vector>
#include <string>
#include <math.h>
#include <conio.h>
#include <Windows.h>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <gtx/string_cast.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <thread>


#include <chrono>
#include <time.h>
#include <ctime>
using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::system_clock;

#include "Camera.h"
#include "DirectionalLight.h"
#include "DataAndSettings.h"
#include "Renderer.h"
#include "Window.h"



int main()
{
	bool shutDowm = false; // bool for controling the while loop

	POINT point; // get cursor position
	POINT lastPoint{-1, -1};

	// add camera and directional light
	Camera camera(glm::vec3(-12.0, 5.0, 20.0), -60.0f, 0.0f);
	DirectionalLight dLight(glm::vec3(10.0, 25.0, -40.0), glm::normalize(glm::vec3(10.0, 25.0, -40.0)), glm::vec3(1.0, 1.0, 1.0));

	// create buffers
	std::vector<glm::vec3> frameBuffer(WIDTH * HEIGHT, glm::vec3(255.0));
	std::vector<float> depthBuffer(WIDTH * HEIGHT, -1000.0);
	std::vector<float> shadowMapBuffer(WIDTH * HEIGHT, -1000.0);  // here I can use different resolution, but it is not necessary
	
	// load cube vertices data
	std::vector<vertex> cubeVertices;
	for (int i = 0; i < 288; i += 8)
	{
		glm::vec3 pos(cubeData[i], cubeData[i + 1], cubeData[i + 2]);
		glm::vec3 normal(cubeData[i + 3], cubeData[i + 4], cubeData[i + 5]);
		glm::vec2 texcoords(cubeData[i + 6], cubeData[i + 7]);
		cubeVertices.push_back(vertex{ pos, normal, texcoords });
	}
	std::vector<vertex> groundVertices;
	for (int i = 0; i < 48 * 16; i += 8)
	{
		glm::vec3 pos(groundData[i], groundData[i + 1], groundData[i + 2]);
		glm::vec3 normal(groundData[i + 3], groundData[i + 4], groundData[i + 5]);
		glm::vec2 texcoords(groundData[i + 6], groundData[i + 7]);
		groundVertices.push_back(vertex{ pos, normal, texcoords });
	}
	
	// prepare textures for the cube
	cv::Mat src = cv::imread("container.png");
	cv::cvtColor(src, src, cv::COLOR_BGR2RGB); // opencv default is BGR, now change to RGB
	std::vector<glm::vec3> containerTex = cvMatToVector(src);
	src = cv::imread("container_specular.png");
	std::vector<glm::vec3> containerSpecularTex = cvMatToVector(src); // specular texture does not care the order of rgb channel

	// time for computing fps
	auto lastFrameTime = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
	float sumTime = 0;
	int sampleNum = 0;
	//float fps = 0;

	//======================================= new window begin =====================================================
	device_t device;
	int states[] = { RENDER_STATE_TEXTURE, RENDER_STATE_COLOR, RENDER_STATE_WIREFRAME };
	int indicator = 0;
	int kbhit = 0;
	float alpha = 1; // this alpha is angle, not transparency
	float pos = 3.5;
	//int screenWidth = 800, screenHeight = 600;
	const wchar_t title[] = L"Software Renderer";
	long long gameStartTime = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();

	if (screenInit(WIDTH, HEIGHT, title))
		return -1;

	deviceInit(&device, WIDTH, HEIGHT, screen_fb);
	device.render_state = RENDER_STATE_TEXTURE;

	// timer related variables
	int frameCount = 0;
	int FPSRemainder = 0;
	int FPSLastRemainder = 0;
	float fps = 0;
	long long currentTime = 0;
	long long lastFPSTime = 0;
	//======================================= new window end =====================================================

	// start rendering
	while (!shutDowm)
	{
		// get time for computing FPS 
		auto currentFrameTime = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
		sumTime += currentFrameTime - lastFrameTime;
		sampleNum++;
		lastFrameTime = currentFrameTime;
		//if (sampleNum % 60 == 0) // every 60 frames update the FPS once
		//{
		//	fps = sampleNum / sumTime * 1000;
		//	sumTime = 0;
		//	sampleNum = 0;  // reset time
		//}

		// compute MVP matrices
		glm::vec3 cameraForward = camera.pos + camera.front;
		glm::mat4 modelMat = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, 1.0f));
		glm::mat4 viewMat = glm::lookAt(camera.pos, cameraForward, glm::vec3(0.0, 1.0, 0.0));
		glm::mat4 projMat = glm::perspective(glm::radians(45.0f), (float)WIDTH / (float)HEIGHT, 1.0f, 300.0f);
		glm::mat4 viewMatLight = glm::lookAt(dLight.pos, dLight.pos-dLight.direction, glm::vec3(0.0, 1.0, 0.0));
		glm::mat4 projMatLight = glm::ortho(-6.0f, 6.0f, -6.0f, 6.0f, lightNearPlane, lightFarPlane);
		glm::mat4 MVMatLight = viewMatLight * modelMat;
		glm::mat4 MVPMatLight = projMatLight * MVMatLight;

		// clean all the buffer
		std::fill(frameBuffer.begin(), frameBuffer.end(), glm::vec3(0.0, 0.0, 0.0));
		std::fill(depthBuffer.begin(), depthBuffer.end(), -1000.0);
		std::fill(shadowMapBuffer.begin(), shadowMapBuffer.end(), -1000.0);
		

		std::vector<std::thread> threads(8);

		
		// draw shadow map
		for (int i = 0; i < 4; i++)
		{
			int totalAmount = groundVertices.size();
			int quarter = totalAmount / 4;
			int begin = i * quarter;
			int end = i == 3 ? totalAmount : (i + 1) * quarter;

			threadedDrawShadowMap(shadowMapBuffer, groundVertices, begin, end, modelMat, viewMatLight, projMatLight);
		}

		for (int i = 0; i < 4; i++)
		{
			int totalAmount = cubeVertices.size();
			int quarter = totalAmount / 4;
			int begin = i * quarter;
			int end = i == 3 ? totalAmount : (i + 1) * quarter;

			threadedDrawShadowMap(shadowMapBuffer, cubeVertices, begin, end, modelMat, viewMatLight, projMatLight);
		}
		

		/*
		for (auto it = groundVertices.begin(); it != groundVertices.end(); it += 3)
		{
			std::vector<vertex> triangle(it, it + 3);
			drawShadowMap(shadowMapBuffer, triangle, modelMat, viewMatLight, projMatLight);
		}
		for (auto it = cubeVertices.begin(); it != cubeVertices.end(); it += 3)
		{
			std::vector<vertex> triangle(it, it + 3);
			drawShadowMap(shadowMapBuffer, triangle, modelMat, viewMatLight, projMatLight);
		}
		*/

		// draw the cube and the ground
		for (auto it = groundVertices.begin(); it != groundVertices.end(); it += 3)
		{
			std::vector<vertex> triangle(it, it + 3);
			drawTriangle(frameBuffer, depthBuffer, triangle, 
				modelMat, viewMat, projMat, camera.pos, 
				dLight, shadowMapBuffer, MVMatLight, MVPMatLight, true, 
				containerTex, containerSpecularTex, false,
				glm::vec3(0.0f, 1.0f, 0.0f), false);
		}
		for (auto it = cubeVertices.begin(); it != cubeVertices.end(); it += 3)
		{
			std::vector<vertex> triangle(it, it + 3);
			drawTriangle(frameBuffer, depthBuffer, triangle, 
				modelMat, viewMat, projMat, camera.pos, 
				dLight, shadowMapBuffer, MVMatLight, MVPMatLight, false, 
				containerTex, containerSpecularTex, true,
				glm::vec3(1.0f, 0.0f, 0.0f), false);
		}

		// draw light source
		modelMat = glm::translate(modelMat, dLight.pos);
		for (auto it = cubeVertices.begin(); it != cubeVertices.end(); it += 3)
		{
			std::vector<vertex> triangle(it, it + 3);
			drawTriangle(frameBuffer, depthBuffer, triangle,
				modelMat, viewMat, projMat, camera.pos,
				dLight, shadowMapBuffer, MVMatLight, MVPMatLight, false, 
				containerTex, containerSpecularTex, false,
				glm::vec3(1.0f, 1.0f, 1.0f), true);
		}

		// ================================ new window =================================

				// calculate time
		currentTime = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
		FPSRemainder = int(currentTime - gameStartTime) % 500;
		float timeRate = int(currentTime - gameStartTime) % 1000 / 1000.0f;

		// calculate FPS
		frameCount++;
		if (FPSRemainder < FPSLastRemainder)
		{
			float elapsedTime = (currentTime - lastFPSTime) * 0.001;
			fps = frameCount / elapsedTime;

			// update time and reset count
			lastFPSTime = currentTime;
			frameCount = 0;
		}
		FPSLastRemainder = FPSRemainder;

		screenDispatch();
		clearBuffers(&device, 1);

		if (screen_keys[VK_UP]) pos -= 0.01f;
		if (screen_keys[VK_DOWN]) pos += 0.01f;
		if (screen_keys[VK_LEFT]) alpha += 0.01f;
		if (screen_keys[VK_RIGHT]) alpha -= 0.01f;

		if (screen_keys[VK_SPACE])
		{
			if (kbhit == 0)
			{
				kbhit = 1;
				if (++indicator >= 3) indicator = 0;
				device.render_state = states[indicator];
			}
		}
		else
		{
			kbhit = 0;
		}

		presentBuffer(&device, frameBuffer);
		printText(&device, fps);
		screenUpdate();
		//Sleep(1); // if no sleep, the fps may be 2000+ when empty

		// get keyboard and cursor input
		if (GetKeyState('W') & 0x8000) camera.pos += camera.front * camera.MovementSpeed;
		if (GetKeyState('S') & 0x8000) camera.pos -= camera.front * camera.MovementSpeed;
		if (GetKeyState('D') & 0x8000) camera.pos += camera.right * camera.MovementSpeed;
		if (GetKeyState('A') & 0x8000) camera.pos -= camera.right * camera.MovementSpeed;
		if (GetKeyState('E') & 0x8000) camera.pos += camera.worldUp * camera.MovementSpeed;
		if (GetKeyState('Q') & 0x8000) camera.pos -= camera.worldUp * camera.MovementSpeed;
		if (GetKeyState(VK_ESCAPE) & 0x8000) shutDowm = true; // press ESCAPE to close the app
		if (GetCursorPos(&point)) { // move mouse to rotate the camera
			if (lastPoint.x < 0)
			{
				lastPoint = point;
				continue;
			}
			POINT deltaPoint = POINT{ point.x - lastPoint.x, point.y - lastPoint.y};
			camera.processMouseMovement(deltaPoint.x, deltaPoint.y);
			lastPoint = point;
		}
	}

	return 0;
}





