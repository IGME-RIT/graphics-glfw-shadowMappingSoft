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

layout(location = 0) out vec4 Color; // Establishes the variable we will pass out of this shader.

layout (binding = 0) uniform sampler2DShadow ShadowMap;
 
in vec3 Position;
in vec3 Normal;
in vec4 Albedo;
in vec4 ShadowCoord;

uniform struct PointLight
{
	vec3 position;
	vec3 Intensity;
}pointLight;

layout (binding = 1) uniform sampler3D OffsetTex;
uniform vec3 OffsetTexsize;

subroutine float shadowSubType();


subroutine uniform shadowSubType shadowSubUniform;

// Basic shadow: just sample the texture and return
subroutine (shadowSubType) 
float basicShadow()
{
	return textureProj(ShadowMap, ShadowCoord);
}

// PCF: Sample the surrounding texels and find the average value
subroutine (shadowSubType) 
float PCFshadow()
{
	float sum = 0;
	
	// read from texture the values of the neighbouring pixels
	sum += textureProjOffset(ShadowMap, ShadowCoord, ivec2(-1,-1));
	sum += textureProjOffset(ShadowMap, ShadowCoord, ivec2(1,-1));
	sum += textureProjOffset(ShadowMap, ShadowCoord, ivec2(-1,1));
	sum += textureProjOffset(ShadowMap, ShadowCoord, ivec2(1,1));

	return sum * 0.25f; //textureProj(ShadowMap, ShadowCoord);
}

// Random sampling
subroutine (shadowSubType) 
float randomSamplingShadow()
{
	float radius = 0.004f;

	ivec3 offsetCoord;
	offsetCoord.xy = ivec2(mod(gl_FragCoord.xy, OffsetTexsize.xy));

	float sum = 0;
	int samplesDiv2 = int (OffsetTexsize.z);
	vec4 sc = ShadowCoord;

	// Sample the texels on the outskirts of the area.
	for( int i=0; i < 4; i++)
	{
		offsetCoord.z = i;
		vec4 offsets = texelFetch(OffsetTex,offsetCoord,0) * radius * ShadowCoord.w;

		sc.xy = ShadowCoord.xy + offsets.xy;
		sum += textureProj(ShadowMap, sc);
		sc.xy = ShadowCoord.xy + offsets.zw;
		sum+= textureProj(ShadowMap, sc);
	}

	float shadow = sum / 8.0f;

	//the average value would be 0 or 1 if all the texels have the same value. 
	// It means that the fragment lies completly within shadow or completly within light.
	if(shadow == 1.0f && shadow == 0.0f)
	{
		// Early exit
		return shadow;
	}

	// If the value is somewhere between 0 and 1, we need to sample more surrounding texels for a more gradual shadow.
	for( int i=4; i< samplesDiv2; i++)
	{
		offsetCoord.z = i;
		vec4 offsets = texelFetch(OffsetTex, offsetCoord, 0) * radius * ShadowCoord.w;
		sc.xy = ShadowCoord.xy + offsets.xy;
		sum += textureProj(ShadowMap, sc);
		sc.xy = ShadowCoord.xy + offsets.zw;
		sum+= textureProj(ShadowMap, sc);
	}

	shadow = sum / float (samplesDiv2 * 2.0f);

	return shadow;
}

// calculate the light's component in coloring the fragment
vec3 diffuseModel (vec3 pos, vec3 norm, vec3 diff)
{
	vec3 s = normalize(pointLight.position - pos);
	float nDotL = max(dot(s, norm),0.0f);
	vec3 diffuse = pointLight.Intensity * (diff * nDotL);
	
	return diffuse;
}

void main(void)
{
	//Set the ambient light value. Models in shadow would be only lit by ambient light
	vec3 Ambient = Albedo.xyz * 0.2f;
	//We had set the texture properties to compare_to_ref
	// So when we sample the texture, it compare it with the current depth value and returns
	// 1 if the point is closer than the one on the texture, else it returns 0.

	float shadow = shadowSubUniform();

	Color = vec4((diffuseModel(Position, Normal, Albedo.xyz) * shadow) + Ambient, 1.0f);
}