#shader vertex
#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
out vec2 texCoord;
out vec3 fragPosition;
out vec3 normal;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 lightColor;
//uniform vec3 lightPosition;

void main()
{
   gl_Position = projection * view * model * vec4(aPos, 1.0f);
   texCoord = vec2(aTexCoord.x, aTexCoord.y);
   fragPosition = vec3(model * vec4(aPos, 1.0f));
   normal = mat3(transpose(inverse(model))) * aNormal;
};

#shader fragment
#version 330 core

struct Material {
	sampler2D diffuse;
	sampler2D specular;
	float shininess;
};

struct DirectionalLight {
	vec3 direction;

	vec3 ambient; 
	vec3 diffuse;
	vec3 specular;
};

struct PointLight {
	vec3 position;

	vec3 ambient;
	vec3 diffuse;
	vec3 specular;

	float constant;
	float linear;
	float quadratic;
};

struct SpotLight {
	vec3 position;
	vec3 direction;
	float cutoff; // cos of cutoff angle for range of angles to be lit or not
	float outerCutoff; // also labeled as gamma in comments

	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

out vec4 fragmentColor;
in vec3 ourColor;
in vec2 texCoord;
in vec3 fragPosition;
in vec3 normal;

uniform vec3 viewPosition;
uniform Material material;

uniform DirectionalLight directionalLight;
uniform PointLight pointLight[2];
uniform SpotLight spotLight;

// GLSL function prototypes
// return type, function name, parameters
vec3 calcDirectionalLighting(DirectionalLight diLight, vec3 normalVec, vec3 viewDirection);
vec3 calcPointLighting(PointLight ptLight, vec3 normalVec, vec3 viewDirection, vec3 fragmentPosition); 
vec3 calcSpotLighting(SpotLight spotLight, vec3 normalVec, vec3 viewDirection, vec3 fragmentPosition);

vec3 calcDirectionalLighting(DirectionalLight diLight, vec3 normalVec, vec3 viewDirection) {
	vec3 lightDirection = normalize(-diLight.direction); // normalize the negative since we do calculations from perspective of light coming from camera
	float diffuseVal = max(dot(normalVec, lightDirection), 0.0); // handles diffuse directional light shading
	vec3 reflectionDirection = reflect(-lightDirection, normalVec);
	float specularVal = pow(max(dot(viewDirection, reflectionDirection), 0.0), 128); // handles specular directional light shading
	vec3 ambientPortion = diLight.ambient * vec3(texture(material.diffuse, texCoord));
	vec3 diffusePortion = diLight.diffuse * diffuseVal * vec3(texture(material.diffuse, texCoord));
	vec3 specularPortion = diLight.specular * specularVal * vec3(texture(material.specular, texCoord));
	return (ambientPortion + diffusePortion + specularPortion);
}

vec3 calcPointLighting(PointLight ptLight, vec3 normalVec, vec3 viewDirection, vec3 fragmentPosition) {
	vec3 lightDirection = normalize(ptLight.position - fragmentPosition);
	float diffuseVal = max(dot(normalVec, lightDirection), 0.0); // handles diffuse point light shading
	vec3 reflectionDirection = reflect(-lightDirection, normalVec);
	float specularVal = pow(max(dot(viewDirection, reflectionDirection), 0.0), 128); // handles specular point light shading
	// calc distance between the point light and the fragment, then calculate the attenuation coefficient using formula 1/(Kc + Kl*d + Kq*d*d)
	float ptLightDistance = length(ptLight.position - fragmentPosition);
	float attenuationVal = 1.0 / (ptLight.constant + ptLight.linear * ptLightDistance + ptLight.quadratic * ptLightDistance * ptLightDistance);
	vec3 ambientPortion = ptLight.ambient * vec3(texture(material.diffuse, texCoord));
	vec3 diffusePortion = ptLight.diffuse * diffuseVal * vec3(texture(material.diffuse, texCoord));
	vec3 specularPortion = ptLight.specular * specularVal * vec3(texture(material.specular, texCoord));
	ambientPortion *= attenuationVal;
	diffusePortion *= attenuationVal;
	specularPortion *= attenuationVal;
	return (ambientPortion + diffusePortion + specularPortion);
}

vec3 calcSpotLighting(SpotLight sptLight, vec3 normalVec, vec3 viewDirection, vec3 fragmentPosition) {
	vec3 lightDirection = normalize(sptLight.position - fragmentPosition);
	float theta = dot(lightDirection, normalize(-sptLight.direction)); // angle between direction the spotlight is pointing and direction to the current fragment, dot prod between the two
	float epsilon = sptLight.cutoff - sptLight.outerCutoff; // cutoffs MUST be different to avoid div by 0 errors
	float intensity = clamp((theta - sptLight.outerCutoff) / epsilon, 0.0, 1.0); // uses clamp to ensure intensity doesn't get outside the 0 to 1 inclusive range
	vec3 color = vec3(1.0, 1.0, 1.0);

	// if the light is within the cutoff range, perform lighting calcs, otherwise, use ambient lighting
	// > for comparison since the greater the angle the smaller the cos value
	if(theta > spotLight.cutoff) {
		float diffuseVal = max(dot(normalVec, lightDirection), 0.0);
		vec3 reflectionDirection = reflect(-lightDirection, normalVec);
		float specularVal = pow(max(dot(viewDirection, reflectionDirection), 0.0), 128);
		vec3 ambientPortion = sptLight.ambient * vec3(texture(material.diffuse, texCoord));
		vec3 diffusePortion = sptLight.diffuse * diffuseVal * vec3(texture(material.diffuse, texCoord));
		vec3 specularPortion = sptLight.specular * specularVal * vec3(texture(material.specular, texCoord));
		// for smooth fade out on edge of cone, uses intensity I = (theta - gamma) / (cutoff - gamma) multiplied by diffuse and specular portion of lighting
		diffusePortion *= intensity;
		specularPortion *= intensity;
		color = ambientPortion + diffusePortion + specularPortion;
	} else {
		color = sptLight.ambient * vec3(texture(material.diffuse, texCoord));
	}

	return color;
}

void main()
{
    vec3 normalVector = normalize(normal); // normalizing the provided normal vector
    vec3 normedViewDirection = normalize(viewPosition - fragPosition);

    // calculate each type of lighting, for as many lights as are provided of each
    // currently using one directional light, two point lights, and one spotlight
    vec3 finalColor = calcDirectionalLighting(directionalLight, normalVector, normedViewDirection);
    for(int i = 0; i < 2; i++) {
		finalColor += calcPointLighting(pointLight[i], normalVector, normedViewDirection, fragPosition);
    }
    finalColor += calcSpotLighting(spotLight, normalVector, normedViewDirection, fragPosition);
    fragmentColor = vec4(finalColor, 1.0);
};