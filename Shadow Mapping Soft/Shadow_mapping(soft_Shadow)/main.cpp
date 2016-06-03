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

#pragma once
#include "GLIncludes.h"
#include "BasicFunctions.h"

#define PI 3.14159265
#define WindowSize 800
#define DIVISIONS 40
#define TextureSize 800.0f
#define speed 0.3f

//Handle to the texture storing the depth
GLuint depthTex;
//Handle to the texture storing the offsets.
GLuint offsetTex;
//Handle to the FBO to which depthTex will be attached.
GLuint fboHandle;

GLuint shadowType;

glm::vec3 offsetTexSize;
glm::mat4 PV;

// A struct to hold the handle to the uniforms in the shader.
struct shaderParams
{
	GLuint vec3_LightPos;
	GLuint vec3_LightIntensity;
	GLuint mat4_MVP;
	GLuint mat4_ModelViewMatrix;
	GLuint mat3_NormalMatrix;
	GLuint mat4_ShadowMatrix;
	GLuint sub_shadow;
	GLuint sampler_offsetTex;
	GLuint vec3_offsetSize;

	//Handles to subroutines in the shader
	GLuint sub_func_basicShadow;
	GLuint sub_func_PCFshadow;
	GLuint sub_func_randomSamplingShadow;

	//This function retrieves the handle to the uniforms and stores it in the respective variables.
	void initUniforms(GLuint programID)
	{
		glUseProgram(programID);
		vec3_LightPos = glGetUniformLocation(programID, "pointLight.position");
		vec3_LightIntensity = glGetUniformLocation(programID, "pointLight.Intensity");
		mat4_MVP = glGetUniformLocation(programID, "MVP");
		mat4_ModelViewMatrix = glGetUniformLocation(programID, "ModelViewMatrix");
		mat3_NormalMatrix = glGetUniformLocation(programID, "NormalMatrix");
		mat4_ShadowMatrix = glGetUniformLocation(programID, "ShadowMatrix");
		sub_shadow = glGetUniformLocation(programID, "shadowSubUniform");
		vec3_offsetSize = glGetUniformLocation(programID, "OffsetTexsize");

		sub_func_basicShadow = glGetSubroutineIndex(programID, GL_FRAGMENT_SHADER, "basicShadow");
		sub_func_PCFshadow = glGetSubroutineIndex(programID, GL_FRAGMENT_SHADER, "PCFshadow");
		sub_func_randomSamplingShadow = glGetSubroutineIndex(programID, GL_FRAGMENT_SHADER, "randomSamplingShadow");
	}
	
}uniforms;

// A struct to store the light's data.
struct LightParams
{
	glm::vec3 forward;
	glm::vec3 position;
	glm::vec3 Intensity;

	glm::mat4 Bias;
	glm::mat4 Projection;
	glm::mat4 View;
	glm::mat4 S;			// S = Bias * Projection * View * by the model matrix of the object being rendered
	
	void initMatrices()
	{
		position = glm::vec3(1.0f, 10.0f, 0.0f);
		Intensity = glm::vec3(1.0f, 1.0f, 1.0f);
		forward = glm::vec3(0);

		//Bias matrix is used to convert the position form screen space (-1 to 1) to texture space ( 0 to 1)
		Bias = {0.5f, 0.0f, 0.0f, 0.0f,
				0.0f, 0.5f, 0.0f, 0.0f,
				0.0f, 0.0f, 0.5f, 0.0f,
				0.5f, 0.5f, 0.5f, 1.0f};
		
		//calculate the view and projection matrix from the perspective of the light
		Projection = glm::perspective(45.0f, TextureSize / TextureSize, 0.1f, 100.0f);
		View = glm::lookAt(position, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));//glm::lookAt(position, forward, glm::vec3(0.0f, 0.0f, 1.0f));
		S = Bias * (Projection * (View));
	}

	//this functions re-calculates the matrices when the position of the light changes.
	void recaliberate()
	{
		View = glm::lookAt(position, forward, glm::vec3(0.0f, 1.0f, 0.0f));
		S = Bias * (Projection * (View));
	}

}light;


float jitter()
{
	return ((float)rand() / (float) RAND_MAX) - 0.5f;
}

//Function to build the offset texture
GLuint buildOffsetTex(int size, int samplesU, int samplesV)
{

	int samples = samplesU * samplesV;
	int bufSize = size * size * samples * 2;
	float* data = new float[bufSize];
	int x1, x2, y1, y2;

	for (int i = 0; i < size; i++)
	{
		for (int j = 0; j < size; j++)
		{
			for (int k = 0; k < samples; k += 2)
			{
				x1 = k % (samplesU);
				y1 = (samples - 1 - k) / samplesU;
				x2 = (k + 1) & samplesU;
				y2 = (samples - 1 - k - 1) / samplesU;

				glm::vec4 v;

				//Center on grid and jitter
				v.x = (x1 + 0.5f) + jitter();
				v.y = (y1 + 0.5f) + jitter();
				v.z = (x1 + 0.5f) + jitter();
				v.w = (y1 + 0.5f) + jitter();

				//Scale between 0 and 1
				v.x /= samplesU;
				v.y /= samplesV;
				v.z /= samplesU;
				v.w /= samplesV;

				//Warp to disk
				int cell = ((k / 2) * size * size + j * size + i) * 4;
				data[cell + 0] = sqrtf(v.y) * cosf(2 * PI * v.x);
				data[cell + 1] = sqrtf(v.y) * cosf(2 * PI * v.x);
				data[cell + 2] = sqrtf(v.w) * cosf(2 * PI * v.z);
				data[cell + 3] = sqrtf(v.w) * cosf(2 * PI * v.z);
			}
		}
	}

	glActiveTexture(GL_TEXTURE1);
	GLuint texID;
	glGenTextures(1, &texID);

	glBindTexture(GL_TEXTURE_3D, texID);
	glTexStorage3D(GL_TEXTURE_3D, 1, GL_RGBA32F, size, size, samples / 2);
	glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, size, size, samples / 2, GL_RGBA, GL_FLOAT, data);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	delete[] data;

	return texID;
}

//This function sets up the geometry we will render. 
void createGeometry()
{
	std::vector<VertexFormat> vertices;

	float radius = 0.5f;
	float pitch, yaw;
	yaw = 0.0f;
	pitch = 0.0f;
	int i, j;
	float pitchDelta = 360 / DIVISIONS;
	float yawDelta = 360 / DIVISIONS;
	glm::vec4 color(0.3f, 0.2f, 0.7f, 2.0f);

	VertexFormat p1, p2, p3, p4;

	for (i = 0; i < DIVISIONS; i++)
	{
		for (j = 0; j < DIVISIONS; j++)
		{
			p1.position.x = radius * sin((pitch)* PI / 180.0) * cos((yaw)* PI / 180.0);
			p1.position.y = radius * sin((pitch)* PI / 180.0) * sin((yaw)* PI / 180.0);;
			p1.position.z = radius * cos((pitch)* PI / 180.0);
			p1.normal = p1.position;
			p1.color = color;


			p2.position.x = radius * sin((pitch)* PI / 180.0) * cos((yaw + yawDelta)* PI / 180.0);
			p2.position.y = radius * sin((pitch)* PI / 180.0) * sin((yaw + yawDelta)* PI / 180.0);;
			p2.position.z = radius * cos((pitch)* PI / 180.0);
			p2.normal = p2.position;
			p2.color = color;

			p3.position.x = radius * sin((pitch + pitchDelta)* PI / 180.0) * cos((yaw + yawDelta)* PI / 180.0);
			p3.position.y = radius * sin((pitch + pitchDelta)* PI / 180.0) * sin((yaw + yawDelta)* PI / 180.0);;
			p3.position.z = radius * cos((pitch + pitchDelta)* PI / 180.0);
			p3.normal = p3.position;
			p3.color = color;

			p4.position.x = radius * sin((pitch + pitchDelta)* PI / 180.0) * cos((yaw)* PI / 180.0);
			p4.position.y = radius * sin((pitch + pitchDelta)* PI / 180.0) * sin((yaw)* PI / 180.0);;
			p4.position.z = radius * cos((pitch + pitchDelta)* PI / 180.0);
			p4.normal = p4.position;
			p4.color = color;

			vertices.push_back(p1);
			vertices.push_back(p2);
			vertices.push_back(p3);
			vertices.push_back(p1);
			vertices.push_back(p3);
			vertices.push_back(p4);

			yaw = yaw + yawDelta;
		}

		pitch += pitchDelta;
	}

	sphere1.base.initBuffer(vertices.size(), &vertices[0]);
	sphere2.base.initBuffer(vertices.size(), &vertices[0]);

	sphere1.origin = glm::vec3(0.0f);
	sphere2.origin = glm::vec3(-1.0f, 0.0f, -2.0f);
	sphere1.radius = radius;
	sphere2.radius = radius;
}

void setFrameBUffer()
{
	GLfloat border[] = { 1.0f, 0.0f, 0.0f, 0.0f };

	glEnable(GL_TEXTURE_2D);

	//generate and bind fbo
	glGenFramebuffers(1, &fboHandle);
	glBindFramebuffer(GL_FRAMEBUFFER, fboHandle);

	//generate the depth buffer
	glGenTextures(1, &depthTex);
	glBindTexture(GL_TEXTURE_2D, depthTex);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT32, TextureSize, TextureSize);
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, TextureSize, TextureSize, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
	//Linear filtering for anti-aliasing purposes
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	// if the texture coordinate, is out of bounds, we want it to return true and not false. hence we set the border value.
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);
	// by setting the compare mode, the texture sampling won't return a rgb value, instead it will return 1 or 0 based on 
	// comparing the the current depth woth the value stored in the texture.
	// For sampling, instead of using texture(), we shall use textureProj()
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LESS);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, depthTex);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTex, 0);

	GLenum drawbuf[] = { GL_NONE };

	glDrawBuffers(1, drawbuf);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "Frame buffer not created. \n" << glCheckFramebufferStatus(GL_FRAMEBUFFER);

	//unbind the frame buffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void setup()
{
	setFrameBUffer();

	createGeometry();

	plane.initBuffer();
	
	glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 1.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 proj = glm::perspective(45.0f, 800.0f / 800.0f, 0.1f, 100.0f);

	PV = proj * view;

	sphere1.MVP = PV * glm::translate(glm::mat4(1), sphere1.origin);
	sphere1.ModelView = view * glm::translate(glm::mat4(1), sphere1.origin);
	sphere1.NormalMatrix = glm::transpose(glm::inverse( glm::mat3(sphere1.ModelView)));

	sphere2.MVP = PV * glm::translate(glm::mat4(1), sphere2.origin);
	sphere2.ModelView = view * glm::translate(glm::mat4(1), sphere2.origin);
	sphere2.NormalMatrix = glm::transpose(glm::inverse(glm::mat3(sphere2.ModelView)));

	plane.MVP = PV * glm::translate(glm::mat4(1), plane.origin);
	plane.ModelView = view * glm::translate(glm::mat4(1), plane.origin);
	plane.NormalMatrix = glm::transpose(glm::inverse(glm::mat3(plane.ModelView)));

	light.initMatrices();

	uniforms.initUniforms(renderProgram);

	offsetTex = buildOffsetTex(16, 4, 8);
	offsetTexSize = glm::vec3(16, 4, 8);

	shadowType = uniforms.sub_func_basicShadow;
}

// Functions called between every frame. game logic
#pragma region util_functions

// This runs once every physics timestep.
void update()
{
}

void firstDrawPass()
{
	glUseProgram(program);

	// GL_Polygonoffset displaces the depth value by an offest which is computed using the values we give as parameters.
	// the first parameter is multiplied by the depth slope and the second parameter is multiplied by "r" which is the smallest value to imply a change in depth.
	// Commenting out the two lines below would produce "shadow acne".
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(10.0f, 15.0f);
	
	//Render from the perspective of the camera
	glBindFramebuffer(GL_FRAMEBUFFER, fboHandle);
	glClear(GL_DEPTH_BUFFER_BIT);
	//glClearDepth(0.5f);
	glViewport(0, 0, WindowSize, WindowSize);
	{
		glCullFace(GL_FRONT);
		glm::mat4 MVP;
		glm::mat4 PV = light.Projection * light.View;

		//Plane
		MVP = PV * (glm::translate(glm::mat4(1), plane.origin));
		glUniformMatrix4fv(uniMVP, 1, GL_FALSE, glm::value_ptr(MVP));
		glBindVertexArray(plane.base.vao);
		glBindBuffer(GL_ARRAY_BUFFER, plane.base.vbo);
		glDrawArrays(GL_TRIANGLES, 0, plane.numberOfVertices);

		//Sphere1
		MVP = PV * (glm::translate(glm::mat4(1), sphere1.origin));
		glUniformMatrix4fv(uniMVP, 1, GL_FALSE, glm::value_ptr(MVP));
		glBindVertexArray(sphere1.base.vao);
		glBindBuffer(GL_ARRAY_BUFFER, sphere1.base.vbo);
		glDrawArrays(GL_TRIANGLES, 0, sphere1.base.numberOfVertices);

		//Sphere2
		MVP = PV * (glm::translate(glm::mat4(1), sphere2.origin));
		glUniformMatrix4fv(uniMVP, 1, GL_FALSE, glm::value_ptr(MVP));
		glBindVertexArray(sphere2.base.vao);
		glBindBuffer(GL_ARRAY_BUFFER, sphere2.base.vbo);
		glDrawArrays(GL_TRIANGLES, 0, sphere2.base.numberOfVertices);

	}

	glDisable(GL_POLYGON_OFFSET_FILL);
}

void secondDrawPass()
{

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	// This function acts on the frabe buffer currently in use. 
	// So if we use this statement before unbinding the framebuffer, it will clear the depth texture attached to it and also all the data we had stored in it.
	glClear(GL_DEPTH_BUFFER_BIT);			
	glUseProgram(renderProgram);
	
	//Rendering to the main window.
	// We have to calculate the shadow matrix for each game object and pass it into the shader
	glViewport(0, 0, WindowSize, WindowSize);
	{
		glCullFace(GL_BACK);
		
		//load the two textures: the shadow map and offsetTexture
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, depthTex);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, offsetTex);

		//Set the subroutine 
		glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 1, &shadowType);
		glUniform3fv(uniforms.vec3_LightPos, 1, glm::value_ptr(light.position));
		glUniform3fv(uniforms.vec3_LightIntensity, 1, glm::value_ptr(light.Intensity));
		glUniform3fv(uniforms.vec3_offsetSize, 1, glm::value_ptr(offsetTexSize));

		glm::mat4 shadowMat;
		
		//Sphere1
		glUniformMatrix4fv(uniforms.mat4_MVP, 1, GL_FALSE, glm::value_ptr(sphere1.MVP));
		glUniformMatrix4fv(uniforms.mat4_ModelViewMatrix, 1, GL_FALSE, glm::value_ptr(sphere1.ModelView));
		glUniformMatrix3fv(uniforms.mat3_NormalMatrix, 1, GL_FALSE, glm::value_ptr(sphere1.NormalMatrix));
		shadowMat = light.S * glm::translate(glm::mat4(1), sphere1.origin);	//Calculating the shadow matrix
		glUniformMatrix4fv(uniforms.mat4_ShadowMatrix, 1, GL_FALSE, glm::value_ptr(shadowMat));
		glBindVertexArray(sphere1.base.vao);
		glBindBuffer(GL_ARRAY_BUFFER, sphere1.base.vbo);
		glDrawArrays(GL_TRIANGLES, 0, sphere1.base.numberOfVertices);

		//Sphere2
		glUniformMatrix4fv(uniforms.mat4_MVP, 1, GL_FALSE, glm::value_ptr(sphere2.MVP));
		glUniformMatrix4fv(uniforms.mat4_ModelViewMatrix, 1, GL_FALSE, glm::value_ptr(sphere2.ModelView));
		glUniformMatrix3fv(uniforms.mat3_NormalMatrix, 1, GL_FALSE, glm::value_ptr(sphere2.NormalMatrix));
		shadowMat = light.S * glm::translate(glm::mat4(1), sphere2.origin);
		glUniformMatrix4fv(uniforms.mat4_ShadowMatrix, 1, GL_FALSE, glm::value_ptr(shadowMat));
		glBindVertexArray(sphere2.base.vao);
		glBindBuffer(GL_ARRAY_BUFFER, sphere2.base.vbo);
		glDrawArrays(GL_TRIANGLES, 0, sphere2.base.numberOfVertices);

		//Plane
		glUniformMatrix4fv(uniforms.mat4_MVP, 1, GL_FALSE, glm::value_ptr(plane.MVP));
		glUniformMatrix4fv(uniforms.mat4_ModelViewMatrix, 1, GL_FALSE, glm::value_ptr(plane.ModelView));
		glUniformMatrix3fv(uniforms.mat3_NormalMatrix, 1, GL_FALSE, glm::value_ptr(plane.NormalMatrix));
		shadowMat = light.S * glm::translate(glm::mat4(1), plane.origin);
		glUniformMatrix4fv(uniforms.mat4_ShadowMatrix, 1, GL_FALSE, glm::value_ptr(shadowMat));
		glBindVertexArray(plane.base.vao);
		glBindBuffer(GL_ARRAY_BUFFER, plane.base.vbo);
		glDrawArrays(GL_TRIANGLES, 0, plane.numberOfVertices);
	}
}

// This function runs every frame
void renderScene()
{
	// Clear the color buffer and the depth buffer
	glClear(GL_COLOR_BUFFER_BIT);

	// Clear the screen to white
	glClearColor(1.0, 1.0, 1.0, 1.0);

	firstDrawPass();

	secondDrawPass();
}

#pragma endregion Helper_functions

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	//This set of controls are used to move the light source 
	if ((action == GLFW_PRESS || action == GLFW_REPEAT))
	{
		if (key == GLFW_KEY_W)
			light.position += glm::vec3(0, 0, -1) * speed;
		if (key == GLFW_KEY_S)
			light.position += glm::vec3(0, 0, 1) * speed;
		if (key == GLFW_KEY_D)
			light.position += glm::vec3(1, 0, 0) * speed;
		if (key == GLFW_KEY_A)
			light.position += glm::vec3(-1, 0, 0) * speed;
		if (key == GLFW_KEY_SPACE)
			light.position += glm::vec3(0, 1, 0) * speed;
		if (key == GLFW_KEY_LEFT_SHIFT && light.position.y > 10.0f)
			light.position += glm::vec3(0, -1, 0) * speed;
		if (key == GLFW_KEY_R)
			light.position = glm::vec3(0.1f, 10, 0);

		if (key == GLFW_KEY_1)
			shadowType = uniforms.sub_func_basicShadow;
		if (key == GLFW_KEY_2)
			shadowType = uniforms.sub_func_PCFshadow;
		if (key == GLFW_KEY_3)
			shadowType = uniforms.sub_func_randomSamplingShadow;

		//Once the light source is changed, the matrices need to be recalculated
		light.recaliberate();
	}
}

void main()
{
	glfwInit();

	// Creates a window given (width, height, title, monitorPtr, windowPtr).
	// Don't worry about the last two, as they have to do with controlling which monitor to display on and having a reference to other windows. Leaving them as nullptr is fine.
	window = glfwCreateWindow(WindowSize, WindowSize, "Shadow Mapping", nullptr, nullptr);
	std::cout << "This example demonstrates the implementation of shadow mapping technique.";
	std::cout << "This example produces soft shadows.\n";
	std::cout << "Use 'w' 'a' 's' 'd' to move the light source in x-z plane.\n";
	std::cout << "you can also use 'left shift' and 'Space' to move the light source higher or lower.\n";
	std::cout << "Use '1' for Hard shadows.\nUse '1' for soft shadows using PFC.\nUse '1' for soft shadows with random sampling.";
	// Makes the OpenGL context current for the created window.
	glfwMakeContextCurrent(window);

	// Sets the number of screen updates to wait before swapping the buffers.
	// Setting this to zero will disable VSync, which allows us to actually get a read on our FPS. Otherwise we'd be consistently getting 60FPS or lower, 
	// since it would match our FPS to the screen refresh rate.
	// Set to 1 to enable VSync.
	glfwSwapInterval(0);

	// Initializes most things needed before the main loop
	init();

	glfwSetKeyCallback(window, key_callback);

	setup();

	// Enter the main loop.
	while (!glfwWindowShouldClose(window))
	{
		// Call to update() which will update the gameobjects.
		update();

		// Call the render function.
		renderScene();

		// Swaps the back buffer to the front buffer
		// Remember, you're rendering to the back buffer, then once rendering is complete, you're moving the back buffer to the front so it can be displayed.
		glfwSwapBuffers(window);

		// Checks to see if any events are pending and then processes them.
		glfwPollEvents();
	}

	// After the program is over, cleanup your data!
	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);
	glDeleteProgram(program);
	// Note: If at any point you stop using a "program" or shaders, you should free the data up then and there.


	// Frees up GLFW memory
	glfwTerminate();
}