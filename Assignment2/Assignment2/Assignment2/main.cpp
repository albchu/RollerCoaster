// CPSC 587 Created By: Andrew Owens
// This is a (very) basic program to
// 1) load shaders from external files, and make a shader program
// 2) make Vertex Array Object and Vertex Buffer Object for the triangle

// take a look at the following sites for further readings:
// opengl-tutorial.org -> The first triangle (New OpenGL, great start)
// antongerdelan.net -> shaders pipeline explained
// ogldev.atspace.co.uk -> good resource 


// NOTE: this dependencies (include/library files) will need to be tweaked if
// you wish to run this on a non lab computer

#include<iostream>
#include<cmath>

#include<GL/glew.h> // include BEFORE glut
#include<GL/glut.h>

#include "ShaderTools.h"

#include <glm/gtx/string_cast.hpp>
#include<glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

using std::cout;
using std::endl;
using std::cerr;

GLuint vaoID;
GLuint basicProgramID;

// Could store these two in an array GLuint[]
GLuint vertBufferID;
GLuint colorBufferID;

glm::mat4 MVP;
glm::mat4 M;
glm::mat4 V;
glm::mat4 P;

float mass = 3;
//float gravity = 9.81;
float spring_constant = 10.0f;
float spring_friction = 50.0f;
glm::vec3 velocity_prev;
glm::vec3 velocity_curr;
float initial_position = 2;
int delay = 10;
glm::vec3 spring_anchor_position = glm::vec3(0.f, 2.f, -10.f);
glm::vec3 initial_mass_position = glm::vec3(5.f, 0.f, -10.f);
glm::vec3 mass_previous_position;
float total_spring_length = 1;

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
void update_velocity();
float get_force_gravity();
float get_distance();
float get_acceleration();
float get_delta_time();
void moveModelTo(glm::mat4 & model, glm::vec3 new_pos);
void updateMatrixColumn(glm::mat4 & matrix, int column, glm::vec3 vector);
float get_delta_time();
float get_total_time();
float get_distance();
glm::vec3 get_force_spring();
glm::vec3 get_velocity(glm::vec3 forces);
glm::vec3 get_position(glm::vec3 velocity);
glm::vec3 get_force_dampening();
glm::vec3 get_spring_vector();

void displayFunc()
{
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	// Use our shader
	glUseProgram( basicProgramID );

	// Use VAO that holds buffer bindings
	// and attribute config of buffers
	glBindVertexArray( vaoID );
	// Draw Quads, start at vertex 0, draw 4 of them (for a quad)
	glDrawArrays( GL_QUADS, 0, 4 );

	glutSwapBuffers();
}

//void idleFunc()
//{
//	// every frame refresh, rotate quad around y axis by 1 degree
////	MVP = MVP * RotateAboutYMatrix( 1.0 );
//    M = M * RotateAboutYMatrix( 1.0 );
//    setupModelViewProjectionTransform();
//
//	// send changes to GPU
//	reloadMVPUniform();
//	
//	glutPostRedisplay();
//}

void resizeFunc( int width, int height )
{
    WIN_WIDTH = width;
    WIN_HEIGHT = height;

    glViewport( 0, 0, width, height );

    loadProjectionMatrix();
    reloadMVPUniform();

    glutPostRedisplay();
}

void generateIDs()
{
	std::string vsSource = loadShaderStringfromFile( "basic_vs.glsl" );
	std::string fsSource = loadShaderStringfromFile( "basic_fs.glsl" );
	basicProgramID = CreateShaderProgram( vsSource, fsSource );

	// load IDs given from OpenGL
	glGenVertexArrays( 1, &vaoID );
	glGenBuffers( 1, &vertBufferID );
	glGenBuffers( 1, &colorBufferID );
}

void deleteIDs()
{
	glDeleteProgram( basicProgramID );
	glDeleteVertexArrays( 1, &vaoID );
	glDeleteBuffers( 1, &vertBufferID );
	glDeleteBuffers( 1, &colorBufferID );
}

void loadProjectionMatrix()
{
    // Perspective Only
    
	// field of view angle 60 degrees
	// window aspect ratio
	// near Z plane > 0
	// far Z plane

	P = glm::perspective(60.0f, static_cast<float>(WIN_WIDTH) / WIN_HEIGHT, 0.01f, 1000.f);
}

void loadModelViewMatrix()
{
	//M = UniformScaleMatrix( 0.25 );	// scale Quad First
    //M = TranslateMatrix( 0, 0, -1.0 ) * M;	// translate away from (0,0,0)
	M = glm::translate(M, glm::vec3(0, 0, -10));
	M = glm::scale(M, glm::vec3(0.25));
    // view doesn't change, but if it did you would use this
   // V = IdentityMatrix();
}

void setupModelViewProjectionTransform()
{
	MVP = P * V * M; // transforms vertices from right to left (odd huh?)
}
 
void reloadMVPUniform()
{
	GLint mvpID = glGetUniformLocation( basicProgramID, "MVP" );
	
	glUseProgram( basicProgramID );
	glUniformMatrix4fv( 	mvpID,		// ID
				1,		// only 1 matrix
				GL_FALSE,	// transpose matrix, glm::mat4 is row major
				glm::value_ptr(MVP)	// pointer to data in glm::mat4
			);
}

void setupVAO()
{
	glBindVertexArray( vaoID );

	glEnableVertexAttribArray( 0 ); // match layout # in shader
	glBindBuffer( GL_ARRAY_BUFFER, vertBufferID );
	glVertexAttribPointer(
		0,		// attribute layout # above
		3,		// # of components (ie XYZ )
		GL_FLOAT,	// type of components
		GL_FALSE,	// need to be normalized?
		0,		// stride
		(void*)0	// array buffer offset
	);

	glEnableVertexAttribArray( 1 ); // match layout # in shader
	glBindBuffer( GL_ARRAY_BUFFER, colorBufferID );
	glVertexAttribPointer(
		1,		// attribute layout # above
		3,		// # of components (ie XYZ )
		GL_FLOAT,	// type of components
		GL_FALSE,	// need to be normalized?
		0,		// stride
		(void*)0	// array buffer offset
	);

	glBindVertexArray( 0 ); // reset to default		
}

void loadBuffer()
{
	// Just basic layout of floats, for a quad
	// 3 floats per vertex, 4 vertices
	std::vector< glm::vec3 > verts;
	verts.push_back( glm::vec3( 1, 1, 0 ) );
	verts.push_back( glm::vec3( 1, -1, 0 ) );
	verts.push_back( glm::vec3( -1, -1, 0 ) );
	verts.push_back( glm::vec3( -1, 1, 0 ) );
	
	glBindBuffer( GL_ARRAY_BUFFER, vertBufferID );
	glBufferData(	GL_ARRAY_BUFFER,	
			sizeof(glm::vec3)*4,	// byte size of glm::vec3, 4 of them
			verts.data(),		// pointer (glm::vec3*) to contents of verts
			GL_STATIC_DRAW );	// Usage pattern of GPU buffer

	// RGB values for the 4 vertices of the quad
	const float colors[] = {
			1.0f,	0.0f,	0.0f,
			0.0f,	1.0f,	0.0f,
			0.0f,	0.0f,	1.0f,
			1.0f,	1.0f,	1.0f };

	glBindBuffer( GL_ARRAY_BUFFER, colorBufferID );
	glBufferData(	GL_ARRAY_BUFFER,
			sizeof(colors),
			colors,
			GL_STATIC_DRAW );
}

void init()
{
	glEnable( GL_DEPTH_TEST );

	// SETUP SHADERS, BUFFERS, VAOs
	mass_previous_position = initial_mass_position;
	generateIDs();
	setupVAO();
	loadBuffer();

    loadModelViewMatrix();
    loadProjectionMatrix();
	setupModelViewProjectionTransform();
	reloadMVPUniform();
}

void timerFunc(int delay)
{
	//M = M * RotateAboutYMatrix(1.0);
	//cout << "Total distance: " << get_distance() << endl;
	
	//cout << glm::to_string(M) << endl;
	//M = TranslateMatrix(0, get_distance(), 0);
	glm::vec3 total_forces = get_force_spring() + get_force_dampening();
	velocity_prev = velocity_curr;
	velocity_curr = get_velocity(total_forces);
	glm::vec3 position = get_position(velocity_curr);
	//cout << "New Total Forces: " << glm::to_string(total_forces) << endl;
	//cout << "Dampening : " << glm::to_string(get_force_dampening()) << endl;
	moveModelTo(M, position);
	cout << "M After: " << glm::to_string(M) << endl;
	mass_previous_position = position;

	//update_velocity();
	cout << "\n" << endl;
	setupModelViewProjectionTransform();

	// send changes to GPU
	reloadMVPUniform();

	glutPostRedisplay();
	glutTimerFunc(delay, timerFunc, delay);
}

void moveModelTo(glm::mat4 & model, glm::vec3 new_pos)
{
	updateMatrixColumn(model, 3, new_pos);
}

void updateMatrixColumn(glm::mat4 & matrix, int column, glm::vec3 vector)
{
	matrix[column][0] = vector.x;
	matrix[column][1] = vector.y;
	matrix[column][2] = vector.z;
}

float get_delta_time()
{
	return double(delay) / 1000;
}

// Returns the top time in seconds
float get_total_time()
{
	return (glutGet(GLUT_ELAPSED_TIME) / double(1000));
}

float get_distance()
{
	//return (mass * get_acceleration() + get_force_gravity()) / spring_constant;
	//initial_position = initial_position * 0.9999;
	return initial_position * cos(spring_constant * get_total_time() / mass);
}

glm::vec3 get_force_dampening()
{
	cout << "delta v " << glm::to_string(velocity_curr - velocity_prev) << endl;
	return (spring_friction * glm::dot((velocity_curr - velocity_prev), get_spring_vector()) * get_spring_vector());
}

glm::vec3 get_spring_vector()
{
	glm::vec3 spring_vector = (mass_previous_position - spring_anchor_position);
	return glm::normalize(spring_vector);
}

glm::vec3 get_force_spring()
{
	glm::vec3 spring_vector = (mass_previous_position - spring_anchor_position);
	//cout << "Spring vector; " << glm::to_string(spring_constant * glm::normalize(spring_vector)) << endl;
	//cout << "(glm::length(spring_vector) - total_spring_length); " << (glm::length(spring_vector) - total_spring_length) << endl;
	return -spring_constant * get_spring_vector() * (glm::length(spring_vector) - total_spring_length);
}

glm::vec3 get_velocity(glm::vec3 forces)
{
	return (velocity_prev + get_delta_time()*forces / mass);
}

glm::vec3 get_position(glm::vec3 velocity)
{
	return mass_previous_position + get_delta_time() * velocity;
}

int main( int argc, char** argv )
{
    glutInit( &argc, argv );
	// Setup FB configuration
	glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH );
	
	glutInitWindowSize( WIN_WIDTH, WIN_HEIGHT );
	glutInitWindowPosition( 0, 0 );

	glutCreateWindow( "Assignment 3 : Mass on Spring" );

	glewExperimental=true; // Needed in Core Profile
	// Comment out if you want to us glBeign etc...
	if( glewInit() != GLEW_OK )
	{
		cerr << "Failed to initialize GLEW" << endl;
		return -1;
	}
	cout << "GL Version: :" << glGetString(GL_VERSION) << endl;

    glutDisplayFunc( displayFunc );
	glutReshapeFunc( resizeFunc );
	glutTimerFunc(delay, timerFunc, delay);
	//glutIdleFunc(idleFunc);
	init(); // our own initialize stuff func

	glutMainLoop();

	// clean up after loop
	deleteIDs();

	return 0;
}
