#pragma once

#include <glm/glm.hpp>
#include <vector>

class Camera
{
public:
	Camera(float verticalFOV, float nearClip, float farClip);

	bool onUpdate(float ts);
	void onResize(uint32_t width, uint32_t height);

	const glm::mat4& getProjection() const { return m_Projection; }
	const glm::mat4& getInverseProjection() const { return m_InverseProjection; }
	const glm::mat4& getView() const { return m_View; }
	const glm::mat4& getInverseView() const { return m_InverseView; }
	
	const glm::vec3& getPosition() const { return m_Position; }
	const glm::vec3& getDirection() const { return m_ForwardDirection; }

	const std::vector<glm::vec3>& getRayDirections() const { return m_RayDirections; }

	float getRotationSpeed();
private:
	void recalculateProjection();
	void recalculateView();
	void recalculateRayDirections();
private:
	glm::mat4 m_Projection{ 1.0f };
	glm::mat4 m_View{ 1.0f };
	glm::mat4 m_InverseProjection{ 1.0f };
	glm::mat4 m_InverseView{ 1.0f };

	float m_VerticalFOV = 45.0f;
	float m_NearClip = 0.1f;
	float m_FarClip = 100.0f;

	glm::vec3 m_Position{0.0f, 0.0f, 0.0f};
	glm::vec3 m_ForwardDirection{0.0f, 0.0f, 0.0f};

	// Cached ray directions
	std::vector<glm::vec3> m_RayDirections;

	glm::vec2 m_LastMousePosition{ 0.0f, 0.0f };

	uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;
};