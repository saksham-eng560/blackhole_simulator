

#include "ray_tracer.h"
#include <cmath>
#include <algorithm>
#include <cstdint>

RayTracer::RayTracer(const BlackHole& bh_, const AccretionDisk& disk_)
    : bh(bh_), disk(disk_)
    , max_steps(2000)
    , escape_radius(500.0)
    , step_size(0.12)
    , enable_stars(true)
    , exposure(12.0)
{}

Vec3 RayTracer::acceleration(const Vec3& pos, const Vec3& vel) const {
    double r2 = pos.length_sq();
    double r  = std::sqrt(r2);


    if (r < 1e-6) return Vec3{0.0, 0.0, 0.0};

    double r5 = r2 * r2 * r;   

    Vec3   L  = cross(pos, vel);
    double h2 = L.length_sq();  

    double coeff = -1.5 * bh.rs * h2 / r5;
    return pos * coeff;
}

void RayTracer::rk4Step(Vec3& pos, Vec3& vel, double dt) const {

    Vec3 k1x = vel;
    Vec3 k1v = acceleration(pos, vel);


    Vec3 p2 = pos + k1x * (dt * 0.5);
    Vec3 v2 = vel + k1v * (dt * 0.5);
    Vec3 k2x = v2;
    Vec3 k2v = acceleration(p2, v2);


    Vec3 p3 = pos + k2x * (dt * 0.5);
    Vec3 v3 = vel + k2v * (dt * 0.5);
    Vec3 k3x = v3;
    Vec3 k3v = acceleration(p3, v3);


    Vec3 p4 = pos + k3x * dt;
    Vec3 v4 = vel + k3v * dt;
    Vec3 k4x = v4;
    Vec3 k4v = acceleration(p4, v4);


    pos += (k1x + 2.0 * k2x + 2.0 * k3x + k4x) * (dt / 6.0);
    vel += (k1v + 2.0 * k2v + 2.0 * k3v + k4v) * (dt / 6.0);
}

double RayTracer::adaptiveStepSize(double r) const {
    double proximity = r / bh.rs;

    if (proximity < 1.5)  return step_size * 0.1;    
    if (proximity < 3.0)  return step_size * 0.4;    
    if (proximity < 6.0)  return step_size * 1.0;    
    if (proximity < 15.0) return step_size * 2.5;    
    if (proximity < 30.0) return step_size * 10.0;   
    if (proximity < 50.0) return step_size * 50.0;   
    return step_size * 500.0;                        
}

double RayTracer::starfield(const Vec3& direction) const {
    Vec3 d = direction.normalized();


    int ix = static_cast<int>(std::floor(d.x * 60.0));
    int iy = static_cast<int>(std::floor(d.y * 60.0));
    int iz = static_cast<int>(std::floor(d.z * 60.0));


    uint32_t h = static_cast<uint32_t>(ix * 73856093u ^ iy * 19349663u ^ iz * 83492791u);
    h = (h ^ (h >> 13)) * 0x5bd1e995;
    h = h ^ (h >> 15);

    double val = static_cast<double>(h & 0xFFFF) / 65535.0;


    if (val > 0.92) {

        return 0.4 + 0.6 * ((val - 0.92) / 0.08);
    }
    return 0.0;
}

TraceResult RayTracer::trace(const Vec3& origin, const Vec3& direction) const {
    TraceResult result{HitType::NONE, 0.0, 0.0};

    Vec3 pos = origin;
    Vec3 vel = direction.normalized();

    for (int step = 0; step < max_steps; ++step) {
        double r = pos.length();


        if (r > escape_radius && pos.dot(vel) > 0.0) {
            if (enable_stars) {
                double brightness = starfield(vel);
                if (brightness > 0.0) {
                    result.hit       = HitType::BACKGROUND_STAR;
                    result.intensity = brightness * 1.2;  
                    return result;
                }
            }
            return result;  
        }


        double dt = adaptiveStepSize(r);
        Vec3 old_pos = pos;
        Vec3 old_vel = vel;
        double old_r = r;
        rk4Step(pos, vel, dt);
        double new_r = pos.length();






        if (new_r <= bh.rs * 1.01 ||
            (old_r > bh.rs && new_r < bh.rs)) {
            result.hit       = HitType::EVENT_HORIZON;
            result.intensity = 0.0;
            return result;
        }





        if (old_pos.y * pos.y < 0.0) {

            double frac = old_pos.y / (old_pos.y - pos.y);
            Vec3 hit_pos = old_pos + (pos - old_pos) * frac;
            hit_pos.y = 0.0;  

            double hit_r = std::sqrt(hit_pos.x * hit_pos.x
                                   + hit_pos.z * hit_pos.z);

            if (disk.isInDisk(hit_r)) {

                Vec3 hit_vel = old_vel + (vel - old_vel) * frac;


                double g = disk.computeFrequencyShift(hit_pos, hit_vel, bh);


                double raw_intensity = disk.computeIntensity(hit_pos, g);


                result.intensity = 1.0 - std::exp(-raw_intensity * exposure);


                result.hue = disk.temperature(hit_pos);

                result.hit = HitType::ACCRETION_DISK;
                return result;
            }
        }
    }


    return result;
}
