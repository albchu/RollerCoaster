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

#include "ShaderTools.h"
#include "Vec3f.h"
#include "Mat4f.h"
#include "OpenGLMatrixTools.h"
#include <math.h>

using namespace std;
using namespace glm;



// Could store these in some data structure
GLuint vaoTrackID;
GLuint trackProgramID;
GLuint trackVertBufferID;
GLuint trackColorBufferID; 

GLuint vaoCarID;
GLuint carProgramID;
GLuint carVertBufferID;
GLuint carColorBufferID;
Vec3f carCenter;

Mat4f trackMVP;
Mat4f carMVP;
Mat4f carM; // model matrix
Mat4f trackM; // model matrix

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
vector< vec3 > trackVerts;
vector< vec3 > carVerts;

bool left_click = false;
bool right_click = false;
bool translate_bool = true;
double delta_x = 0;
double delta_y = 0;
double delta_z = 0;

//typedef vector< Vec3f > VecV3f;

void printInfo()
{
	cout << "Using GLEW " << glewGetString(GLEW_VERSION) << endl;
	cout << "Vendor: " << glGetString(GL_VENDOR) << endl;
	cout << "Renderer: " << glGetString(GL_RENDERER) << endl;
	cout << "Version: " << glGetString(GL_VERSION) << endl;
	cout << "GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
}

void render()
{
	setupModelViewProjectionTransform();
	// send changes to GPU
	reloadMVPUniform();
	glutPostRedisplay();
}

void loadVec3FromFile(vector<vec3> & vecs, string const & fileName)
{
	ifstream file(fileName);

	if (!file)
	{
		throw runtime_error("Unable to open file.");
	}

	string line;
	size_t index;
	stringstream ss(ios_base::in);

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

		float x, y, z;
		if ((ss >> x >> y >> z) && (!ss || !ss.eof() || ss.good()))
		{
			throw runtime_error("Error read file: "
				+ line
				+ " (line: "
				+ std::to_string(lineNum)
				+ ")");
		}
		else
		{
			vecs.push_back(vec3(x,y,z));
		}
	}
	file.close();
}

void displayFunc()
{
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	// Use our shader

	// Use VAO that holds buffer bindings
	// and attribute config of buffers

	// Draw track
	glUseProgram(trackProgramID);
	glBindVertexArray(vaoTrackID);
	GLfloat width = 50;
	glLineWidth(width);
	//glDrawArrays(GL_POINTS, 0, trackVerts.size());
	glDrawArrays(GL_LINE_LOOP, 0, trackVerts.size());

	// Draw track
	glUseProgram(carProgramID);
	glBindVertexArray(vaoCarID);
	//glDrawArrays(GL_POINTS, 0, carVerts.size());
	
	glDrawArrays(GL_TRIANGLES, 0, carVerts.size());

	glutSwapBuffers();
}

//Keyboard commands for manipulating the enviorment
void keyboardFunc(unsigned char key, int x, int y){

	if (key == 'o')		// Orient view matrix to identity matrix 
	{
		V = IdentityMatrix();
	}
	if (key == 'r')		// Rotate Mode
		translate_bool = false;
	if (key == 't')		// Translate Mode
		translate_bool = true;
	render();
}

void mouseMove(int x, int y) 
{
	if (left_click)
	{
		delta_y -= y;
		delta_x -= x;

		if (translate_bool)
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
		V = TranslateMatrix(0, 0, delta_z/500) * V;
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
	//trackM = trackM * RotateAboutYMatrix(0.01);
	carM = carM * RotateAboutYMatrix(-0.01);
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

void generateCarID(string vsSource, string fsSource)
{
	carProgramID = CreateShaderProgram(vsSource, fsSource);

	// load IDs given from OpenGL
	glGenVertexArrays(1, &vaoCarID);
	glGenBuffers(1, &carVertBufferID);
	glGenBuffers(1, &carColorBufferID);
}

void generateTrackID(string vsSource, string fsSource)
{
	trackProgramID = CreateShaderProgram(vsSource, fsSource);

	// load IDs given from OpenGL
	glGenVertexArrays(1, &vaoTrackID);
	glGenBuffers(1, &trackVertBufferID);
	glGenBuffers(1, &trackColorBufferID);
}

void generateIDs()
{
	string vsSource = loadShaderStringfromFile("./basic_vs.glsl");
	string fsSource = loadShaderStringfromFile("./basic_fs.glsl");

	generateCarID(vsSource, fsSource);
	generateTrackID(vsSource, fsSource);
}
void deleteIDs()
{
	glDeleteProgram(trackProgramID);
	glDeleteProgram(carProgramID);

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
                                50 ); // far plane depth
}

void loadModelViewMatrix()
{
	trackM = UniformScaleMatrix(1);	// scale Quad First
	//trackM = TranslateMatrix(0, 0, -2.0) * trackM;	// translate away from (0,0,0)

	//Vec3f trackStart = trackVerts.at(0);
	//float translateX = trackStart.x();
	//float translateY = trackStart.z();
	//float translateZ = trackStart.x();
	//cout << "Track start: " << trackStart << endl;
	////carM = TranslateMatrix(0, 0, -2.0) * carM;	// translate away from (0,0,0)
	////carM = IdentityMatrix();	// scale Quad First
	carM = UniformScaleMatrix(.05);	// scale Quad First
	cout << "CarM: " << carM << endl;
	//carM = TranslateMatrix(translateX, translateY, translateZ) * carM;	// Translate car model matrix to begining of track matrix
	cout << "CarM: " << carM << endl;

    // view doesn't change, but if it did you would use this
    V = IdentityMatrix();
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
		GL_TRUE,	// transpose matrix, Mat4f is row major
		trackMVP.data()	// pointer to data in Mat4f
		);
}

void reloadCarMVPUniform()
{
	GLint mvpID = glGetUniformLocation(carProgramID, "MVP");

	glUseProgram(carProgramID);
	glUniformMatrix4fv(mvpID,		// ID
		1,		// only 1 matrix
		GL_TRUE,	// transpose matrix, Mat4f is row major
		carMVP.data()	// pointer to data in Mat4f
		);
}

void reloadMVPUniform()
{
	reloadCarMVPUniform();
	reloadTrackMVPUniform();
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
vector < vec3 > subdivide(vector < vec3 > verts)
{
	vector < vec3 > new_verts;
	for (int i = 0; i < verts.size(); i++)
	{
		vec3 const& v1 = verts[i];
		vec3 const& v2 = verts[(i + 1) % verts.size()];
		vec3 mid = (v1 + v2) * 0.5f;
		new_verts.push_back(v1);
		new_verts.push_back(mid);
	}
	return new_verts;
}

// Returns a subdivided vector array that offsets each existing point in vector halfway to its next neighbor
vector < vec3 > offset(vector < vec3 > verts)
{
	vector < vec3 > new_verts;
	for (int i = 0; i < verts.size(); i++)
	{
		vec3 const& v1 = verts[i];
		vec3 const& v2 = verts[(i + 1) % verts.size()];
		vec3 mid = (v1 + v2) * 0.5f;
		new_verts.push_back(mid);
	}
	return new_verts;
}

// Returns a subdivided vector array that goes to the specified depth of subdivision
vector < vec3 > subdivision(vector < vec3 > original_verts, int depth)
{
	vector < vec3 > new_verts = original_verts;
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
void loadTrackBuffer(string file_path)
{
	loadVec3FromFile(trackVerts, file_path);
	cout << glm::to_string(trackVerts[0]) << endl;
	cout << value_ptr(trackVerts[0]) << endl;
	trackVerts = subdivision(trackVerts, 3);

	glBindBuffer(GL_ARRAY_BUFFER, trackVertBufferID);
	glBufferData(GL_ARRAY_BUFFER,
		sizeof(vec3) * trackVerts.size(),	// byte size of Vec3f, 4 of them
		trackVerts.data(),		// pointer (Vec3f*) to contents of verts
		GL_STATIC_DRAW);	// Usage pattern of GPU buffer

	// RGB values for the vertices
	vector<vec3> colors;
	for (int i = 0; i < trackVerts.size(); i++)
	{
		float r = getRandFloat(0.50, 1.0);
		float g = getRandFloat(0.50, 1.0);
		float b = getRandFloat(0.50, 1.0);
		colors.push_back(vec3(r, g, b));
	}

	glBindBuffer(GL_ARRAY_BUFFER, trackColorBufferID);
	glBufferData(GL_ARRAY_BUFFER,
		sizeof(vec3)*colors.size(),
		colors.data(),
		GL_STATIC_DRAW);
}
Vec3f center(vector<Vec3f> verts)
{
	return Vec3f(0, 0, 0);	//Unsure how to get center of object so im going to hack it to start at center
}

void loadCarBuffer(string file_path)
{
	loadVec3FromFile(carVerts, file_path);

	glBindBuffer(GL_ARRAY_BUFFER, carVertBufferID);
	glBufferData(GL_ARRAY_BUFFER,
		sizeof(Vec3f) * carVerts.size(),	// byte size of Vec3f, 4 of them
		carVerts.data(),		// pointer (Vec3f*) to contents of verts
		GL_STATIC_DRAW);	// Usage pattern of GPU buffer

	// Locate center of car
	//carCenter = center(carVerts);

	// RGB values for the vertices
	vector<vec3> colors;
	for (int i = 0; i < carVerts.size(); i++)
	{
		float r = getRandFloat(0.50, 1.0);
		float g = getRandFloat(0.50, 1.0);
		float b = getRandFloat(0.50, 1.0);
		colors.push_back(vec3(r, g, b));
	}

	glBindBuffer(GL_ARRAY_BUFFER, carColorBufferID);
	glBufferData(GL_ARRAY_BUFFER,
		sizeof(vec3)*colors.size(),
		colors.data(),
		GL_STATIC_DRAW);
}

void init(string track_file_path, string car_file_path)
{
	glEnable( GL_DEPTH_TEST );

	// SETUP SHADERS, BUFFERS, VAOs

	generateIDs();
	setupTrackVAO();
	loadTrackBuffer(track_file_path);
	
	setupCarVAO();
	loadCarBuffer(car_file_path);

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
	printInfo();
    glutDisplayFunc( displayFunc );
	glutReshapeFunc( resizeFunc );
    glutIdleFunc( idleFunc );		
	glutMouseFunc(mouseButton);
	glutMotionFunc(mouseMove);
	glutKeyboardFunc(keyboardFunc);
	string track("C:\\Users\\Albert\\git\\CPSC-587\\Assignment1\\RollerCoaster\\RollerCoaster\\track.txt");
	string car("C:\\Users\\Albert\\git\\CPSC-587\\Assignment1\\RollerCoaster\\RollerCoaster\\car.txt");
	init(track, car); // our own initialize stuff func

	glutMainLoop();

	// clean up after loop
	deleteIDs();

	return 0;
}
