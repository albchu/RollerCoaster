// CPSC 587 Assignment 1
// Albert Chu 10059388
// Adapted and using code originally written by Andrew Owens
#include <sstream>

#include<iostream>
#include<cmath>

#include<GL/glew.h> // include BEFORE glut
#include<glut.h>

#include "ShaderTools.h"
#include "Vec3f.h"
#include "Mat4f.h"
#include "OpenGLMatrixTools.h"
#include <math.h>

using std::cout;
using std::endl;
using std::cerr;

GLuint vaoID;
GLuint basicProgramID;

// Could store these two in an array GLuint[]
GLuint vertBufferID;
GLuint colorBufferID;

Mat4f MVP;
Mat4f M;
Mat4f V;
Mat4f P;

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
int main( int, char** );
// function declarations
std::vector< Vec3f > verts;
typedef std::vector< Vec3f > VecV3f;

void loadVec3fFromFile(VecV3f & vecs, std::string const & fileName)
{
	using std::string;
	using std::stringstream;
	using std::istream_iterator;

	std::ifstream file(fileName);

	if (!file)
	{
		throw std::runtime_error("Unable to open file.");
	}

	string line;
	size_t index;
	stringstream ss(std::ios_base::in);

	size_t lineNum = 0;
	vecs.clear();

	while (getline(file, line))
	{
		++lineNum;

		// remove comments	
		index = line.find_first_of("#");
		if (index != string::npos)
		{
			line.erase(index, string::npos);
		}

		// removes leading/tailing junk
		line.erase(0, line.find_first_not_of(" \t\r\n\v\f"));
		index = line.find_last_not_of(" \t\r\n\v\f") + 1;
		if (index != string::npos)
		{
			line.erase(index, string::npos);
		}

		if (line.empty())
		{
			continue; // empty or commented out line
		}

		ss.str(line);
		ss.clear();

		Vec3f v;
		ss >> v;

		if (!ss || !ss.eof() || ss.good())
		{
			throw std::runtime_error("Error read file: "
				+ line
				+ " (line: "
				+ std::to_string(lineNum)
				+ ")");
		}
		else
		{
			vecs.push_back(v);
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
	// Draw Quads, start at vertex 0, draw verts.size() of them (for a quad)
	cout << "Verts size is " << verts.size() << endl;
	glDrawArrays(GL_LINE_LOOP, 0, verts.size());
	//glDrawArrays(GL_POINTS, 0, 8);

	glutSwapBuffers();
}

void idleFunc()
{
	// every frame refresh, rotate quad around y axis by 1 degree
//	MVP = MVP * RotateAboutYMatrix( 1.0 );
    M = M * RotateAboutYMatrix( 0.1 );
    setupModelViewProjectionTransform();

	// send changes to GPU
	reloadMVPUniform();
	
	glutPostRedisplay();
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

//Albert Note: I dont know what this function does but without it, bad shit happens I comment it out
void generateIDs()
{
	std::string vsSource = loadShaderStringfromFile( "./basic_vs.glsl" );
	std::string fsSource = loadShaderStringfromFile( "./basic_fs.glsl" );
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

    P = PerspectiveProjection(  60, // FOV
                                static_cast<float>(WIN_WIDTH)/WIN_HEIGHT, // Aspect
                                0.01,   // near plane
                                5 ); // far plane depth
}

void loadModelViewMatrix()
{
    M = UniformScaleMatrix( 0.25 );	// scale Quad First
    M = TranslateMatrix( 0, 0, -1.0 ) * M;	// translate away from (0,0,0)

    // view doesn't change, but if it did you would use this
    V = IdentityMatrix();
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
				GL_TRUE,	// transpose matrix, Mat4f is row major
				MVP.data()	// pointer to data in Mat4f
			);
}


//Albert Note: I dont know what this function does 
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

// Returns a subdivided vector array that goes to the specified depth of subdivision
std::vector < Vec3f > subdivide(std::vector < Vec3f > verts)
{
	std::vector < Vec3f > new_verts;
	for (int i = 0; i < verts.size(); i++)
	{
		Vec3f const& v1 = verts[i];
		Vec3f const& v2 = verts[(i + 1) % verts.size()];
		Vec3f mid = (v1 + v2) * 0.5;
		new_verts.push_back(v1);
		new_verts.push_back(mid);
	}
	return new_verts;
}

// Returns a subdivided vector array that goes to the specified depth of subdivision
std::vector < Vec3f > subdivision(std::vector < Vec3f > original_verts, int depth)
{
	cout << "WELCOME TO SUBDIVISION()" << endl;
	cout << "Original verts size is " << verts.size() << endl;
	std::vector < Vec3f > new_verts = original_verts;
	
	for (int i = 0; i < depth; i++)
	{
		new_verts = subdivide(new_verts);
	}
	cout << "New verts size is " << new_verts.size() << endl;

	//Delete original points
	int shifted_index = pow(2, depth);
	for (int i = original_verts.size() - 1; i >= 0; i--)
	{
		int delete_index = i * shifted_index;
		if (delete_index + 1 >= new_verts.size())	//Simple sanity check to ensure calculations is correct
			cerr << "Delete index wasnt calculated correctly. " << delete_index << " < new_verts.size() (" << new_verts.size() << ")" << endl;
		new_verts.erase(new_verts.begin() + delete_index);
	}
	cout << "New verts size is " << new_verts.size() << " after deleting original points" << endl;

	return new_verts;
}

float getRandFloat(float low, float high)
{
	return (low + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (high - low))));
}

void loadBuffer(std::string file_path)
{
	loadVec3fFromFile(verts, file_path);
	//verts.push_back( Vec3f( 1, 1, 4 ) );
	//verts.push_back( Vec3f( 1, -1, 3 ) );
	//verts.push_back( Vec3f( -1, -1, -2 ) );
	//verts.push_back( Vec3f( -1, 1, 1 ) );
	//verts.push_back(Vec3f(-1, 1, 2));

	cout << "Original verts size is " << verts.size() << endl;
	verts = subdivision(verts, 4);
	cout << "Subdivided verts size is " << verts.size() << endl;

	glBindBuffer( GL_ARRAY_BUFFER, vertBufferID );
	glBufferData(	GL_ARRAY_BUFFER,	
		sizeof(Vec3f) * verts.size(),	// byte size of Vec3f, 4 of them
		verts.data(),		// pointer (Vec3f*) to contents of verts
			GL_STATIC_DRAW );	// Usage pattern of GPU buffer

	// RGB values for the vertices
	std::vector<Vec3f> colors;
	for (int i = 0; i < verts.size(); i++)
	{
		float r = getRandFloat(0.50, 1.0);
		float g = getRandFloat(0.50, 1.0);
		float b = getRandFloat(0.50, 1.0);
		cout << r << endl;
		colors.push_back(Vec3f(r, g, b));
	}

	glBindBuffer( GL_ARRAY_BUFFER, colorBufferID );
	glBufferData(	GL_ARRAY_BUFFER,
		sizeof(Vec3f)*colors.size(),
			colors.data(),
			GL_STATIC_DRAW );
}

void init(std::string file_path)
{
	glEnable( GL_DEPTH_TEST );

	// SETUP SHADERS, BUFFERS, VAOs

	generateIDs();
	setupVAO();
	loadBuffer(file_path);

    loadModelViewMatrix();
    loadProjectionMatrix();
	setupModelViewProjectionTransform();
	reloadMVPUniform();
}

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
		cerr << "Failed to initialize GLEW" << endl;
		return -1;
	}
	cout << "GL Version: :" << glGetString(GL_VERSION) << endl;

    glutDisplayFunc( displayFunc );
	glutReshapeFunc( resizeFunc );
    glutIdleFunc( idleFunc );		// Stop the window from rotating until we reach the bead stage
	std::string file("C:\\Users\\Albert\\git\\CPSC-587\\Assignment1\\RollerCoaster\\RollerCoaster\\test.txt");
	init(file); // our own initialize stuff func

	glutMainLoop();

	// clean up after loop
	deleteIDs();

	return 0;
}
