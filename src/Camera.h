#pragma once
#define NOMINMAX
#include <DirectXMath.h>
#include <Windows.h>

using namespace DirectX;

class Camera {
public:
    Camera();
    ~Camera() = default;

    void SetPosition(float x, float y, float z);
    void SetRotation(float pitch, float yaw);
    void Rotate(float pitchDelta, float yawDelta);
    bool ProcessMouseLook(int deltaX, int deltaY);

    void Jump();
    bool IsGrounded() const { return m_isGrounded; }
    float GetVerticalVelocity() const { return m_verticalVelocity; }
    float GetGroundY() const { return m_groundY; }
    void SetGroundY(float groundY) { m_groundY = groundY; }
    void SetJumpSpeed(float jumpSpeed) { m_jumpSpeed = jumpSpeed; }
    void SetGravity(float gravity) { m_gravity = gravity; }
    void SetMouseSensitivity(float sensitivity) { m_mouseSensitivity = sensitivity; }
    float GetMouseSensitivity() const { return m_mouseSensitivity; }
    float GetPitch() const { return m_pitch; }
    float GetYaw() const { return m_yaw; }
    const XMFLOAT3& GetPosition() const { return m_position; }
    const XMFLOAT3& GetForward() const { return m_forward; }
    const XMFLOAT3& GetRight() const { return m_right; }
    const XMFLOAT3& GetUp() const { return m_up; }

    bool UpdatePhysics(float deltaTime);
    bool Update(float deltaTime, int mouseDeltaX = 0, int mouseDeltaY = 0);

    XMMATRIX GetViewMatrix() const;
    XMMATRIX GetProjectionMatrix(float aspectRatio) const;

private:
    XMFLOAT3 m_position;
    XMFLOAT3 m_right;
    XMFLOAT3 m_up;
    XMFLOAT3 m_forward;

    float m_pitch;
    float m_yaw;
    float m_moveSpeed;
    float m_rotationSpeed;
    float m_mouseSensitivity;

    // Physics / Jump
    float m_verticalVelocity;
    float m_jumpSpeed;
    float m_gravity;
    float m_groundY;
    bool m_isGrounded;

    void UpdateVectors();
};
