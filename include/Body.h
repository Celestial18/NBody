#ifndef Body_H
#define Body_H

/*Define properties for Body*/
//AOS version for cPU
struct Body{
    float position_x, position_y; //Position of Body
    float velocity_x, velocity_y; //Velocity of Body
    float acceleration_x, acceleration_y; //Acceleration of Body
    float mass; //Mass of Body

    // Constructor
    Body(float x, float y, float vx, float vy, float m);

    // Function to reset acceleration
    void resetAcceleration();
    
 };

 //SOA version for OpenCL
 struct BodySOA {
    std::vector<float> position_x, position_y;
    std::vector<float> velocity_x, velocity_y;
    std::vector<float> acceleration_x, acceleration_y;
    std::vector<float> mass;

    size_t size() const { return mass.size(); }

 };

 #endif
