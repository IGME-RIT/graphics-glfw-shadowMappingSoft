/*
Title: Shadow mapping (Soft Shadows)
File Name: main.cpp
Copyright © 2015
Original authors: Srinivasan Thiagarajan
Written under the supervision of David I. Schwartz, Ph.D., and
supported by a professional development seed grant from the B. Thomas
Golisano College of Computing & Information Sciences
(https://www.rit.edu/gccis) at the Rochester Institute of Technology.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or (at
your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

Description:
This example demonstrates the implementation of soft shadows using OpenGL.
It builds on the previous example of hard shadows, and creates soft shadows
from antialiasing the hard shadows.
This examples two spearate ways to implement anti-aliasing: using
Percentage-closer-filtering (PCF) and random sampling technique.

Percentage closer filtering(PCF): Here we sample the area around
the fragment and determine the percentage of the area that is closer
to the light source (shadow). The percentage is then used to scale
the amount of shading that the fragment receives. The overall effect is a
blurring of the shadow's edges.
We sample a limited number of texels surrounding the fragment in
the first implementation. We enable linear filtering and sample 4
texels surrounding fragment. This results in a total sampling for 16 texels.
These results are then averaged toghether and used for shadow value.

Random sampling:
This method is used when we desire substantially wide shadows. Since a large
number of samples would be required to produce a larger and more blurred shadow,
it would be ineffecient to use in area in complete shadows. So, instead of sampling
texels around the fragment's position (in shadow map space) using a constant set of
offsets, we use a random, circular pattern of offsets.
In addition, we sample only the outer edges of the circle first in order to
determine wheather or not the fragment is in an area that is completely inside or
outside of the shadow.
This is implemented by creating a texture that stores the offsets that are generated
randomly. Then sampling the texture to find the offsets and sampling the shadow map
with the offset values.
Once the fragment has been determined to be in the blurry region, we further use the
offsets to sample the texels closer to the pixel. These values are then averaged and
used as shadow value for that pixel.

Instructions: Use "1,2 and 3" to change the shadow.
Use "w,a,s and d" to move the light source located on top ofthe object.
Use "Space" and "LeftShift" to move the light source up or down respectively.

References:
OpenGL 4 Shading language Cookbook
*/

#ifndef _GL_INCLUDES_H
#define _GL_INCLUDES_H

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <soil\SOIL.h>
#include "glew\glew.h"
#include "glfw\glfw3.h"
#include "glm\glm.hpp"
#include "glm\gtc\matrix_transform.hpp"
#include "glm\gtc\type_ptr.hpp"
#include "glm\gtc\quaternion.hpp"
#include "glm\gtx\quaternion.hpp"

// We create a VertexFormat struct, which defines how the data passed into the shader code wil be formatted
struct VertexFormat
{
	glm::vec4 color;	// A vector4 for color has 4 floats: red, green, blue, and alpha
	glm::vec3 position;	// A vector3 for position has 3 float: x, y, and z coordinates
	glm::vec3 normal;	
	
	// Default constructor
	VertexFormat()
	{
		position = glm::vec3(0.0f);
		normal = glm::vec3(0);
		color = glm::vec4(0.0f);
	}

	// Constructor
	VertexFormat(const glm::vec3 &pos, const glm::vec3 &norm, const glm::vec4 &iColor)
	{
		position = pos;
		normal = norm;
		color = iColor;
	}
};

#endif _GL_INCLUDES_H