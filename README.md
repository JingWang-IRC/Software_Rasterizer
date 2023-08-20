# Software_Rasterizer
This project is a software rasterizer including
(This is not using any graphics API, and only rendered by CPU):
1. Camera movement
2. Blinn-Phong shading
3. PCF based soft shadow
4. Map texture on object
5. Window system using Windows API
6. Rendering acceleration by making use of bounding boxes of each triangle mesh

## Control:
WSAD for moving the camera, mouse for the orientation.

## Demo video:
A wooden box with metal frame on a green plane. The distant white cube is a directional light source (to indicate the light direction).

https://github.com/JingWang-IRC/Software_Rasterizer/assets/75773315/a6ece25e-e9be-4fd1-bb33-d6b94ec057d4


## To do:
1. Remove the OpenCV code (which was used for opening window)
2. Make use of multiple CPU cores for acceleration
3. Replace Blinn-Phone model with PBR
