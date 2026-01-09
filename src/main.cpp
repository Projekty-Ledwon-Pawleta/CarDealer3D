#include <glad/glad.h>
#include <GL/freeglut.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <vector>

#include "Shader.h"
#include "Model.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// --- USTAWIENIA OKNA ---
int windowWidth = 1200;
int windowHeight = 800;

// --- KAMERA ---
// Ustawiamy sztywną wysokość "oczu" (np. 1.7 metra - wzrost człowieka)
const float PLAYER_HEIGHT = 1.7f;

glm::vec3 cameraPos   = glm::vec3(0.0f, PLAYER_HEIGHT, 5.0f);
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

unsigned int VAO, VBO;

unsigned int floorTexture;
unsigned int wallTexture;
unsigned int ceilingTexture;

unsigned int tireTexture;
unsigned int steelTexture;
unsigned int glassTexture;
unsigned int redTexture;
unsigned int blackTexture;
unsigned int lightTexture;


Shader* ourShader = nullptr;

std::vector<Model*> carModels;
std::vector<unsigned int> assignedPaints;

std::vector<unsigned int> availablePaints;

// Konfiguracja
const int CAR_COUNT = 5; // Ile aut chcemy wczytać?
float carSpacing = 3.0f; // Odstęp między autami (w metrach)

glm::vec3 currentLightColor = glm::vec3(1.0f, 0.0f, 0.0f);

unsigned int loadTexture(const char* path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrChannels;
    // OpenGL ma 0,0 na dole, a obrazki na górze - musimy obrócić
    stbi_set_flip_vertically_on_load(true); 
    unsigned char *data = stbi_load(path, &width, &height, &nrChannels, 0);
    
    if (data) {
        GLenum format;
        if (nrChannels == 1) format = GL_RED;
        else if (nrChannels == 3) format = GL_RGB; // .jpg zazwyczaj
        else if (nrChannels == 4) format = GL_RGBA; // .png zazwyczaj

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        // Ustawienia powtarzania (GL_REPEAT) i filtrowania
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

void setupRoom() {
    float size = 10.0f;  // Promień pokoju (od środka do ściany)
    float height = 4.0f; // Wysokość

    // Format: x, y, z,   u, v,   nx, ny, nz
    float vertices[] = {
        // --- 1. PODŁOGA (Y=0, Normalna w górę) ---
         -size, 0.0f, -size,  0.0f, 1.0f,   0.0f, 1.0f, 0.0f,
         -size, 0.0f,  size,  0.0f, 0.0f,   0.0f, 1.0f, 0.0f,
          size, 0.0f,  size,  1.0f, 0.0f,   0.0f, 1.0f, 0.0f,
          size, 0.0f,  size,  1.0f, 0.0f,   0.0f, 1.0f, 0.0f,
          size, 0.0f, -size,  1.0f, 1.0f,   0.0f, 1.0f, 0.0f,
         -size, 0.0f, -size,  0.0f, 1.0f,   0.0f, 1.0f, 0.0f,

        // --- 2. SUFIT (Y=height, Normalna w dół) ---
         -size, height, -size,  0.0f, 1.0f,   0.0f, -1.0f, 0.0f,
          size, height, -size,  1.0f, 1.0f,   0.0f, -1.0f, 0.0f,
          size, height,  size,  1.0f, 0.0f,   0.0f, -1.0f, 0.0f,
          size, height,  size,  1.0f, 0.0f,   0.0f, -1.0f, 0.0f,
         -size, height,  size,  0.0f, 0.0f,   0.0f, -1.0f, 0.0f,
         -size, height, -size,  0.0f, 1.0f,   0.0f, -1.0f, 0.0f,

        // --- 3. ŚCIANA TYLNA (Z=-size, Normalna w stronę kamery +Z) ---
         -size, 0.0f,   -size,  0.0f, 0.0f,   0.0f, 0.0f, 1.0f,
          size, 0.0f,   -size,  1.0f, 0.0f,   0.0f, 0.0f, 1.0f,
          size, height, -size,  1.0f, 1.0f,   0.0f, 0.0f, 1.0f,
          size, height, -size,  1.0f, 1.0f,   0.0f, 0.0f, 1.0f,
         -size, height, -size,  0.0f, 1.0f,   0.0f, 0.0f, 1.0f,
         -size, 0.0f,   -size,  0.0f, 0.0f,   0.0f, 0.0f, 1.0f,

        // --- 4. ŚCIANA LEWA (X=-size, Normalna w prawo +X) ---
         -size, 0.0f,   size,   0.0f, 0.0f,   1.0f, 0.0f, 0.0f,
         -size, 0.0f,  -size,   1.0f, 0.0f,   1.0f, 0.0f, 0.0f,
         -size, height,-size,   1.0f, 1.0f,   1.0f, 0.0f, 0.0f,
         -size, height,-size,   1.0f, 1.0f,   1.0f, 0.0f, 0.0f,
         -size, height, size,   0.0f, 1.0f,   1.0f, 0.0f, 0.0f,
         -size, 0.0f,   size,   0.0f, 0.0f,   1.0f, 0.0f, 0.0f,

        // --- 5. ŚCIANA PRAWA (X=size, Normalna w lewo -X) ---
          size, 0.0f,  -size,   1.0f, 0.0f,  -1.0f, 0.0f, 0.0f,
          size, 0.0f,   size,   0.0f, 0.0f,  -1.0f, 0.0f, 0.0f,
          size, height, size,   0.0f, 1.0f,  -1.0f, 0.0f, 0.0f,
          size, height, size,   0.0f, 1.0f,  -1.0f, 0.0f, 0.0f,
          size, height,-size,   1.0f, 1.0f,  -1.0f, 0.0f, 0.0f,
          size, 0.0f,  -size,   1.0f, 0.0f,  -1.0f, 0.0f, 0.0f,
        // --- 6. ŚCIANA PRZEDNIA (Z=size, Normalna w tył -Z) ---
         -size, 0.0f,   size,   0.0f, 0.0f,   0.0f, 0.0f, -1.0f,
          size, 0.0f,   size,   1.0f, 0.0f,   0.0f, 0.0f, -1.0f,
          size, height, size,   1.0f, 1.0f,   0.0f, 0.0f, -1.0f,
          size, height, size,   1.0f, 1.0f,   0.0f, 0.0f, -1.0f,
         -size, height, size,   0.0f, 1.0f,   0.0f, 0.0f, -1.0f,
         -size, 0.0f,   size,   0.0f, 0.0f,   0.0f, 0.0f, -1.0f
    };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // Ważne: sizeof(vertices) automatycznie pobierze rozmiar całej tablicy
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    int stride = 8 * sizeof(float);
    // 1. Poz
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    // 2. Tex
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // 3. Norm
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);
}

// --- POPRAWIONA LOGIKA RUCHU (CHODZENIE) ---
void doMovement() {
    float cameraSpeed = 2.5f * deltaTime; 

    // 1. Tworzymy wektor, który patrzy tam gdzie kamera, ale PŁASKO (Y=0)
    // Dzięki temu idąc "do przodu" (W) nie lecimy w górę ani w dół
    glm::vec3 frontFlat = glm::normalize(glm::vec3(cameraFront.x, 0.0f, cameraFront.z));
    
    // 2. Wektor "w prawo" (zawsze jest płaski, bo cameraUp jest (0,1,0))
    glm::vec3 rightFlat = glm::normalize(glm::cross(cameraFront, cameraUp));

    if (keys['w'])
        cameraPos += cameraSpeed * frontFlat;
    if (keys['s'])
        cameraPos -= cameraSpeed * frontFlat;
    if (keys['a'])
        cameraPos -= cameraSpeed * rightFlat;
    if (keys['d'])
        cameraPos += cameraSpeed * rightFlat;

    // 3. GRAWITACJA / BLOKADA WYSOKOŚCI
    // Zapewniamy, że gracz zawsze ma oczy na tej samej wysokości
    cameraPos.y = PLAYER_HEIGHT;
}

void display() {
    float currentFrame = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    doMovement();

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ourShader->use();

    // --- LAMPA 1 (GŁÓWNA - BIAŁA - SUFIT) ---
    ourShader->setVec3("lightPos", 0.0f, 15.0f, 0.0f); 
    ourShader->setVec3("lightColor", 1.0f, 1.0f, 1.0f); // Białe światło

    // --- LAMPA 2 (DODATKOWA - np. CZERWONY NEON W ROGU) ---
    ourShader->setVec3("lightPos2", 0.0f, 2.0f, 0.0f); 
    
    // Ustawiamy kolor na intensywny czerwony (R=1, G=0, B=0)
    ourShader->setVec3("lightColor2", currentLightColor.x, currentLightColor.y, currentLightColor.z);
    // Kamera
    ourShader->setVec3("viewPos", cameraPos.x, cameraPos.y, cameraPos.z);

    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)windowWidth / (float)windowHeight, 0.1f, 100.0f);
    
    ourShader->setMat4("view", view);
    ourShader->setMat4("projection", projection);

// --- RYSOWANIE POMIESZCZENIA ---
    glm::mat4 model = glm::mat4(1.0f);
    ourShader->setMat4("model", model);
    ourShader->setInt("useTexture", 1);
    
    // Resetujemy kolor na biały, żeby tekstury nie były przyciemnione
    ourShader->setVec3("objectColor", 1.0f, 1.0f, 1.0f); 

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO);

    // 1. PODŁOGA (Pierwsze 6 wierzchołków)
    glBindTexture(GL_TEXTURE_2D, floorTexture);
    ourShader->setFloat("tiling", 10.0f); 
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // 2. SUFIT (Kolejne 6 wierzchołków -> start od 6)
    glBindTexture(GL_TEXTURE_2D, ceilingTexture);
    ourShader->setFloat("tiling", 3.0f);
    glDrawArrays(GL_TRIANGLES, 6, 6);

    // 3. ŚCIANY
    glBindTexture(GL_TEXTURE_2D, wallTexture);
    ourShader->setFloat("tiling", 1.0f); 
    
    glDrawArrays(GL_TRIANGLES, 12, 24);

    // --- RYSOWANIE SAMOCHODÓW W PĘTLI ---
    // Obliczamy pozycję startową, żeby środkowe auto było na środku (X=0)
    float startX = -((CAR_COUNT - 1) * carSpacing) / 2.0f;

    for(int i = 0; i < carModels.size(); i++) {
        model = glm::mat4(1.0f);
        float xPos = startX + (i * carSpacing);
        model = glm::translate(model, glm::vec3(xPos, 0.65f, 0.0f)); 
        model = glm::scale(model, glm::vec3(2.0f)); 

        ourShader->setMat4("model", model);

        Model* currentCar = carModels[i];
        unsigned int currentPaint = assignedPaints[i];

        for(unsigned int j = 0; j < currentCar->meshes.size(); j++) {
            Mesh& mesh = currentCar->meshes[j];
            std::string name = mesh.materialName;
            float currentTiling = 1.0f;
            
            ourShader->setInt("useTexture", 1);
            ourShader->setVec3("objectColor", 1.0f, 1.0f, 1.0f); // Reset
            glActiveTexture(GL_TEXTURE0);

            // =========================================================
            // METODA 1: AUTO NR 1 (SKINOWANIE / UV MAPPING)
            // =========================================================
            if (i == 0) {
                // To jest Car 1. Ma dedykowaną teksturę car_paint_1.jpg
                // Nakładamy ją na wszystko, chyba że trafimy na oponę.
                
                if(name.find("Glass") != std::string::npos) {
                    // Opony nadal chcemy gumowe
                    glBindTexture(GL_TEXTURE_2D, glassTexture);
                    currentTiling = 1.0f;
                } else {
                    glBindTexture(GL_TEXTURE_2D, currentPaint);
                    currentTiling = 1.0f;
                }
            }
            // =========================================================
            // METODA 2: AUTA NR 2-5 (MATERIAL MAPPING / TILING)
            // =========================================================
            else {
                // Tutaj sprawdzamy nazwy materiałów i dobieramy teksturę

                // 1. Opony
                if(name.find("Black") != std::string::npos || 
                   name.find("Tire") != std::string::npos  || 
                   name.find("Rubber") != std::string::npos) {
                    glBindTexture(GL_TEXTURE_2D, tireTexture);
                }
                // 2. Stal / Chrom
                else if(name.find("steel") != std::string::npos || 
                        name.find("Chrome") != std::string::npos) {
                    glBindTexture(GL_TEXTURE_2D, steelTexture);
                }
                // 3. Czerwone światła
                else if(name.find("Red") != std::string::npos) {
                    glBindTexture(GL_TEXTURE_2D, redTexture);
                }
                // 4. Jasne światła
                else if(name.find("Light") != std::string::npos) {
                    glBindTexture(GL_TEXTURE_2D, lightTexture);
                }
                // 5. Szyby
                else if(name.find("glass") != std::string::npos || 
                        name.find("Window") != std::string::npos) {
                    glBindTexture(GL_TEXTURE_2D, glassTexture);
                }
                // 6. Karoseria (wszystko inne)
                else {
                    glBindTexture(GL_TEXTURE_2D, currentPaint);
                    currentTiling = 4.0f; // Powtarzamy teksturę lakieru (ziarno)
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
        // Losujemy 3 liczby od 0.0 do 1.0
        float r = (float)rand() / RAND_MAX;
        float g = (float)rand() / RAND_MAX;
        float b = (float)rand() / RAND_MAX;
        
        currentLightColor = glm::vec3(r, g, b);
        std::cout << "Nowy kolor swiatla: " << r << ", " << g << ", " << b << std::endl;
    }

    if(key == 'p' || key == 'P') {
        for(int i = 1; i < CAR_COUNT; i++) {
            // Losujemy indeks z puli dostępnych lakierów (0-7)
            int randomPaintIndex = rand() % availablePaints.size();
            
            // Przypisujemy nowy lakier do auta
            assignedPaints[i] = availablePaints[randomPaintIndex];
        }
        std::cout << "Przemalowano auta!" << std::endl;
    }
}

void keyboardUp(unsigned char key, int x, int y) {
    keys[key] = false;
}

// --- POPRAWIONA MYSZKA (NIESKOŃCZONY OBRÓT) ---
void mouseCallback(int x, int y) {
    // Obliczamy środek ekranu
    int centerX = windowWidth / 2;
    int centerY = windowHeight / 2;

    // Jeśli kursor jest na środku (bo my go tam przenieśliśmy), ignorujemy to wywołanie
    if(x == centerX && y == centerY) return;

    // Obliczamy różnicę względem ŚRODKA ekranu
    float xoffset = x - centerX;
    float yoffset = centerY - y; // Odwrócone Y

    // Przenosimy kursor z powrotem na środek (magia nieskończoności)
    glutWarpPointer(centerX, centerY);

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw   += xoffset;
    pitch += yoffset;

    // Blokada fikołków
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
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(windowWidth, windowHeight);
    glutCreateWindow("Salon 3D - Spacer PwAG"); // Tytuł zgodny z dokumentem

    if (!gladLoadGL()) return -1;
    glEnable(GL_DEPTH_TEST);

    ourShader = new Shader("shaders/shader.vert", "shaders/shader.frag");

    setupRoom();

    floorTexture = loadTexture("textures/floor.png");
    wallTexture  = loadTexture("textures/wall_texture.jpg");
    ceilingTexture = loadTexture("textures/ceiling.jpg");
    
    tireTexture  = loadTexture("textures/tire_texture.jpg");
    steelTexture = loadTexture("textures/steel_texture.jpg");
    glassTexture = loadTexture("textures/glass_texture.jpg");
    redTexture   = loadTexture("textures/red_texture.jpg");
    lightTexture = loadTexture("textures/light_texture.jpg");

    std::cout << "Ladowanie palety lakierow..." << std::endl;
    for(int i = 1; i <= 8; i++) {
        // Ładujemy car_paint_1.jpg do car_paint_8.jpg
        std::string texPath = "textures/car_paint_" + std::to_string(i) + ".jpg";
        availablePaints.push_back(loadTexture(texPath.c_str()));
    }

// --- 2. ŁADUJEMY AUTA ---
    std::cout << "Ladowanie 5 samochodow..." << std::endl;
    for(int i = 1; i <= CAR_COUNT; i++) {
        std::string modelPath = "models/car-" + std::to_string(i) + ".obj";
        
        std::cout << "Ladowanie: " << modelPath << std::endl;
        
        Model* newCar = new Model(modelPath);
        carModels.push_back(newCar);
        
        // Przypisujemy domyślny lakier z załadowanej wcześniej palety
        // (i-1) % 8  -> zapewnia, że bierzemy lakiery po kolei, a jak się skończą to od początku
        assignedPaints.push_back(availablePaints[(i-1) % 8]);
    }
    
    glutDisplayFunc(display);
    glutReshapeFunc(resize);
    glutKeyboardFunc(keyboardDown);
    glutKeyboardUpFunc(keyboardUp);
    
    // Rejestracja ruchu myszy
    glutPassiveMotionFunc(mouseCallback);

    // --- KLUCZOWE: UKRYCIE KURSORA ---
    // Kursor znika, a my sterujemy kamerą bez ograniczeń
    glutSetCursor(GLUT_CURSOR_NONE); 
    
    // Przenieś kursor na środek na start
    glutWarpPointer(windowWidth / 2, windowHeight / 2);

    glutMainLoop();

    delete ourShader;
    for(auto car : carModels) delete car;

    return 0;
}