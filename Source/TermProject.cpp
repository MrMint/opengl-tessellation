
#include <pez.h>
#include <vectormath_aos.h>
#include <glsw.h>
#include <glew.h>

#define MAX_PATH          260
using namespace Vectormath::Aos;

	const int VK_W = 0x57;
	const int VK_S = 0x53;
	const int VK_T =0x54;

enum { SlotPosition, SlotNormal };
static void CreateIcosahedron();
static void LoadEffect();

typedef struct {
    GLuint Projection;
    GLuint Modelview;
    GLuint NormalMatrix;
    GLuint LightPosition;
    GLuint AmbientMaterial;
    GLuint DiffuseMaterial;
    GLuint CameraPosition;
	GLuint Terrain;
	GLuint Normalizetess;
} ShaderUniforms;

static GLsizei IndexCount;
static const GLuint PositionSlot = 0;
static GLuint ProgramHandle;
static Matrix4 ProjectionMatrix;
static Matrix4 ModelviewMatrix;
static Matrix3 NormalMatrix;
static ShaderUniforms Uniforms;
static Vector3 CameraPosition;
Point3 eyePosition = Point3(0, 0, -100);
static Vector3 Theta = Vector3(0,0,0);
static int downX,downY = 0;
static bool doRotate = false;
GLuint Terrain;
static bool Normalizetess =false;

const char* PezInitialize(int width, int height)
{
	//Create Geometry
    CreateIcosahedron();

	//Load Shaders
    LoadEffect();

	//Bind Uniforms for shaders
    Uniforms.Projection = glGetUniformLocation(ProgramHandle, "Projection");
    Uniforms.Modelview = glGetUniformLocation(ProgramHandle, "Modelview");
    Uniforms.NormalMatrix = glGetUniformLocation(ProgramHandle, "NormalMatrix");
    Uniforms.LightPosition = glGetUniformLocation(ProgramHandle, "LightPosition");
    Uniforms.AmbientMaterial = glGetUniformLocation(ProgramHandle, "AmbientMaterial");
    Uniforms.DiffuseMaterial = glGetUniformLocation(ProgramHandle, "DiffuseMaterial");
    Uniforms.CameraPosition = glGetUniformLocation(ProgramHandle, "CameraPosition");
	Uniforms.Terrain = glGetUniformLocation(ProgramHandle, "Terrain");
	Uniforms.Normalizetess = glGetUniformLocation(ProgramHandle, "Normalizetess");
    
	// Set up the projection matrix:
    const float HalfWidth = (PEZ_VIEWPORT_WIDTH / PEZ_VIEWPORT_HEIGHT ) / 2.0f;
    const float HalfHeight = HalfWidth * PEZ_VIEWPORT_HEIGHT / PEZ_VIEWPORT_WIDTH;
    ProjectionMatrix = Matrix4::frustum(-HalfWidth, +HalfWidth, -HalfHeight, +HalfHeight, 5, 1500);

    // Initialize various state:
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

    return "Tessellation Demo";
}

void PezRender(GLuint fbo)
{
	//Update/Set the uniforms used by the shaders
	Vector4 lightPosition = Vector4(0.25, 0.25, 1, 0);
    
	glUniform3fv(Uniforms.LightPosition, 1, &lightPosition[0]);
	glUniformMatrix4fv(Uniforms.Projection, 1, 0, &ProjectionMatrix[0][0]);
    glUniformMatrix4fv(Uniforms.Modelview, 1, 0, &ModelviewMatrix[0][0]);
	glUniform3f(Uniforms.AmbientMaterial, 0.1f, 0.1f, 0.04f);
    glUniform3f(Uniforms.DiffuseMaterial, 0.1f, 0.2f, 1.0f);
	glUniform3fv(Uniforms.CameraPosition, 1,&CameraPosition[0]);
	
	if(Normalizetess)glUniform1i(Uniforms.Normalizetess, 1);
	else glUniform1i(Uniforms.Normalizetess, 0);
	Matrix3 nm = NormalMatrix;
	float packed[9] = { nm[0][0], nm[1][0], nm[2][0],
                        nm[0][1], nm[1][1], nm[2][1],
                        nm[0][2], nm[1][2], nm[2][2] };
	glUniformMatrix3fv(Uniforms.NormalMatrix, 1, 0, &packed[0]);

  
	// Render the scene:
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glPatchParameteri(GL_PATCH_VERTICES, 3);
    glDrawElements(GL_PATCHES, IndexCount, GL_UNSIGNED_INT, 0);

}

void PezUpdate(unsigned int elapsedMicroseconds)
{
	//Create the rotation matrix based on theta
    Matrix4 rotation = Matrix4::rotationX(Theta.getX());
	rotation *= Matrix4::rotationY(Theta.getY());
	rotation *= Matrix4::rotationZ(Theta.getZ());
	
	//Create the modelviewmatrix
	Point3 targetPosition = Point3(0, 0, 0);
    Vector3 upVector = Vector3(0, 1, 0);
    Matrix4 lookAt = Matrix4::lookAt(eyePosition,targetPosition, upVector);
	ModelviewMatrix = lookAt * rotation;
    NormalMatrix = ModelviewMatrix.getUpper3x3();
	
	//Set the camera position used by the shaders
	CameraPosition.setX(eyePosition.getX());
	CameraPosition.setY(eyePosition.getY());
	CameraPosition.setZ(eyePosition.getZ());
	
	//Handle Input
	if (PezIsPressing(VK_W)) eyePosition.setZ(eyePosition.getZ() + 1);
	if (PezIsPressing(VK_S)) eyePosition.setZ(eyePosition.getZ() - 1);
	if (PezIsPressing(VK_T)) Normalizetess = !Normalizetess;
}

void PezHandleMouse(int x, int y, int action)
{
	//Handle Mouse Input
	if(action == PEZ_DOWN)
	{
		downX = x;
		downY = y;
		doRotate = true;
	}
	if(doRotate)
	if(action == PEZ_MOVE)
	{
		Theta.setY(Theta.getY() + (x-downX)/1000.0);
		Theta.setX(Theta.getX() + (downY-y)/1000.0);
	}

	if(action == PEZ_UP)
	{
		doRotate = false;
	}
		
		
}

//Load Shaders
//Code based on tutorials
static void LoadEffect()
{
    GLint compileSuccess, linkSuccess;
    GLchar compilerSpew[256];

    glswInit();
    glswSetPath("", ".glsl");
    glswAddDirectiveToken("*", "#version 400");

    const char* vsSource = glswGetShader("TermProjectShader.Vertex");
    const char* tcsSource = glswGetShader("TermProjectShader.TessControl");
    const char* tesSource = glswGetShader("TermProjectShader.TessEval");
    const char* gsSource = glswGetShader("TermProjectShader.Geometry");
    const char* fsSource = glswGetShader("TermProjectShader.Fragment");
    const char* msg = "Can't find %s shader.  Does '../BicubicPath.glsl' exist?\n";
    PezCheckCondition(vsSource != 0, msg, "vertex");
    PezCheckCondition(tcsSource != 0, msg, "tess control");
    PezCheckCondition(tesSource != 0, msg, "tess eval");
    PezCheckCondition(gsSource != 0, msg, "geometry");
    PezCheckCondition(fsSource != 0, msg, "fragment");

    GLuint vsHandle = glCreateShader(GL_VERTEX_SHADER);
    GLuint tcsHandle = glCreateShader(GL_TESS_CONTROL_SHADER);
    GLuint tesHandle = glCreateShader(GL_TESS_EVALUATION_SHADER);
    GLuint gsHandle = glCreateShader(GL_GEOMETRY_SHADER);
    GLuint fsHandle = glCreateShader(GL_FRAGMENT_SHADER);

    glShaderSource(vsHandle, 1, &vsSource, 0);
    glCompileShader(vsHandle);
    glGetShaderiv(vsHandle, GL_COMPILE_STATUS, &compileSuccess);
    glGetShaderInfoLog(vsHandle, sizeof(compilerSpew), 0, compilerSpew);
    PezCheckCondition(compileSuccess, "Vertex Shader Errors:\n%s", compilerSpew);

    glShaderSource(tcsHandle, 1, &tcsSource, 0);
    glCompileShader(tcsHandle);
    glGetShaderiv(tcsHandle, GL_COMPILE_STATUS, &compileSuccess);
    glGetShaderInfoLog(tcsHandle, sizeof(compilerSpew), 0, compilerSpew);
    PezCheckCondition(compileSuccess, "Tess Control Shader Errors:\n%s", compilerSpew);

    glShaderSource(tesHandle, 1, &tesSource, 0);
    glCompileShader(tesHandle);
    glGetShaderiv(tesHandle, GL_COMPILE_STATUS, &compileSuccess);
    glGetShaderInfoLog(tesHandle, sizeof(compilerSpew), 0, compilerSpew);
    PezCheckCondition(compileSuccess, "Tess Eval Shader Errors:\n%s", compilerSpew);

    glShaderSource(gsHandle, 1, &gsSource, 0);
    glCompileShader(gsHandle);
    glGetShaderiv(gsHandle, GL_COMPILE_STATUS, &compileSuccess);
    glGetShaderInfoLog(gsHandle, sizeof(compilerSpew), 0, compilerSpew);
    PezCheckCondition(compileSuccess, "Geometry Shader Errors:\n%s", compilerSpew);

    glShaderSource(fsHandle, 1, &fsSource, 0);
    glCompileShader(fsHandle);
    glGetShaderiv(fsHandle, GL_COMPILE_STATUS, &compileSuccess);
    glGetShaderInfoLog(fsHandle, sizeof(compilerSpew), 0, compilerSpew);
    PezCheckCondition(compileSuccess, "Fragment Shader Errors:\n%s", compilerSpew);

    ProgramHandle = glCreateProgram();
    glAttachShader(ProgramHandle, vsHandle);
    glAttachShader(ProgramHandle, tcsHandle);
    glAttachShader(ProgramHandle, tesHandle);
    glAttachShader(ProgramHandle, gsHandle);
    glAttachShader(ProgramHandle, fsHandle);
    glBindAttribLocation(ProgramHandle, PositionSlot, "Position");
    glLinkProgram(ProgramHandle);
    glGetProgramiv(ProgramHandle, GL_LINK_STATUS, &linkSuccess);
    glGetProgramInfoLog(ProgramHandle, sizeof(compilerSpew), 0, compilerSpew);
    PezCheckCondition(linkSuccess, "Shader Link Errors:\n%s", compilerSpew);

    glUseProgram(ProgramHandle);
}


//Create Geometry
static void CreateIcosahedron()
{
    const int Faces[] = {
        2, 1, 0,
        3, 2, 0,
        4, 3, 0,
        5, 4, 0,
        1, 5, 0,

        11, 6,  7,
        11, 7,  8,
        11, 8,  9,
        11, 9,  10,
        11, 10, 6,

        1, 2, 6,
        2, 3, 7,
        3, 4, 8,
        4, 5, 9,
        5, 1, 10,

        2,  7, 6,
        3,  8, 7,
        4,  9, 8,
        5, 10, 9,
        1, 6, 10 };

    const float Verts[] = {
         0.000f,  0.000f,  1.000f,
         0.894f,  0.000f,  0.447f,
         0.276f,  0.851f,  0.447f,
        -0.724f,  0.526f,  0.447f,
        -0.724f, -0.526f,  0.447f,
         0.276f, -0.851f,  0.447f,
         0.724f,  0.526f, -0.447f,
        -0.276f,  0.851f, -0.447f,
        -0.894f,  0.000f, -0.447f,
        -0.276f, -0.851f, -0.447f,
         0.724f, -0.526f, -0.447f,
         0.000f,  0.000f, -1.000f };

    IndexCount = sizeof(Faces) / sizeof(Faces[0]);

    // Create the VAO:
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // Create the VBO for positions:
    GLuint positions;
    GLsizei stride = 3 * sizeof(float);
    glGenBuffers(1, &positions);
    glBindBuffer(GL_ARRAY_BUFFER, positions);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Verts), Verts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(PositionSlot);
    glVertexAttribPointer(PositionSlot, 3, GL_FLOAT, GL_FALSE, stride, 0);

    // Create the VBO for indices:
    GLuint indices;
    glGenBuffers(1, &indices);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Faces), Faces, GL_STATIC_DRAW);


}
