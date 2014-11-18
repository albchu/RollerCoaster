// CPSC 587 
// Assignment 3: Chain Pendulum
//
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

float gravity = 3.0f;
int num_points = 6;
float global_spring_constant = 3.0f;
float global_spring_friction = 1.0f;
float global_point_mass = 3.0f;
float global_spring_length = 2.0f;
int delay = 10;

int WIN_WIDTH = 800, WIN_HEIGHT = 600;

struct _Point
{
	std::vector<glm::vec3> neighbors;
	float mass;
	float spring_length;
	float spring_constant;
	float spring_friction;
	glm::vec3 velocity_curr;
	glm::vec3 velocity_prev;
	glm::vec3 position_curr;
	glm::vec3 position_prev;
	glm::vec3 forces;
};

typedef _Point Point;
typedef std::vector<Point> points;
points pointsList;
std::vector<glm::vec3> verts;

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
//float get_distance();
glm::vec3 get_force_dampening(Point & point_j, Point & point_x);
glm::vec3 get_spring_vector(Point & point_j, Point & point_x);
glm::vec3 get_force_spring(Point & point_j, Point & point_x);
glm::vec3 get_velocity(Point & point, glm::vec3 forces);
glm::vec3 get_position(Point & point, glm::vec3 velocity);
float getRandFloat(float low, float high);

void displayFunc()
{
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	// Use our shader
	glUseProgram( basicProgramID );

	// Use VAO that holds buffer bindings
	// and attribute config of buffers
	glBindVertexArray( vaoID );
	glLineWidth(20.0);
	glDrawArrays( GL_LINE_STRIP, 0, verts.size() );

	glutSwapBuffers();
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
	P = glm::perspective(60.0f, static_cast<float>(WIN_WIDTH) / WIN_HEIGHT, 0.01f, 1000.f);
}

void loadModelViewMatrix()
{
	M = glm::translate(M, glm::vec3(0, 0, -10));
	M = glm::scale(M, glm::vec3(0.15));
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

std::vector<glm::vec3> get_current_positions(std::vector<Point> & points)
{
	std::vector<glm::vec3> verts;
	for (Point point : points)
	{
		cout << "loadBuffer: " << glm::to_string(point.position_curr) << endl;
		verts.push_back(point.position_curr);
		verts.push_back(point.position_curr);
	}
	return verts;
}

void loadBuffer()
{
	verts = get_current_positions(pointsList);

	glBindBuffer( GL_ARRAY_BUFFER, vertBufferID );
	glBufferData(	GL_ARRAY_BUFFER,	
			sizeof(glm::vec3)*verts.size(),	// byte size of glm::vec3, 4 of them
			verts.data(),		// pointer (glm::vec3*) to contents of verts
			GL_STATIC_DRAW );	// Usage pattern of GPU buffer


	// RGB values for the vertices
	std::vector<glm::vec3> colors;
	for (int i = 0; i < verts.size(); i++)
	{
		float r = getRandFloat(0, 1.0);
		float g = getRandFloat(0.0, 1.0);
		float b = getRandFloat(1.0, 1.0);
		colors.push_back(glm::vec3(r, g, b));
	}

	glBindBuffer(GL_ARRAY_BUFFER, colorBufferID);
	glBufferData(GL_ARRAY_BUFFER,
		sizeof(glm::vec3)*colors.size(),
		colors.data(),
		GL_STATIC_DRAW);
}

float getRandFloat(float low, float high)
{
	return (low + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (high - low))));
}

void initBuffer()
{
	int start_point = -10;
	for (int i = 0; i < num_points; i++)
	{
		Point aPoint;
		aPoint.position_curr = glm::vec3(start_point + (i * 5), 30, 0);
		aPoint.spring_constant = global_spring_constant;
		aPoint.spring_friction = global_spring_friction;
		aPoint.spring_length = global_spring_length;
		aPoint.mass = global_point_mass;
		pointsList.push_back(aPoint);
	}
}


void init()
{
	glEnable( GL_DEPTH_TEST );

	// SETUP SHADERS, BUFFERS, VAOs
	//mass_previous_position = initial_mass_position;
	generateIDs();
	setupVAO();
	initBuffer();
	loadBuffer();

    loadModelViewMatrix();
    loadProjectionMatrix();
	setupModelViewProjectionTransform();
	reloadMVPUniform();
}

void update_position(Point & point, glm::vec3 position)
{
	point.position_prev = point.position_curr;
	point.position_curr = position;
}

void update_velocity(Point & point, glm::vec3 velocity)
{
	point.velocity_prev = point.velocity_curr;
	point.velocity_curr = velocity;
}

float get_force_gravity(Point & point)
{
	return gravity * point.mass;
}

void timerFunc(int delay)
{

	std::vector<glm::vec3> spring_forces;
	std::vector<glm::vec3> dampen_forces;
	for (int i = 0; i < pointsList.size(); i++)	// i is instantiated to 1 to allow the first element 0 to be the anchor
	{
		spring_forces.push_back(glm::vec3(0.0, 0.0, 0.0));
		dampen_forces.push_back(glm::vec3(0.0, 0.0, 0.0));
	}
	for (int i = 1; i < pointsList.size(); i++)	// i is instantiated to 1 to allow the first element 0 to be the anchor
	{
		Point & point_j = pointsList.at(i - 1);
		Point & point_i = pointsList.at(i);
		glm::vec3 a_spring_force = get_force_spring(point_j, point_i);
		glm::vec3 a_dampen_force = get_force_dampening(point_j, point_i);
		spring_forces[i] += a_spring_force;
		dampen_forces[i] += a_dampen_force;
	}

	for (int i = 1; i < pointsList.size() - 1; i++)	// i is instantiated to 1 to allow the first element 0 to be the anchor
	{
		Point & point_j = pointsList.at(i + 1);
		Point & point_i = pointsList.at(i);
		glm::vec3 a_spring_force = get_force_spring(point_j, point_i);
		glm::vec3 a_dampen_force = get_force_dampening(point_j, point_i);
		spring_forces[i] += a_spring_force;
		dampen_forces[i] += a_dampen_force;
	}

	for (int i = 1; i < pointsList.size(); i++)	// i is instantiated to 1 to allow the first element 0 to be the anchor
	{
		cout << "Loop Index: " << i << endl;
		Point & point = pointsList.at(i);
		glm::vec3 total_forces = spring_forces.at(i) + dampen_forces.at(i) - glm::vec3(0, get_force_gravity(point), 0);
		glm::vec3 new_velocity = get_velocity(point, total_forces);
		glm::vec3 position = get_position(point, new_velocity);
		update_position(point, position);
		update_velocity(point, new_velocity);	
	}
	cout << "CYCLE COMPLETED\n" << endl;
	setupModelViewProjectionTransform();

	// send changes to GPU
	loadBuffer();
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
	return double(delay) / 100;
}

// Returns the top time in seconds
float get_total_time()
{
	return (glutGet(GLUT_ELAPSED_TIME) / double(1000));
}

glm::vec3 get_force_dampening(Point & point_j, Point & point_x)
{
	glm::vec3 spring_vector = get_spring_vector(point_j, point_x);
	return (point_x.spring_friction * (glm::dot((point_j.velocity_prev - point_x.velocity_curr), spring_vector)) * spring_vector);
}

glm::vec3 get_spring_vector(Point & point_j, Point & point_x)
{
	glm::vec3 spring_vector = (point_j.position_curr - point_x.position_curr);
	return glm::normalize(spring_vector);
}

glm::vec3 get_force_spring(Point & point_j, Point & point_x)
{
	glm::vec3 spring_vector = get_spring_vector(point_j, point_x);
	return point_x.spring_constant * spring_vector * (glm::length(point_j.position_curr - point_x.position_curr) - point_x.spring_length);
}

glm::vec3 get_velocity(Point & point, glm::vec3 forces)
{
	return (point.velocity_curr + (get_delta_time() * forces / point.mass));
}

glm::vec3 get_position(Point & point, glm::vec3 velocity)
{
	return point.position_curr + get_delta_time() * velocity;
}

int main( int argc, char** argv )
{
    glutInit( &argc, argv );
	// Setup FB configuration
	glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH );
	
	glutInitWindowSize( WIN_WIDTH, WIN_HEIGHT );
	glutInitWindowPosition( 0, 0 );

	glutCreateWindow( "Assignment 3 : Chain Pendulum" );

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
	init(); // our own initialize stuff func

	glutMainLoop();

	// clean up after loop
	deleteIDs();

	return 0;
}
