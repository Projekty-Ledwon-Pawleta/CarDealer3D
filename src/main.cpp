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
int windowWidth = 800;
int windowHeight = 600;

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

Shader* ourShader = nullptr;
Model* ourCar = nullptr;

unsigned int carTexture;
unsigned int tireTexture;

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

void setupFloor() {
    // Podłoga 20x20
    float vertices[] = {
         -10.0f, 0.0f, -10.0f, 0.0f, 10.0f,
          10.0f, 0.0f, -10.0f, 10.0f, 10.0f,
         -10.0f, 0.0f,  10.0f, 0.0f, 0.0f,

          10.0f, 0.0f, -10.0f, 10.0f, 10.0f,
          10.0f, 0.0f,  10.0f, 10.0f, 0.0f,
         -10.0f, 0.0f,  10.0f, 0.0f, 0.0f
    };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Atrybut 1: Tekstura (2 floaty) - zaczyna się od 3-ciego float w rzędzie
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
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

    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)windowWidth / (float)windowHeight, 0.1f, 100.0f);
    glm::mat4 model = glm::mat4(1.0f);

    // Wysyłamy macierze za pomocą helperów z klasy
    ourShader->setMat4("view", view);
    ourShader->setMat4("projection", projection);
    ourShader->setMat4("model", model);
    
    // Obsługa tekstury
    ourShader->setInt("useTexture", 1);
    ourShader->setInt("texture1", 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, floorTexture);
    
    ourShader->setFloat("tiling", 1.0f);

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 0.7f, 0.0f)); // Pozycja 0,0,0
    model = glm::scale(model, glm::vec3(2.0f)); // Skala (zmieniaj jeśli auto jest gigantyczne)
    ourShader->setMat4("model", model);

    for(unsigned int i = 0; i < ourCar->meshes.size(); i++) {
        Mesh& mesh = ourCar->meshes[i];
        std::string name = mesh.materialName;

        // Domyślne ustawienia (reset)
        ourShader->setInt("useTexture", 0); 
        ourShader->setVec3("objectColor", 0.5f, 0.5f, 0.5f); // Szary domyślny

        float currentTiling = 1.0f;

        // LOGIKA MATERIAŁÓW (Na podstawie nazw z Blendera)
        
        // 1. Opony i czarne elementy
        if(name.find("Black") != std::string::npos || name.find("black") != std::string::npos) {
            ourShader->setInt("useTexture", 1);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, tireTexture); // Tekstura opony

            currentTiling = 15.0f;
        }
        // 2. Karoseria (Główny materiał)
        else if(name.find("Material") != std::string::npos || name.find("Red") != std::string::npos) { 
            // Zakładam, że _10_Material to karoseria, a Red to też pomalujemy na karoserię
            ourShader->setInt("useTexture", 1);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, carTexture); // Zielony metal

            currentTiling = 2.0f;
        }
        // 3. Szyby (Szkło)
        else if(name.find("glass") != std::string::npos) {
            ourShader->setInt("useTexture", 0); // Bez tekstury, sam kolor
            // Półprzezroczysty niebieski (wymaga włączenia blendowania w OpenGL, na razie damy lity)
            ourShader->setVec3("objectColor", 0.2f, 0.3f, 0.5f); 
        }
        // 4. Reszta (Stal itp.)
        else {
             ourShader->setVec3("objectColor", 0.3f, 0.3f, 0.3f); // Ciemnoszary
        }

        ourShader->setFloat("tiling", currentTiling);

        // Narysuj tę konkretną część
        mesh.Draw(*ourShader);
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
    ourCar = new Model("models/car-6.obj");

    setupFloor();

    floorTexture = loadTexture("textures/floor.png");

    carTexture = loadTexture("textures/car_paint_1.jpg");
    tireTexture = loadTexture("textures/tire_texture.jpg");

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
    return 0;
}