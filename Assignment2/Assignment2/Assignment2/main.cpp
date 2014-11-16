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
#include "Vec3f.h"

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

double mass = 5;
double gravity = 9.81;
double spring_constant = 5;
double velocity_prev = 0;
double initial_position = 1;
int delay = 500;

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
double get_force_gravity();
double get_distance();
double get_acceleration();
double get_delta_time();
int main( int, char** );
// function declarations

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
	M = glm::translate(M, glm::vec3(0, 0, -1));
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
				GL_TRUE,	// transpose matrix, glm::mat4 is row major
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
	std::vector< Vec3f > verts;
	verts.push_back( Vec3f( 1, 1, 0 ) );
	verts.push_back( Vec3f( 1, -1, 0 ) );
	verts.push_back( Vec3f( -1, -1, 0 ) );
	verts.push_back( Vec3f( -1, 1, 0 ) );
	
	glBindBuffer( GL_ARRAY_BUFFER, vertBufferID );
	glBufferData(	GL_ARRAY_BUFFER,	
			sizeof(Vec3f)*4,	// byte size of Vec3f, 4 of them
			verts.data(),		// pointer (Vec3f*) to contents of verts
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
	cout << "Total distance: " << get_distance() << endl;
	//cout << "Total delta time: " << get_delta_time() << endl;
	//cout << "Total acceleration: " << get_acceleration() << endl;
	//cout << "Total gravity: " << get_force_gravity() << endl;
	//cout << "M Before: " << M << endl;
	//M = TranslateMatrix(0, get_distance(), 0);
	//cout << "M After: " << M << endl;

	//update_velocity();
	cout << "\n" << endl;
	setupModelViewProjectionTransform();

	// send changes to GPU
	reloadMVPUniform();

	glutPostRedisplay();
	glutTimerFunc(delay, timerFunc, delay);
}

// The following are formulas derived from a system with only up and down forces

double get_delta_time()
{
	return double(delay) / 1000;
}

// Returns the top time in seconds
double get_total_time()
{
	return (glutGet(GLUT_ELAPSED_TIME) / double(1000));
}
//
//double get_acceleration()
//{
//	double denom = (mass + (1.5)*(pow(get_delta_time(), 2)));
//	//cout << "Acceleration Denominator: " << std::to_string(denom) << endl;
//	return (get_force_gravity() - (spring_constant * velocity_prev * get_delta_time())) / denom;
//}
//
//double get_force_gravity()
//{
//	return mass * gravity;
//}

double get_distance()
{
	//return (mass * get_acceleration() + get_force_gravity()) / spring_constant;
	return initial_position * cos(spring_constant * get_total_time() / mass);
}

//void update_velocity()
//{
//	cout << "Old velocity previous: " << velocity_prev << endl;
//	velocity_prev = (get_acceleration() * get_delta_time()) + (velocity_prev); // Damping is too hard, using hack to reduce previous velocity every time
//	cout << "New velocity previous: " << velocity_prev << endl;
//}

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
	//glutTimerFunc(delay, timerFunc, delay);
	//glutIdleFunc(idleFunc);
	init(); // our own initialize stuff func

	glutMainLoop();

	// clean up after loop
	deleteIDs();

	return 0;
}
