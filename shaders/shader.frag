#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

uniform sampler2D texture1;
uniform vec3 objectColor;
uniform int useTexture;

uniform vec3 lightPos;  // Pozycja światła
uniform vec3 viewPos;   // Pozycja kamery (do błysku)
uniform vec3 lightColor;

void main() {
    // 1. AMBIENT (Światło otoczenia)
    // Stałe, słabe światło, żeby cienie nie były idealnie czarne
    float ambientStrength = 0.4;
    vec3 ambient = ambientStrength * lightColor;
  	
    // 2. DIFFUSE (Światło rozproszone - TO JEST DYNAMICZNE OŚWIETLENIE)
    // Obliczamy kąt między normalną ściany a kierunkiem do światła
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0); // Jeśli kąt > 90 stopni, to 0 (cień)
    vec3 diffuse = diff * lightColor;
    
    // 3. SPECULAR (Błysk / Odblask)
    // Obliczamy odbicie światła w stronę kamery
    float specularStrength = 0.8; // Siła błysku (dla aut wysoka)
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);  
    // 32 to "shininess" - im wyższa liczba, tym mniejszy i ostrzejszy punkt światła
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32); 
    vec3 specular = specularStrength * spec * lightColor;  
        
    // Sumujemy składniki światła
    vec3 lighting = (ambient + diffuse + specular);

    // Pobieramy kolor obiektu (z tekstury lub koloru)
    vec4 baseColor;
    if(useTexture == 1) {
        baseColor = texture(texture1, TexCoord);
    } else {
        baseColor = vec4(objectColor, 1.0);
    }

    // Mnożymy światło * kolor
    FragColor = vec4(lighting, 1.0) * baseColor;
}