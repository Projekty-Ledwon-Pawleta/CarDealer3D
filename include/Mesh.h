#ifndef MESH_H
#define MESH_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Shader.h"

#include <string>
#include <vector>

// Struktura reprezentująca jeden wierzchołek
struct Vertex {
    glm::vec3 Position;  // Gdzie jest?
    glm::vec3 Normal;    // W którą stronę patrzy ściana? (do oświetlenia)
    glm::vec2 TexCoords; // Jak nałożyć teksturę?
};

// Struktura reprezentująca teksturę
struct Texture {
    unsigned int id;
    std::string type; // np. "texture_diffuse"
    std::string path; // ścieżka do pliku
};

class Mesh {
public:
    // Dane siatki
    std::vector<Vertex>       vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture>      textures;
    unsigned int VAO;

    std::string materialName;

    // Konstruktor
    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures, std::string name = "") {
        this->vertices = vertices;
        this->indices = indices;
        this->textures = textures;
        this->materialName = name;

        // Teraz ustawiamy bufory (to co robiłeś ręcznie w setupFloor, tutaj dzieje się automagicznie)
        setupMesh();
    }

    // Funkcja rysująca siatkę
    void Draw(Shader &shader) {
        // Obsługa tekstur
        unsigned int diffuseNr  = 1;
        unsigned int specularNr = 1;

        for(unsigned int i = 0; i < textures.size(); i++) {
            glActiveTexture(GL_TEXTURE0 + i); // aktywuj odpowiednią jednostkę tekstur
            
            // Pobieramy nazwę i numer (np. texture_diffuse1)
            std::string number;
            std::string name = textures[i].type;
            if(name == "texture_diffuse")
                number = std::to_string(diffuseNr++);
            else if(name == "texture_specular")
                number = std::to_string(specularNr++); // to przyda się później do błysku (specular)

            // Ustawiamy w shaderze (np. material.texture_diffuse1)
            shader.setInt(("material." + name + number).c_str(), i);
            glBindTexture(GL_TEXTURE_2D, textures[i].id);
        }
        
        // Rysowanie
        glBindVertexArray(VAO);
        // Uwaga: używamy glDrawElements (z indeksami), a nie glDrawArrays!
        glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        // Reset
        glActiveTexture(GL_TEXTURE0);
    }

private:
    unsigned int VBO, EBO;

    void setupMesh() {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);
        
        // Wrzucamy wierzchołki
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

        // Wrzucamy indeksy (EBO - Element Buffer Object) - wymaganie projektowe!
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

        // 1. Pozycja (location = 0)
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        
        // --- ZMIANA KOLEJNOŚCI TUTAJ ---

        // 2. Tekstury (location = 1) - Żeby pasowało do Shadera i Podłogi!
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));

        // 3. Normalne (location = 2) - Przesuwamy na później (przydadzą się do światła)
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));

        glBindVertexArray(0);
    }
};
#endif