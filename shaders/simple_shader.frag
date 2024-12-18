#version 450

layout (location=0) in vec4 fragColor;
layout (location=1) in vec3 fragPosWorld;
layout (location=2) in vec3 fragNormalWorld;
layout (location=3) in vec2 fragUv;

layout(location = 0) out vec4 outColor;

struct PointLight {
	vec4 position;
	vec4 color;
};

layout(set = 0, binding = 0) uniform GlobalUbo {
	mat4 projectionMatrix;
	mat4 viewMatrix;
	mat4 inverseViewMatrix;
	vec4 ambientLightColor;
	PointLight pointLights[10];
	int numLights;
} ubo;

layout(set = 1, binding = 0) uniform GameObjectBufferData {
  mat4 modelMatrix;
  mat4 normalMatrix;
} gameObject;


layout (set = 1, binding = 1) uniform sampler2D diffuseMap;

layout(push_constant) uniform Push {
	mat4 modelMatrix;
	mat3 normalMatrix;
} push;

void main() { 
	vec3 diffuseLight = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w;
	vec3 specularLight = vec3(0.0);
	vec3 surfaceNormal = normalize(fragNormalWorld);

	vec3 cameraPosWorld = ubo.inverseViewMatrix[3].xyz;
	vec3 viewDirection = normalize(cameraPosWorld - fragPosWorld);

	for (int i = 0; i < ubo.numLights; i++) {
		PointLight light = ubo.pointLights[i];
		vec3 directionToLight = light.position.xyz - fragPosWorld;
		float attenuation = 1.0 / dot(directionToLight, directionToLight);
		directionToLight = normalize(directionToLight);
		float cosAngIncidence = max(dot(surfaceNormal, directionToLight), 0);
		vec3 intensity = light.color.xyz * light.color.w * attenuation;

		diffuseLight += intensity * cosAngIncidence;

		vec3 halfAngle = normalize(directionToLight + viewDirection);
		float blinnTerm = dot(surfaceNormal, halfAngle);
		blinnTerm = clamp(blinnTerm, 0, 1);
		blinnTerm = pow(blinnTerm, 32.0);
		specularLight += intensity * blinnTerm;
	}
	vec3 color = texture(diffuseMap, fragUv).xyz;

  	outColor = vec4(vec3(diffuseLight) * color, 1.0f) + vec4(specularLight, 1.0) * fragColor;
}