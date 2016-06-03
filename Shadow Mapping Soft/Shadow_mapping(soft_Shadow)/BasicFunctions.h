/*
Title: Shadow mapping (Soft Shadows)
File Name: main.cpp
Copyright � 2015
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

#include "GLIncludes.h"

GLuint renderProgram;		//This program contains the shader which are used to render the final image and do the final calculations

// Global data members
#pragma region Base_data
// This is your reference to your shader program.
// This will be assigned with glCreateProgram().
// This program will run on your GPU.
GLuint program;

// These are your references to your actual compiled shaders
GLuint vertex_shader;
GLuint fragment_shader;

// This is a reference to your uniform MVP matrix in your vertex shader
GLuint uniMVP;

// Reference to the window object being created by GLFW.
GLFWwindow* window;
#pragma endregion Base_data								  

//This struct consists of the basic stuff needed for getting the shape on the screen.
struct stuff_for_drawing{

	GLuint vao;

	//This stores the address the buffer/memory in the GPU. It acts as a handle to access the buffer memory in GPU.
	GLuint vbo;

	//This will be used to tell the GPU, how many vertices will be needed to draw during drawcall.
	int numberOfVertices;

	//This function gets the number of vertices and all the vertex values and stores them in the buffer.
	void initBuffer(int numVertices, VertexFormat* vertices)
	{
		numberOfVertices = numVertices;

		glGenVertexArrays(1, &vao);

		// This generates buffer object names
		// The first parameter is the number of buffer objects, and the second parameter is a pointer to an array of buffer objects (yes, before this call, vbo was an empty variable)
		glGenBuffers(1, &vbo);

		glBindVertexArray(vao);
		//// Binds a named buffer object to the specified buffer binding point. Give it a target (GL_ARRAY_BUFFER) to determine where to bind the buffer.
		//// There are several different target parameters, GL_ARRAY_BUFFER is for vertex attributes, feel free to Google the others to find out what else there is.
		//// The second paramter is the buffer object reference. If no buffer object with the given name exists, it will create one.
		//// Buffer object names are unsigned integers (like vbo). Zero is a reserved value, and there is no default buffer for each target (targets, like GL_ARRAY_BUFFER).
		//// Passing in zero as the buffer name (second parameter) will result in unbinding any buffer bound to that target, and frees up the memory.
		glBindBuffer(GL_ARRAY_BUFFER, vbo);

		//// Creates and initializes a buffer object's data.
		//// First parameter is the target, second parameter is the size of the buffer, third parameter is a pointer to the data that will copied into the buffer, and fourth parameter is the 
		//// expected usage pattern of the data. Possible usage patterns: GL_STREAM_DRAW, GL_STREAM_READ, GL_STREAM_COPY, GL_STATIC_DRAW, GL_STATIC_READ, GL_STATIC_COPY, GL_DYNAMIC_DRAW, 
		//// GL_DYNAMIC_READ, or GL_DYNAMIC_COPY
		//// Stream means that the data will be modified once, and used only a few times at most. Static means that the data will be modified once, and used a lot. Dynamic means that the data 
		//// will be modified repeatedly, and used a lot. Draw means that the data is modified by the application, and used as a source for GL drawing. Read means the data is modified by 
		//// reading data from GL, and used to return that data when queried by the application. Copy means that the data is modified by reading from the GL, and used as a source for drawing.
		glBufferData(GL_ARRAY_BUFFER, sizeof(VertexFormat) * numVertices, vertices, GL_STATIC_DRAW);

		//// By default, all client-side capabilities are disabled, including all generic vertex attribute arrays.
		//// When enabled, the values in a generic vertex attribute array will be accessed and used for rendering when calls are made to vertex array commands (like glDrawArrays/glDrawElements)
		//// A GL_INVALID_VALUE will be generated if the index parameter is greater than or equal to GL_MAX_VERTEX_ATTRIBS
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexFormat), (void*)16);
		
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VertexFormat), (void*)28);

		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(VertexFormat), (void*)0);

		glBindVertexArray(0);
	}
};


struct Sphere
{
	glm::vec3 origin;
	float radius;
	glm::mat4 MVP;
	glm::mat4 ModelView;
	glm::mat3 NormalMatrix;
	stuff_for_drawing base;
}sphere1, sphere2;


struct Plane
{
	//Construct the plane here 
	stuff_for_drawing base;
	unsigned int numberOfVertices;
	glm::mat4 MVP;
	glm::mat4 ModelView;
	glm::mat3 NormalMatrix;
	glm::vec3 origin;

	void initBuffer()
	{
		numberOfVertices = 6;

		VertexFormat A, B, C, D;

		/*
		
			A-----------------------B
			|						|
			|						|
			C-----------------------D

		*/

		A.position = glm::vec3(-10.0f, 0.0f, -10.0f);
		A.normal = glm::vec3(0.0f, 1.0f, 0.0f);
		A.color = glm::vec4(0.75f, 0.75f, 0.75f, 1.0f);

		B.position = glm::vec3(10.0f, 0.0f, -10.0f);
		B.normal = glm::vec3(0.0f, 1.0f, 0.0f);
		B.color = glm::vec4(0.75f, 0.75f, 0.75f, 1.0f);

		C.position = glm::vec3(-10.0f, 0.0f, 10.0f);
		C.normal = glm::vec3(0.0f, 1.0f, 0.0f);
		C.color = glm::vec4(0.75f, 0.75f, 0.75f, 1.0f);

		D.position = glm::vec3(10.0f, 0.0f, 10.0f);
		D.normal = glm::vec3(0.0f, 1.0f, 0.0f);
		D.color = glm::vec4(0.75f, 0.75f, 0.75f, 1.0f);

		std::vector<VertexFormat> planeVerts;
		
		planeVerts.push_back(A);
		planeVerts.push_back(B);
		planeVerts.push_back(C);

		planeVerts.push_back(B);
		planeVerts.push_back(D);
		planeVerts.push_back(C);

		numberOfVertices = 6;
		base.initBuffer(numberOfVertices, &planeVerts[0]);

		origin = glm::vec3(0.0f, -0.5f, 0.0f);
	}

}plane;


std::string readShader(std::string fileName)
{
	std::string shaderCode;
	std::string line;

	// We choose ifstream and std::ios::in because we are opening the file for input into our program.
	// If we were writing to the file, we would use ofstream and std::ios::out.
	std::ifstream file(fileName, std::ios::in);

	// This checks to make sure that we didn't encounter any errors when getting the file.
	if (!file.good())
	{
		std::cout << "Can't read file: " << fileName.data() << std::endl;

		// Return so we don't error out.
		return "";
	}

	// ifstream keeps an internal "get" position determining the location of the element to be read next
	// seekg allows you to modify this location, and tellg allows you to get this location
	// This location is stored as a streampos member type, and the parameters passed in must be of this type as well
	// seekg parameters are (offset, direction) or you can just use an absolute (position).
	// The offset parameter is of the type streamoff, and the direction is of the type seekdir (an enum which can be ios::beg, ios::cur, or ios::end referring to the beginning, 
	// current position, or end of the stream).
	file.seekg(0, std::ios::end);					// Moves the "get" position to the end of the file.
	shaderCode.resize((unsigned int)file.tellg());	// Resizes the shaderCode string to the size of the file being read, given that tellg will give the current "get" which is at the end of the file.
	file.seekg(0, std::ios::beg);					// Moves the "get" position to the start of the file.

	// File streams contain two member functions for reading and writing binary data (read, write). The read function belongs to ifstream, and the write function belongs to ofstream.
	// The parameters are (memoryBlock, size) where memoryBlock is of type char* and represents the address of an array of bytes are to be read from/written to.
	// The size parameter is an integer that determines the number of characters to be read/written from/to the memory block.
	file.read(&shaderCode[0], shaderCode.size());	// Reads from the file (starting at the "get" position which is currently at the start of the file) and writes that data to the beginning
	// of the shaderCode variable, up until the full size of shaderCode. This is done with binary data, which is why we must ensure that the sizes are all correct.

	file.close(); // Now that we're done, close the file and return the shaderCode.

	return shaderCode;
}

// This method will consolidate some of the shader code we've written to return a GLuint to the compiled shader.
// It only requires the shader source code and the shader type.
GLuint createShader(std::string sourceCode, GLenum shaderType)
{
	// glCreateShader, creates a shader given a type (such as GL_VERTEX_SHADER) and returns a GLuint reference to that shader.
	GLuint shader = glCreateShader(shaderType);
	const char *shader_code_ptr = sourceCode.c_str(); // We establish a pointer to our shader code string
	const int shader_code_size = sourceCode.size();   // And we get the size of that string.

	// glShaderSource replaces the source code in a shader object
	// It takes the reference to the shader (a GLuint), a count of the number of elements in the string array (in case you're passing in multiple strings), a pointer to the string array 
	// that contains your source code, and a size variable determining the length of the array.
	glShaderSource(shader, 1, &shader_code_ptr, &shader_code_size);
	glCompileShader(shader); // This just compiles the shader, given the source code.

	GLint isCompiled = 0;

	// Check the compile status to see if the shader compiled correctly.
	glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);

	if (isCompiled == GL_FALSE)
	{
		char infolog[1024];
		glGetShaderInfoLog(shader, 1024, NULL, infolog);

		// Print the compile error.
		std::cout << "The shader failed to compile with the error:" << std::endl << infolog << std::endl;

		// Provide the infolog in whatever manor you deem best.
		// Exit with failure.
		glDeleteShader(shader); // Don't leak the shader.

		// NOTE: I almost always put a break point here, so that instead of the program continuing with a deleted/failed shader, it stops and gives me a chance to look at what may 
		// have gone wrong. You can check the console output to see what the error was, and usually that will point you in the right direction.
	}

	return shader;
}

// Initialization code
void init()
{
	// Initializes the glew library
	glewInit();

	// Enables the depth test, which you will want in most cases. You can disable this in the render loop if you need to.
	glEnable(GL_DEPTH_TEST);

	// Read in the shader code from a file.
	std::string vertShader = readShader("VertexShader.glsl");
	std::string fragShader = readShader("FragmentShader.glsl");

	// createShader consolidates all of the shader compilation code
	vertex_shader = createShader(vertShader, GL_VERTEX_SHADER);
	fragment_shader = createShader(fragShader, GL_FRAGMENT_SHADER);

	// A shader is a program that runs on your GPU instead of your CPU. In this sense, OpenGL refers to your groups of shaders as "programs".
	// Using glCreateProgram creates a shader program and returns a GLuint reference to it.
	program = glCreateProgram();
	glAttachShader(program, vertex_shader);		// This attaches our vertex shader to our program.
	glAttachShader(program, fragment_shader);	// This attaches our fragment shader to our program.

	// This links the program, using the vertex and fragment shaders to create executables to run on the GPU.
	glLinkProgram(program);

	uniMVP = glGetUniformLocation(program, "MVP");



	vertShader = readShader("LightVertexShader.glsl");
	fragShader = readShader("LightFragShader.glsl");

	vertex_shader = createShader(vertShader, GL_VERTEX_SHADER);
	fragment_shader = createShader(fragShader, GL_FRAGMENT_SHADER);

	renderProgram = glCreateProgram();
	glAttachShader(renderProgram, vertex_shader);
	glAttachShader(renderProgram, fragment_shader);
	glLinkProgram(renderProgram);

	glFrontFace(GL_CW);
	glEnable(GL_CULL_FACE);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}
