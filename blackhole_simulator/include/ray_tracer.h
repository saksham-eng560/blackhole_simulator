
#pragma once

#include "vec3.h"
#include "black_hole.h"
#include "accretion_disk.h"
#include <utility>

enum class HitType {
    NONE,             
    EVENT_HORIZON,    
    ACCRETION_DISK,   
    BACKGROUND_STAR   
};

struct TraceResult {
    HitType hit;          
    double  intensity;    
    double  hue;          
};

class RayTracer {
public:
    const BlackHole&    bh;     
    const AccretionDisk& disk;  

    int    max_steps;       
    double escape_radius;   
    double step_size;       
    bool   enable_stars;    
    double exposure;        

    RayTracer(const BlackHole& bh, const AccretionDisk& disk);


    TraceResult trace(const Vec3& origin, const Vec3& direction) const;

private:

    Vec3 acceleration(const Vec3& pos, const Vec3& vel) const;


    void rk4Step(Vec3& pos, Vec3& vel, double dt) const;


    double adaptiveStepSize(double r) const;


    std::pair<double, double> starfield(const Vec3& direction) const;
};
