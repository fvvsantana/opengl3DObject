#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec3 LightingColor; // resulting color from lighting calculations
out vec2 TexCoords;

struct PointLight {
    vec3 position;
    
    float constant;
    float linear;
    float quadratic;
    
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

#define MAX_POINT_LIGHT_NUMBER 100

uniform vec3 viewPos;
uniform PointLight pointLights[MAX_POINT_LIGHT_NUMBER];
uniform int numberOfPointLights;

uniform float shininess; //ns

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// function prototypes
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 pos, vec3 viewDir, float ka, float kd, float ks);

void main()
{
    
    TexCoords = aTexCoords;
    gl_Position = projection * view * model * vec4(aPos, 1.0);


    vec3 pos = vec3(model * vec4(aPos, 1.0));
    vec3 norm = normalize(mat3(transpose(inverse(model))) * aNormal);
    vec3 viewDir = normalize(viewPos - pos);

    // properties
    float ka = 0.01;
    float kd = 0.1;
    float ks = 0.1;
    
    // sum the point lights contribution
    vec3 result = vec3(0.0);
    if(numberOfPointLights == 0){
        //result = vec3(1.0, 0.5, 0.31);
        result = vec3(1.0);

    }else{
        for(int i = 0; i < numberOfPointLights; i++){
            result += CalcPointLight(pointLights[i], norm, pos, viewDir, ka, ks, kd);    
        }
    }
    
    LightingColor = result;
}

// calculates the color when using a point light.
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 pos, vec3 viewDir, float ka, float kd, float ks){
    // L (Light vector)
    vec3 lightDir = normalize(light.position - pos);
    // N * L (diffuse shading)
    float diff = max(dot(normal, lightDir), 0.0);
    // R (reflection vector)
    vec3 reflectDir = reflect(-lightDir, normal);
    // (N * R)^ns (specular shading)
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);

    // calculate the distance between the light and the fragment
    float distance = length(light.position - pos);
    // calculate attenuation
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));    

    // combine results
    vec3 ambient = ka * light.ambient;
    vec3 diffuse = kd * light.diffuse * diff;
    vec3 specular = ks * light.specular * spec;

    return (ambient + diffuse + specular) * attenuation;
}

