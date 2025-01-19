//main.cpp
#define _CRT_SECURE_NO_WARNINGS 
#define STB_IMAGE_IMPLEMENTATION
#pragma comment(lib, "glew32.lib")
#pragma comment(lib, "freeglut.lib")

#include <Windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <gl/glew.h>
#include <gl/freeglut.h>
#include <gl/freeglut_ext.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <random>
#include <string>
#include "open_obj.h"
#include "open_file.h"
#include "stb_image.h"

using namespace std;

//---------------------------------------
// 게임 진행 관련 전역 변수
//---------------------------------------
// 체크포인트 위치 배열 (플레이어가 도달해야 하는 지점들)
float checkpointPositions[4] = { 0.0f, 100.0f, 130.0f, 160.0f };

// 난수 생성용 (on-off 플랫폼 시간 제어)
random_device rd;
mt19937 gen(rd());
uniform_int_distribution<int> onoffRandomTime(0, 400);

//---------------------------------------
// 텍스처 관련 변수
//---------------------------------------
void InitTexture();
int widthImage, heightImage, numberOfChannel;
unsigned int textures[6];

//---------------------------------------
// Transform 구조체 : 오브젝트의 위치, 회전, 스케일을 처리
//---------------------------------------
struct Transform
{
    glm::vec3 position = glm::vec3(0.0f); // 위치
    glm::vec3 rotation = glm::vec3(0.0f); // 회전
    glm::vec3 scale = glm::vec3(1.0f);    // 스케일

    glm::mat4 GetTransform()
    {
        glm::mat4 T = glm::translate(glm::mat4(1.0f), position);
        glm::mat4 S = glm::scale(glm::mat4(1.0f), scale);
        glm::mat4 RX = glm::rotate(glm::mat4(1.0f), glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        glm::mat4 RY = glm::rotate(glm::mat4(1.0f), glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 RZ = glm::rotate(glm::mat4(1.0f), glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
        return T * RX * RY * RZ * S;
    }
};

//---------------------------------------
// OBJECT 구조체 : 기본 오브젝트 구조
//---------------------------------------
struct OBJECT {
    GLuint vao = 0, vbo[4] = { 0 };
    Transform worldmatrix;
    Transform modelmatrix;
    OBJECT* parent{ nullptr };

    glm::vec3* vertexdata = nullptr;
    glm::vec3* normaldata = nullptr;
    glm::vec4* colordata = nullptr;
    glm::vec3* texturedata = nullptr;

    int vertex_count = 0;

    // OBJ 파일 읽어들이기 (open_obj.h 사용)
    void ReadObj(string fileName)
    {
        Model model;
        read_obj_file(fileName.c_str(), &model);

        vertex_count = (int)(model.face_count * 3);

        vertexdata = new glm::vec3[vertex_count];
        normaldata = new glm::vec3[vertex_count];
        colordata = new glm::vec4[vertex_count];
        texturedata = new glm::vec3[vertex_count];

        // faces 정보로부터 정점 데이터 추출
        for (int i = 0; i < (int)model.face_count; i++)
        {
            unsigned int v1 = model.faces[i].v1;
            unsigned int v2 = model.faces[i].v2;
            unsigned int v3 = model.faces[i].v3;

            vertexdata[i * 3 + 0] = glm::vec3(model.vertices[v1].x, model.vertices[v1].y, model.vertices[v1].z);
            vertexdata[i * 3 + 1] = glm::vec3(model.vertices[v2].x, model.vertices[v2].y, model.vertices[v2].z);
            vertexdata[i * 3 + 2] = glm::vec3(model.vertices[v3].x, model.vertices[v3].y, model.vertices[v3].z);
        }

        // 노멀 계산
        for (int i = 0; i < (int)model.face_count; i++)
        {
            glm::vec3 normal = glm::cross(vertexdata[i * 3 + 1] - vertexdata[i * 3 + 0],
                vertexdata[i * 3 + 2] - vertexdata[i * 3 + 0]);
            normaldata[i * 3 + 0] = normal;
            normaldata[i * 3 + 1] = normal;
            normaldata[i * 3 + 2] = normal;
        }

        // 컬러 및 텍스처 기본값 설정
        for (int i = 0; i < vertex_count; i++)
        {
            colordata[i] = glm::vec4(1.0f, 1.0f, 1.0f, 0.5f);
            texturedata[i] = glm::vec3(0.0f);
        }

        free(model.vertices);
        free(model.faces);
    }

    glm::mat4 GetTransform()
    {
        if (parent)
            return parent->GetTransform() * worldmatrix.GetTransform();
        return worldmatrix.GetTransform();
    }

    glm::mat4 GetmodelTransform()
    {
        return modelmatrix.GetTransform();
    }
};

//---------------------------------------
// CUBE 구조체 : 큐브 오브젝트
//---------------------------------------
struct CUBE :OBJECT
{
    double width = 0.25, depth = 0.25, height = 0.25;

    void Init()
    {
        for (int i = 0; i < vertex_count; i++)
        {
            vertexdata[i] -= glm::vec3(0.5f, 0.5f, 0.5f);
        }
        for (int i = 0; i < 6; i++)
        {
            texturedata[i * 6 + 0] = glm::vec3(0.0f, 0.0f, 0.0f);
            texturedata[i * 6 + 1] = glm::vec3(1.0f, 0.0f, 0.0f);
            texturedata[i * 6 + 2] = glm::vec3(1.0f, 1.0f, 0.0f);
            texturedata[i * 6 + 3] = glm::vec3(0.0f, 0.0f, 0.0f);
            texturedata[i * 6 + 4] = glm::vec3(1.0f, 1.0f, 0.0f);
            texturedata[i * 6 + 5] = glm::vec3(0.0f, 1.0f, 0.0f);
        }

        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        glGenBuffers(4, vbo);

        glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
        glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(glm::vec3), vertexdata, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
        glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(glm::vec4), colordata, GL_STATIC_DRAW);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(1);

        glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
        glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(glm::vec3), normaldata, GL_STATIC_DRAW);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(2);

        glBindBuffer(GL_ARRAY_BUFFER, vbo[3]);
        glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(glm::vec3), texturedata, GL_STATIC_DRAW);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(3);
    }

    void draw(int shaderID, int textureIndex)
    {
        unsigned int modelLocation = glGetUniformLocation(shaderID, "model");
        glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(GetTransform() * GetmodelTransform()));
        glBindVertexArray(vao);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textures[textureIndex]);
        glDrawArrays(GL_TRIANGLES, 0, vertex_count);
    }
};

//---------------------------------------
// SPHERE 구조체 : 구체 오브젝트
//---------------------------------------
struct SPHERE :OBJECT
{
    void Init()
    {
        for (int i = 0; i < vertex_count; i++)
        {
            colordata[i].x = 1.0f;
            colordata[i].y = 1.0f;
            colordata[i].z = 1.0f;
            colordata[i].a = 0.5f;
        }
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        glGenBuffers(3, vbo);

        glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
        glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(glm::vec3), vertexdata, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
        glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(glm::vec4), colordata, GL_STATIC_DRAW);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(1);

        glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
        glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(glm::vec3), normaldata, GL_STATIC_DRAW);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(2);
    }

    void draw(int shaderID)
    {
        unsigned int modelLocation = glGetUniformLocation(shaderID, "model");
        glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(GetTransform() * GetmodelTransform()));
        glBindVertexArray(vao);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textures[2]);
        glDrawArrays(GL_TRIANGLES, 0, vertex_count);
    }
};

CUBE cube;
CUBE skybox;
CUBE minicube;
CUBE checkpoint[4];
CUBE rotatePlane[5];

CUBE onOffPlatforms[9];
int onOffPlatformsCount = 9;
int onOffPlatformGlobalTime = 0;

CUBE randomPlatforms[9];
int randomPlatformsCount = 9;
int randomPlatformsTime[9];

SPHERE sphere;

GLfloat XYZcolors[6][3] = {
    { 1.0, 0.0, 0.0 }, { 1.0, 0.0, 0.0 },
    { 0.0, 1.0, 0.0 }, { 0.0, 1.0, 0.0 },
    { 0.0, 0.0, 1.0 }, { 0.0, 0.0, 1.0 }
};

//---------------------------------------
// 카메라 및 게임 진행 관련 전역 변수 (질문한 변수명들 변경)
//---------------------------------------
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, -4.0f);
glm::vec3 cameraDirection = glm::vec3(0.0f, 0.0f, 200.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

// 기본 행렬
glm::mat4 model = glm::mat4(1.0f);
glm::mat4 view = glm::mat4(1.0f);
glm::mat4 projection = glm::mat4(1.0f);

GLuint vao, vbo[3];

GLchar* vertexSource, * fragmentSource;
GLuint vertexShader, fragmentShader;
GLuint shaderProgramID;

int windowWidth = 800;
int windowHeight = 600;

float g_fXAngle = 0.0f;
float g_fYAngle = 0.0f;
float g_fZAngle = 0.0f;
float g_fPrevXAngle = 0.0f;
float g_fPrevYAngle = 0.0f;
float g_fWheelScale = 0.15f;
float g_fOriginX = 0.0f;
float g_fOriginY = 0.0f;
bool g_bLeftButton = false;
float g_fFovY = 45.0f;
float g_fNearClip = 0.1f;
float g_fFarClip = 200.0f;
float g_fPerspectiveZ = -2.0f;

float g_fCameraDistance = 15.0f;
float g_fCameraHeight = 3.0f;
float g_fCameraAngle = 180.0f;

bool g_bStart = false;
bool g_bEndPoint = false;
bool g_bAdminMode = false;
bool g_bViewpoint = false;
bool g_bCurrentViewpoint = false;

int g_nJumpSelection = 0;
float g_fJumpSize = 0.1f;
const float g_fGravity = 0.01f;
const float g_fInitialHeight = 0.5f;
const float g_fJumpInitialVelocity = 0.3f;
float g_fJumpVelocity = g_fJumpInitialVelocity;
bool g_bUpKeyPressed = false;
bool g_bDownKeyPressed = false;
bool g_bLeftKeyPressed = false;
bool g_bRightKeyPressed = false;
bool g_bFalling = false;
int g_nCheckNum = 0;
float g_fSpeed = 0.3f;
float g_fFall = 0.0f;
float g_fAngles[5] = {};
glm::vec3 g_v3Destination;
bool g_bEndpointMessageShown = false;

bool g_bViewpointTransitioning = false;
float g_fViewpointTransitionTime = 0.0f;
float g_fViewpointTransitionDuration = 1.0f;
float g_fStartCameraPosYAdd = 0.0f;
float g_fStartCameraPosZAdd = 0.0f;
float g_fEndCameraPosYAdd = 0.0f;
float g_fEndCameraPosZAdd = 0.0f;

// 함수 선언
void make_shaderProgram();
void make_vertexShaders();
void make_fragmentShaders();
GLvoid drawScene();
GLvoid Reshape(int w, int h);
GLvoid Keyboard(unsigned char key, int x, int y);
void InitBuffer();
GLvoid Motion(int x, int y);
GLvoid TimerFunction(int value);
GLvoid SpecialKeys(int key, int x, int y);
GLvoid mouseWheel(int button, int dir, int x, int y);
GLvoid SpecialKeysUp(int key, int x, int y);

// 구체 이동 함수
void moveSphere()
{
    // 구체의 이동에 따라 회전 방향이 결정됨
    // 앞으로 이동 시: sphere.modelmatrix.rotation.x += speed * 50.0f
    if (g_bUpKeyPressed)
    {
        sphere.worldmatrix.position.z += g_fSpeed;
        sphere.modelmatrix.rotation.x += g_fSpeed * 50.0f;
        cameraDirection.z += g_fSpeed;
    }
    // 뒤로 이동 시: sphere.modelmatrix.rotation.x -= speed * 50.0f
    if (g_bDownKeyPressed)
    {
        sphere.worldmatrix.position.z -= g_fSpeed;
        sphere.modelmatrix.rotation.x -= g_fSpeed * 50.0f;
        cameraDirection.z -= g_fSpeed;
    }
    // 왼쪽 이동 시: sphere.modelmatrix.rotation.z += speed * 50.0f (반대방향)
    if (g_bLeftKeyPressed)
    {
        sphere.worldmatrix.position.x += g_fSpeed;
        sphere.modelmatrix.rotation.z -= g_fSpeed * 50.0f;
    }
    // 오른쪽 이동 시: sphere.modelmatrix.rotation.z -= speed * 50.0f
    if (g_bRightKeyPressed)
    {
        sphere.worldmatrix.position.x -= g_fSpeed;
        sphere.modelmatrix.rotation.z += g_fSpeed * 50.0f;
    }
}

int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowPosition(100, 100);
    glutInitWindowSize(windowWidth, windowHeight);
    glutCreateWindow("Example1");

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK)
    {
        std::cerr << "GLEW 초기화 실패" << std::endl;
        exit(EXIT_FAILURE);
    }
    else
    {
        std::cout << "GLEW Initialized\n";
    }

    cube.ReadObj("cube.obj");
    skybox.ReadObj("cube.obj");
    minicube.ReadObj("cube.obj");
    sphere.ReadObj("sphere.obj");

    for (int i = 0; i < 4; i++)
    {
        checkpoint[i].ReadObj("cube.obj");
    }
    for (int i = 0; i < 5; i++)
    {
        rotatePlane[i].ReadObj("cube.obj");
    }
    for (int i = 0; i < onOffPlatformsCount; i++)
    {
        onOffPlatforms[i].ReadObj("cube.obj");
    }
    for (int i = 0; i < randomPlatformsCount; i++)
    {
        randomPlatforms[i].ReadObj("cube.obj");
    }

    make_shaderProgram();
    InitBuffer();
    InitTexture();
    glEnable(GL_DEPTH_TEST);

    glutTimerFunc(10, TimerFunction, 1);
    glutDisplayFunc(drawScene);
    glutReshapeFunc(Reshape);
    glutKeyboardFunc(Keyboard);
    glutSpecialFunc(SpecialKeys);
    glutSpecialUpFunc(SpecialKeysUp);
    glutMotionFunc(Motion);
    glutMouseWheelFunc(mouseWheel);

    glutMainLoop();
    return 0;
}

GLvoid drawScene()
{
    glUseProgram(shaderProgramID);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
    glBufferData(GL_ARRAY_BUFFER, 9 * sizeof(GLfloat), XYZcolors, GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(1);

    int viewLocation = glGetUniformLocation(shaderProgramID, "view");
    view = glm::lookAt(cameraPos, cameraDirection, cameraUp);
    glUniformMatrix4fv(viewLocation, 1, GL_FALSE, &view[0][0]);

    glm::mat4 perspect = glm::mat4(1.0f);
    perspect = glm::perspective(glm::radians(g_fFovY), (float)windowWidth / (float)windowHeight, g_fNearClip, g_fFarClip);
    perspect = glm::translate(perspect, glm::vec3(0.0f, 0.0f, g_fPerspectiveZ));
    unsigned int projectionLocation = glGetUniformLocation(shaderProgramID, "projection");
    glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, glm::value_ptr(perspect));

    glm::mat4 lightmatrix = minicube.GetTransform();
    glm::vec3 lightposition = glm::vec3(lightmatrix[3]);

    unsigned int lightPosLocation = glGetUniformLocation(shaderProgramID, "lightPos");
    glUniform3f(lightPosLocation, lightposition.x, lightposition.y, lightposition.z);
    unsigned int lightColorLocation = glGetUniformLocation(shaderProgramID, "lightColor");
    glUniform3f(lightColorLocation, 1.0f, 1.0f, 1.0f);
    unsigned int objColorLocation = glGetUniformLocation(shaderProgramID, "objectColor");
    glUniform3f(objColorLocation, 1.0f, 0.5f, 0.3f);

    model = glm::mat4(1.0f);

    // currentViewpoint가 false일 때 구체 렌더링 (3인칭 시점일 때만 구체가 보이도록)
    if (!g_bCurrentViewpoint)
    {
        sphere.draw(shaderProgramID);
    }

    skybox.draw(shaderProgramID, 1);

    for (int i = 0; i < 4; i++)
    {
        checkpoint[i].draw(shaderProgramID, 2);
    }

    for (int i = 0; i < 5; i++)
    {
        rotatePlane[i].draw(shaderProgramID, 3);
    }

    for (int i = 0; i < onOffPlatformsCount; i++)
    {
        if (onOffPlatforms[i].worldmatrix.scale.x <= 0.0f || onOffPlatforms[i].worldmatrix.scale.z <= 0.0f)
            continue;
        onOffPlatforms[i].draw(shaderProgramID, 4);
    }

    for (int i = 0; i < randomPlatformsCount; i++)
    {
        if (randomPlatforms[i].worldmatrix.scale.x <= 0.0f || randomPlatforms[i].worldmatrix.scale.z <= 0.0f)
            continue;
        randomPlatforms[i].draw(shaderProgramID, 5);
    }

    glutSwapBuffers();
}

GLvoid Reshape(int w, int h)
{
    glViewport(0, 0, w, h);
}

void InitBuffer()
{
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(2, vbo);

    cube.Init();
    minicube.Init();
    sphere.Init();
    skybox.Init();

    for (int i = 0; i < 4; i++)
    {
        checkpoint[i].Init();
        checkpoint[i].worldmatrix.position.y -= 0.5f;
        checkpoint[i].worldmatrix.position.z = checkpointPositions[i];
        checkpoint[i].worldmatrix.scale = glm::vec3(7.0f, 0.3f, 7.0f);
        checkpoint[i].width = 7.0f / 2.0f;
        checkpoint[i].depth = 0.3f / 2.0f;
        checkpoint[i].height = 7.0f / 2.0f;
        checkpoint[i].worldmatrix.position.x = 0.0f;
    }

    for (int i = 0; i < 5; i++)
    {
        rotatePlane[i].Init();
        rotatePlane[i].worldmatrix.position.y -= 0.5f;
        rotatePlane[i].worldmatrix.scale = glm::vec3(10.0f, 0.3f, 10.0f);
        rotatePlane[i].width = 10.0f / 2.0f;
        rotatePlane[i].depth = 0.3f / 2.0f;
        rotatePlane[i].height = 10.0f / 2.0f;
        rotatePlane[i].worldmatrix.position.x = 0.0f;
    }

    rotatePlane[0].worldmatrix.position.z = 15.0f;
    rotatePlane[1].worldmatrix.position.z = 35.0f;
    rotatePlane[2].worldmatrix.position.z = 55.0f;
    rotatePlane[3].worldmatrix.position.z = 75.0f;
    rotatePlane[4].worldmatrix.position.z = 90.0f;
    rotatePlane[4].worldmatrix.scale = glm::vec3(6.0f, 0.3f, 6.0f);
    rotatePlane[4].width = 6.0f / 2.0f;
    rotatePlane[4].depth = 0.3f / 2.0f;
    rotatePlane[4].height = 6.0f / 2.0f;

    int cols = 3;
    int rows = 3;
    float spacing = 6.0f;

    for (int i = 0; i < onOffPlatformsCount; i++)
    {
        onOffPlatforms[i].Init();
        onOffPlatforms[i].worldmatrix.position.y -= 0.5f;

        int row = i % rows;
        int col = i / rows;

        onOffPlatforms[i].worldmatrix.position.x = (col - 1) * spacing;
        float baseZ = checkpointPositions[1] + spacing;
        onOffPlatforms[i].worldmatrix.position.z = baseZ + row * spacing;
        onOffPlatforms[i].worldmatrix.scale = glm::vec3(0.0f, 0.3f, 0.0f);
        onOffPlatforms[i].width = 2.5f;
        onOffPlatforms[i].depth = 0.15f;
        onOffPlatforms[i].height = 2.5f;
    }

    for (int i = 0; i < randomPlatformsCount; i++)
    {
        randomPlatforms[i].Init();
        randomPlatforms[i].worldmatrix.position.y -= 0.5f;

        int row = i % rows;
        int col = i / rows;

        randomPlatforms[i].worldmatrix.position.x = (col - 1) * spacing;
        float baseZ = checkpointPositions[2] + spacing;
        randomPlatforms[i].worldmatrix.position.z = baseZ + row * spacing;
        randomPlatforms[i].worldmatrix.scale = glm::vec3(0.0f, 0.3f, 0.0f);
        randomPlatforms[i].width = 2.5f;
        randomPlatforms[i].depth = 0.15f;
        randomPlatforms[i].height = 2.5f;
        randomPlatformsTime[i] = onoffRandomTime(gen);
    }

    sphere.worldmatrix.scale = glm::vec3(0.5f, 0.5f, 0.5f);

    skybox.worldmatrix.scale = glm::vec3(200.0f, 200.0f, 200.0f);

    cube.worldmatrix.position.z = 10.0f;

    minicube.worldmatrix.position.z = -7.0f;
    minicube.modelmatrix.scale = glm::vec3(0.35f, 0.35f, 0.35f);
}

void make_shaderProgram()
{
    make_vertexShaders();
    make_fragmentShaders();
    shaderProgramID = glCreateProgram();
    glAttachShader(shaderProgramID, vertexShader);
    glAttachShader(shaderProgramID, fragmentShader);
    glLinkProgram(shaderProgramID);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    glUseProgram(shaderProgramID);
}

void make_vertexShaders()
{
    vertexSource = open_file_to_buf("vertex.glsl");
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, (const GLchar**)&vertexSource, 0);
    glCompileShader(vertexShader);

    GLint result;
    GLchar errorLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &result);
    if (!result)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, errorLog);
        std::cout << "ERROR: vertex shader 컴파일 실패\n" << errorLog << std::endl;
        return;
    }
    free(vertexSource);
}

void make_fragmentShaders()
{
    fragmentSource = open_file_to_buf("fragment.glsl");
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, (const GLchar**)&fragmentSource, 0);
    glCompileShader(fragmentShader);

    GLint result;
    GLchar errorLog[512];
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &result);
    if (!result)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, errorLog);
        std::cout << "ERROR: fragment shader 컴파일 실패\n" << errorLog << std::endl;
        return;
    }
    free(fragmentSource);
}

GLvoid Keyboard(unsigned char key, int x, int y)
{
    switch (key) {
    case 'a':
        g_bAdminMode = !g_bAdminMode;
        break;
    case 'b': // B키 눌렀을 때 어드민 모드 토글
        g_bAdminMode = !g_bAdminMode;
        break;
    case 32: // 스페이스바로 점프
        if (g_nJumpSelection == 0)
        {
            g_nJumpSelection = 1;
        }
        break;
    case 'v':
        if (!g_bViewpointTransitioning) {
            bool oldViewpoint = g_bViewpoint;
            g_bViewpoint = !g_bViewpoint;

            g_bViewpointTransitioning = true;
            g_fViewpointTransitionTime = 0.0f;

            float prevYAdd = (oldViewpoint ? -3.1f : 0.0f);
            float prevZAdd = (oldViewpoint ? 17.0f : 0.0f);

            float targetYAdd = (g_bViewpoint ? -3.1f : 0.0f);
            float targetZAdd = (g_bViewpoint ? 17.0f : 0.0f);

            g_fStartCameraPosYAdd = prevYAdd;
            g_fStartCameraPosZAdd = prevZAdd;
            g_fEndCameraPosYAdd = targetYAdd;
            g_fEndCameraPosZAdd = targetZAdd;
        }
        break;
    case 'q':
        glutLeaveMainLoop();
        break;
    case 27: // ESC
        glutLeaveMainLoop();
        break;
    }
    glutPostRedisplay();
}

GLvoid SpecialKeys(int key, int x, int y)
{
    switch (key) {
    case GLUT_KEY_UP:
        g_bUpKeyPressed = true;
        break;
    case GLUT_KEY_DOWN:
        g_bDownKeyPressed = true;
        break;
    case GLUT_KEY_LEFT:
        g_bLeftKeyPressed = true;
        break;
    case GLUT_KEY_RIGHT:
        g_bRightKeyPressed = true;
        break;
    }
    glutPostRedisplay();
}

GLvoid SpecialKeysUp(int key, int x, int y) {
    switch (key) {
    case GLUT_KEY_UP:
        g_bUpKeyPressed = false;
        break;
    case GLUT_KEY_DOWN:
        g_bDownKeyPressed = false;
        break;
    case GLUT_KEY_LEFT:
        g_bLeftKeyPressed = false;
        break;
    case GLUT_KEY_RIGHT:
        g_bRightKeyPressed = false;
        break;
    }
}

GLvoid Motion(int x, int y)
{
    if (g_bLeftButton)
    {
        g_fYAngle = (float)x - g_fOriginX;
        g_fXAngle = (float)y - g_fOriginY;
        g_fXAngle += g_fPrevXAngle;
        g_fYAngle += g_fPrevYAngle;

        g_fYAngle /= 2.0f;
        g_fXAngle /= 2.0f;
    }
    glutPostRedisplay();
}

GLvoid mouseWheel(int button, int dir, int x, int y)
{
    if (dir > 0)
    {
        g_fWheelScale += (float)dir * 0.1f;
    }
    else if (dir < 0)
    {
        g_fWheelScale += (float)dir * 0.1f;
        if (g_fWheelScale < 0.1f)
        {
            g_fWheelScale = 0.1f;
        }
    }
    glutPostRedisplay();
}

GLvoid TimerFunction(int value)
{
    switch (value)
    {
    case 1:
        if (!g_bStart && !g_bEndPoint)
        {
            sphere.worldmatrix.scale = glm::vec3(1.0f, 1.0f, 1.0f);
            moveSphere();


            if (g_nJumpSelection == 1)
            {
                sphere.worldmatrix.position.y += g_fJumpVelocity;
                g_fJumpVelocity -= g_fGravity;
            }

            float currentYAdd;
            float currentZAdd;
            if (g_bViewpointTransitioning) {
                g_fViewpointTransitionTime += 0.01f;
                float t = g_fViewpointTransitionTime / g_fViewpointTransitionDuration;
                if (t > 1.0f) {
                    t = 1.0f;
                    g_bViewpointTransitioning = false;
                    g_bCurrentViewpoint = g_bViewpoint;
                }

                currentYAdd = (1.0f - t) * g_fStartCameraPosYAdd + t * g_fEndCameraPosYAdd;
                currentZAdd = (1.0f - t) * g_fStartCameraPosZAdd + t * g_fEndCameraPosZAdd;

                if (g_bViewpoint == false) {
                    g_bCurrentViewpoint = false;
                }
            }
            else {
                currentYAdd = (g_bViewpoint ? -3.1f : 0.0f);
                currentZAdd = (g_bViewpoint ? 17.0f : 0.0f);
                g_bCurrentViewpoint = g_bViewpoint;
            }

            cameraPos.x = sphere.worldmatrix.position.x + g_fCameraDistance * sin(glm::radians(g_fCameraAngle));
            cameraPos.y = sphere.worldmatrix.position.y + g_fCameraHeight + currentYAdd;
            cameraPos.z = sphere.worldmatrix.position.z + g_fCameraDistance * cos(glm::radians(g_fCameraAngle)) + currentZAdd;

            if (sphere.worldmatrix.position.y <= g_fInitialHeight) {
                sphere.worldmatrix.position.y = g_fInitialHeight;
                g_fJumpVelocity = g_fJumpInitialVelocity;
                g_nJumpSelection = 0;
            }

            for (int i = 0; i < 5; i++)
            {
                if (i % 2 == 0)
                {
                    rotatePlane[i].worldmatrix.rotation.y += 1.0f;
                    if (!g_bUpKeyPressed && !g_bDownKeyPressed && !g_bRightKeyPressed && !g_bLeftKeyPressed) {
                        g_fAngles[i] += 1.0f;
                    }
                    else {
                        g_fAngles[i] = 0.0f;
                    }
                }
                else
                {
                    rotatePlane[i].worldmatrix.rotation.y -= 1.0f;
                    if (!g_bUpKeyPressed && !g_bDownKeyPressed && !g_bRightKeyPressed && !g_bLeftKeyPressed) {
                        g_fAngles[i] -= 1.0f;
                    }
                    else {
                        g_fAngles[i] = 0.0f;
                    }
                }
            }

            onOffPlatformGlobalTime++;
            {
                int cycle = 1000;
                int t = onOffPlatformGlobalTime % cycle;
                float scaleValue;
                if (t <= 500)
                {
                    scaleValue = (t / 500.0f) * 5.0f;
                }
                else
                {
                    scaleValue = ((1000.0f - t) / 500.0f) * 5.0f;
                }

                for (int i = 0; i < onOffPlatformsCount; i++)
                {
                    onOffPlatforms[i].worldmatrix.scale = glm::vec3(scaleValue, 0.3f, scaleValue);
                }
            }

            for (int i = 0; i < randomPlatformsCount; i++)
            {
                randomPlatformsTime[i]++;
                if (randomPlatformsTime[i] > 0 && randomPlatformsTime[i] < 200)
                {
                    randomPlatforms[i].worldmatrix.scale = glm::vec3(5.0f, 0.3f, 5.0f);
                }
                else if (randomPlatformsTime[i] >= 200 && randomPlatformsTime[i] < 400)
                {
                    randomPlatforms[i].worldmatrix.scale = glm::vec3(0.0f, 0.0f, 0.0f);
                }
                else if (randomPlatformsTime[i] >= 400)
                {
                    randomPlatformsTime[i] = 0;
                }
            }

            g_bFalling = true;

            // 체크포인트 충돌 판정
            for (int i = 0; i < 4; i++)
            {
                if (((sphere.worldmatrix.position.x > (checkpoint[i].worldmatrix.position.x - checkpoint[i].width)) &&
                    (sphere.worldmatrix.position.x < (checkpoint[i].worldmatrix.position.x + checkpoint[i].width)) &&
                    (sphere.worldmatrix.position.z > (checkpoint[i].worldmatrix.position.z - checkpoint[i].height)) &&
                    (sphere.worldmatrix.position.z < (checkpoint[i].worldmatrix.position.z + checkpoint[i].height)))
                    || g_nJumpSelection == 1)
                {
                    g_bFalling = false;
                    if (i == 3)
                    {
                        g_bEndPoint = true;
                    }
                    break;
                }
            }

            // 회전 플랫폼 충돌 판정
            for (int i = 0; i < 5; i++)
            {
                if (((sphere.worldmatrix.position.x > (rotatePlane[i].worldmatrix.position.x - rotatePlane[i].width)) &&
                    (sphere.worldmatrix.position.x < (rotatePlane[i].worldmatrix.position.x + rotatePlane[i].width)) &&
                    (sphere.worldmatrix.position.z > (rotatePlane[i].worldmatrix.position.z - rotatePlane[i].height)) &&
                    (sphere.worldmatrix.position.z < (rotatePlane[i].worldmatrix.position.z + rotatePlane[i].height))))
                {
                    g_bFalling = false;
                    break;
                }
            }

            // On-Off 플랫폼 충돌 판정
            for (int i = 0; i < onOffPlatformsCount; i++)
            {
                float effectiveWidth = onOffPlatforms[i].width * (onOffPlatforms[i].worldmatrix.scale.x / 5.0f);
                float effectiveHeight = onOffPlatforms[i].height * (onOffPlatforms[i].worldmatrix.scale.z / 5.0f);
                if (effectiveWidth < 0.1f || effectiveHeight < 0.1f)
                    continue;

                if (((sphere.worldmatrix.position.x > (onOffPlatforms[i].worldmatrix.position.x - effectiveWidth)) &&
                    (sphere.worldmatrix.position.x < (onOffPlatforms[i].worldmatrix.position.x + effectiveWidth)) &&
                    (sphere.worldmatrix.position.z > (onOffPlatforms[i].worldmatrix.position.z - effectiveHeight)) &&
                    (sphere.worldmatrix.position.z < (onOffPlatforms[i].worldmatrix.position.z + effectiveHeight)))
                    || g_nJumpSelection == 1)
                {
                    g_bFalling = false;
                    break;
                }
            }

            // 랜덤 플랫폼 충돌 판정
            for (int i = 0; i < randomPlatformsCount; i++)
            {
                if (randomPlatforms[i].worldmatrix.scale.x <= 0.1f || randomPlatforms[i].worldmatrix.scale.z <= 0.1f)
                    continue;

                float effectiveWidth = randomPlatforms[i].width * (randomPlatforms[i].worldmatrix.scale.x / 5.0f);
                float effectiveHeight = randomPlatforms[i].height * (randomPlatforms[i].worldmatrix.scale.z / 5.0f);

                if (((sphere.worldmatrix.position.x > (randomPlatforms[i].worldmatrix.position.x - effectiveWidth)) &&
                    (sphere.worldmatrix.position.x < (randomPlatforms[i].worldmatrix.position.x + effectiveWidth)) &&
                    (sphere.worldmatrix.position.z > (randomPlatforms[i].worldmatrix.position.z - effectiveHeight)) &&
                    (sphere.worldmatrix.position.z < (randomPlatforms[i].worldmatrix.position.z + effectiveHeight)))
                    || g_nJumpSelection == 1)
                {
                    g_bFalling = false;
                    break;
                }
            }

            g_nCheckNum = 0;
            for (int i = 0; i < 3; i++)
            {
                if (sphere.worldmatrix.position.z >= checkpointPositions[i] && sphere.worldmatrix.position.z < checkpointPositions[i + 1])
                {
                    g_nCheckNum = i;
                    break;
                }
            }
            if (sphere.worldmatrix.position.z >= checkpointPositions[3])
            {
                g_nCheckNum = 3;
            }

            if (g_bFalling && !g_bAdminMode)
            {
                g_fFall += 0.3f;
                sphere.worldmatrix.position.y -= g_fFall;
                if (sphere.worldmatrix.position.y < -15.0f)
                {
                    sphere.worldmatrix.position = checkpoint[g_nCheckNum].worldmatrix.position;
                    g_bFalling = false;
                    g_fFall = 0.0f;
                }
            }

            skybox.worldmatrix.position = sphere.worldmatrix.position;
            skybox.worldmatrix.position.z += 50.0f;
        }
        else if (g_bEndPoint && !g_bEndpointMessageShown)
        {
            g_bEndpointMessageShown = true;
            int result = MessageBoxA(NULL, "플레이 해주셔서 감사합니다.\n확인 버튼을 누르면 게임을 종료합니다.", "게임 종료", MB_OK | MB_ICONQUESTION);
            if (result == IDOK) {
                glutLeaveMainLoop();
            }
        }
        break;
    }
    glutPostRedisplay();
    glutTimerFunc(10, TimerFunction, 1);
}

void InitTexture()
{
    int widthimage1, heightimage1, numberOfChannel1;
    stbi_set_flip_vertically_on_load(true);

    glGenTextures(6, textures);

    unsigned char* data2 = stbi_load("resource/background.jpg", &widthimage1, &heightimage1, &numberOfChannel1, 0);
    glBindTexture(GL_TEXTURE_2D, textures[1]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    if (data2)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, widthimage1, heightimage1, 0, GL_RGB, GL_UNSIGNED_BYTE, data2);
        stbi_image_free(data2);
    }

    stbi_set_flip_vertically_on_load(false);

    unsigned char* data = stbi_load("resource/checkpoint.png", &widthimage1, &heightimage1, &numberOfChannel1, 0);
    glBindTexture(GL_TEXTURE_2D, textures[2]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GLenum format = GL_RGB;
    if (numberOfChannel1 == 4)
        format = GL_RGBA;

    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, format, widthimage1, heightimage1, 0, format, GL_UNSIGNED_BYTE, data);
        stbi_image_free(data);
    }

    unsigned char* data4 = stbi_load("resource/rotation.png", &widthimage1, &heightimage1, &numberOfChannel1, 0);
    glBindTexture(GL_TEXTURE_2D, textures[3]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    if (data4)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, widthimage1, heightimage1, 0, GL_RGB, GL_UNSIGNED_BYTE, data4);
        stbi_image_free(data4);
    }

    unsigned char* data5 = stbi_load("resource/onoff.png", &widthimage1, &heightimage1, &numberOfChannel1, 0);
    glBindTexture(GL_TEXTURE_2D, textures[4]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    if (data5)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, widthimage1, heightimage1, 0, GL_RGB, GL_UNSIGNED_BYTE, data5);
        stbi_image_free(data5);
    }


    unsigned char* data6 = stbi_load("resource/random.png", &widthimage1, &heightimage1, &numberOfChannel1, 0);
    glBindTexture(GL_TEXTURE_2D, textures[5]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    if (data6)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, widthimage1, heightimage1, 0, GL_RGB, GL_UNSIGNED_BYTE, data6);
        stbi_image_free(data6);
    }

    glUseProgram(shaderProgramID);
    int tLocation = glGetUniformLocation(shaderProgramID, "outTexture");
    glUniform1i(tLocation, 0);
}
