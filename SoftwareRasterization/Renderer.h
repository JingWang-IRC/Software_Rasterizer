#pragma once
#include <glm.hpp>

#include "DataAndSettings.h"
#include "DirectionalLight.h"
#include "Tools.h"



void drawShadowMap(std::vector<float>& depthBuffer, const std::vector<vertex>& vertices,
	glm::mat4 modelMat, glm::mat4 viewMat, glm::mat4 projMat)
{
	glm::vec3 viewPosV0 = viewMat * modelMat * glm::vec4(vertices[0].pos, 1.0);
	glm::vec3 viewPosV1 = viewMat * modelMat * glm::vec4(vertices[1].pos, 1.0);
	glm::vec3 viewPosV2 = viewMat * modelMat * glm::vec4(vertices[2].pos, 1.0);
	glm::vec4 NDCPosV0WithW = projMat * glm::vec4(viewPosV0, 1.0);
	glm::vec4 NDCPosV1WithW = projMat * glm::vec4(viewPosV1, 1.0);
	glm::vec4 NDCPosV2WithW = projMat * glm::vec4(viewPosV2, 1.0);
	float wV0 = std::max(NDCPosV0WithW.w, 0.001f); // if divided by 0 or minus value cause problem
	float wV1 = std::max(NDCPosV1WithW.w, 0.001f);
	float wV2 = std::max(NDCPosV2WithW.w, 0.001f);
	glm::vec3 NDCPosV0 = glm::vec3(NDCPosV0WithW.x / wV0, NDCPosV0WithW.y / wV0, NDCPosV0WithW.z / wV0);
	glm::vec3 NDCPosV1 = glm::vec3(NDCPosV1WithW.x / wV1, NDCPosV1WithW.y / wV1, NDCPosV1WithW.z / wV1);
	glm::vec3 NDCPosV2 = glm::vec3(NDCPosV2WithW.x / wV2, NDCPosV2WithW.y / wV2, NDCPosV2WithW.z / wV2);
	glm::vec2 screenPosV0 = (glm::vec2(NDCPosV0.x, NDCPosV0.y) + glm::vec2(1.0));
	glm::vec2 screenPosV1 = (glm::vec2(NDCPosV1.x, NDCPosV1.y) + glm::vec2(1.0));
	glm::vec2 screenPosV2 = (glm::vec2(NDCPosV2.x, NDCPosV2.y) + glm::vec2(1.0));
	screenPosV0 = glm::vec2(screenPosV0.x / 2, screenPosV0.y / 2);
	screenPosV1 = glm::vec2(screenPosV1.x / 2, screenPosV1.y / 2);
	screenPosV2 = glm::vec2(screenPosV2.x / 2, screenPosV2.y / 2);

	// compute bounding box
	int bbLeftX = std::min(1.0f, std::max(0.0f, std::min(std::min(screenPosV0.x, screenPosV1.x), screenPosV2.x))) * WIDTH;
	int bbRightX = std::max(0.0f, std::min(1.0f, std::max(std::max(screenPosV0.x, screenPosV1.x), screenPosV2.x))) * WIDTH;
	int bbBottomY = std::min(1.0f, std::max(0.0f, std::min(std::min(screenPosV0.y, screenPosV1.y), screenPosV2.y))) * HEIGHT;
	int bbTopY = std::max(0.0f, std::min(1.0f, std::max(std::max(screenPosV0.y, screenPosV1.y), screenPosV2.y))) * HEIGHT;

	// make sure boounding box covers the whole triangle
	bbTopY = std::max(0, (int)HEIGHT - bbTopY - 1);
	bbBottomY = std::min(HEIGHT, HEIGHT - bbBottomY + 1);
	bbLeftX = std::max(0, bbLeftX - 1);
	bbRightX = std::min((int)WIDTH, bbRightX + 1);

	// render pixels
	for (int i = bbTopY; i < bbBottomY; i++) // i and j starts from top-left
	{
		for (int j = bbLeftX; j < bbRightX; j++)
		{
			glm::vec2 pos(float(j + 0.5), float(i + 0.5)); // the (0, 0) is top-left
			glm::vec2 screenPos((pos.x) / WIDTH, (HEIGHT - pos.y) / HEIGHT); // change to NDC but move bottom-left to (0, 0) instead of (-1, -1)

			// get weight for triangle interpolation
			glm::vec3 weight = scrrenSpaceCenterWeight(screenPos, screenPosV0, screenPosV1, screenPosV2);

			if (weight.x >= 0 ) // test depth and if it is inside triangle
			{
					float z0 = std::abs(viewPosV0.z) < 0.01f ? (viewPosV0.z >= 0 ? 0.01 : -0.01) : viewPosV0.z; // to avoid project correction whe ninterpolation
					float z1 = std::abs(viewPosV1.z) < 0.01f ? (viewPosV1.z >= 0 ? 0.01 : -0.01) : viewPosV1.z; // when z is close to zero, bug happens
					float z2 = std::abs(viewPosV2.z) < 0.01f ? (viewPosV2.z >= 0 ? 0.01 : -0.01) : viewPosV2.z; // this method still cannot perfectly solve it
					float zp = 1 / (weight.r / z0 + weight.g / z1 + weight.b / z2);
					float depth = zp;
				
				if (depth >= depthBuffer[i * WIDTH + j] && depth <= 0)
				{
					depthBuffer[i * WIDTH + j] = depth;
				}
			}
		}
	}
}


void drawTriangle(std::vector<glm::vec3>& frameBuffer, std::vector<float>& depthBuffer, const std::vector<vertex>& vertices,
	glm::mat4 modelMat, glm::mat4 viewMat, glm::mat4 projMat,
	glm::vec3 cameraPos, DirectionalLight light,
	const std::vector<float>& shadowMapBuffer, glm::mat4 MVMatLight, glm::mat4 MVPMatLight, bool drawShadow,
	const std::vector<glm::vec3>& texAlbedo, const std::vector<glm::vec3>& texSpecular, bool drawTex,
	glm::vec3 color, bool pureColor)
{
	// get the postion, normal and texcoords of three vertices in different spaces
	glm::vec3 worldPosV0 = modelMat * glm::vec4(vertices[0].pos, 1.0);
	glm::vec3 worldPosV1 = modelMat * glm::vec4(vertices[1].pos, 1.0);
	glm::vec3 worldPosV2 = modelMat * glm::vec4(vertices[2].pos, 1.0);
	glm::vec3 viewPosV0 = viewMat * glm::vec4(worldPosV0, 1.0);
	glm::vec3 viewPosV1 = viewMat * glm::vec4(worldPosV1, 1.0);
	glm::vec3 viewPosV2 = viewMat * glm::vec4(worldPosV2, 1.0);
	glm::vec4 NDCPosV0WithW = projMat * glm::vec4(viewPosV0, 1.0);
	glm::vec4 NDCPosV1WithW = projMat * glm::vec4(viewPosV1, 1.0);
	glm::vec4 NDCPosV2WithW = projMat * glm::vec4(viewPosV2, 1.0);
	float wV0 = std::max(NDCPosV0WithW.w, 0.001f); // if divided by 0 or minus value cause problem
	float wV1 = std::max(NDCPosV1WithW.w, 0.001f);
	float wV2 = std::max(NDCPosV2WithW.w, 0.001f);
	glm::vec3 NDCPosV0 = glm::vec3(NDCPosV0WithW.x / wV0, NDCPosV0WithW.y / wV0, NDCPosV0WithW.z / wV0);
	glm::vec3 NDCPosV1 = glm::vec3(NDCPosV1WithW.x / wV1, NDCPosV1WithW.y / wV1, NDCPosV1WithW.z / wV1);
	glm::vec3 NDCPosV2 = glm::vec3(NDCPosV2WithW.x / wV2, NDCPosV2WithW.y / wV2, NDCPosV2WithW.z / wV2);
	glm::vec2 screenPosV0 = (glm::vec2(NDCPosV0.x, NDCPosV0.y) + glm::vec2(1.0));
	glm::vec2 screenPosV1 = (glm::vec2(NDCPosV1.x, NDCPosV1.y) + glm::vec2(1.0));
	glm::vec2 screenPosV2 = (glm::vec2(NDCPosV2.x, NDCPosV2.y) + glm::vec2(1.0));
	screenPosV0 = glm::vec2(screenPosV0.x / 2, screenPosV0.y / 2);
	screenPosV1 = glm::vec2(screenPosV1.x / 2, screenPosV1.y / 2);
	screenPosV2 = glm::vec2(screenPosV2.x / 2, screenPosV2.y / 2);

	// compute bounding box
	int bbLeftX = std::min(1.0f, std::max(0.0f, std::min(std::min(screenPosV0.x, screenPosV1.x), screenPosV2.x))) * WIDTH;
	int bbRightX = std::max(0.0f, std::min(1.0f, std::max(std::max(screenPosV0.x, screenPosV1.x), screenPosV2.x))) * WIDTH;
	int bbBottomY = std::min(1.0f, std::max(0.0f, std::min(std::min(screenPosV0.y, screenPosV1.y), screenPosV2.y))) * HEIGHT;
	int bbTopY = std::max(0.0f, std::min(1.0f, std::max(std::max(screenPosV0.y, screenPosV1.y), screenPosV2.y))) * HEIGHT;

	// make sure boounding box covers the whole triangle
	bbTopY = std::max(0, (int)HEIGHT - bbTopY - 1);
	bbBottomY = std::min(HEIGHT, HEIGHT - bbBottomY + 1);
	bbLeftX = std::max(0, bbLeftX - 1);
	bbRightX = std::min((int)WIDTH, bbRightX + 1);

	// render pixels
	for (int i = bbTopY; i < bbBottomY; i++) // i and j starts from top-left
	{
		for (int j = bbLeftX; j < bbRightX; j++)
		{
			glm::vec2 pos(float(j + 0.5), float(i + 0.5)); // the (0, 0) is top-left
			glm::vec2 screenPos((pos.x) / WIDTH, (HEIGHT - pos.y) / HEIGHT); // change to NDC but move bottom-left to (0, 0) instead of (-1, -1)

			// get weight for triangle interpolation
			glm::vec3 weight = scrrenSpaceCenterWeight(screenPos, screenPosV0, screenPosV1, screenPosV2);


			if (weight.x >= 0) // test depth and if it is inside triangle and NDC
			{
				float z0 = std::abs(viewPosV0.z) < 0.01f ? (viewPosV0.z >= 0 ? 0.01 : -0.01) : viewPosV0.z; // to avoid project correction whe ninterpolation
				float z1 = std::abs(viewPosV1.z) < 0.01f ? (viewPosV1.z >= 0 ? 0.01 : -0.01) : viewPosV1.z; // when z is close to zero, bug happens
				float z2 = std::abs(viewPosV2.z) < 0.01f ? (viewPosV2.z >= 0 ? 0.01 : -0.01) : viewPosV2.z; // this method still cannot perfectly solve it
				float zp = 1 / (weight.r / z0 + weight.g / z1 + weight.b / z2);
				float depth = zp;

				if (depth >= depthBuffer[i * WIDTH + j] && depth <= 0)
				{
					if (!pureColor)
					{
						// use interpolation with perspective projection correction to get attributions
						glm::vec3 worldPos = interpolation(weight.x, weight.y, weight.z, worldPosV0, worldPosV1, worldPosV2, z0, z1, z2, zp);
						// the normals of cubes are axis aligned, so we can omit some transformation on normals 
						glm::vec3 normal = glm::normalize(interpolation(weight.x, weight.y, weight.z, vertices[0].normal, vertices[1].normal, vertices[2].normal, z0, z1, z2, zp));

						glm::vec3 lightViewPos = MVMatLight * glm::vec4(worldPos, 1.0f);
						glm::vec4 lightNDCPosWithW = MVPMatLight * glm::vec4(worldPos, 1.0f);
						glm::vec3 lightNDCPos = glm::vec3(lightNDCPosWithW.x / lightNDCPosWithW.w, lightNDCPosWithW.y / lightNDCPosWithW.w, lightNDCPosWithW.z / lightNDCPosWithW.w);
						glm::vec2 shadowMapPos = glm::vec2((lightNDCPos.x + 1) / 2, (lightNDCPos.y + 1) / 2);

						// Percentage-closing filtering
						float occlusion = 0;
						if (drawShadow)
						{
							int filterSize = 1;
							int sampleNum = 0;
							for (int i = -filterSize; i < filterSize + 1; i++)
							{
								for (int j = -filterSize; j < filterSize + 1; j++)
								{
									float x = shadowMapPos.x + j / (float)WIDTH;
									float y = shadowMapPos.y + i / (float)HEIGHT;
									float shadowMapDepth = sampleTexture(shadowMapBuffer, x, y, WIDTH, HEIGHT);
									occlusion += lightViewPos.z + 0.15 < shadowMapDepth;
									sampleNum++;
								}
							}
							occlusion = occlusion / sampleNum;
						}

						// get the relative positions
						glm::vec3 pToCamera = glm::normalize(cameraPos - worldPos);
						glm::vec3 pToLight = glm::normalize(light.direction);
						glm::vec3 pHalfVector = glm::normalize(pToCamera + pToLight);
						float cosineD = glm::dot(normal, pToLight);
						float cosineS = glm::dot(normal, pHalfVector);

						// compute lighting
						glm::vec3 finalColor;
						if (drawTex)
						{
							// get texture value
							glm::vec2 texcoords = interpolation(weight.x, weight.y, weight.z, vertices[0].texcoords, vertices[1].texcoords, vertices[2].texcoords, z0, z1, z2, zp);
							glm::vec3 albedo = sampleTexture(texAlbedo, texcoords.x, texcoords.y, TEX_WIDTH, TEX_HEIGHT);
							float specularRate = sampleTexture(texSpecular, texcoords.x, texcoords.y, TEX_WIDTH, TEX_HEIGHT).x;

							glm::vec3 diffuse = glm::vec3(std::max(0.0f, cosineD)) * albedo * light.color;
							glm::vec3 specular = std::pow(std::max(0.0f, cosineS), 32.0f) * light.color * specularRate;
							glm::vec3 ambient = 0.3f * albedo;
							finalColor = (diffuse + specular) * (1 - occlusion) + ambient;
						}
						else
						{
							glm::vec3 diffuse = glm::vec3(std::max(0.0f, cosineD)) * color * light.color;
							glm::vec3 specular = std::pow(std::max(0.0f, cosineS), 32.0f) * light.color * 0.3f;
							glm::vec3 ambient = 0.15f * color;
							finalColor = (diffuse + specular) * (1 - occlusion) + ambient;
						}
						frameBuffer[i * WIDTH + j] = finalColor * 255.0f;
						depthBuffer[i * WIDTH + j] = depth;
					}
					else
					{
						frameBuffer[i * WIDTH + j] = color * 255.0f;
					}
				}
			}
		}
	}
}


void threadedDrawShadowMap(std::vector<float>& depthBuffer, const std::vector<vertex>& vertices, int begin, int end,
	glm::mat4 modelMat, glm::mat4 viewMat, glm::mat4 projMat)
{
	for (auto it = vertices.begin() + begin; it != vertices.begin() + end; it += 3)
	{
		std::vector<vertex> triangle(it, it + 3);
		drawShadowMap(depthBuffer, triangle, modelMat, viewMat, projMat);
	}
}