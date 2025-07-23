/*
* @author Salina Maharjan
* @date 2025-07-22
* To compile your source code, please use the following command:
*  mkdir build
*  cd build
*  cmake ..
*  make
*  cd ..
*  ./build/nbody
*/
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <SFML/Graphics.hpp> 
#include <random>
#include <chrono>
#include "include/utils.h"
#include "include/Body.h"

constexpr float G = 1.f;
constexpr float dt = .1f;
constexpr float eps = 1e-1f;
constexpr unsigned int n_bodies = 20;
constexpr float center_mass = 1000.f;
constexpr float TARGET_FPS = 165.f;
const sf::Time FRAME_DURATION = sf::seconds(1.f / TARGET_FPS);

void compute_forces(std::vector<Body>& bodies, float G, float eps){

    //Reset all acceleration to 0 
     for (auto& body : bodies) {
        body.acceleration_x = 0.0f;
        body.acceleration_y = 0.0f;
    }
    // Compute gravitational acceleration of all other bodies

    for (size_t i = 0; i < bodies.size(); ++i){
        for (size_t j = 0; j < bodies.size(); ++j){
            if (i == j) continue;
            
            //Calculate distance in x and y to the other body.
            float dx = bodies[j].position_x - bodies[i].position_x;
            float dy = bodies[j].position_y - bodies[i].position_y;

            //Calculate the cubed inverse distance, including the softening factor eps.
            float dist = std::sqrt(dx * dx + dy * dy + eps * eps);
            float inv_dist3 = 1.f / (dist * dist * dist);

            //Calculate the force applied from body Bj to body Bi
            float f = G * bodies[j].mass * inv_dist3;

            //Sum und update the acceleration inboth directions for body Bi
            bodies[i].acceleration_x += f * dx;
            bodies[i].acceleration_y += f * dy; 

        }
    }

}

void integrate_bodies(std::vector<Body>& bodies, float dt, float width, float height){

    for (auto& body : bodies) {

        //Update velocity of all bodies
        body.velocity_x = body.velocity_x + body.acceleration_x * dt;
        body.velocity_y = body.velocity_y + body.acceleration_y * dt;

        //Update position of all bodies
        body.position_x = body.position_x + body.velocity_x * dt;
        body.position_y = body.position_y + body.velocity_y * dt;

        // Toroidal wrapping
        if (body.position_x < -width / 2) body.position_x += width;
        if (body.position_x >  width / 2) body.position_x -= width;
        if (body.position_y < -height / 2) body.position_y += height;
        if (body.position_y >  height / 2) body.position_y -= height;
    }

}

// Helper function to assign color based on mass
sf::Color mass_to_color(float m) {
    float norm = std::min(1.0f, m / 10.0f);
    return sf::Color(255 * norm, 50, 255 * (1- norm));
}

float orbital_velocity_scalar(float M, float r) {
    return std::sqrt(M / r);  // G = 1.0 assumed
}

// Initialze the bodies randomly in the universe
std::vector<Body> initialize_bodies(unsigned int n, float center_mass, float width, float height){
    std::vector<Body> bodies;
    bodies.reserve(n);  // Reserve space for efficiency

    //random generator
    std::mt19937 rng(42);  // Fixed seed for reproducibility
    std::uniform_real_distribution<float> angle_dist(0.0f, 2.0f * M_PI);
    std::uniform_real_distribution<float> radius_dist(50.0f, std::min(width, height) / 2.f - 20.f);
    std::uniform_real_distribution<float> mass_dist(0.5f, 10.f);

    // Add central massive body at (0, 0)
    bodies.push_back(Body(0, 0, 0, 0, center_mass));

    // Add orbiting bodies around the center
    for (unsigned int i = 0; i < n - 1; ++i) {
        float angle = angle_dist(rng);
        float radius = radius_dist(rng);
        float mass = mass_dist(rng);

        float x = std::cos(angle) * radius;
        float y = std::sin(angle) * radius;

        float speed = orbital_velocity_scalar(center_mass, radius);
        float vx = -std::sin(angle) * speed;
        float vy = std::cos(angle) * speed;

        bodies.push_back(Body(x, y, vx, vy, mass));
    }

    return bodies;

}

// Main Loop 
int main(){
    // Window properties
    //constexpr can be evaluated at compile time 
    constexpr int WIDTH = 2560;
    constexpr int HEIGHT = 1440;

    bool use_gpu = gpu_available(); //Change between OpenCL and CPU

    std::vector<Body> bodies = initialize_bodies(n_bodies, center_mass, static_cast<float>(WIDTH), static_cast<float>(HEIGHT));

    size_t n = bodies.size();

    std::vector<float> pos_x(n), pos_y(n), vel_x(n), vel_y(n), acc_x(n), acc_y(n), mass(n);
    
    for (size_t i = 0; i < n; ++i) {
        pos_x[i] = bodies[i].position_x;
        pos_y[i] = bodies[i].position_y;
        vel_x[i] = bodies[i].velocity_x;
        vel_y[i] = bodies[i].velocity_y;
        acc_x[i] = 0.0f;
        acc_y[i] = 0.0f;
        mass[i] = bodies[i].mass;
    }

    cl_int err;
    cl_mem buf_px, buf_py, buf_vx, buf_vy, buf_ax, buf_ay, buf_m;

    if (use_gpu){
        // Initialize OpenCL once at startup
        init_opencl("../opencl/NBody.cl");

        // Create OpenCL buffers

        buf_px = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, n * sizeof(float), pos_x.data(), &err);
        buf_py = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, n * sizeof(float), pos_y.data(), &err);
        buf_vx = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, n * sizeof(float), vel_x.data(), &err);
        buf_vy = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, n * sizeof(float), vel_y.data(), &err);
        buf_ax = clCreateBuffer(context, CL_MEM_READ_WRITE, n * sizeof(float), nullptr, &err);
        buf_ay = clCreateBuffer(context, CL_MEM_READ_WRITE, n * sizeof(float), nullptr, &err);
        buf_m  = clCreateBuffer(context, CL_MEM_READ_ONLY  | CL_MEM_COPY_HOST_PTR, n * sizeof(float), mass.data(), &err);
    }
   

    // Create the SFML window
    sf::RenderWindow window(sf::VideoMode(WIDTH, HEIGHT), "N-Body Simulation");

    // Load the font
    sf::Font font;
    if (!font.loadFromFile("OpenSans-Bold.ttf")) {
    std::cerr << "Failed to load font\n";
    }

    // Prepare the FPS text display
    sf::Text fpsText("", font, 18);               // Start with empty string, use the font and size 18
    fpsText.setFillColor(sf::Color::White);       // Set the text color
    fpsText.setPosition(10, 5);                   // Top-left corner

    // Clock for frame timing and FPS measurement
    sf::Clock frameClock;
    sf::Clock fpsClock;
    float lastFPS = 0.0f;
    int frame_count = 0;

    // Start total timer
    const auto total_start = std::chrono::high_resolution_clock::now();

    while (window.isOpen() && frame_count < 10000){
        sf::Event e;
        while(window.pollEvent(e)){ if (e.type == sf::Event::Closed) window.close();}
       
        const auto compute_start = std::chrono::high_resolution_clock::now();

        if (use_gpu){

            const unsigned int n_uint = static_cast<unsigned int>(n); 
            const float width_f = static_cast<float>(WIDTH);  
            const float height_f = static_cast<float>(HEIGHT); 

            clSetKernelArg(kernel_force, 0, sizeof(cl_mem), &buf_px);
            clSetKernelArg(kernel_force, 1, sizeof(cl_mem), &buf_py);
            clSetKernelArg(kernel_force, 2, sizeof(cl_mem), &buf_ax);
            clSetKernelArg(kernel_force, 3, sizeof(cl_mem), &buf_ay);
            clSetKernelArg(kernel_force, 4, sizeof(cl_mem), &buf_m);
            clSetKernelArg(kernel_force, 5, sizeof(unsigned int), &n);
            clSetKernelArg(kernel_force, 6, sizeof(float), &G);
            clSetKernelArg(kernel_force, 7, sizeof(float), &eps);

            const size_t global_size = n;
            clEnqueueNDRangeKernel(queue, kernel_force, 1, nullptr, &global_size, nullptr, 0, nullptr, nullptr);

            // Run integrate_bodies kernel
            clSetKernelArg(kernel_integrate, 0, sizeof(cl_mem), &buf_px);
            clSetKernelArg(kernel_integrate, 1, sizeof(cl_mem), &buf_py);
            clSetKernelArg(kernel_integrate, 2, sizeof(cl_mem), &buf_vx);
            clSetKernelArg(kernel_integrate, 3, sizeof(cl_mem), &buf_vy);
            clSetKernelArg(kernel_integrate, 4, sizeof(cl_mem), &buf_ax);
            clSetKernelArg(kernel_integrate, 5, sizeof(cl_mem), &buf_ay);
            clSetKernelArg(kernel_integrate, 6, sizeof(float), &dt);
            clSetKernelArg(kernel_integrate, 7, sizeof(float), &WIDTH);
            clSetKernelArg(kernel_integrate, 8, sizeof(float), &HEIGHT);

            clEnqueueNDRangeKernel(queue, kernel_integrate, 1, nullptr, &global_size, nullptr, 0, nullptr, nullptr);
            
            // Read positions back for rendering
            clEnqueueReadBuffer(queue, buf_px, CL_TRUE, 0, n * sizeof(float), pos_x.data(), 0, nullptr, nullptr);
            clEnqueueReadBuffer(queue, buf_py, CL_TRUE, 0, n * sizeof(float), pos_y.data(), 0, nullptr, nullptr);

            for (size_t i = 0; i < n; ++i) {
                bodies[i].position_x = pos_x[i];
                bodies[i].position_y = pos_y[i];
            }

        } else {
            //CPU version
            compute_forces(bodies, G, eps);
            integrate_bodies(bodies, dt, WIDTH, HEIGHT);
        }
        const auto compute_end = std::chrono::high_resolution_clock::now();
        const std::chrono::duration<double, std::milli> compute_duration = compute_end - compute_start;

        /*if (use_gpu)
            std::cout << "[OpenCL] Compute time: " << compute_duration.count() << " ms\n";
        else
            std::cout << "[CPU] Compute time: " << compute_duration.count() << " ms\n";*/

        // Clear screen
        window.clear(sf::Color::Black);

        // Drawing of bodies
        for (const Body& b : bodies) {
            sf::CircleShape circle(b.mass > 50.0f ? 6.0f : 2.0f);
            circle.setFillColor(mass_to_color(b.mass));
            circle.setPosition(WIDTH / 2 + b.position_x, HEIGHT / 2 + b.position_y);
            circle.setOrigin(circle.getRadius(), circle.getRadius());
            window.draw(circle);
        }

        // FPS calculation and display
        const float frameTime = fpsClock.restart().asSeconds();
        lastFPS = 1.0f / frameTime;
        fpsText.setString("FPS: " + std::to_string(static_cast<int>(lastFPS)));
        window.draw(fpsText);

        // Show everything
        window.display();

        frame_count++;
        
        // Frame rate control
        sf::Time frameElapsed = frameClock.getElapsedTime();
        if (frameElapsed < FRAME_DURATION) {
            sf::sleep(FRAME_DURATION - frameElapsed);
        }
        frameClock.restart();
    }
    const auto total_end = std::chrono::high_resolution_clock::now();
    const double total_time = std::chrono::duration<double, std::milli>(total_end - total_start).count();
    if (use_gpu)
        std::cout << "[GPU] Total time for 10000 frames: " << total_time << " ms\n";
    else
        std::cout << "[CPU] Total time for 10000 frames: " << total_time << " ms\n";
    
        //Cleanup
        if (use_gpu){
        clReleaseMemObject(buf_px);
        clReleaseMemObject(buf_py);
        clReleaseMemObject(buf_vx);
        clReleaseMemObject(buf_vy);
        clReleaseMemObject(buf_ax);
        clReleaseMemObject(buf_ay);
        clReleaseMemObject(buf_m);

        cleanup_opencl(); 
    }
    return 0;

}

/*int main() {
    const int WIDTH = 2560;
    const int HEIGHT = 1440;
    const size_t n = 1000; // use a larger number to see speed differences

    std::vector<Body> bodies = initialize_bodies(n, center_mass, WIDTH, HEIGHT);
    std::vector<float> pos_x(n), pos_y(n), vel_x(n), vel_y(n), acc_x(n), acc_y(n), mass(n);

    for (size_t i = 0; i < n; ++i) {
        pos_x[i] = bodies[i].position_x;
        pos_y[i] = bodies[i].position_y;
        vel_x[i] = bodies[i].velocity_x;
        vel_y[i] = bodies[i].velocity_y;
        acc_x[i] = 0.0f;
        acc_y[i] = 0.0f;
        mass[i] = bodies[i].mass;
    }

    // --- CPU VERSION TIMING ---
    auto cpu_start = std::chrono::high_resolution_clock::now();

    for (int step = 0; step < 100; ++step) {
        compute_forces(bodies, G, eps);
        integrate_bodies(bodies, dt, WIDTH, HEIGHT);
    }

    auto cpu_end = std::chrono::high_resolution_clock::now();
    auto cpu_duration = std::chrono::duration_cast<std::chrono::milliseconds>(cpu_end - cpu_start).count();
    std::cout << "CPU version time: " << cpu_duration << " ms\n";


    // --- GPU VERSION TIMING ---
    init_opencl("opencl/NBody.cl");
    cl_int err;

    cl_mem buf_px = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, n * sizeof(float), pos_x.data(), &err);
    cl_mem buf_py = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, n * sizeof(float), pos_y.data(), &err);
    cl_mem buf_vx = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, n * sizeof(float), vel_x.data(), &err);
    cl_mem buf_vy = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, n * sizeof(float), vel_y.data(), &err);
    cl_mem buf_ax = clCreateBuffer(context, CL_MEM_READ_WRITE, n * sizeof(float), nullptr, &err);
    cl_mem buf_ay = clCreateBuffer(context, CL_MEM_READ_WRITE, n * sizeof(float), nullptr, &err);
    cl_mem buf_m  = clCreateBuffer(context, CL_MEM_READ_ONLY  | CL_MEM_COPY_HOST_PTR, n * sizeof(float), mass.data(), &err);

    size_t global_size = n;

    auto gpu_start = std::chrono::high_resolution_clock::now();

    for (int step = 0; step < 100; ++step) {
        clSetKernelArg(kernel_force, 0, sizeof(cl_mem), &buf_px);
        clSetKernelArg(kernel_force, 1, sizeof(cl_mem), &buf_py);
        clSetKernelArg(kernel_force, 2, sizeof(cl_mem), &buf_ax);
        clSetKernelArg(kernel_force, 3, sizeof(cl_mem), &buf_ay);
        clSetKernelArg(kernel_force, 4, sizeof(cl_mem), &buf_m);
        clSetKernelArg(kernel_force, 5, sizeof(unsigned int), &n);
        clSetKernelArg(kernel_force, 6, sizeof(float), &G);
        clSetKernelArg(kernel_force, 7, sizeof(float), &eps);

        clEnqueueNDRangeKernel(queue, kernel_force, 1, nullptr, &global_size, nullptr, 0, nullptr, nullptr);

        clSetKernelArg(kernel_integrate, 0, sizeof(cl_mem), &buf_px);
        clSetKernelArg(kernel_integrate, 1, sizeof(cl_mem), &buf_py);
        clSetKernelArg(kernel_integrate, 2, sizeof(cl_mem), &buf_vx);
        clSetKernelArg(kernel_integrate, 3, sizeof(cl_mem), &buf_vy);
        clSetKernelArg(kernel_integrate, 4, sizeof(cl_mem), &buf_ax);
        clSetKernelArg(kernel_integrate, 5, sizeof(cl_mem), &buf_ay);
        clSetKernelArg(kernel_integrate, 6, sizeof(float), &dt);
        clSetKernelArg(kernel_integrate, 7, sizeof(float), &WIDTH);
        clSetKernelArg(kernel_integrate, 8, sizeof(float), &HEIGHT);

        clEnqueueNDRangeKernel(queue, kernel_integrate, 1, nullptr, &global_size, nullptr, 0, nullptr, nullptr);
    }

    clFinish(queue);

    auto gpu_end = std::chrono::high_resolution_clock::now();
    auto gpu_duration = std::chrono::duration_cast<std::chrono::milliseconds>(gpu_end - gpu_start).count();
    std::cout << "OpenCL (GPU) version time: " << gpu_duration << " ms\n";

    return 0;
}*/



