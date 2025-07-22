#include <vector>
#include "Body.h"

//Constructor for the struct Body
Body::Body(float x, float y, float vx, float vy, float m)
    : position_x(x), position_y(y), velocity_x(vx), velocity_y(vy), acceleration_x(0), acceleration_y(0), mass(m) {}

//Resets value of acceleration to 0
void Body::resetAcceleration() {
    acceleration_x = 0.0f;
    acceleration_y = 0.0f;
}
                       


