//Kernel to compute gravitational forces for all bodies
__kernel void compute_forces(
    __global float* pos_x, __global float* pos_y,
    __global float* acc_x, __global float* acc_y,
    __global float* mass,
    const unsigned int n,
    const float G,
    const float eps
){
    int i = get_global_id(0);

    float ax = 0.0f, ay = 0.0f;

    // Loop over all bodies to compute gravitational influence on body i
    for (int j = 0; j < n; ++j) {
        if (i == j) continue; // Skip self-interaction

        // Compute difference in position
        float dx = pos_x[j] - pos_x[i];
        float dy = pos_y[j] - pos_y[i];

        // Compute softened distance and its inverse cube
        float dist = sqrt(dx * dx + dy * dy + eps * eps);
        float inv_dist3 = 1.0f / (dist * dist * dist);

        // Newton's gravitational law: F = G * m / r^2 (used here with acceleration form)
        float f = G * mass[j] * inv_dist3;

        // Accumulate acceleration components
        ax += f * dx;
        ay += f * dy;
    }

    // Store the computed accelerations for body i
    acc_x[i] = ax;
    acc_y[i] = ay;

}

// Kernel to update body positions and velocities 
__kernel void integrate_bodies(
    __global float* pos_x, __global float* pos_y,   
    __global float* vel_x, __global float* vel_y,  
    __global float* acc_x, __global float* acc_y,   
    const float dt,                                  
    const float width,                              
    const float height                               
) {

    int i = get_global_id(0); 

    // Velocity update using acceleration
    vel_x[i] += acc_x[i] * dt;
    vel_y[i] += acc_y[i] * dt;

    // Position update using velocity
    pos_x[i] += vel_x[i] * dt;
    pos_y[i] += vel_y[i] * dt;

    // Toroidal wrapping 
    if (pos_x[i] < -width / 2)  pos_x[i] += width;
    if (pos_x[i] >  width / 2)  pos_x[i] -= width;
    if (pos_y[i] < -height / 2) pos_y[i] += height;
    if (pos_y[i] >  height / 2) pos_y[i] -= height;
}