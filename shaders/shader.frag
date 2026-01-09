#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

uniform sampler2D texture1;
uniform vec3 objectColor;
uniform int useTexture;

// --- OŚWIETLENIE 1 (Główne - Białe) ---
uniform vec3 lightPos;  
uniform vec3 lightColor;

// --- OŚWIETLENIE 2 (Dodatkowe - Kolorowe) ---
uniform vec3 lightPos2;  
uniform vec3 lightColor2;

uniform vec3 viewPos; // Kamera

void main() {
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    // ==================================================
    // 1. ŚWIATŁO OTOCZENIA (AMBIENT) - Wspólne dla sceny
    // ==================================================
    float ambientStrength = 0.1;
    // Ambient bierzemy tylko z pierwszego światła, żeby nie było za jasno
    vec3 ambient = ambientStrength * lightColor; 

    // ==================================================
    // 2. OBLICZENIA DLA LAMPY NR 1
    // ==================================================
    // Diffuse 1
    vec3 lightDir1 = normalize(lightPos - FragPos);
    float diff1 = max(dot(norm, lightDir1), 0.0);
    vec3 diffuse1 = diff1 * lightColor;

    // Specular 1
    vec3 reflectDir1 = reflect(-lightDir1, norm);  
    float spec1 = pow(max(dot(viewDir, reflectDir1), 0.0), 32);
    vec3 specular1 = 0.8 * spec1 * lightColor;

    // ==================================================
    // 3. OBLICZENIA DLA LAMPY NR 2 (Nowa lampa)
    // ==================================================
    // Diffuse 2
    vec3 lightDir2 = normalize(lightPos2 - FragPos);
    float diff2 = max(dot(norm, lightDir2), 0.0);
    vec3 diffuse2 = diff2 * lightColor2; // Używamy koloru 2

    // Specular 2
    vec3 reflectDir2 = reflect(-lightDir2, norm);
    float spec2 = pow(max(dot(viewDir, reflectDir2), 0.0), 32);
    vec3 specular2 = 0.8 * spec2 * lightColor2; // Używamy koloru 2

    // ==================================================
    // SUMOWANIE WYNIKÓW
    // ==================================================
    // Dodajemy wpływ obu świateł do siebie
    vec3 resultLighting = ambient + (diffuse1 + specular1) + (diffuse2 + specular2);

    // Pobranie koloru obiektu
    vec4 baseColor;
    if(useTexture == 1) {
        baseColor = texture(texture1, TexCoord);
    } else {
        baseColor = vec4(objectColor, 1.0);
    }

    FragColor = vec4(resultLighting, 1.0) * baseColor;
}