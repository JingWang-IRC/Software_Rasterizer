#pragma once
#include <glm.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>

float interpolation(float alpha, float beta, float gamma, float att0, float att1, float att2,
	float z0, float z1, float z2, float zp)
{
	float interpolation = alpha * att0 / z0 + beta * att1 / z1 + gamma * att2 / z2;
	return interpolation * zp;
}

glm::vec2 interpolation(float alpha, float beta, float gamma, glm::vec2 att0, glm::vec2 att1, glm::vec2 att2,
	float z0, float z1, float z2, float zp)
{
	glm::vec2 interpolation = alpha * att0 / z0 + beta * att1 / z1 + gamma * att2 / z2;
	return interpolation * zp;
}


glm::vec3 interpolation(float alpha, float beta, float gamma, glm::vec3 att0, glm::vec3 att1, glm::vec3 att2,
	float z0, float z1, float z2, float zp)
{
	glm::vec3 interpolation = alpha * att0 / z0 + beta * att1 / z1 + gamma * att2 / z2;
	return interpolation * zp;
}

glm::vec3 scrrenSpaceCenterWeight(const glm::vec2& p, const glm::vec2& v0, const glm::vec2& v1, const glm::vec2& v2)
{
	float weightV0 = (-(p.x - v1.x) * (v2.y - v1.y) + (p.y - v1.y) * (v2.x - v1.x)) / (-(v0.x - v1.x) * (v2.y - v1.y) + (v0.y - v1.y) * (v2.x - v1.x));
	float weightV1 = (-(p.x - v2.x) * (v0.y - v2.y) + (p.y - v2.y) * (v0.x - v2.x)) / (-(v1.x - v2.x) * (v0.y - v2.y) + (v1.y - v2.y) * (v0.x - v2.x));
	float weightV2 = 1 - weightV0 - weightV1;

	if (weightV0 >= 0 && weightV1 >= 0 && weightV2 >= 0 && weightV0 + weightV1 + weightV2 <= 1)
		return glm::vec3(weightV0, weightV1, weightV2);
	else
		return glm::vec3(-1.0);
}

float sampleTexture(const std::vector<float>& depthBuffer, float x, float y, const int texWidth, const int texHeight)
{
	float w = std::min(float(texWidth), std::max(0.0f, x * texWidth));
	float h = std::min(float(texHeight), std::max(0.0f, texHeight - y * texHeight));

	int floorW = std::floor(w);
	int floorH = std::floor(h);

	float A = depthBuffer[floorH * texWidth + floorW];
	float B = depthBuffer[floorH * texWidth + std::min(floorW + 1, texWidth - 1)];
	float C = depthBuffer[std::min(floorH + 1, texHeight - 1) * texWidth + floorW];
	float D = depthBuffer[std::min(floorH + 1, texHeight - 1) * texWidth + std::min(floorW + 1, texWidth - 1)];


	float AB = (w - floorW) * B + (floorW + 1 - w) * A;
	float CD = (w - floorW) * D + (floorW + 1 - w) * C;
	float ABCD = (h - floorH) * CD + (floorH + 1 - h) * AB;

	return ABCD;
}

glm::vec3 sampleTexture(const std::vector<glm::vec3>& depthBuffer, float x, float y, const int texWidth, const int texHeight)
{
	float w = std::min(float(texWidth), std::max(0.0f, x * texWidth));
	float h = std::min(float(texHeight), std::max(0.0f, texHeight - y * texHeight));

	int floorW = std::floor(w);
	int floorH = std::floor(h);

	glm::vec3 A = depthBuffer[floorH * texWidth + floorW];
	glm::vec3 B = depthBuffer[floorH * texWidth + std::min(floorW + 1, texWidth - 1)];
	glm::vec3 C = depthBuffer[std::min(floorH + 1, texHeight - 1) * texWidth + floorW];
	glm::vec3 D = depthBuffer[std::min(floorH + 1, texHeight - 1) * texWidth + std::min(floorW + 1, texWidth - 1)];


	glm::vec3 AB = (w - floorW) * B + (floorW + 1 - w) * A;
	glm::vec3 CD = (w - floorW) * D + (floorW + 1 - w) * C;
	glm::vec3 ABCD = (h - floorH) * CD + (floorH + 1 - h) * AB;

	return ABCD;
}

std::vector<glm::vec3> cvMatToVector(const cv::Mat input)
{
	int height = input.rows;
	int width = input.cols;
	std::vector<glm::vec3> pRgb;
	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			float rgb[3];
			for (int k = 0; k < 3; k++)
			{
				rgb[k] = (float)input.at<cv::Vec3b>(i, j)[k] / 255.0;
			}
			pRgb.push_back(glm::vec3(rgb[0], rgb[1], rgb[2]));
		}
	}
	return pRgb;
}

std::vector<glm::vec3> ConvertBufferR2RGB(const std::vector<float> & input) // input ranges from -1 ~ 1, NDC
{
	std::vector<glm::vec3> output;
	output.reserve(input.size());

	for (auto it = input.begin(); it != input.end(); it++)
	{
		output.push_back(glm::vec3( (-*it + 1) / 2 * 255, 0, 0)); // make the depth value negative so red is very close, while black is far away
	}

	return output;
}