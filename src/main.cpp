#include <glad/glad.h>
#include <GL/freeglut.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <vector>

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

// --- SHADERY ---
const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    void main() {
        gl_Position = projection * view * model * vec4(aPos, 1.0);
    }
)";

const char* fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;
    void main() {
        FragColor = vec4(0.4, 0.4, 0.4, 1.0); // Szara podłoga
    }
)";

unsigned int shaderProgram;
unsigned int VAO, VBO;

void setupShaders() {
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

void setupFloor() {
    // Podłoga 20x20
    float vertices[] = {
         -10.0f, 0.0f, -10.0f,
          10.0f, 0.0f, -10.0f,
         -10.0f, 0.0f,  10.0f,
          10.0f, 0.0f, -10.0f,
          10.0f, 0.0f,  10.0f,
         -10.0f, 0.0f,  10.0f
    };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
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

    glUseProgram(shaderProgram);

    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)windowWidth / (float)windowHeight, 0.1f, 100.0f);
    glm::mat4 model = glm::mat4(1.0f);

    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glutSwapBuffers();
    glutPostRedisplay();
}

void keyboardDown(unsigned char key, int x, int y) {
    if(key == 27) glutLeaveMainLoop();
    keys[key] = true;
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

    setupShaders();
    setupFloor();

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
    return 0;
}