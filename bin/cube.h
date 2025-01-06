#ifndef CUBE_H
#define CUBE_H

#include <glm/glm.hpp>

class Cube {
public:
    Cube(float startX, float startZ, float startRotation, float startSpeed);

    void moveForward();
    void moveBackward();
    void rotateLeft();
    void rotateRight();

    void getX();
    float getZ() const;
    float getRotation() const;
    float getSpeed() const;

    void setX(float newX);
    void setZ(float newZ);
    void setRotation(float newRotation);

private:
    float x, z, rotation, speed;
};

#endif


#endif 