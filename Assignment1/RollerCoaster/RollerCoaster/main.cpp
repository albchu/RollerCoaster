// CPSC 587 Assignment 1
// Albert Chu 10059388
// Adapted and using code originally written by Andrew Owens
#include <sstream>

#include<iostream>
#include<cmath>

#include<GL/glew.h> // include BEFORE glut
#include<GL/freeglut.h>

#include "ShaderTools.h"
#include "Vec3f.h"
#include "Mat4f.h"
#include "OpenGLMatrixTools.h"
#include <math.h>

using std::cout;
using std::endl;
using std::cerr;


GLuint basicProgramID;

// Could store these in some data structure
GLuint vaoTrackID;
GLuint trackVertBufferID;
GLuint trackColorBufferID; 

GLuint vaoCarID;
GLuint carVertBufferID;
GLuint carColorBufferID;

Mat4f MVP;
Mat4f M; // model matrix
Mat4f V; // view matrix
Mat4f P; // projection matrix can be left alone

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
std::vector< Vec3f > trackVerts;
std::vector< Vec3f > carVerts;

bool left_click = false;
bool right_click = false;
bool translate = true;
double delta_x = 0;
double delta_y = 0;
double delta_z = 0;

typedef std::vector< Vec3f > VecV3f;

void render()
{
	setupModelViewProjectionTransform();
	// send changes to GPU
	reloadMVPUniform();
	glutPostRedisplay();
}

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

	// Draw track
	glBindVertexArray(vaoTrackID);
	GLfloat width = 50;
	glLineWidth(width);
	glDrawArrays(GL_LINE_LOOP, 0, trackVerts.size());

	// Draw track
	glBindVertexArray(vaoCarID);
	glDrawArrays(GL_TRIANGLES, 0, carVerts.size());

	glutSwapBuffers();
}

//Keyboard commands for manipulating the enviorment
void keyboardFunc(unsigned char key, int x, int y){

	if (key == 'o')		// Orient view matrix to identity matrix 
		V = IdentityMatrix();
	if (key == 'r')		// Rotate Mode
		translate = false;
	if (key == 't')		// Translate Mode
		translate = true;
	render();
}

void mouseMove(int x, int y) 
{
	if (left_click)
	{
		delta_y -= y;
		delta_x -= x;

		if (translate)
			V = TranslateMatrix(-delta_x / 300, delta_y/500, 0) * V;
		else
		{
			V = V * RotateAboutYMatrix(delta_x / 10);
			V = V * RotateAboutXMatrix(delta_y / 10);
		}

		delta_x = x;
		delta_y = y;
	}
	if (right_click)
	{
		delta_z -= y;
		if (translate)
			V = TranslateMatrix(0, 0, delta_z/500) * V;
		else
		{
			V = V * RotateAboutZMatrix(delta_y / 100);
		}
		delta_z = y;
	}
	render();
}

void mouseButton(int button, int state, int x, int y)
{
	switch (button)
	{
	case GLUT_LEFT_BUTTON:
		cout << "Left Button" << endl;
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
		cerr << "Encountered an error with mouse button : " << button << ", state : " << state << endl;
	}
		cout << "Button: " << button << ", State: " << state << endl;
		printf("Button %s At %d %d\n", (state == GLUT_DOWN) ? "Down" : "Up", x, y);
}

void idleFunc()
{
	// every frame refresh, rotate quad around y axis by 1 degree
//	MVP = MVP * RotateAboutYMatrix( 1.0 );
    M = M * RotateAboutYMatrix( 0.01 );
	render();
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
	std::string vsSource = loadShaderStringfromFile( "./basic_vs.glsl" );
	std::string fsSource = loadShaderStringfromFile( "./basic_fs.glsl" );
	basicProgramID = CreateShaderProgram( vsSource, fsSource );

	// load IDs given from OpenGL
	glGenVertexArrays(1, &vaoTrackID);
	glGenBuffers(1, &trackVertBufferID);
	glGenBuffers(1, &trackColorBufferID);

	glGenVertexArrays(1, &vaoCarID);
	glGenBuffers(1, &carVertBufferID);
	glGenBuffers(1, &carColorBufferID);
}

void deleteIDs()
{
	glDeleteProgram( basicProgramID );

	glDeleteVertexArrays(1, &vaoTrackID);
	glDeleteBuffers(1, &trackVertBufferID);
	glDeleteBuffers(1, &trackColorBufferID);

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

    P = PerspectiveProjection(  60, // FOV
                                static_cast<float>(WIN_WIDTH)/WIN_HEIGHT, // Aspect
                                0.01,   // near plane
                                5 ); // far plane depth
}

void loadModelViewMatrix()
{
    M = UniformScaleMatrix( 0.25 );	// scale Quad First
    M = TranslateMatrix( 0, 0, -2.0 ) * M;	// translate away from (0,0,0)

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

// Returns a subdivided vector array that offsets each existing point in vector halfway to its next neighbor
std::vector < Vec3f > offset(std::vector < Vec3f > verts)
{
	std::vector < Vec3f > new_verts;
	for (int i = 0; i < verts.size(); i++)
	{
		Vec3f const& v1 = verts[i];
		Vec3f const& v2 = verts[(i + 1) % verts.size()];
		Vec3f mid = (v1 + v2) * 0.5;
		new_verts.push_back(mid);
	}
	return new_verts;
}

// Returns a subdivided vector array that goes to the specified depth of subdivision
std::vector < Vec3f > subdivision(std::vector < Vec3f > original_verts, int depth)
{
	std::vector < Vec3f > new_verts = original_verts;
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

// Sets up track vertex buffer from file, subdivide and process vectors to be smooth, Assign colors to the vertex shaders
void loadTrackBuffer(std::string file_path)
{
	loadVec3fFromFile(trackVerts, file_path);

	cout << "Original verts size is " << trackVerts.size() << endl;
	trackVerts = subdivision(trackVerts, 3);
	cout << "Subdivided verts size is " << trackVerts.size() << endl;

	glBindBuffer(GL_ARRAY_BUFFER, trackVertBufferID);
	glBufferData(GL_ARRAY_BUFFER,
		sizeof(Vec3f) * trackVerts.size(),	// byte size of Vec3f, 4 of them
		trackVerts.data(),		// pointer (Vec3f*) to contents of verts
		GL_STATIC_DRAW);	// Usage pattern of GPU buffer

	// RGB values for the vertices
	std::vector<Vec3f> colors;
	for (int i = 0; i < trackVerts.size(); i++)
	{
		float r = getRandFloat(0.50, 1.0);
		float g = getRandFloat(0.50, 1.0);
		float b = getRandFloat(0.50, 1.0);
		cout << r << endl;
		colors.push_back(Vec3f(r, g, b));
	}

	glBindBuffer(GL_ARRAY_BUFFER, trackColorBufferID);
	glBufferData(GL_ARRAY_BUFFER,
		sizeof(Vec3f)*colors.size(),
		colors.data(),
		GL_STATIC_DRAW);
}

void loadCarBuffer(std::string file_path)
{
	loadVec3fFromFile(carVerts, file_path);

	glBindBuffer(GL_ARRAY_BUFFER, carVertBufferID);
	glBufferData(GL_ARRAY_BUFFER,
		sizeof(Vec3f) * carVerts.size(),	// byte size of Vec3f, 4 of them
		carVerts.data(),		// pointer (Vec3f*) to contents of verts
		GL_STATIC_DRAW);	// Usage pattern of GPU buffer

	// RGB values for the vertices
	std::vector<Vec3f> colors;
	for (int i = 0; i < carVerts.size(); i++)
	{
		float r = getRandFloat(0.50, 1.0);
		float g = getRandFloat(0.50, 1.0);
		float b = getRandFloat(0.50, 1.0);
		cout << r << endl;
		colors.push_back(Vec3f(r, g, b));
	}

	glBindBuffer(GL_ARRAY_BUFFER, carColorBufferID);
	glBufferData(GL_ARRAY_BUFFER,
		sizeof(Vec3f)*colors.size(),
		colors.data(),
		GL_STATIC_DRAW);
}

void init(std::string track_file_path, std::string car_file_path)
{
	glEnable( GL_DEPTH_TEST );

	// SETUP SHADERS, BUFFERS, VAOs

	generateIDs();
	setupTrackVAO();
	setupCarVAO();
	loadTrackBuffer(track_file_path);
	loadCarBuffer(car_file_path);

    loadModelViewMatrix();
    loadProjectionMatrix();
	setupModelViewProjectionTransform();
	reloadMVPUniform();
}
void printInfo()
{
	cout << "Using GLEW " << glewGetString(GLEW_VERSION) << endl;
	cout << "Vendor: " << glGetString(GL_VENDOR) << endl;
	cout << "Renderer: " << glGetString(GL_RENDERER) << endl;
	cout << "Version: " << glGetString(GL_VERSION) << endl;
	cout << "GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
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
	printInfo();

    glutDisplayFunc( displayFunc );
	glutReshapeFunc( resizeFunc );
    glutIdleFunc( idleFunc );		
	glutMouseFunc(mouseButton);
	glutMotionFunc(mouseMove);
	glutKeyboardFunc(keyboardFunc);
	std::string track("C:\\Users\\Albert\\git\\CPSC-587\\Assignment1\\RollerCoaster\\RollerCoaster\\track.txt");
	std::string car("C:\\Users\\Albert\\git\\CPSC-587\\Assignment1\\RollerCoaster\\RollerCoaster\\car.txt");
	init(track, car); // our own initialize stuff func

	glutMainLoop();

	// clean up after loop
	deleteIDs();

	return 0;
}
