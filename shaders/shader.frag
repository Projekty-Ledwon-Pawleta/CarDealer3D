#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

uniform sampler2D texture1;
uniform vec3 objectColor;
uniform int useTexture;

// Parametr połysku (nowy!)
uniform float shine; // <<< ZMIANA: Dodajemy zmienną sterującą połyskiem

// Oświetlenie 1
uniform vec3 lightPos;  
uniform vec3 lightColor;

// 
uniform vec3 lightPos2;  
uniform vec3 lightColor2;
uniform vec3 lightDir2;    
uniform float lightCutoff; 

// Oświetlenie 3 (Lampa w pokoju)
uniform vec3 lightPos3;
uniform vec3 lightColor3;

uniform vec3 viewPos; 

void main() {
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    // 1. Ambient
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * lightColor;

    // 2. Światło główne
    vec3 lightDir1 = normalize(lightPos - FragPos);
    float diff1 = max(dot(norm, lightDir1), 0.0);
    vec3 diffuse1 = diff1 * lightColor;

    vec3 reflectDir1 = reflect(-lightDir1, norm);  
    float spec1 = pow(max(dot(viewDir, reflectDir1), 0.0), 32);
    // Używamy zmiennej 'shine' zamiast 0.8
    vec3 specular1 = shine * spec1 * lightColor; // <<< ZMIANA

    // 3. Latarka
    vec3 lightDir2Calc = normalize(lightPos2 - FragPos);
    float theta = dot(lightDir2Calc, normalize(-lightDir2));
    float outerCutoff = lightCutoff - 0.08;
    float epsilon = lightCutoff - outerCutoff;
    float intensity = clamp((theta - outerCutoff) / epsilon, 0.0, 1.0);

    float diff2 = max(dot(norm, lightDir2Calc), 0.0);
    vec3 diffuse2 = diff2 * lightColor2 * intensity;

    vec3 reflectDir2 = reflect(-lightDir2Calc, norm);
    float spec2 = pow(max(dot(viewDir, reflectDir2), 0.0), 32);
    // Używamy zmiennej 'shine' zamiast 0.8
    vec3 specular2 = shine * spec2 * lightColor2 * intensity; // <<< ZMIANA

    float distance = length(lightPos2 - FragPos);
    float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * distance * distance);
    diffuse2 *= attenuation;
    specular2 *= attenuation;

    // 4. Lampa w pokoju
    float distance3 = length(lightPos3 - FragPos);
    
    // Parametry (1.0, 0.14, 0.07) sprawiają, że światło gaśnie na dystansie ok. 15-20 metrów.
    float attenuation3 = 1.0 / (1.0 + 0.045 * distance3 + 0.0075 * distance3 * distance3);
    
    vec3 lightDir3 = normalize(lightPos3 - FragPos);
    float diff3 = max(dot(norm, lightDir3), 0.0);
    vec3 diffuse3 = diff3 * lightColor3 * attenuation3;

    vec3 reflectDir3 = reflect(-lightDir3, norm);
    float spec3 = pow(max(dot(viewDir, reflectDir3), 0.0), 32);
    vec3 specular3 = shine * spec3 * lightColor3 * attenuation3;

    vec3 resultLighting = ambient + (diffuse1 + specular1) + (diffuse2 + specular2) + (diffuse3 + specular3);

    vec4 baseColor;
    if(useTexture == 1) {
        baseColor = texture(texture1, TexCoord);
    } else {
        baseColor = vec4(objectColor, 1.0);
    }

    FragColor = vec4(resultLighting, 1.0) * baseColor;
}