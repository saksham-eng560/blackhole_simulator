
#pragma once

#include "vec3.h"
#include <cmath>
#include <algorithm>

class Camera {
public:
    double cam_distance;   
    double cam_theta;      
    double cam_phi;        
    double cam_roll;
    double fov;            


    Camera(double distance = 30.0, double theta = 1.2,
           double phi = 0.0, double fov_deg = 45.0, double roll = 0.0);


    Vec3 getPosition() const;


    Vec3 generateRayDirection(double u, double v, double aspect_ratio) const;


    void rotateTheta(double delta);
    void rotatePhi(double delta);
    void rotateRoll(double delta);
    void zoom(double factor);
    void adjustFOV(double delta_deg);
};
