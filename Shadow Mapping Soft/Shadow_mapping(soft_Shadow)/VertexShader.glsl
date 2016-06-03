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

#version 430 core // Identifies the version of the shader, this line must be on a separate line from the rest of the shader code
 
layout(location = 0) in vec3 in_position;	// Get in a vec3 for position
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec4 in_color;		// Get in a vec4 for color

uniform mat4 MVP; // Our uniform MVP matrix to modify our position values

void main(void)
{
	gl_Position = MVP * vec4(in_position, 1.0);
}