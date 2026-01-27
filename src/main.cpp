#include <glad/glad.h>
#include <GL/freeglut.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <vector>
#include <cmath> 
#include <cstdlib>
#include <ctime>

#include "Shader.h"
#include "Model.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// --- USTAWIENIA OKNA ---
int windowWidth = 1200;
int windowHeight = 800;

// --- KAMERA ---
const float PLAYER_HEIGHT = 1.7f;
glm::vec3 cameraPos   = glm::vec3(0.0f, PLAYER_HEIGHT, 12.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f,  0.0f);

// Myszka
float yaw   = -90.0f;
float pitch =  0.0f;
bool firstMouse = true;

// Czas
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Klawisze
bool keys[1024];
bool isWireframe = false;

// --- ZASOBY ---
unsigned int VAO, VBO; // Pokój
unsigned int platformVAO, platformVBO; // Platformy
unsigned int grassVAO, grassVBO; // Trawa
int platformVertexCount = 0;

unsigned int floorTexture;
unsigned int wallTexture;
unsigned int ceilingTexture;
unsigned int grassTexture;

unsigned int tireTexture;
unsigned int steelTexture;
unsigned int glassTexture;
unsigned int redTexture;
unsigned int lightTexture;

Shader* ourShader = nullptr;

std::vector<Model*> carModels;
std::vector<unsigned int> assignedPaints;
std::vector<unsigned int> availablePaints;

// --- OŚWIETLENIE I DZIEŃ/NOC ---
glm::vec3 currentLightColor = glm::vec3(1.0f, 1.0f, 1.0f); 
glm::vec3 roomLightColor = glm::vec3(1.0f, 0.8f, 0.6f);
bool isRoomLightOn = true; // Czy światło się świeci?
glm::vec3 savedRoomLightColor = glm::vec3(1.0f, 0.8f, 0.6f);
bool isDay = true; 

// --- KONFIGURACJA SCENY ---
const int CAR_COUNT = 5; 
float carSpacing = 4.0f; 

// --- ANIMACJE ---
bool isRotating = false;
float rotationAngle = 0.0f;
float rotationSpeed = 30.0f;

bool isHovering = false;

unsigned int loadTexture(const char* path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true); 
    unsigned char *data = stbi_load(path, &width, &height, &nrChannels, 0);
    
    if (data) {
        GLenum format;
        if (nrChannels == 1) format = GL_RED;
        else if (nrChannels == 3) format = GL_RGB; 
        else if (nrChannels == 4) format = GL_RGBA; 

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    } else {
        std::cout << "Nie udalo sie wczytac tekstury: " << path << std::endl;
        stbi_image_free(data);
    }
    return textureID;
}

void setupVegetation() {
    float size = 100.0f; 
    float yPos = -0.05f; 

    float vertices[] = {
         -size, yPos, -size,  0.0f, size,   0.0f, 1.0f, 0.0f,
         -size, yPos,  size,  0.0f, 0.0f,   0.0f, 1.0f, 0.0f,
          size, yPos,  size,  size, 0.0f,   0.0f, 1.0f, 0.0f,
          size, yPos,  size,  size, 0.0f,   0.0f, 1.0f, 0.0f,
          size, yPos, -size,  size, size,   0.0f, 1.0f, 0.0f,
         -size, yPos, -size,  0.0f, size,   0.0f, 1.0f, 0.0f
    };

    glGenVertexArrays(1, &grassVAO);
    glGenBuffers(1, &grassVBO);
    glBindVertexArray(grassVAO);
    glBindBuffer(GL_ARRAY_BUFFER, grassVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    int stride = 8 * sizeof(float);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);
}

void setupPlatform() {
    std::vector<float> verts;
    int segments = 64; 
    float radius = 2.0f; 
    float height = 0.15f; 

    for(int i = 0; i < segments; i++) {
        float theta = (float)i / segments * 2.0f * M_PI;
        float nextTheta = (float)(i + 1) / segments * 2.0f * M_PI;
        float x = cos(theta) * radius; float z = sin(theta) * radius;
        float nx = cos(nextTheta) * radius; float nz = sin(nextTheta) * radius;

        verts.insert(verts.end(), {0.0f, height, 0.0f,  0.5f, 0.5f,  0.0f, 1.0f, 0.0f}); 
        verts.insert(verts.end(), {x, height, z,        x*0.25f+0.5f, z*0.25f+0.5f, 0.0f, 1.0f, 0.0f});
        verts.insert(verts.end(), {nx, height, nz,      nx*0.25f+0.5f, nz*0.25f+0.5f, 0.0f, 1.0f, 0.0f});
        
        verts.insert(verts.end(), {x, height, z,        0.0f, 1.0f,  x, 0.0f, z});
        verts.insert(verts.end(), {x, 0.0f, z,          0.0f, 0.0f,  x, 0.0f, z});
        verts.insert(verts.end(), {nx, 0.0f, nz,        1.0f, 0.0f,  nx, 0.0f, nz});
        verts.insert(verts.end(), {x, height, z,        0.0f, 1.0f,  x, 0.0f, z});
        verts.insert(verts.end(), {nx, 0.0f, nz,        1.0f, 0.0f,  nx, 0.0f, nz});
        verts.insert(verts.end(), {nx, height, nz,      1.0f, 1.0f,  nx, 0.0f, nz});
    }
    platformVertexCount = verts.size() / 8;
    glGenVertexArrays(1, &platformVAO);
    glGenBuffers(1, &platformVBO);
    glBindVertexArray(platformVAO);
    glBindBuffer(GL_ARRAY_BUFFER, platformVBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);
    int stride = 8 * sizeof(float);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);
}

void setupRoom() {
    float size = 15.0f;  
    float height = 6.0f; 

    // Wymiary bramy frontowej
    float holeHalfWidth = 5.0f;
    float holeHeight = 4.0f;

    // Wymiary OKIEN (Dziury w bocznych ścianach)
    // Okno będzie od wysokości 1.5 do 4.5
    // Szerokość okna: od Z = -5 do Z = 5
    float winY_Bot = 1.5f;
    float winY_Top = 4.5f;
    float winZ_Gap = 5.0f; // Połowa szerokości okna

    float vertices[] = {
         // --- PODŁOGA ---
         -size, 0.0f, -size,  0.0f, 10.0f,   0.0f, 1.0f, 0.0f,
         -size, 0.0f,  size,  0.0f, 0.0f,   0.0f, 1.0f, 0.0f,
          size, 0.0f,  size,  10.0f, 0.0f,   0.0f, 1.0f, 0.0f,
          size, 0.0f,  size,  10.0f, 0.0f,   0.0f, 1.0f, 0.0f,
          size, 0.0f, -size,  10.0f, 10.0f,   0.0f, 1.0f, 0.0f,
         -size, 0.0f, -size,  0.0f, 10.0f,   0.0f, 1.0f, 0.0f,

         // --- SUFIT ---
         -size, height, -size,  0.0f, 10.0f,   0.0f, -1.0f, 0.0f,
          size, height, -size,  10.0f, 10.0f,   0.0f, -1.0f, 0.0f,
          size, height,  size,  10.0f, 0.0f,   0.0f, -1.0f, 0.0f,
          size, height,  size,  10.0f, 0.0f,   0.0f, -1.0f, 0.0f,
         -size, height,  size,  0.0f, 0.0f,   0.0f, -1.0f, 0.0f,
         -size, height, -size,  0.0f, 10.0f,   0.0f, -1.0f, 0.0f,

         // --- ŚCIANA TYLNA (Pełna) ---
         -size, 0.0f,   -size,  0.0f, 0.0f,   0.0f, 0.0f, 1.0f,
          size, 0.0f,   -size,  1.0f, 0.0f,   0.0f, 0.0f, 1.0f,
          size, height, -size,  1.0f, 1.0f,   0.0f, 0.0f, 1.0f,
          size, height, -size,  1.0f, 1.0f,   0.0f, 0.0f, 1.0f,
         -size, height, -size,  0.0f, 1.0f,   0.0f, 0.0f, 1.0f,
         -size, 0.0f,   -size,  0.0f, 0.0f,   0.0f, 0.0f, 1.0f,

         // --- ŚCIANA LEWA (Z OKNEM) ---
         // 1. Pasek Dolny (pod oknem)
         -size, 0.0f, size,        0.0f, 0.0f,   1.0f, 0.0f, 0.0f,
         -size, 0.0f, -size,       1.0f, 0.0f,   1.0f, 0.0f, 0.0f,
         -size, winY_Bot, -size,   1.0f, 0.3f,   1.0f, 0.0f, 0.0f,
         -size, winY_Bot, -size,   1.0f, 0.3f,   1.0f, 0.0f, 0.0f,
         -size, winY_Bot, size,    0.0f, 0.3f,   1.0f, 0.0f, 0.0f,
         -size, 0.0f, size,        0.0f, 0.0f,   1.0f, 0.0f, 0.0f,

         // 2. Pasek Górny (nad oknem)
         -size, winY_Top, size,    0.0f, 0.7f,   1.0f, 0.0f, 0.0f,
         -size, winY_Top, -size,   1.0f, 0.7f,   1.0f, 0.0f, 0.0f,
         -size, height, -size,     1.0f, 1.0f,   1.0f, 0.0f, 0.0f,
         -size, height, -size,     1.0f, 1.0f,   1.0f, 0.0f, 0.0f,
         -size, height, size,      0.0f, 1.0f,   1.0f, 0.0f, 0.0f,
         -size, winY_Top, size,    0.0f, 0.7f,   1.0f, 0.0f, 0.0f,

         // 3. Filar Tylny (za oknem)
         -size, winY_Bot, -winZ_Gap, 0.7f, 0.3f,  1.0f, 0.0f, 0.0f,
         -size, winY_Bot, -size,     1.0f, 0.3f,  1.0f, 0.0f, 0.0f,
         -size, winY_Top, -size,     1.0f, 0.7f,  1.0f, 0.0f, 0.0f,
         -size, winY_Top, -size,     1.0f, 0.7f,  1.0f, 0.0f, 0.0f,
         -size, winY_Top, -winZ_Gap, 0.7f, 0.7f,  1.0f, 0.0f, 0.0f,
         -size, winY_Bot, -winZ_Gap, 0.7f, 0.3f,  1.0f, 0.0f, 0.0f,

         // 4. Filar Przedni (przed oknem)
         -size, winY_Bot, size,      0.0f, 0.3f,  1.0f, 0.0f, 0.0f,
         -size, winY_Bot, winZ_Gap,  0.3f, 0.3f,  1.0f, 0.0f, 0.0f,
         -size, winY_Top, winZ_Gap,  0.3f, 0.7f,  1.0f, 0.0f, 0.0f,
         -size, winY_Top, winZ_Gap,  0.3f, 0.7f,  1.0f, 0.0f, 0.0f,
         -size, winY_Top, size,      0.0f, 0.7f,  1.0f, 0.0f, 0.0f,
         -size, winY_Bot, size,      0.0f, 0.3f,  1.0f, 0.0f, 0.0f,


          // --- ŚCIANA PRAWA (Z OKNEM) ---
          // 1. Pasek Dolny
          size, 0.0f, -size,       1.0f, 0.0f,  -1.0f, 0.0f, 0.0f,
          size, 0.0f, size,        0.0f, 0.0f,  -1.0f, 0.0f, 0.0f,
          size, winY_Bot, size,    0.0f, 0.3f,  -1.0f, 0.0f, 0.0f,
          size, winY_Bot, size,    0.0f, 0.3f,  -1.0f, 0.0f, 0.0f,
          size, winY_Bot, -size,   1.0f, 0.3f,  -1.0f, 0.0f, 0.0f,
          size, 0.0f, -size,       1.0f, 0.0f,  -1.0f, 0.0f, 0.0f,

          // 2. Pasek Górny
          size, winY_Top, -size,   1.0f, 0.7f,  -1.0f, 0.0f, 0.0f,
          size, winY_Top, size,    0.0f, 0.7f,  -1.0f, 0.0f, 0.0f,
          size, height, size,      0.0f, 1.0f,  -1.0f, 0.0f, 0.0f,
          size, height, size,      0.0f, 1.0f,  -1.0f, 0.0f, 0.0f,
          size, height, -size,     1.0f, 1.0f,  -1.0f, 0.0f, 0.0f,
          size, winY_Top, -size,   1.0f, 0.7f,  -1.0f, 0.0f, 0.0f,

          // 3. Filar Tylny
          size, winY_Bot, -size,     1.0f, 0.3f,  -1.0f, 0.0f, 0.0f,
          size, winY_Bot, -winZ_Gap, 0.7f, 0.3f,  -1.0f, 0.0f, 0.0f,
          size, winY_Top, -winZ_Gap, 0.7f, 0.7f,  -1.0f, 0.0f, 0.0f,
          size, winY_Top, -winZ_Gap, 0.7f, 0.7f,  -1.0f, 0.0f, 0.0f,
          size, winY_Top, -size,     1.0f, 0.7f,  -1.0f, 0.0f, 0.0f,
          size, winY_Bot, -size,     1.0f, 0.3f,  -1.0f, 0.0f, 0.0f,

          // 4. Filar Przedni
          size, winY_Bot, winZ_Gap,  0.3f, 0.3f,  -1.0f, 0.0f, 0.0f,
          size, winY_Bot, size,      0.0f, 0.3f,  -1.0f, 0.0f, 0.0f,
          size, winY_Top, size,      0.0f, 0.7f,  -1.0f, 0.0f, 0.0f,
          size, winY_Top, size,      0.0f, 0.7f,  -1.0f, 0.0f, 0.0f,
          size, winY_Top, winZ_Gap,  0.3f, 0.7f,  -1.0f, 0.0f, 0.0f,
          size, winY_Bot, winZ_Gap,  0.3f, 0.3f,  -1.0f, 0.0f, 0.0f,


          // --- ŚCIANA PRZEDNIA Z BRAMĄ ---
          // 1. Lewy filar
         -size, 0.0f, size,           0.0f, 0.0f,  0.0f, 0.0f, -1.0f,
         -holeHalfWidth, 0.0f, size,  0.3f, 0.0f,  0.0f, 0.0f, -1.0f,
         -holeHalfWidth, height, size,0.3f, 1.0f,  0.0f, 0.0f, -1.0f,
         -holeHalfWidth, height, size,0.3f, 1.0f,  0.0f, 0.0f, -1.0f,
         -size, height, size,         0.0f, 1.0f,  0.0f, 0.0f, -1.0f,
         -size, 0.0f, size,           0.0f, 0.0f,  0.0f, 0.0f, -1.0f,

          // 2. Prawy filar
          holeHalfWidth, 0.0f, size,  0.7f, 0.0f,  0.0f, 0.0f, -1.0f,
          size, 0.0f, size,           1.0f, 0.0f,  0.0f, 0.0f, -1.0f,
          size, height, size,         1.0f, 1.0f,  0.0f, 0.0f, -1.0f,
          size, height, size,         1.0f, 1.0f,  0.0f, 0.0f, -1.0f,
          holeHalfWidth, height, size,0.7f, 1.0f,  0.0f, 0.0f, -1.0f,
          holeHalfWidth, 0.0f, size,  0.7f, 0.0f,  0.0f, 0.0f, -1.0f,

          // 3. Górna belka
         -holeHalfWidth, holeHeight, size,  0.3f, 0.6f,  0.0f, 0.0f, -1.0f,
          holeHalfWidth, holeHeight, size,  0.7f, 0.6f,  0.0f, 0.0f, -1.0f,
          holeHalfWidth, height, size,      0.7f, 1.0f,  0.0f, 0.0f, -1.0f,
          holeHalfWidth, height, size,      0.7f, 1.0f,  0.0f, 0.0f, -1.0f,
         -holeHalfWidth, height, size,      0.3f, 1.0f,  0.0f, 0.0f, -1.0f,
         -holeHalfWidth, holeHeight, size,  0.3f, 0.6f,  0.0f, 0.0f, -1.0f
    };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    int stride = 8 * sizeof(float);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);
}

void doMovement() {
    float cameraSpeed = 2.5f * deltaTime; 
    glm::vec3 frontFlat = glm::normalize(glm::vec3(cameraFront.x, 0.0f, cameraFront.z));
    glm::vec3 rightFlat = glm::normalize(glm::cross(cameraFront, cameraUp));

    if (keys['w']) cameraPos += cameraSpeed * frontFlat;
    if (keys['s']) cameraPos -= cameraSpeed * frontFlat;
    if (keys['a']) cameraPos -= cameraSpeed * rightFlat;
    if (keys['d']) cameraPos += cameraSpeed * rightFlat;
    cameraPos.y = PLAYER_HEIGHT;
}

void display() {
    float currentFrame = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    doMovement();

    if (isRotating) {
        rotationAngle += rotationSpeed * deltaTime;
        if (rotationAngle > 360.0f) rotationAngle -= 360.0f;
    }

    // --- LOGIKA DZIEŃ / NOC ---
    if (isDay) {
        glClearColor(0.5f, 0.7f, 1.0f, 1.0f); 
    } else {
        glClearColor(0.05f, 0.05f, 0.15f, 1.0f);
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ourShader->use();

    // --- OŚWIETLENIE ---
    // 1. Główne światło
    ourShader->setVec3("lightPos", 0.0f, 20.0f, 0.0f); 
    ourShader->setVec3("lightColor", currentLightColor.x, currentLightColor.y, currentLightColor.z); 
    
    // 2. Światło drugie (Latarka w nocy)
    if (!isDay) {
        // --- LATARKA (KIERUNKOWA + MIĘKKA) ---
        ourShader->setVec3("lightPos2", cameraPos.x, cameraPos.y, cameraPos.z); 
        ourShader->setVec3("lightDir2", cameraFront.x, cameraFront.y, cameraFront.z);
        ourShader->setFloat("lightCutoff", glm::cos(glm::radians(12.5f)));
        ourShader->setVec3("lightColor2", 1.0f, 1.0f, 1.0f); 
    } else {
        ourShader->setVec3("lightColor2", 0.0f, 0.0f, 0.0f); 
    }

    // 3. NOWA LAMPA SUFITOWA (Wnętrze) - Losowany kolor
    ourShader->setVec3("lightPos3", 0.0f, 5.5f, 0.0f); 
    ourShader->setVec3("lightColor3", roomLightColor.x, roomLightColor.y, roomLightColor.z);
    
    ourShader->setVec3("viewPos", cameraPos.x, cameraPos.y, cameraPos.z);

    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)windowWidth / (float)windowHeight, 0.1f, 100.0f);
    ourShader->setMat4("view", view);
    ourShader->setMat4("projection", projection);

    // --- 1. TRAWA (MATOWA) ---
    ourShader->setFloat("shine", 0.0f); 
    
    glm::mat4 model = glm::mat4(1.0f);
    ourShader->setMat4("model", model);
    ourShader->setInt("useTexture", 1);
    
    if(isDay) ourShader->setVec3("objectColor", 1.0f, 1.0f, 1.0f);
    else ourShader->setVec3("objectColor", 0.5f, 0.5f, 0.6f);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, grassTexture);
    glBindVertexArray(grassVAO);
    ourShader->setFloat("tiling", 20.0f); 
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // --- 2. SALON (LEKKI POŁYSK) ---
    ourShader->setFloat("shine", 0.1f); 
    
    glBindVertexArray(VAO);
    ourShader->setVec3("objectColor", 1.0f, 1.0f, 1.0f); 

    glBindTexture(GL_TEXTURE_2D, floorTexture);
    ourShader->setFloat("tiling", 10.0f); 
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindTexture(GL_TEXTURE_2D, ceilingTexture);
    ourShader->setFloat("tiling", 3.0f);
    glDrawArrays(GL_TRIANGLES, 6, 6);

    glBindTexture(GL_TEXTURE_2D, wallTexture);
    ourShader->setFloat("tiling", 1.0f); 
    
    // Teraz rysujemy ŚCIANY (Tylna pełna + Lewa z oknem + Prawa z oknem + Przednia z bramą)
    // 6 trójkątów (tylna) + 24 trójkąty (lewa - 4 części) + 24 trójkąty (prawa - 4 części) + 18 trójkątów (brama)
    // Razem = 72 trójkąty ścian.
    // Offset w buforze VAO: 12 (bo 6 podłoga + 6 sufit już były)
    glDrawArrays(GL_TRIANGLES, 12, 72); 

    // --- 3. AUTA (MOCNY POŁYSK) ---
    ourShader->setFloat("shine", 0.8f); 
    
    float startX = -((CAR_COUNT - 1) * carSpacing) / 2.0f + 2.0f; 

    for(int i = 0; i < carModels.size(); i++) {
        float xPos = startX + (i * carSpacing);

        if (i == 0) {
            float moveAllX = -1.0f; float moveAllZ = 0.0f; 
            glm::mat4 platformModel = glm::mat4(1.0f);
            platformModel = glm::translate(platformModel, glm::vec3(xPos + moveAllX, 0.0f, 0.0f + moveAllZ)); 
            platformModel = glm::rotate(platformModel, glm::radians(rotationAngle), glm::vec3(0.0f, 1.0f, 0.0f));
            ourShader->setMat4("model", platformModel);
            ourShader->setFloat("tiling", 1.0f);
            glBindTexture(GL_TEXTURE_2D, steelTexture);
            glBindVertexArray(platformVAO);
            glDrawArrays(GL_TRIANGLES, 0, platformVertexCount);

            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(xPos + moveAllX, 0.65f, 0.0f + moveAllZ)); 
            model = glm::rotate(model, glm::radians(rotationAngle), glm::vec3(0.0f, 1.0f, 0.0f));
            float fixX = 0.0f; float fixZ = 0.0f;   
            model = glm::translate(model, glm::vec3(fixX, 0.0f, fixZ));
            model = glm::scale(model, glm::vec3(1.8f)); 
            ourShader->setMat4("model", model);
        }
        else if (i == 4) {
            float hoverHeight = 0.0f;
            if(isHovering) hoverHeight = sin(currentFrame * 3.0f) * 0.5f + 0.5f; 

            glm::mat4 platformModel = glm::mat4(1.0f);
            platformModel = glm::translate(platformModel, glm::vec3(xPos, hoverHeight, 0.0f)); 
            ourShader->setMat4("model", platformModel);
            ourShader->setFloat("tiling", 1.0f);
            glBindTexture(GL_TEXTURE_2D, steelTexture); 
            glBindVertexArray(platformVAO);
            glDrawArrays(GL_TRIANGLES, 0, platformVertexCount);

            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(xPos, 0.65f + hoverHeight + 0.15f, 0.0f)); 
            model = glm::scale(model, glm::vec3(2.0f)); 
            ourShader->setMat4("model", model);
        }
        else {
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(xPos, 0.65f, 0.0f)); 
            model = glm::scale(model, glm::vec3(2.0f)); 
            ourShader->setMat4("model", model);
        }

        Model* currentCar = carModels[i];
        unsigned int currentPaint = assignedPaints[i];

        for(unsigned int j = 0; j < currentCar->meshes.size(); j++) {
            Mesh& mesh = currentCar->meshes[j];
            std::string name = mesh.materialName;
            float currentTiling = 1.0f;
            
            ourShader->setInt("useTexture", 1);
            ourShader->setVec3("objectColor", 1.0f, 1.0f, 1.0f); 
            glActiveTexture(GL_TEXTURE0);

            if (i == 0) { 
                if(name.find("Glass") != std::string::npos) glBindTexture(GL_TEXTURE_2D, glassTexture);
                else glBindTexture(GL_TEXTURE_2D, currentPaint);
            }
            else { 
                if(name.find("Black") != std::string::npos || name.find("Tire") != std::string::npos) glBindTexture(GL_TEXTURE_2D, tireTexture);
                else if(name.find("steel") != std::string::npos || name.find("Chrome") != std::string::npos) glBindTexture(GL_TEXTURE_2D, steelTexture);
                else if(name.find("Red") != std::string::npos) glBindTexture(GL_TEXTURE_2D, redTexture);
                else if(name.find("Light") != std::string::npos) glBindTexture(GL_TEXTURE_2D, lightTexture);
                else if(name.find("glass") != std::string::npos) glBindTexture(GL_TEXTURE_2D, glassTexture);
                else {
                    glBindTexture(GL_TEXTURE_2D, currentPaint);
                    currentTiling = 4.0f; 
                }
            }
            ourShader->setFloat("tiling", currentTiling);
            mesh.Draw(*ourShader);
        }
    }
    glutSwapBuffers();
    glutPostRedisplay();
}

void keyboardDown(unsigned char key, int x, int y) {
    if(key == 27) glutLeaveMainLoop();
    keys[key] = true;

    if(key == 'z' || key == 'Z') {
        isWireframe = !isWireframe;
        if(isWireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        else glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    if(key == 'c' || key == 'C') {
        float r = (float)rand() / RAND_MAX;
        float g = (float)rand() / RAND_MAX;
        float b = (float)rand() / RAND_MAX;
        roomLightColor = glm::vec3(r, g, b);
        std::cout << "Nowy kolor swiatla: " << r << ", " << g << ", " << b << std::endl;
    }

    if(key == 'x' || key == 'X') {
        if(isRoomLightOn) {
            // JEŚLI JEST WŁĄCZONE -> WYŁĄCZAMY
            savedRoomLightColor = roomLightColor; // 1. Zapamiętaj aktualny kolor
            roomLightColor = glm::vec3(0.0f, 0.0f, 0.0f); // 2. Ustaw czarny (brak światła)
            isRoomLightOn = false; // 3. Zmień flagę
            std::cout << "Swiatlo w pokoju: WYLACZONE" << std::endl;
        } 
        else {
            // JEŚLI JEST WYŁĄCZONE -> WŁĄCZAMY
            roomLightColor = savedRoomLightColor; // 1. Przywróć zapamiętany kolor
            isRoomLightOn = true; // 2. Zmień flagę
            std::cout << "Swiatlo w pokoju: WLACZONE" << std::endl;
        }
    }

    if(key == 'p' || key == 'P') {
        for(int i = 1; i < CAR_COUNT; i++) {
            int randomPaintIndex = rand() % availablePaints.size();
            assignedPaints[i] = availablePaints[randomPaintIndex];
        }
    }

    if(key == 'r' || key == 'R') isRotating = !isRotating;
    if(key == 'h' || key == 'H') isHovering = !isHovering;

    if(key == 'n' || key == 'N') {
        isDay = !isDay;
        if(isDay) currentLightColor = glm::vec3(1.0f, 1.0f, 1.0f); 
        else      currentLightColor = glm::vec3(0.05f, 0.05f, 0.08f); 
        std::cout << "Tryb: " << (isDay ? "DZIEN" : "NOC") << std::endl;
    }
}

void keyboardUp(unsigned char key, int x, int y) {
    keys[key] = false;
}

void mouseCallback(int x, int y) {
    int centerX = windowWidth / 2;
    int centerY = windowHeight / 2;
    if(x == centerX && y == centerY) return;
    float xoffset = x - centerX;
    float yoffset = centerY - y; 
    glutWarpPointer(centerX, centerY);
    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;
    yaw   += xoffset;
    pitch += yoffset;
    if(pitch > 89.0f) pitch = 89.0f;
    if(pitch < -89.0f) pitch = -89.0f;
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}

void resize(int width, int height) {
    windowWidth = width;
    windowHeight = height;
    glViewport(0, 0, width, height);
}

int main(int argc, char** argv) {
    srand(time(NULL));

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(windowWidth, windowHeight);
    glutCreateWindow("Salon 3D - Spacer PwAG"); 

    if (!gladLoadGL()) return -1;
    glEnable(GL_DEPTH_TEST);

    ourShader = new Shader("shaders/shader.vert", "shaders/shader.frag");

    setupRoom();
    setupPlatform(); 
    setupVegetation(); 

    floorTexture = loadTexture("textures/floor.png");
    wallTexture  = loadTexture("textures/wall_texture.jpg");
    ceilingTexture = loadTexture("textures/ceiling.jpg");
    grassTexture = loadTexture("textures/grass.png");
    
    tireTexture  = loadTexture("textures/tire_texture.jpg");
    steelTexture = loadTexture("textures/steel_texture.jpg");
    glassTexture = loadTexture("textures/glass_texture.jpg");
    redTexture   = loadTexture("textures/red_texture.jpg");
    lightTexture = loadTexture("textures/light_texture.jpg");

    std::cout << "Ladowanie palety lakierow..." << std::endl;
    for(int i = 1; i <= 8; i++) {
        std::string texPath = "textures/car_paint_" + std::to_string(i) + ".jpg";
        availablePaints.push_back(loadTexture(texPath.c_str()));
    }

    std::cout << "Ladowanie 5 samochodow..." << std::endl;
    for(int i = 1; i <= CAR_COUNT; i++) {
        std::string modelPath = "models/car-" + std::to_string(i) + ".obj";
        std::cout << "Ladowanie: " << modelPath << std::endl;
        Model* newCar = new Model(modelPath);
        carModels.push_back(newCar);
        assignedPaints.push_back(availablePaints[(i-1) % 8]);
    }
    
    glutDisplayFunc(display);
    glutReshapeFunc(resize);
    glutKeyboardFunc(keyboardDown);
    glutKeyboardUpFunc(keyboardUp);
    glutPassiveMotionFunc(mouseCallback);
    glutSetCursor(GLUT_CURSOR_NONE); 
    glutWarpPointer(windowWidth / 2, windowHeight / 2);

    glutMainLoop();

    delete ourShader;
    for(auto car : carModels) delete car;

    return 0;
}