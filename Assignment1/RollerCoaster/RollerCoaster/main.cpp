// CPSC 587 Assignment 1
// Albert Chu 10059388
// Adapted and using code originally written by Andrew Owens
#include <sstream>

#include<iostream>
#include<cmath>

#include<GL/glew.h> // include BEFORE glut
#include<GL/freeglut.h>

#include <glm/gtx/string_cast.hpp>
#include<glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "ShaderTools.h"
#include <math.h>
#include <algorithm>    // std::max



GLuint structProgramID;
GLuint vaoStructID;
GLuint structVertBufferID;
GLuint structColorBufferID;

// Could store these in some data structure
GLuint trackProgramID;
GLuint vaoTrackID;
GLuint trackVertBufferID;
GLuint trackColorBufferID; 
float track_max_height;
float track_min_height;

GLuint vaoCarID;
GLuint carProgramID;
GLuint carVertBufferID;
GLuint carColorBufferID;
glm::vec3 carCenter;

glm::mat4 trackMVP;
glm::mat4 carMVP;
glm::mat4 carM; // model matrix
glm::mat4 trackM; // model matrix

glm::mat4 V; // view matrix
glm::mat4 P; // projection matrix can be left alone

int curIndex = 0; // The vertex immediately behind the car
int delay = 1;
float GRAVITY = 9.81f;
float car_scale = 0.5f;

int WIN_WIDTH = 800, WIN_HEIGHT = 600;

// function declarations... just to keep things kinda organized.
void displayFunc();
void resizeFunc();
void idleFunc();
void init();
void generateIDs();
void deleteIDs();
void setupVAO();
void loadBuffer();
void loadProjectionMatrix();
void loadModelViewMatrix();
void setupModelViewProjectionTransform();
void reloadMVPUniform();
glm::vec3 center(std::vector<glm::vec3> verts);
void set_tnb_frame(float speed, glm::mat4 & modelMatrix, std::vector<glm::vec3> & verts, int & index);
void moveCarTo(glm::vec3 new_pos);
glm::vec3 getMatrixColumn(glm::mat4 matrix, int column);
void updateMatrixColumn(glm::mat4 & matrix, int column, glm::vec3 vector);
float get_speed(glm::vec3 position);
int main( int, char** );


// function declarations
std::vector< glm::vec3 > track_verts;
std::vector< glm::vec3 > rail_verts;	// This is what is going to end up being rendered for the track
std::vector< glm::vec3 > struct_verts;	// Roller coaster supports
std::vector< glm::vec3 > car_verts;

bool left_click = false;
bool right_click = false;
bool translate_bool = true;
float delta_x = 0;
float delta_y = 0;
float delta_z = 0;

int lastTime;

//typedef std::vector< Vec3f > VecV3f;

void printInfo()
{
	std::cout << "Using GLEW " << glewGetString(GLEW_VERSION) << std::endl;
	std::cout << "Vendor: " << glGetString(GL_VENDOR) << std::endl;
	std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;
	std::cout << "Version: " << glGetString(GL_VERSION) << std::endl;
	std::cout << "GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
}

void render()
{
	setupModelViewProjectionTransform();
	// send changes to GPU
	reloadMVPUniform();
	glutPostRedisplay();
}

void loadVec3FromFile(std::vector<glm::vec3> & vecs, std::string const & fileName)
{
	std::ifstream file(fileName);

	if (!file)
	{
		throw std::runtime_error("Unable to open file.");
	}

	std::string line;
	size_t index;
	std::stringstream ss(std::ios_base::in);

	size_t lineNum = 0;
	vecs.clear();

	while (getline(file, line))
	{
		++lineNum;

		// remove comments	
		index = line.find_first_of("#");
		if (index != std::string::npos)
		{
			line.erase(index, std::string::npos);
		}

		// removes leading/tailing junk
		line.erase(0, line.find_first_not_of(" \t\r\n\v\f"));
		index = line.find_last_not_of(" \t\r\n\v\f") + 1;
		if (index != std::string::npos)
		{
			line.erase(index, std::string::npos);
		}

		if (line.empty())
		{
			continue; // empty or commented out line
		}

		ss.str(line);
		ss.clear();

		float x, y, z;
		if ((ss >> x >> y >> z) && (!ss || !ss.eof() || ss.good()))
		{
			throw std::runtime_error("Error read file: "
				+ line
				+ " (line: "
				+ std::to_string(lineNum)
				+ ")");
		}
		else
		{
			vecs.push_back(glm::vec3(x,y,z));
		}
	}
	file.close();
}

void displayFunc()
{
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	// Draw track
	glUseProgram(trackProgramID);
	glBindVertexArray(vaoTrackID);
	GLfloat width = 5;
	glLineWidth(width);
	//glDrawArrays(GL_POINTS, 0, track_verts.size());
	glDrawArrays(GL_LINE_LOOP, 0, rail_verts.size());

	// Draw Struct
	glBindVertexArray(vaoStructID);
	width = 25;
	glLineWidth(width);
	//glDrawArrays(GL_POINTS, 0, track_verts.size());
	glDrawArrays(GL_LINE_LOOP, 0, struct_verts.size());

	// Draw car
	glUseProgram(carProgramID);
	glBindVertexArray(vaoCarID);
	//glDrawArrays(GL_POINTS, 0, car_verts.size());
	
	glDrawArrays(GL_TRIANGLES, 0, car_verts.size());

	glutSwapBuffers();
}


//Keyboard commands for manipulating the enviorment
void keyboardFunc(unsigned char key, int x, int y){

	if (key == 'o')		// Orient view matrix to identity matrix 
	{
		V = glm::mat4(1.0f);
	}
	if (key == 'r')		// Rotate Mode
		translate_bool = false;
	if (key == 't')		// Translate Mode
		translate_bool = true;

	if (key == 'm')		// Translate Mode
		moveCarTo(glm::vec3(1, 1, 1));;
	if (key == 'n')		// Translate Mode
		moveCarTo(glm::vec3(0, 0, 0));;


	render();
}

void mouseMove(int x, int y) 
{
	if (left_click)
	{
		delta_y -= y;
		delta_x -= x;

		if (translate_bool)
			V = glm::translate(V, glm::vec3(-delta_x / 300, delta_y / 500, 0));
		else
		{
			V = glm::rotate(V, delta_y / 10, glm::vec3(-1.0f, 0.0f, 0.0f));
			V = glm::rotate(V, delta_x / 10, glm::vec3(0.0f, 1.0f, 0.0f));
		}

		delta_x = x;
		delta_y = y;
	}
	if (right_click)
	{
		delta_z -= y;
		V = glm::translate(V, glm::vec3(0, 0, delta_z / 500));
		delta_z = y;
	}
	render();
}

void mouseButton(int button, int state, int x, int y)
{
	switch (button)
	{
	case GLUT_LEFT_BUTTON:
		std::cout << "Left Button" << std::endl;
		if (state == GLUT_DOWN){
			left_click = true;
			delta_x = x;
			delta_y = y;
		}
		else {
			left_click = false;
		}
		break;
	case GLUT_MIDDLE_BUTTON:
		break;
	case GLUT_RIGHT_BUTTON:
		if (state == GLUT_DOWN){
			right_click = true;
			delta_z = y;
		}
		else {
			right_click = false;
		}
		break;
	default:
		std::cerr << "Encountered an error with mouse button : " << button << ", state : " << state << std::endl;
	}
		std::cout << "Button: " << button << ", State: " << state << std::endl;
		printf("Button %s At %d %d\n", (state == GLUT_DOWN) ? "Down" : "Up", x, y);
}

void resizeFunc( int width, int height )
{
    WIN_WIDTH = width;
    WIN_HEIGHT = height;

    glViewport( 0, 0, width, height );

    loadProjectionMatrix();
    reloadMVPUniform();

    glutPostRedisplay();
}

void generateCarID(std::string vsSource, std::string fsSource)
{
	carProgramID = CreateShaderProgram(vsSource, fsSource);

	// load IDs given from OpenGL
	glGenVertexArrays(1, &vaoCarID);
	glGenBuffers(1, &carVertBufferID);
	glGenBuffers(1, &carColorBufferID);
}

void generateTrackID(std::string vsSource, std::string fsSource)
{
	trackProgramID = CreateShaderProgram(vsSource, fsSource);

	// load IDs given from OpenGL
	glGenVertexArrays(1, &vaoTrackID);
	glGenBuffers(1, &trackVertBufferID);
	glGenBuffers(1, &trackColorBufferID);
}

void generateStructID(std::string vsSource, std::string fsSource)
{
	structProgramID = CreateShaderProgram(vsSource, fsSource);

	// load IDs given from OpenGL
	glGenVertexArrays(1, &vaoStructID);
	glGenBuffers(1, &structVertBufferID);
	glGenBuffers(1, &structColorBufferID);
}

void generateIDs()
{
	std::string vsSource = loadShaderStringfromFile("./basic_vs.glsl");
	std::string fsSource = loadShaderStringfromFile("./basic_fs.glsl");

	generateCarID(vsSource, fsSource);
	generateTrackID(vsSource, fsSource);
	generateStructID(vsSource, fsSource);
}

void deleteIDs()
{
	glDeleteProgram(trackProgramID);
	glDeleteProgram(carProgramID);

	glDeleteVertexArrays(1, &vaoTrackID);
	glDeleteBuffers(1, &trackVertBufferID);
	glDeleteBuffers(1, &trackColorBufferID);

	glDeleteVertexArrays(1, &vaoStructID);
	glDeleteBuffers(1, &structVertBufferID);
	glDeleteBuffers(1, &structColorBufferID);

	glDeleteVertexArrays(1, &vaoCarID);
	glDeleteBuffers(1, &carVertBufferID);
	glDeleteBuffers(1, &carColorBufferID);
}

void loadProjectionMatrix()
{
    // Perspective Only
    
	// field of view angle 60 degrees
	// window aspect ratio
	// near Z plane > 0
	// far Z plane

    //P = PerspectiveProjection(  60, // FOV
    //                            static_cast<float>(WIN_WIDTH)/WIN_HEIGHT, // Aspect
    //                            0.01,   // near plane
    //                            50 ); // far plane depth
	P = glm::perspective(60.0f, static_cast<float>(WIN_WIDTH) / WIN_HEIGHT, 0.01f, 1000.f);
	P = glm::translate(P, glm::vec3(-5, -5, -30));
}

void loadModelViewMatrix()
{
	//trackM = UniformScaleMatrix(1);	// glm::scale Quad First
	//trackM = TranslateMatrix(0, 0, -2.0) * trackM;	// glm::translate away from (0,0,0)


	glm::vec3 trackStart = track_verts.at(curIndex);
	curIndex = 1;

	carM = glm::scale(carM, glm::vec3(car_scale));
	moveCarTo(trackStart);
	//carM = glm::translate(carM, glm::vec3(translateX, translateY, translateZ));
}

void setupModelViewProjectionTransform()
{
	trackMVP = P * V * trackM; // transforms vertices from right to left (odd huh?)
	carMVP = P * V * carM; // transforms vertices from right to left (odd huh?)
}
 
void reloadTrackMVPUniform()
{
	GLint mvpID = glGetUniformLocation(trackProgramID, "MVP");

	glUseProgram(trackProgramID);
	glUniformMatrix4fv(mvpID,		// ID
		1,		// only 1 matrix
		GL_FALSE,	// transpose matrix, Mat4f is row major
		glm::value_ptr(trackMVP)	// pointer to data in Mat4f
		);
}

void reloadCarMVPUniform()
{
	GLint mvpID = glGetUniformLocation(carProgramID, "MVP");

	glUseProgram(carProgramID);
	glUniformMatrix4fv(mvpID,		// ID
		1,		// only 1 matrix
		GL_FALSE,	// transpose matrix, Mat4f is row major
		glm::value_ptr(carMVP)	// pointer to data in Mat4f
		);
}

void reloadMVPUniform()
{
	reloadCarMVPUniform();
	reloadTrackMVPUniform();
}


void setupStructVAO()
{
	glBindVertexArray(vaoStructID);

	glEnableVertexAttribArray(0); // match layout # in shader
	glBindBuffer(GL_ARRAY_BUFFER, structVertBufferID);
	glVertexAttribPointer(
		0,		// attribute layout # above
		3,		// # of components (ie XYZ )
		GL_FLOAT,	// type of components
		GL_FALSE,	// need to be normalized?
		0,		// stride
		(void*)0	// array buffer offset
		);

	glEnableVertexAttribArray(1); // match layout # in shader
	glBindBuffer(GL_ARRAY_BUFFER, structColorBufferID);
	glVertexAttribPointer(
		1,		// attribute layout # above
		3,		// # of components (ie XYZ )
		GL_FLOAT,	// type of components
		GL_FALSE,	// need to be normalized?
		0,		// stride
		(void*)0	// array buffer offset
		);

	glBindVertexArray(0); // reset to default		
}

void setupTrackVAO()
{
	glBindVertexArray(vaoTrackID);

	glEnableVertexAttribArray(0); // match layout # in shader
	glBindBuffer(GL_ARRAY_BUFFER, trackVertBufferID);
	glVertexAttribPointer(
		0,		// attribute layout # above
		3,		// # of components (ie XYZ )
		GL_FLOAT,	// type of components
		GL_FALSE,	// need to be normalized?
		0,		// stride
		(void*)0	// array buffer offset
		);

	glEnableVertexAttribArray(1); // match layout # in shader
	glBindBuffer(GL_ARRAY_BUFFER, trackColorBufferID);
	glVertexAttribPointer(
		1,		// attribute layout # above
		3,		// # of components (ie XYZ )
		GL_FLOAT,	// type of components
		GL_FALSE,	// need to be normalized?
		0,		// stride
		(void*)0	// array buffer offset
		);

	glBindVertexArray(0); // reset to default		
}

void setupCarVAO()
{
	glBindVertexArray(vaoCarID);

	glEnableVertexAttribArray(0); // match layout # in shader
	glBindBuffer(GL_ARRAY_BUFFER, carVertBufferID);
	glVertexAttribPointer(
		0,		// attribute layout # above
		3,		// # of components (ie XYZ )
		GL_FLOAT,	// type of components
		GL_FALSE,	// need to be normalized?
		0,		// stride
		(void*)0	// array buffer offset
		);

	glEnableVertexAttribArray(1); // match layout # in shader
	glBindBuffer(GL_ARRAY_BUFFER, carColorBufferID);
	glVertexAttribPointer(
		1,		// attribute layout # above
		3,		// # of components (ie XYZ )
		GL_FLOAT,	// type of components
		GL_FALSE,	// need to be normalized?
		0,		// stride
		(void*)0	// array buffer offset
		);

	glBindVertexArray(0); // reset to default		
}

// Returns a subdivided vector array that adds another point in between each existing point in vector
std::vector< glm::vec3 > subdivide(std::vector< glm::vec3 > verts)
{
	std::vector< glm::vec3 > new_verts;
	for (int i = 0; i < verts.size(); i++)
	{
		glm::vec3 const& v1 = verts[i];
		glm::vec3 const& v2 = verts[(i + 1) % verts.size()];
		glm::vec3 mid = (v1 + v2) * 0.5f;
		new_verts.push_back(v1);
		new_verts.push_back(mid);
	}
	return new_verts;
}

// Returns a subdivided vector array that offsets each existing point in vector halfway to its next neighbor
std::vector< glm::vec3 > offset(std::vector< glm::vec3 > verts)
{
	std::vector< glm::vec3 > new_verts;
	for (int i = 0; i < verts.size(); i++)
	{
		glm::vec3 const& v1 = verts[i];
		glm::vec3 const& v2 = verts[(i + 1) % verts.size()];
		glm::vec3 mid = (v1 + v2) * 0.5f;
		new_verts.push_back(mid);
	}
	return new_verts;
}

// Returns a subdivided vector array that goes to the specified depth of subdivision
std::vector< glm::vec3 > subdivision(std::vector< glm::vec3 > original_verts, int depth)
{
	std::vector< glm::vec3 > new_verts = original_verts;
	for (int i = 0; i < depth; i++)
	{
		new_verts = subdivide(new_verts);
		new_verts = offset(new_verts);
	}
	return new_verts;
}

float getRandFloat(float low, float high)
{
	return (low + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (high - low))));
}

// Sets the max and min y from the vector of vec3s
void set_max_min_y(std::vector<glm::vec3> verts, float & max, float & min)
{
	for (glm::vec3 vert : verts)
	{
		if (vert.y > max)
			max = vert.y;
		if (vert.y < min)
			min = vert.y;
	}
}

// Based on some given track, constructs a vector of rail points at some offset
void set_track_rails(std::vector<glm::vec3> & track, std::vector<glm::vec3> & rails)
{
	std::vector<glm::vec3> in_rail;	//List of inside railling points
	std::vector<glm::vec3> out_rail;	//List of outside railling points
	
	//int track_index = 0;

	float rail_offset = 0.9f;
	for (int i = 0; i < track.size(); i++)
	{
		float speed = get_speed(track[i]) / 4;
		glm::mat4 matrix;
		set_tnb_frame(speed, matrix, track, i);
		glm::vec3 binormal = getMatrixColumn(matrix, 1);
		glm::vec3 in_vert = track[i] + (binormal * rail_offset);
		glm::vec3 out_vert = track[i] - (binormal * rail_offset);

		in_rail.push_back(in_vert);
		out_rail.push_back(out_vert);
	}

	//Order the verts in a way that will render nicely with gl_line_loop
	for (int i = 0; i < track.size(); i++)
	{
		glm::vec3 in1 = in_rail[i];
		glm::vec3 in2 = in_rail[(i + 1) % track.size()];
		glm::vec3 out1 = out_rail[i];
		glm::vec3 out2 = out_rail[(i + 1) % track.size()];
		rails.push_back(in2);
		rails.push_back(in1);

		rails.push_back(in1);
		rails.push_back(out1);

		rails.push_back(out1);
		rails.push_back(out2);

		rails.push_back(out2);
		rails.push_back(in2);
	}
}

// Based on some given track, constructs a vector of rail points at some offset
void set_rail_structs(std::vector<glm::vec3> & rails, std::vector<glm::vec3> & structs, int track_min)
{
	int struct_offset = 15;
	float struct_len = track_min + 15.f;	//This is how far the rollercoaster track appears off the ground
	for (int i = 0; i < rails.size(); i += struct_offset)
	{
		glm::vec3 strut_rail = rails[i % rails.size()];
		glm::vec3 strut_ground = glm::vec3(strut_rail.x, -struct_len, strut_rail.z);
		
		structs.push_back(strut_ground);
		structs.push_back(strut_rail);

		structs.push_back(strut_rail);
		structs.push_back(strut_ground);
	}
}


glm::vec3 getMatrixColumn(glm::mat4 matrix, int column)
{
	return glm::vec3(matrix[column][0], matrix[column][1], matrix[column][2]);
}

// Sets up track vertex buffer from file, subdivide and process vectors to be smooth, Assign colors to the vertex shaders
void loadStructBuffer()
{
	//loadVec3FromFile(track_verts, file_path);

	//track_verts = subdivision(track_verts, 3);

	//set_max_min_y(track_verts, track_max_height, track_min_height);

	//set_track_rails(track_verts, rail_verts);
	set_rail_structs(rail_verts, struct_verts, track_min_height);

	glBindBuffer(GL_ARRAY_BUFFER, structVertBufferID);
	glBufferData(GL_ARRAY_BUFFER,
		sizeof(glm::vec3) * struct_verts.size(),	// byte size of Vec3f, 4 of them
		struct_verts.data(),		// pointer (Vec3f*) to contents of verts
		GL_STATIC_DRAW);	// Usage pattern of GPU buffer

	carCenter = center(car_verts);

	// RGB values for the vertices
	std::vector<glm::vec3> colors;
	for (int i = 0; i < struct_verts.size(); i++)
	{
		float r = getRandFloat(0.30, 1.0);
		float g = getRandFloat(0.60, 1.0);
		float b = getRandFloat(0.70, 1.0);
		colors.push_back(glm::vec3(r, g, b));
		//colors.push_back(glm::vec3(218, 0, 0));
	}

	glBindBuffer(GL_ARRAY_BUFFER, structColorBufferID);
	glBufferData(GL_ARRAY_BUFFER,
		sizeof(glm::vec3)*colors.size(),
		colors.data(),
		GL_STATIC_DRAW);
}

// Sets up track vertex buffer from file, subdivide and process vectors to be smooth, Assign colors to the vertex shaders
void loadTrackBuffer(std::string file_path)
{
	loadVec3FromFile(track_verts, file_path);

	track_verts = subdivision(track_verts, 3);

	set_max_min_y(track_verts, track_max_height, track_min_height);

	set_track_rails(track_verts, rail_verts);
	//set_rail_structs(rail_verts, struct_verts, track_min_height);

	glBindBuffer(GL_ARRAY_BUFFER, trackVertBufferID);
	glBufferData(GL_ARRAY_BUFFER,
		sizeof(glm::vec3) * rail_verts.size(),	// byte size of Vec3f, 4 of them
		rail_verts.data(),		// pointer (Vec3f*) to contents of verts
		GL_STATIC_DRAW);	// Usage pattern of GPU buffer

	// RGB values for the vertices
	std::vector<glm::vec3> colors;
	for (int i = 0; i < rail_verts.size(); i++)
	{
		//float r = getRandFloat(0.50, 1.0);
		//float g = getRandFloat(0.50, 1.0);
		//float b = getRandFloat(0.50, 1.0);
		//colors.push_back(glm::vec3(r, g, b));
		colors.push_back(glm::vec3(218, 0, 0));
	}

	glBindBuffer(GL_ARRAY_BUFFER, trackColorBufferID);
	glBufferData(GL_ARRAY_BUFFER,
		sizeof(glm::vec3)*colors.size(),
		colors.data(),
		GL_STATIC_DRAW);
}

glm::vec3 center(std::vector<glm::vec3> verts)
{
	return glm::vec3(0, 0, 0);	//Unsure how to get center of object so im going to hack it to start at center
}

void loadCarBuffer(std::string file_path)
{
	loadVec3FromFile(car_verts, file_path);
	carCenter = center(car_verts);
	glBindBuffer(GL_ARRAY_BUFFER, carVertBufferID);
	glBufferData(GL_ARRAY_BUFFER,
		sizeof(glm::vec3) * car_verts.size(),	// byte size of Vec3f, 4 of them
		car_verts.data(),		// pointer (Vec3f*) to contents of verts
		GL_STATIC_DRAW);	// Usage pattern of GPU buffer

	// RGB values for the vertices
	std::vector<glm::vec3> colors;
	for (int i = 0; i < car_verts.size(); i++)
	{
		float r = getRandFloat(0, 1.0);
		float g = getRandFloat(0.0, 1.0);
		float b = getRandFloat(1.0, 1.0);
		colors.push_back(glm::vec3(r, g, b));
	}

	glBindBuffer(GL_ARRAY_BUFFER, carColorBufferID);
	glBufferData(GL_ARRAY_BUFFER,
		sizeof(glm::vec3)*colors.size(),
		colors.data(),
		GL_STATIC_DRAW);
}

void init(std::string track_file_path, std::string car_file_path)
{
	glEnable( GL_DEPTH_TEST );

	// SETUP SHADERS, BUFFERS, VAOs
	generateIDs();
	setupTrackVAO();
	loadTrackBuffer(track_file_path);

	setupStructVAO();
	loadStructBuffer();
	
	setupCarVAO();
	loadCarBuffer(car_file_path);

    loadModelViewMatrix();
    loadProjectionMatrix();
	setupModelViewProjectionTransform();
	reloadMVPUniform();
}

void updateMatrixColumn(glm::mat4 & matrix, int column, glm::vec3 vector)
{
	matrix[column][0] = vector.x;
	matrix[column][1] = vector.y;
	matrix[column][2] = vector.z;
}

//Moves a car from whatever its current center position to a new position
void moveCarTo(glm::vec3 new_pos)
{
	updateMatrixColumn(carM, 3, new_pos);
	carCenter = new_pos;
	render();
}

// Returns the next car position along the track given a speed and time
glm::vec3 next_car_position(float speed, float delata_time_ms)
{
	float total_distance = speed * (delata_time_ms / 800);	// Total distance that must be travelled given the speed and the time passed since last render
	float distance_travelled = 0;
	//std::cout << "get_next_car_position: Total distance: " << total_distance << std::endl;
	// Get to the proper segment
	glm::vec3 pos, next_pos;
	pos = carCenter;
	//pos = track_verts[curIndex % track_verts.size()];
	next_pos = track_verts[curIndex % track_verts.size()];
	while ((distance_travelled + glm::distance(pos, next_pos)) < total_distance)
	{
		//std::cout << "\tWhileLoop: Distance between point[" << (curIndex) << " to " << curIndex + 1 << "] is \n\t" << glm::to_string(pos) << "\n\t and \n\t" << glm::to_string(next_pos) << " is " << glm::distance(pos, next_pos) << std::endl;
		//std::cout << "\tWhileLoop: Travelled distance: " << distance_travelled << std::endl;
		distance_travelled += glm::distance(pos, next_pos);
		pos = track_verts[curIndex % track_verts.size()];
		next_pos = track_verts[(curIndex + 1) % track_verts.size()];
		curIndex++;
	}
	//std::cout << "Found correct segment between [" << curIndex - 1 << " and " << curIndex << "]" << std::endl;
	//std::cout << "\nDistance between point[" << (curIndex - 1) << " to " << curIndex << "] is \n" << glm::to_string(pos) << "\n\tand \n\t" << glm::to_string(next_pos) << " is " << glm::distance(pos, next_pos) << std::endl;
	//std::cout << "\tDistance remaining is : \n\t((" << total_distance << " - " << distance_travelled << ") / " << glm::distance(pos, next_pos) << ")" << " = " 
	//	<< ((total_distance - distance_travelled)) << std::endl;
	//std::cout << "\tVector is: " << glm::to_string(next_pos - pos) << std::endl;
	//std::cout << "\tNormalized vector is: " << glm::to_string(glm::normalize(next_pos - pos)) << std::endl;
	glm::vec3 car_pos = glm::normalize(next_pos - pos) * ((total_distance - distance_travelled));
	car_pos = carCenter + car_pos;
	//std::cout << "\tNew vector is: " << glm::to_string(car_pos) << "\n" << std::endl;
	curIndex = curIndex % track_verts.size();
	return car_pos;
}

float get_speed(glm::vec3 position)
{
	int speed_scalar = 3;
	return (glm::sqrt(2 * GRAVITY * (track_max_height - position.y)) + 2) * speed_scalar;
}

// Assumption: curIndex is set at the index immediately in front of where ever carCenter is at.
void set_tnb_frame(float speed, glm::mat4 & modelMatrix, std::vector<glm::vec3> & verts, int & index)
{
	//std::cout << "CurIndex is: " << curIndex << std::endl;
	//std::cout << "set_car_rotation START" << std::endl;
	glm::vec3 p1 = verts[(index - 1) % verts.size()];
	glm::vec3 point = verts[index % verts.size()];
	glm::vec3 p3 = verts[(index + 1) % verts.size()];

	// Compare the length of segment curIndex - 1 to curIndex, and length of segment curIndex to curIndex + 1
	float seg1 = glm::distance(p1, point);
	float seg2 = glm::distance(point, p3);
	float max_len = std::min(seg1, seg2);

	glm::vec3 point_before = point + glm::normalize(p3 - point) * max_len;
	glm::vec3 point_after = point + glm::normalize(p1 - point) * max_len;
	
	//std::cout << "p1 \t" << glm::to_string(p1) << std::endl;
	//std::cout << "point \t" << glm::to_string(point) << std::endl;
	//std::cout << "p3 \t" << glm::to_string(p3) << std::endl;
	//std::cout << "seg1 \t" << glm::to_string(seg1) << std::endl;
	//std::cout << "seg2 \t" << glm::to_string(seg2) << std::endl;
	//std::cout << "max_len \t" << glm::to_string(max_len) << std::endl;

	//std::cout << "point_before \t" << glm::to_string(point_before) << std::endl;
	//std::cout << "point_after \t" << glm::to_string(point_after) << std::endl;

	float c = glm::length(point_after - point);
	float h = glm::length((point_after - (point * 2.f) + point_before));
	float radius = (pow(c, 2) + (4 * pow(h, 2))) / (8 * h);
	float speed_squared = (pow(speed,2));
	glm::vec3 gravity_vector = glm::vec3(0, 1, 0);
	glm::vec3 tangent = glm::normalize((point_after - point) * 0.5f);
	glm::vec3 normal = glm::normalize(((speed_squared / radius) * glm::normalize((point_after - (point * 2.f) + point_before) * 0.25f)) + gravity_vector);
	glm::vec3 binormal = glm::normalize(glm::cross(tangent, normal));
	updateMatrixColumn(modelMatrix, 0, tangent);
	updateMatrixColumn(modelMatrix, 2, normal);
	updateMatrixColumn(modelMatrix, 1, binormal);
	//std::cout << "set_car_rotation END\n" << std::endl;
}

void timerFunc(int delay)
{
	float speed = get_speed(carCenter);
	moveCarTo(next_car_position(speed, delay));
	set_tnb_frame(speed, carM, track_verts, curIndex);
	carM = glm::scale(carM, glm::vec3(car_scale));
	//V = glm::rotate(V, 0.1f, glm::vec3(0.0f, 1.0f, 0.0f));

	glutPostRedisplay();
	glutTimerFunc(delay, timerFunc, delay);
}

//void idleFunc()
//{
//	carM = glm::rotate(carM, 0.1f, glm::vec3(0.0f, 1.0f, 0.0f));
//	trackM = glm::rotate(trackM, 0.1f, glm::vec3(0.0f, 1.0f, 0.0f));
//}

int main( int argc, char** argv )
{
    glutInit( &argc, argv );
	// Setup FB configuration
	glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH );
	
	glutInitWindowSize( WIN_WIDTH, WIN_HEIGHT );
	glutInitWindowPosition( 0, 0 );

	glutCreateWindow( "Assignment 1: Rollercoaster" );

	glewExperimental=true; // Needed in Core Profile
	// Comment out if you want to us glBeign etc...
	if( glewInit() != GLEW_OK )
	{
		std::cerr << "Failed to initialize GLEW" << std::endl;
		return -1;
	}
	printInfo();
    glutDisplayFunc( displayFunc );
	glutReshapeFunc( resizeFunc );
	glutTimerFunc(delay, timerFunc, delay);
	//glutIdleFunc(idleFunc);
	glutMouseFunc(mouseButton);
	glutMotionFunc(mouseMove);
	glutKeyboardFunc(keyboardFunc);
	std::string track("track.txt");
	std::string car("car.txt");
	init(track, car); // our own initialize stuff func
	std::cout << "Track Verts total size is: " << track_verts.size() << std::endl;
	glutMainLoop();

	// clean up after loop
	deleteIDs();

	return 0;
}
