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

    void Jump();
    bool IsGrounded() const { return m_isGrounded; }
    float GetVerticalVelocity() const { return m_verticalVelocity; }
    float GetGroundY() const { return m_groundY; }
    void SetGroundY(float groundY) { m_groundY = groundY; }
    void SetJumpSpeed(float jumpSpeed) { m_jumpSpeed = jumpSpeed; }
    void SetGravity(float gravity) { m_gravity = gravity; }
    const XMFLOAT3& GetPosition() const { return m_position; }

    bool UpdatePhysics(float deltaTime);
    bool Update(float deltaTime);

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

    // Physics / Jump
    float m_verticalVelocity;
    float m_jumpSpeed;
    float m_gravity;
    float m_groundY;
    bool m_isGrounded;

    void UpdateVectors();
};
