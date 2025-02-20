#include <iostream>
#include <GL/glew.h>
#include <3dgl/3dgl.h>
#include <GL/glut.h>
#include <GL/freeglut_ext.h>

// Include GLM core features
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#pragma comment (lib, "glew32.lib")

using namespace std;
using namespace _3dgl;
using namespace glm;


//WATER REFLECTION
// 
// Find the reflection surface (point and normal) rotation of the entire mirror around the Y axis (ry) and swivel of the mirror around the X axis (rx)
vec3 p(0, 0.f ,0);


vec3 n(0, -1,0);
 //vec3 n(sin(ry)* cos(rx), sin(rx), cos(ry)* cos(rx));
 
// reflection matrix
float a = n.x, b = n.y, c = n.z, d = -dot(p, n);


// parameters of the reflection plane: Ax + By + Cz + d = 0

mat4 matrixReflection = mat4(1 - 2 * a * a, -2 * a * b, -2 * a * c, 0,
	-2 * a * b, 1 - 2 * b * b, -2 * b * c, 0,
	-2 * a * c, -2 * b * c, 1 - 2 * c * c, 0,
	-2 * a * d, -2 * b * d, -2 * c * d, 1);




// 3D models
C3dglModel table;
C3dglModel vase;
C3dglModel chicken;
C3dglModel lamp;
C3dglModel room;
C3dglModel mirror;
unsigned nPyramidBuf = 0;

// texture ids
GLuint idTexWood;
GLuint idTexCloth;
GLuint idTexNone;

// GLSL Program
C3dglProgram program;

// The View Matrix
mat4 matrixView;

// Camera & navigation
float maxspeed = 4.f;	// camera max speed
float accel = 4.f;		// camera acceleration
vec3 _acc(0), _vel(0);	// camera acceleration and velocity vectors
float _fov = 60.f;		// field of view (zoom)

// light switches
float fAmbient = 1, fDir = 1, fPoint1 = 1, fPoint2 = 1;


GLuint reflectionFBO, reflectionTexture, depthBuffer;

void setupReflectionFBO(int width, int height) {
	glGenFramebuffers(1, &reflectionFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, reflectionFBO);

	// Create texture for storing reflection
	glGenTextures(1, &reflectionTexture);
	glBindTexture(GL_TEXTURE_2D, reflectionTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, reflectionTexture, 0);

	// Create depth buffer
	glGenRenderbuffers(1, &depthBuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		std::cerr << "Framebuffer not complete!" << std::endl;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}




bool init()
{
	// rendering states
	glEnable(GL_DEPTH_TEST);	// depth test is necessary for most 3D scenes
	glEnable(GL_NORMALIZE);		// normalization is needed by AssImp library models
	glShadeModel(GL_SMOOTH);	// smooth shading mode is the default one; try GL_FLAT here!
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);	// this is the default one; try GL_LINE!

	// Initialise Shaders
	C3dglShader vertexShader;
	C3dglShader fragmentShader;

	if (!vertexShader.create(GL_VERTEX_SHADER)) return false;
	if (!vertexShader.loadFromFile("shaders/basic.vert.shader")) return false;
	if (!vertexShader.compile()) return false;

	if (!fragmentShader.create(GL_FRAGMENT_SHADER)) return false;
	if (!fragmentShader.loadFromFile("shaders/basic.frag.shader")) return false;
	if (!fragmentShader.compile()) return false;

	if (!program.create()) return false;
	if (!program.attach(vertexShader)) return false;
	if (!program.attach(fragmentShader)) return false;
	if (!program.link()) return false;
	if (!program.use(true)) return false;

	// glut additional setup
	glutSetVertexAttribCoord3(program.getAttribLocation("aVertex"));
	glutSetVertexAttribNormal(program.getAttribLocation("aNormal"));

	// load your 3D models here!
	if (!table.load("models\\table.obj")) return false;
	if (!vase.load("models\\vase.obj")) return false;
	if (!chicken.load("models\\chicken.obj")) return false;
	if (!lamp.load("models\\lamp.obj")) return false;
	if (!room.load("models\\LivingRoomObj\\room2.obj")) return false;
	room.loadMaterials("models\\LivingRoomObj\\");
	if (!mirror.load("models\\ad.fbx")) return false;
 

	// create pyramid
	float vermals[] = {
	  -4, 0,-4, 0, 4,-7, 4, 0,-4, 0, 4,-7, 0, 7, 0, 0, 4,-7, 
	  -4, 0, 4, 0, 4, 7, 4, 0, 4, 0, 4, 7, 0, 7, 0, 0, 4, 7, 
	  -4, 0,-4,-7, 4, 0,-4, 0, 4,-7, 4, 0, 0, 7, 0,-7, 4, 0,
	   4, 0,-4, 7, 4, 0, 4, 0, 4, 7, 4, 0, 0, 7, 0, 7, 4, 0, 
	  -4, 0,-4, 0,-1, 0,-4, 0, 4, 0,-1, 0, 4, 0,-4, 0,-1, 0, 
	   4, 0, 4, 0,-1, 0,-4, 0, 4, 0,-1, 0, 4, 0,-4, 0,-1, 0 };

	// Generate 1 buffer name
	glGenBuffers(1, &nPyramidBuf);
	// Bind (activate) the buffer
	glBindBuffer(GL_ARRAY_BUFFER, nPyramidBuf);
	// Send data to the buffer
	glBufferData(GL_ARRAY_BUFFER, sizeof(vermals), vermals, GL_STATIC_DRAW);

	// Setup Lights - note that diffuse and specular values are moved to onRender
	program.sendUniform("lightEmissive.color", vec3(0));
	program.sendUniform("lightDir.direction", vec3(1.0, 0.5, 1.0));
	program.sendUniform("lightPoint1.position", vec3(-2.95, 4.24, -1.0));
	program.sendUniform("lightPoint2.position", vec3(1.05, 4.24, 1.0));

	// Setup materials
	program.sendUniform("materialAmbient", vec3(1.0, 1.0, 1.0));
	program.sendUniform("materialDiffuse", vec3(1.0, 1.0, 1.0));
	program.sendUniform("materialSpecular", vec3(0.0, 0.0, 0.0));
	program.sendUniform("shininess", 10.0);

	// create & load textures
	C3dglBitmap bm;
	glActiveTexture(GL_TEXTURE0);
	
	// cloth texture
	bm.load("models/cloth.png", GL_RGBA);
	if (!bm.getBits()) return false;
	glGenTextures(1, &idTexCloth);
	glBindTexture(GL_TEXTURE_2D, idTexCloth);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bm.getWidth(), bm.getHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, bm.getBits());

	// wood texture
	bm.load("models/oak.png", GL_RGBA);
	if (!bm.getBits()) return false;
	glGenTextures(1, &idTexWood);
	glBindTexture(GL_TEXTURE_2D, idTexWood);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bm.getWidth(), bm.getHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, bm.getBits());

	// none (simple-white) texture
	glGenTextures(1, &idTexNone);
	glBindTexture(GL_TEXTURE_2D, idTexNone);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	BYTE bytes[] = { 255, 255, 255 };
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_BGR, GL_UNSIGNED_BYTE, &bytes);

	// Send the texture info to the shaders
	program.sendUniform("texture0", 0);

	// Initialise the View Matrix (initial position of the camera)
	matrixView = rotate(mat4(1), radians(12.f), vec3(1, 0, 0));
	matrixView *= lookAt(
		vec3(1.0, 5.0, 6.0),
		vec3(1.0, 5.0, 0.0),
		vec3(0.0, 1.0, 0.0));

	// setup the screen background colour
	glClearColor(0.18f, 0.25f, 0.22f, 1.0f);   // deep grey background

	setupReflectionFBO(1280,720);
	return true;
}


void renderScene(mat4& matrixView, float time, float deltaTime)
{
	mat4 m;

	// setup lights
	program.sendUniform("lightAmbient.color", fAmbient * vec3(22.05, 22.05, 22.05));
	program.sendUniform("lightDir.diffuse", fDir * vec3(0.3, 0.3, 0.3));	  // dimmed white light
	program.sendUniform("lightPoint1.diffuse", fPoint1 * vec3(0.5, 0.5, 0.5));
	program.sendUniform("lightPoint1.specular", fPoint1 * vec3(1.0, 1.0, 1.0));
	program.sendUniform("lightPoint2.diffuse", fPoint2 * vec3(0.5, 0.5, 0.5));
	program.sendUniform("lightPoint2.specular", fPoint2 * vec3(1.0, 1.0, 1.0));

	// setup materials
	program.sendUniform("materialSpecular", vec3(0.0, 0.0, 0.0));

	// room
	program.sendUniform("materialDiffuse", vec3(1.0, 1.0, 1.0));
	program.sendUniform("materialAmbient", vec3(1.0, 1.0, 1.0));
	m = matrixView;

	m = scale(m, vec3(0.03f, 0.03f, 0.03f));

	room.render(m);

 
	// table & chairs
	program.sendUniform("materialDiffuse", vec3(1.0, 1.0, 1.0));
	program.sendUniform("materialAmbient", vec3(1.0, 1.0, 1.0));
	glBindTexture(GL_TEXTURE_2D, idTexWood);
	m = matrixView;
	m = scale(m, vec3(0.004f, 0.004f, 0.004f));
	table.render(1, m);
	glBindTexture(GL_TEXTURE_2D, idTexCloth);
	table.render(0, m);
	m = rotate(m, radians(180.f), vec3(0.f, 1.f, 0.f));
	table.render(0, m);
	m = translate(m, vec3(250, 0, 0));
	m = rotate(m, radians(90.f), vec3(0.f, 1.f, 0.f));
	table.render(0, m);
	m = translate(m, vec3(0, 0, -500));
	m = rotate(m, radians(180.f), vec3(0.f, 1.f, 0.f));
	table.render(0, m);

	// vase
	program.sendUniform("materialDiffuse", vec3(0.2, 0.4, 0.8));
	program.sendUniform("materialAmbient", vec3(0.2, 0.4, 0.8));
	program.sendUniform("materialSpecular", vec3(1.0, 1.0, 1.0));
	glBindTexture(GL_TEXTURE_2D, idTexNone);
	m = matrixView;
	m = translate(m, vec3(0.f, 3.f, 0.f));
	m = scale(m, vec3(0.12f, 0.12f, 0.12f));
	vase.render(m);

	// teapot
	program.sendUniform("materialDiffuse", vec3(0.1, 0.8, 0.3));
	program.sendUniform("materialAmbient", vec3(0.1, 0.8, 0.3));
	program.sendUniform("materialSpecular", vec3(1.0, 1.0, 1.0));
	m = matrixView;
	m = translate(m, vec3(1.8f, 3.4f, 0.0f));
	program.sendUniform("matrixModelView", m);
	glutSolidTeapot(0.5);

	// pyramid
	program.sendUniform("materialDiffuse", vec3(1.0, 0.2, 0.2));
	program.sendUniform("materialAmbient", vec3(1.0, 0.2, 0.2));
	program.sendUniform("materialSpecular", vec3(0.0, 0.0, 0.0));
	m = matrixView;
	m = translate(m, vec3(-1.5f, 3.7f, 0.5f));
	m = rotate(m, radians(180.f), vec3(1, 0, 0));
	m = rotate(m, radians(-40 * time), vec3(0, 1, 0));
	m = scale(m, vec3(0.1f, 0.1f, 0.1f));
	program.sendUniform("matrixModelView", m);

	GLuint attribVertex = program.getAttribLocation("aVertex");
	GLuint attribNormal = program.getAttribLocation("aNormal");
	glBindBuffer(GL_ARRAY_BUFFER, nPyramidBuf);
	glEnableVertexAttribArray(attribVertex);
	glEnableVertexAttribArray(attribNormal);
	glVertexAttribPointer(attribVertex, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), 0);
	glVertexAttribPointer(attribNormal, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glDrawArrays(GL_TRIANGLES, 0, 18);
	glDisableVertexAttribArray(GL_VERTEX_ARRAY);
	glDisableVertexAttribArray(GL_NORMAL_ARRAY);

	// chicken
	program.sendUniform("materialDiffuse", vec3(0.8, 0.8, 0.2));
	program.sendUniform("materialAmbient", vec3(0.8, 0.8, 0.2));
	program.sendUniform("materialSpecular", vec3(0.6, 0.6, 1.0));
	m = translate(m, vec3(0, -5, 0));
	m = scale(m, vec3(0.2f, 0.2f, 0.2f));
	m = rotate(m, radians(180.f), vec3(1, 0, 0));
	chicken.render(m);

	// lamp 1
	m = matrixView;
	m = translate(m, vec3(-2.2f, 3.075f, -1.0f));
	m = scale(m, vec3(0.02f, 0.02f, 0.02f));
	lamp.render(m);

	// lamp 2
	m = matrixView;
	m = translate(m, vec3(1.8f, 3.075f, 1.0f));
	m = scale(m, vec3(0.02f, 0.02f, 0.02f));
	lamp.render(m);

	// light bulb 1
	program.sendUniform("materialDiffuse", vec3(0.8, 0.8, 0.8));
	program.sendUniform("materialAmbient", vec3(1.0, 1.0, 1.0));
	program.sendUniform("lightEmissive.color", vec3(fPoint1));
	m = matrixView;
	m = translate(m, vec3(-2.95f, 4.24f, -1.0f));
	m = scale(m, vec3(0.1f, 0.1f, 0.1f));
	program.sendUniform("matrixModelView", m);
	glutSolidSphere(1, 32, 32);

	// light bulb 2
	program.sendUniform("materialDiffuse", vec3(0.8, 0.8, 0.8));
	program.sendUniform("materialAmbient", vec3(1.0, 1.0, 1.0));
	program.sendUniform("lightEmissive.color", vec3(fPoint2));
	m = matrixView;
	m = translate(m, vec3(1.05f, 4.24f, 1.0f));
	m = scale(m, vec3(0.1f, 0.1f, 0.1f));
	program.sendUniform("matrixModelView", m);
	glutSolidSphere(1, 32, 32);
	program.sendUniform("lightEmissive.color", vec3(0));
}

 
void planarReflection2(mat4& matrixView, float time, float deltaTime)
{
	glBindFramebuffer(GL_FRAMEBUFFER, reflectionFBO);
	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport); // Save original viewport

	glViewport(0, 0, 1280, 720);
	glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); // Clear buffers

	// Enable stencil test for reflection
	glEnable(GL_STENCIL_TEST);
	glStencilFunc(GL_ALWAYS, 1, 1);
	glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);

	glDisable(GL_DEPTH_TEST);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

	mat4 m = matrixView;
	m = translate(m, vec3(0, 0.1f, 0));
	m = scale(m, vec3(0.5, 1, 0.5));

	mirror.render(m);

	glStencilFunc(GL_EQUAL, 1, 1);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

	glEnable(GL_DEPTH_TEST);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	glEnable(GL_CLIP_PLANE0);
	vec4 planeClip = vec4(0.0f, 1.0f, 0.0f, 0.0f);
	program.sendUniform("planeClip", planeClip);

	matrixView *= matrixReflection;
	program.sendUniform("matrixView", matrixView);

	renderScene(matrixView, time, deltaTime);

	glDisable(GL_CLIP_PLANE0);
	glDisable(GL_STENCIL_TEST);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);


	matrixView *= matrixReflection;
}



	

 
 


void renderMirrorSurface() {
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
 
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, reflectionTexture);

	mat4 m = matrixView;
	m = translate(m, vec3(0, 0.f, 0));
	m = scale(m, vec3(0.5, 1, 0.5));

	mirror.render(m);

	glDisable(GL_BLEND);
}


void onRender()
{
	// these variables control time & animation
	static float prev = 0;
	float time = glutGet(GLUT_ELAPSED_TIME) * 0.001f;	// time since start in seconds
	float deltaTime = time - prev;						// time since last frame
	prev = time;										// framerate is 1/deltaTime

	// clear screen and buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	// setup the View Matrix (camera)
	_vel = clamp(_vel + _acc * deltaTime, -vec3(maxspeed), vec3(maxspeed));
	float pitch = getPitch(matrixView);
	matrixView = rotate(translate(rotate(mat4(1),
		pitch, vec3(1, 0, 0)),	// switch the pitch off
		_vel * deltaTime),		// animate camera motion (controlled by WASD keys)
		-pitch, vec3(1, 0, 0))	// switch the pitch on
		* matrixView;


	planarReflection2(matrixView, time, deltaTime);
	program.sendUniform("matrixView", matrixView);
	// Render the normal scene
	renderScene(matrixView, time, deltaTime);

	// Enable blending to render reflection on the mirror
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Bind the reflection texture and render the mirror surface with reflection
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, reflectionTexture);

	mat4 m = matrixView;
	m = translate(m, vec3(0, 0.1f, 0)); // Small offset
	m = scale(m, vec3(0.5, 1, 0.5)); // Mirror surface scaling
	mirror.render(m); // Render the mirror with reflection texture

	// Disable blending after rendering the mirror
	//glDisable(GL_BLEND);
	 
	// essential for double-buffering technique
	glutSwapBuffers();

	// proceed the animation
	glutPostRedisplay();
}

// called before window opened or resized - to setup the Projection Matrix
void onReshape(int w, int h)
{
	float ratio = w * 1.0f / h;      // we hope that h is not zero
	glViewport(0, 0, w, h);
	program.sendUniform("matrixProjection", perspective(radians(_fov), ratio, 0.02f, 1000.f));
}

// Handle WASDQE keys
void onKeyDown(unsigned char key, int x, int y)
{
	switch (tolower(key))
	{
	case 'w': _acc.z = accel; break;
	case 's': _acc.z = -accel; break;
	case 'a': _acc.x = accel; break;
	case 'd': _acc.x = -accel; break;
	case 'e': _acc.y = accel; break;
	case 'q': _acc.y = -accel; break;
	case '1': fPoint1 = 1 - fPoint1; break;
	case '2': fPoint2 = 1 - fPoint2; break;
	case '9': fDir = 1 - fDir; break;
	case '0': fAmbient = 1 - fAmbient; break;
	}
}

// Handle WASDQE keys (key up)
void onKeyUp(unsigned char key, int x, int y)
{
	switch (tolower(key))
	{
	case 'w':
	case 's': _acc.z = _vel.z = 0; break;
	case 'a':
	case 'd': _acc.x = _vel.x = 0; break;
	case 'q':
	case 'e': _acc.y = _vel.y = 0; break;
	}
}

// Handle arrow keys and Alt+F4
void onSpecDown(int key, int x, int y)
{
	maxspeed = glutGetModifiers() & GLUT_ACTIVE_SHIFT ? 20.f : 4.f;
	switch (key)
	{
	case GLUT_KEY_F4:		if ((glutGetModifiers() & GLUT_ACTIVE_ALT) != 0) exit(0); break;
	case GLUT_KEY_UP:		onKeyDown('w', x, y); break;
	case GLUT_KEY_DOWN:		onKeyDown('s', x, y); break;
	case GLUT_KEY_LEFT:		onKeyDown('a', x, y); break;
	case GLUT_KEY_RIGHT:	onKeyDown('d', x, y); break;
	case GLUT_KEY_PAGE_UP:	onKeyDown('q', x, y); break;
	case GLUT_KEY_PAGE_DOWN:onKeyDown('e', x, y); break;
	case GLUT_KEY_F11:		glutFullScreenToggle();
	}
}

// Handle arrow keys (key up)
void onSpecUp(int key, int x, int y)
{
	maxspeed = glutGetModifiers() & GLUT_ACTIVE_SHIFT ? 20.f : 4.f;
	switch (key)
	{
	case GLUT_KEY_UP:		onKeyUp('w', x, y); break;
	case GLUT_KEY_DOWN:		onKeyUp('s', x, y); break;
	case GLUT_KEY_LEFT:		onKeyUp('a', x, y); break;
	case GLUT_KEY_RIGHT:	onKeyUp('d', x, y); break;
	case GLUT_KEY_PAGE_UP:	onKeyUp('q', x, y); break;
	case GLUT_KEY_PAGE_DOWN:onKeyUp('e', x, y); break;
	}
}

// Handle mouse click
void onMouse(int button, int state, int x, int y)
{
	glutSetCursor(state == GLUT_DOWN ? GLUT_CURSOR_CROSSHAIR : GLUT_CURSOR_INHERIT);
	glutWarpPointer(glutGet(GLUT_WINDOW_WIDTH) / 2, glutGet(GLUT_WINDOW_HEIGHT) / 2);
	if (button == 1)
	{
		_fov = 60.0f;
		onReshape(glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT));
	}
}

// handle mouse move
void onMotion(int x, int y)
{
	glutWarpPointer(glutGet(GLUT_WINDOW_WIDTH) / 2, glutGet(GLUT_WINDOW_HEIGHT) / 2);

	// find delta (change to) pan & pitch
	float deltaYaw = 0.005f * (x - glutGet(GLUT_WINDOW_WIDTH) / 2);
	float deltaPitch = 0.005f * (y - glutGet(GLUT_WINDOW_HEIGHT) / 2);

	if (abs(deltaYaw) > 0.3f || abs(deltaPitch) > 0.3f)
		return;	// avoid warping side-effects

	// View = Pitch * DeltaPitch * DeltaYaw * Pitch^-1 * View;
	constexpr float maxPitch = radians(80.f);
	float pitch = getPitch(matrixView);
	float newPitch = glm::clamp(pitch + deltaPitch, -maxPitch, maxPitch);
	matrixView = rotate(rotate(rotate(mat4(1.f),
		newPitch, vec3(1.f, 0.f, 0.f)),
		deltaYaw, vec3(0.f, 1.f, 0.f)), 
		-pitch, vec3(1.f, 0.f, 0.f)) 
		* matrixView;
}

void onMouseWheel(int button, int dir, int x, int y)
{
	_fov = glm::clamp(_fov - dir * 5.f, 5.0f, 175.f);
	onReshape(glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT));
}

int main(int argc, char **argv)
{
	// init GLUT and create Window
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(1280, 720);
	glutCreateWindow("3DGL Scene: Textured Room");

	// init glew
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		C3dglLogger::log("GLEW Error {}", (const char*)glewGetErrorString(err));
		return 0;
	}
	C3dglLogger::log("Using GLEW {}", (const char*)glewGetString(GLEW_VERSION));

	// register callbacks
	glutDisplayFunc(onRender);
	glutReshapeFunc(onReshape);
	glutKeyboardFunc(onKeyDown);
	glutSpecialFunc(onSpecDown);
	glutKeyboardUpFunc(onKeyUp);
	glutSpecialUpFunc(onSpecUp);
	glutMouseFunc(onMouse);
	glutMotionFunc(onMotion);
	glutMouseWheelFunc(onMouseWheel);

	C3dglLogger::log("Vendor: {}", (const char *)glGetString(GL_VENDOR));
	C3dglLogger::log("Renderer: {}", (const char *)glGetString(GL_RENDERER));
	C3dglLogger::log("Version: {}", (const char*)glGetString(GL_VERSION));
	C3dglLogger::log("");

	// init light and everything – not a GLUT or callback function!
	if (!init())
	{
		C3dglLogger::log("Application failed to initialise\r\n");
		return 0;
	}

	// enter GLUT event processing cycle
	glutMainLoop();

	return 1;
}




void planarReflection(mat4& matrixView, float time, float deltaTime) {

	//program.sendUniform("planeClip", vec4(a, b, c, d));
	//program.sendUniform("matrixView", matrixView);



	//SAVE TO BUFFFER
		// Bind framebuffer for rendering reflection
	glBindFramebuffer(GL_FRAMEBUFFER, reflectionFBO);
	glViewport(0, 0, 1280, 720); // Use framebuffer size

	// Bind framebuffer for rendering reflection
	glBindFramebuffer(GL_FRAMEBUFFER, reflectionFBO);
	glViewport(0, 0, 1280, 720); // Use framebuffer size

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear the reflection texture

	mat4 reflectedView = matrixView * matrixReflection;

	// Enable clipping plane (to avoid rendering objects below the mirror plane)
	glEnable(GL_CLIP_PLANE0);

	renderScene(reflectedView, time, deltaTime); // render the scene objects


	glDisable(GL_CLIP_PLANE0);
	// Unbind framebuffer (switch back to default screen rendering)
	//glBindFramebuffer(GL_FRAMEBUFFER, 0);
//	glViewport(0, 0, 1280, 720);

}