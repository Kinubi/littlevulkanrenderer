#include "keyboard_movement_controller.h"

#include <cmath>
#include <glm/ext/vector_float3.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
#include <limits>

namespace lvr {
void KeyboardMovementController::moveInPlaneXZ(
	GLFWwindow* window, float dt, GameObject& gameObject) {
	glm::vec3 rotate{0};
	if (glfwGetKey(window, keys.lookRight) == GLFW_PRESS) rotate.y += 1.0f;
	if (glfwGetKey(window, keys.lookLeft) == GLFW_PRESS) rotate.y -= 1.0f;
	if (glfwGetKey(window, keys.lookUp) == GLFW_PRESS) rotate.x += 1.0f;
	if (glfwGetKey(window, keys.lookDown) == GLFW_PRESS) rotate.x -= 1.0f;

	if (glm::dot(rotate, rotate) > std::numeric_limits<float>::epsilon()) {
		gameObject.tranform.rotation += lookSpeed * dt * glm::normalize(rotate);
	}

	gameObject.tranform.rotation.x = glm::clamp(gameObject.tranform.rotation.x, -1.5f, 1.5f);
	gameObject.tranform.rotation.y = glm::mod(gameObject.tranform.rotation.y, glm::two_pi<float>());

	float yaw = gameObject.tranform.rotation.y;
	const glm::vec3 forwardDir(sin(yaw), 0.0f, cos(yaw));
	const glm::vec3 righDir{forwardDir.z, 0.0f, forwardDir.x};

	const glm::vec3 upDir{0.0f, -1.0f, 0.0f};

	glm::vec3 moveDir{0.0f};
	if (glfwGetKey(window, keys.moveForward) == GLFW_PRESS) moveDir += forwardDir;
	if (glfwGetKey(window, keys.moveBackward) == GLFW_PRESS) moveDir -= forwardDir;
	if (glfwGetKey(window, keys.moveUp) == GLFW_PRESS) moveDir += upDir;
	if (glfwGetKey(window, keys.moveDown) == GLFW_PRESS) moveDir -= upDir;
	if (glfwGetKey(window, keys.moveRight) == GLFW_PRESS) moveDir += righDir;
	if (glfwGetKey(window, keys.moveLeft) == GLFW_PRESS) moveDir -= righDir;

	if (glm::dot(moveDir, moveDir) > std::numeric_limits<float>::epsilon()) {
		gameObject.tranform.translation += moveSpeed * dt * glm::normalize(moveDir);
	}
}
}  // namespace lvr