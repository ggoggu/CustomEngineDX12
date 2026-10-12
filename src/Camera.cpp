#include "Camera.h"
#include <algorithm>

Camera::Camera()
    : m_position(0.0f, 0.0f, -5.0f),
      m_pitch(0.0f),
      m_yaw(0.0f),
      m_moveSpeed(5.0f),
      m_rotationSpeed(0.1f),
      m_mouseSensitivity(0.15f),
      m_verticalVelocity(0.0f),
      m_jumpSpeed(7.0f),
      m_gravity(20.0f),
      m_groundY(0.0f),
      m_isGrounded(true)
{
    UpdateVectors();
}

void Camera::SetPosition(float x, float y, float z)
{
    m_position = { x, y, z };
    m_groundY = y;
    m_verticalVelocity = 0.0f;
    m_isGrounded = true;
}

void Camera::SetRotation(float pitch, float yaw)
{
    m_pitch = pitch;
    m_yaw = yaw;
    UpdateVectors();
}

void Camera::Rotate(float pitchDelta, float yawDelta)
{
    m_pitch += pitchDelta;
    m_yaw += yawDelta;

    // Clamp pitch
    if (m_pitch > 89.0f) m_pitch = 89.0f;
    if (m_pitch < -89.0f) m_pitch = -89.0f;

    // Normalize yaw within [-360.0f, 360.0f]
    while (m_yaw > 360.0f) m_yaw -= 360.0f;
    while (m_yaw < -360.0f) m_yaw += 360.0f;

    UpdateVectors();
}

bool Camera::ProcessMouseLook(int deltaX, int deltaY)
{
    if (deltaX == 0 && deltaY == 0)
    {
        return false;
    }

    float yawDelta = static_cast<float>(deltaX) * m_mouseSensitivity;
    float pitchDelta = static_cast<float>(deltaY) * m_mouseSensitivity;

    Rotate(pitchDelta, yawDelta);
    return true;
}

void Camera::Jump()
{
    if (m_isGrounded)
    {
        m_verticalVelocity = m_jumpSpeed;
        m_isGrounded = false;
    }
}

bool Camera::UpdatePhysics(float deltaTime)
{
    if (m_isGrounded && m_verticalVelocity == 0.0f)
    {
        return false;
    }

    float dt = (deltaTime > 0.1f) ? 0.1f : deltaTime;
    m_position.y += m_verticalVelocity * dt;
    m_verticalVelocity -= m_gravity * dt;

    if (m_position.y <= m_groundY)
    {
        m_position.y = m_groundY;
        m_verticalVelocity = 0.0f;
        m_isGrounded = true;
    }

    return true;
}

void Camera::UpdateVectors()
{
    float pitch = XMConvertToRadians(m_pitch);
    float yaw = XMConvertToRadians(m_yaw);

    XMMATRIX rotation = XMMatrixRotationRollPitchYaw(pitch, yaw, 0.0f);
    XMVECTOR forward = XMVector3TransformCoord(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), rotation);
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMVECTOR right = XMVector3Cross(up, forward);
    up = XMVector3Cross(forward, right);

    XMStoreFloat3(&m_forward, XMVector3Normalize(forward));
    XMStoreFloat3(&m_right, XMVector3Normalize(right));
    XMStoreFloat3(&m_up, XMVector3Normalize(up));
}

bool Camera::Update(float deltaTime, int mouseDeltaX, int mouseDeltaY)
{
    bool moved = false;
    XMVECTOR pos = XMLoadFloat3(&m_position);
    XMVECTOR forward = XMLoadFloat3(&m_forward);
    XMVECTOR right = XMLoadFloat3(&m_right);

    // Keep horizontal movement on the XZ plane
    XMVECTOR forwardXZ = XMVectorSet(m_forward.x, 0.0f, m_forward.z, 0.0f);
    if (XMVectorGetX(XMVector3LengthSq(forwardXZ)) > 0.0001f)
    {
        forwardXZ = XMVector3Normalize(forwardXZ);
    }
    else
    {
        forwardXZ = forward;
    }

    XMVECTOR rightXZ = XMVectorSet(m_right.x, 0.0f, m_right.z, 0.0f);
    if (XMVectorGetX(XMVector3LengthSq(rightXZ)) > 0.0001f)
    {
        rightXZ = XMVector3Normalize(rightXZ);
    }
    else
    {
        rightXZ = right;
    }

    float currentY = m_position.y;

    if (GetAsyncKeyState('W') & 0x8000) { pos += forwardXZ * m_moveSpeed * deltaTime; moved = true; }
    if (GetAsyncKeyState('S') & 0x8000) { pos -= forwardXZ * m_moveSpeed * deltaTime; moved = true; }
    if (GetAsyncKeyState('D') & 0x8000) { pos += rightXZ * m_moveSpeed * deltaTime; moved = true; }
    if (GetAsyncKeyState('A') & 0x8000) { pos -= rightXZ * m_moveSpeed * deltaTime; moved = true; }

    XMStoreFloat3(&m_position, pos);
    m_position.y = currentY; // Ensure horizontal movement doesn't overwrite vertical position

    // Spacebar triggers jump
    if (GetAsyncKeyState(VK_SPACE) & 0x8000)
    {
        if (m_isGrounded)
        {
            Jump();
            moved = true;
        }
    }

    // Advance jump & gravity simulation
    if (UpdatePhysics(deltaTime))
    {
        moved = true;
    }

    // Mouse look rotation
    if (mouseDeltaX != 0 || mouseDeltaY != 0)
    {
        m_yaw += static_cast<float>(mouseDeltaX) * m_mouseSensitivity;
        m_pitch += static_cast<float>(mouseDeltaY) * m_mouseSensitivity;
        moved = true;
    }

    // Camera rotation with arrow keys
    if (GetAsyncKeyState(VK_UP) & 0x8000)   { m_pitch -= m_rotationSpeed * 100.0f * deltaTime; moved = true; }
    if (GetAsyncKeyState(VK_DOWN) & 0x8000) { m_pitch += m_rotationSpeed * 100.0f * deltaTime; moved = true; }
    if (GetAsyncKeyState(VK_RIGHT) & 0x8000){ m_yaw += m_rotationSpeed * 100.0f * deltaTime; moved = true; }
    if (GetAsyncKeyState(VK_LEFT) & 0x8000) { m_yaw -= m_rotationSpeed * 100.0f * deltaTime; moved = true; }

    // Clamp pitch
    if (m_pitch > 89.0f) m_pitch = 89.0f;
    if (m_pitch < -89.0f) m_pitch = -89.0f;

    // Normalize yaw within [-360.0f, 360.0f]
    while (m_yaw > 360.0f) m_yaw -= 360.0f;
    while (m_yaw < -360.0f) m_yaw += 360.0f;

    if (moved)
    {
        UpdateVectors();
    }
    
    return moved;
}

XMMATRIX Camera::GetViewMatrix() const
{
    XMVECTOR pos = XMLoadFloat3(&m_position);
    XMVECTOR forward = XMLoadFloat3(&m_forward);
    XMVECTOR up = XMLoadFloat3(&m_up);
    return XMMatrixLookAtLH(pos, pos + forward, up);
}

XMMATRIX Camera::GetProjectionMatrix(float aspectRatio) const
{
    return XMMatrixPerspectiveFovLH(XM_PIDIV4, aspectRatio, 0.1f, 100.0f);
}
