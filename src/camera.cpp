#include <camera.hpp>

// Constructor with vectors
Camera::Camera(glm::vec3 position, glm::vec3 up, glm::vec3 lookAt)
    : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY)
{
    Position = position;
    WorldUp = up;
    Yaw = glm::degrees(glm::atan(lookAt.x - position.x, position.z - lookAt.z)) - 90.f;
    Pitch = glm::degrees(glm::atan((lookAt.y - position.y)/(position.z - lookAt.z)));
    updateCameraVectors();
}

// Constructor with scalar values
Camera::Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch)
    : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY)
{
    Position = glm::vec3(posX, posY, posZ);
    WorldUp = glm::vec3(upX, upY, upZ);
    Yaw = yaw;
    Pitch = pitch;
    updateCameraVectors();
}

// Returns the view matrix using the LookAt Matrix
ml::matrix<float> Camera::GetViewMatrix()
{
    ml::matrix<float> orientation(4, 4, true);
    ml::matrix<float> translation(4, 4, true);

    orientation[0][0] = Right.x;
    orientation[0][1] = Right.y;
    orientation[0][2] = Right.z;

    orientation[1][0] = Up.x;
    orientation[1][1] = Up.y;
    orientation[1][2] = Up.z;
    
    orientation[2][0] = -Front.x;
    orientation[2][1] = -Front.y;
    orientation[2][2] = -Front.z;

    translation[0][3] = -Position.x;
    translation[1][3] = -Position.y;
    translation[2][3] = -Position.z;
    
    return (orientation * translation).transpose();
}

// Processes input received from any keyboard-like input system.
//Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
void Camera::ProcessKeyboard(Camera_Movement direction, float deltaTime)
{
    float velocity = MovementSpeed * deltaTime;
    if (direction == FORWARD)
        Position += Front * velocity;
    if (direction == BACKWARD)
        Position -= Front * velocity;
    if (direction == LEFT)
        Position -= Right * velocity;
    if (direction == RIGHT)
        Position += Right * velocity;
    if (direction == DOWN)
        Position -= Up * velocity;
    if (direction == UP)
        Position += Up * velocity;
}

// Processes input received from a mouse input system. Expects the offset value in both the x and y direction.
void Camera::ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch)
{
    xoffset *= MouseSensitivity;
    yoffset *= MouseSensitivity;

    Yaw   += xoffset;
    Pitch += yoffset;

    // Make sure that when pitch is out of bounds, screen doesn't get flipped
    if (constrainPitch)
    {
        if (Pitch > 89.0f)
            Pitch = 89.0f;
        if (Pitch < -89.0f)
            Pitch = -89.0f;
    }

    // Update Front, Right and Up Vectors using the updated Euler angles
    updateCameraVectors();
}

// Calculates the front vector from the Camera's (updated) Euler Angles
void Camera::updateCameraVectors()
{
    // Calculate the new Front vector
    glm::vec3 front;
    front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    front.y = sin(glm::radians(Pitch));
    front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    Front = glm::normalize(front);
    // Also re-calculate the Right and Up vector
    // Normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
    Right = glm::normalize(glm::cross(Front, WorldUp));
    Up    = glm::normalize(glm::cross(Right, Front));
}
