// CPSC 587 
// Assignment 4: Boids
//
#include <sstream>
#include<iostream>
#include<cmath>

#include<GL/glew.h> // include BEFORE glut
#include<GL/glut.h>

#include "ShaderTools.h"

#include <ctime>
#include <glm/gtx/string_cast.hpp>
#include <glm/glm.hpp>
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

//float gravity = 3.0f;
//int num_points = 6;
//float global_spring_constant = 3.0f;
//float global_spring_friction = 1.0f;
//float global_point_mass = 3.0f;
//float global_spring_length = 2.0f;
float RADIUS_AVOID = 0.5f;		// The max distance threshold where a boid will move away from its neighbor
float RADIUS_COHERANCE = 2.0f;	// The max distance threshold where a boid will move in the same velocity as its neighbor
float RADIUS_ATTRACT = 5.0f;	// The max distance threshold before a boid will move towards that neighbor
float RADIUS_MAX = 15.0f;	// The max distance threshold before a boid will move towards that neighbor
float velocity_scalar = 0.002f;
float velocity_avoid_scalar = 0.02f;
float velocity_coherance_scalar = 0.008f;
float velocity_attract_scalar = 0.04f;

bool left_click = false;
bool right_click = false;
bool middle_click = false;
float delta_x = 0;
float delta_y = 0;
float delta_z = 0;

int delay = 1;	// Time delay for timerFunc to rerender another frame

int WIN_WIDTH = 800, WIN_HEIGHT = 600;

struct _Boid
{
	glm::vec3 head;
	glm::vec3 left;
	glm::vec3 right;
	glm::vec3 position;
	glm::vec3 velocity;
};
typedef _Boid Boid;
typedef std::vector<Boid> boids;
std::vector<glm::vec3> verts;	// Holds all the verts to draw out. Will be updated by boids
std::vector<glm::vec3> obstacle_verts;	// Holds all the obstacle verts to draw out. Will be updated by boids
boids boidsList;

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
void update_velocity(Boid & boid_i, Boid & boid_j);
float get_force_gravity();
float get_distance();
float get_acceleration();
float get_delta_time();
void moveModelTo(glm::mat4 & model, glm::vec3 new_pos);
void updateMatrixColumn(glm::mat4 & matrix, int column, glm::vec3 vector);
float get_delta_time();
float get_total_time();
void update_position(Boid & boid);
void move_boid_to(Boid & boid, glm::vec3 position);
float getRandFloat(float low, float high);

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
			vecs.push_back(glm::vec3(x, y, z));
		}
	}
	file.close();
}

void displayFunc()
{
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	// Use our shader
	glUseProgram( basicProgramID );

	// Use VAO that holds buffer bindings
	// and attribute config of buffers
	glBindVertexArray( vaoID );
	//glLineWidth(20.0);
	glDrawArrays( GL_TRIANGLES, 0, verts.size() );

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
	M = glm::translate(M, glm::vec3(0, 0, -50));
	//M = glm::scale(M, glm::vec3(-10));
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

std::vector<glm::vec3> get_current_positions(std::vector<Boid> & boids)
{
	std::vector<glm::vec3> verts;
	for (Boid boid : boids)
	{
		verts.push_back(boid.head);
		verts.push_back(boid.left);
		verts.push_back(boid.right);
	}
	return verts;
}

void loadBuffer()
{
	verts = get_current_positions(boidsList);
	verts.insert(verts.end(), obstacle_verts.begin(), obstacle_verts.end());
	glBindBuffer( GL_ARRAY_BUFFER, vertBufferID );
	glBufferData(	GL_ARRAY_BUFFER,	
			sizeof(glm::vec3)*verts.size(),	// byte size of glm::vec3, 4 of them
			verts.data(),		// pointer (glm::vec3*) to contents of verts
			GL_STATIC_DRAW );	// Usage pattern of GPU buffer
	
	//cout << "Verts: " << verts.size() << endl;
	//for (glm::vec3 vert : verts)
	//	cout << glm::to_string(vert) << endl;
	// RGB values for the vertices
	std::vector<glm::vec3> colors;
	if ((verts.size() % 3) !=0)
		cerr << "Cannot load colors for boids since wrong number of vertices detected" << endl;
	for (int i = 0; i < verts.size(); i += 3)
	{
		//Head Color
		colors.push_back(glm::vec3(1.0, 0, 0));
		colors.push_back(glm::vec3(0, 0, 1.0));
		colors.push_back(glm::vec3(0, 0, 1.0));
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

// Returns a boid where the center defines where the shape will be generated
Boid createBoid(glm::vec3 boidCenter, glm::vec3 velocity)
{
	Boid aBoid;
	aBoid.velocity = velocity;
	move_boid_to(aBoid, boidCenter);
	return aBoid;
}

void move_boid_to(Boid & boid, glm::vec3 position)
{
	boid.position = position;
	boid.head = glm::vec3(0, 0.3, 0.2) + position;
	boid.left = glm::vec3(-0.15, -0.15, -0.3) + position;
	boid.right = glm::vec3(0.15, -0.15, 0.3) + position;
}

void initBuffer(std::string obstacles_file)
{
	loadVec3FromFile(obstacle_verts, obstacles_file);
	int num_boids = 100;
	//srand(std::time(NULL));
	for (int i = 0; i < num_boids; i++)
		boidsList.push_back(createBoid(glm::vec3(getRandFloat(-10, 10), getRandFloat(-10, 10), getRandFloat(0, 3)), glm::vec3(getRandFloat(0, 2), 0, 0)));
}

void initBuffer(std::string boid_positions_file, std::string boid_velocities_file, std::string obstacles_file)
{
	std::vector<glm::vec3> boid_positions;
	std::vector<glm::vec3> boid_velocities;
	
	loadVec3FromFile(boid_positions, boid_positions_file);
	loadVec3FromFile(boid_velocities, boid_velocities_file);
	loadVec3FromFile(obstacle_verts, obstacles_file);

	for (int i = 0; i < boid_positions.size(); i++)
	{
		glm::vec3 boid_position = boid_positions.at(i);
		glm::vec3 boid_velocity;
		if (i < boid_velocities.size())
			boid_velocity = boid_velocities.at(i);
		boidsList.push_back(createBoid(boid_position, boid_velocity));
	}
}

void init(std::string boid_positions, std::string boid_velocities, std::string obstacles)
{
	glEnable( GL_DEPTH_TEST );

	// SETUP SHADERS, BUFFERS, VAOs
	//mass_previous_position = initial_mass_position;
	generateIDs();
	setupVAO();
	//initBuffer(boid_positions, boid_velocities, obstacles);
	initBuffer(obstacles);	//Generate randomized flock
	loadBuffer();

	loadModelViewMatrix();
	loadProjectionMatrix();
	setupModelViewProjectionTransform();
	reloadMVPUniform();
}

//float get_force_gravity(Point & point)
//{
//	return gravity * point.mass;
//}

void timerFunc(int delay)
{
	for (int i = 0; i < boidsList.size(); i++)
	{
		for (int j = 0; j < boidsList.size(); j++)
		{
			if (i != j)
			{
				Boid & boid_i = boidsList.at(i);
				Boid & boid_j = boidsList.at(j);

				update_velocity(boid_i, boid_j);
			}
		}
	}
	for (int i = 0; i < boidsList.size(); i++)
	{
		Boid & boid = boidsList.at(i);
		update_position(boid);
	}
	setupModelViewProjectionTransform();

	// send changes to GPU
	loadBuffer();
	reloadMVPUniform();
	//M = glm::rotate(M, 0.9f, glm::vec3(0.0f, 1.0f, 0.0f));
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


void update_velocity(Boid & boid_i, Boid & boid_j)
{
	float radius_total = glm::length(boid_i.position - boid_j.position);
	glm::vec3 distance = boid_j.position - boid_i.position;
	glm::vec3 velocity = glm::vec3(0, 0, 0);
	if (radius_total <= RADIUS_MAX)
	{
		if (radius_total < RADIUS_AVOID)
		{
			//velocity_avoid_scalar = 1 / pow(radius_total, 2);
			velocity += (velocity_avoid_scalar * (-distance));
		}
		else if (radius_total < RADIUS_COHERANCE)
		{
			velocity += velocity_coherance_scalar * boid_j.velocity;
		} 
		else// if (radius_total < RADIUS_ATTRACT)
		{
			velocity += velocity_attract_scalar * distance;
		}
	}
	boid_i.velocity += (velocity_scalar * velocity);
}

void update_position(Boid & boid)
{
	glm::vec3 new_pos = boid.position + get_delta_time() * boid.velocity;
	move_boid_to(boid, new_pos);
}

void mouseMove(int x, int y)
{
	if (left_click)
	{
		//boidsList.push_back(createBoid(glm::vec3(x, y, 0), glm::vec3(0, 0, 0)));
	}
	if (middle_click)
	{
		delta_z -= y;
		V = glm::translate(V, glm::vec3(0, 0, delta_z / 100));
		delta_z = y;
	}
	if (right_click)
	{
		delta_y -= y;
		delta_x -= x;
		M = glm::rotate(M, delta_y / 10, glm::vec3(-1.0f, 0.0f, 0.0f));
		M = glm::rotate(M, delta_x / 10, glm::vec3(0.0f, 1.0f, 0.0f));
		delta_x = x;
		delta_y = y;
	}
	loadBuffer();
	reloadMVPUniform();
	glutPostRedisplay();
}

void mouseButton(int button, int state, int x, int y)
{
	switch (button)
	{
	case GLUT_LEFT_BUTTON:
		std::cout << "Left Button" << std::endl;
		if (state == GLUT_DOWN){
			left_click = true;
		}
		else {
			left_click = false;
		}
		break;
	case GLUT_MIDDLE_BUTTON:
			if (state == GLUT_DOWN){
				middle_click = true;
				delta_z = y;
			}
			else {
				middle_click = false;
			}
		break;
	case GLUT_RIGHT_BUTTON:
		if (state == GLUT_DOWN){
			right_click = true;
			delta_x = x;
			delta_y = y;
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

int main( int argc, char** argv )
{
	std::string boid_positions("boid_positions.txt");
	std::string boid_velocities("boid_velocities.txt");
	std::string obstacles("obstacles.txt");

	glutInit( &argc, argv );
	// Setup FB configuration
	glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH );
	
	glutInitWindowSize( WIN_WIDTH, WIN_HEIGHT );
	glutInitWindowPosition( 0, 0 );

	glutCreateWindow( "Assignment 4 : Boids Simulation" );

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
	glutMouseFunc(mouseButton);
	glutMotionFunc(mouseMove);
	init(boid_positions, boid_velocities, obstacles); // our own initialize stuff func

	glutMainLoop();

	// clean up after loop
	deleteIDs();

	return 0;
}
