

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

std::pair<double, double> RayTracer::starfield(const Vec3& direction) const {
    Vec3 d = direction.normalized();

    auto hash = [](int x, int y, int z) -> double {
        uint32_t h = static_cast<uint32_t>(x * 73856093u ^ y * 19349663u ^ z * 83492791u);
        h = (h ^ (h >> 13)) * 0x5bd1e995;
        h = h ^ (h >> 15);
        return static_cast<double>(h & 0xFFFF) / 65535.0;
    };

    // 1. Small Galaxy Swirls (spiral galaxies rendered using '*' and '.')
    struct GalaxySwirl {
        Vec3 center;
        Vec3 u, v;
        double radius;
    };
    static const auto init_galaxy = [](Vec3 c, double rad) -> GalaxySwirl {
        c = c.normalized();
        Vec3 up = (std::abs(c.y) < 0.9) ? Vec3(0, 1, 0) : Vec3(1, 0, 0);
        Vec3 u = up.cross(c).normalized();
        Vec3 v = c.cross(u).normalized();
        return { c, u, v, rad };
    };
    static const GalaxySwirl galaxies[3] = {
        init_galaxy(Vec3(-0.5,  0.6, -0.6), 0.24),
        init_galaxy(Vec3( 0.7, -0.4,  0.6), 0.26),
        init_galaxy(Vec3(-0.6, -0.5,  0.6), 0.22)
    };

    for (const auto& g : galaxies) {
        double dot = d.dot(g.center);
        double cos_r = std::cos(g.radius);
        if (dot > cos_r) {
            double lx = d.dot(g.u);
            double ly = d.dot(g.v);
            double r = std::sqrt(lx * lx + ly * ly) / g.radius;
            if (r <= 1.0) {
                double theta = std::atan2(ly, lx);
                
                // Galactic core
                if (r < 0.08) {
                    return { 0.95, 0.85 }; // hue=0.85 -> '*'
                }

                // Two-arm spiral: theta - winding * r
                double arm_phase = theta - 7.0 * r;
                double arm_dist = std::abs(std::sin(arm_phase));
                double arm_width = 0.28 * (1.0 - 0.4 * r);

                if (arm_dist < arm_width) {
                    int gx = static_cast<int>(std::floor(d.x * 120.0));
                    int gy = static_cast<int>(std::floor(d.y * 120.0));
                    int gz = static_cast<int>(std::floor(d.z * 120.0));
                    double ghash = hash(gx, gy, gz);

                    if (arm_dist < arm_width * 0.45 && ghash > 0.42) {
                        return { 0.8 + 0.2 * (1.0 - r), 0.85 }; // hue=0.85 -> '*'
                    } else if (ghash > 0.58) {
                        return { 0.5 + 0.3 * (1.0 - r), 0.2 };  // hue=0.2  -> '.'
                    }
                }
            }
        }
    }

    // 2. Star Clusters: concentrated groups of '*' characters
    struct Cluster {
        Vec3 center;
        double radius;
    };
    static const Cluster clusters[4] = {
        { Vec3( 0.6,  0.7,  0.3).normalized(), 0.18 },
        { Vec3(-0.7,  0.5, -0.5).normalized(), 0.16 },
        { Vec3( 0.3, -0.8,  0.5).normalized(), 0.20 },
        { Vec3(-0.5, -0.4,  0.7).normalized(), 0.15 }
    };

    for (const auto& c : clusters) {
        double dot = d.dot(c.center);
        double cos_r = std::cos(c.radius);
        if (dot > cos_r) {
            double dist = std::acos(std::clamp(dot, -1.0, 1.0)) / c.radius;
            int cx = static_cast<int>(std::floor(d.x * 75.0));
            int cy = static_cast<int>(std::floor(d.y * 75.0));
            int cz = static_cast<int>(std::floor(d.z * 75.0));
            double chash = hash(cx, cy, cz);
            
            double threshold = 0.72 + 0.22 * dist;
            if (chash > threshold) {
                return { 0.7 + 0.3 * (1.0 - dist), 0.6 }; // hue=0.6 -> '*'
            }
        }
    }

    // 3. Random location stars ('*' scattered on random locations)
    int ix2 = static_cast<int>(std::floor((d.x + 12.34) * 85.0));
    int iy2 = static_cast<int>(std::floor((d.y + 56.78) * 85.0));
    int iz2 = static_cast<int>(std::floor((d.z + 90.12) * 85.0));
    double val2 = hash(ix2, iy2, iz2);
    if (val2 > 0.987) {
        return { 0.6 + 0.4 * ((val2 - 0.987) / 0.013), 0.5 }; // hue=0.5 -> '*'
    }

    // 4. Distinct single-dot stars ('.' at distinct locations)
    int ix1 = static_cast<int>(std::floor(d.x * 130.0));
    int iy1 = static_cast<int>(std::floor(d.y * 130.0));
    int iz1 = static_cast<int>(std::floor(d.z * 130.0));
    double val1 = hash(ix1, iy1, iz1);
    if (val1 > 0.988) {
        return { 0.4 + 0.4 * ((val1 - 0.988) / 0.012), 0.1 }; // hue=0.1 -> '.'
    }

    return { 0.0, 0.0 };
}

TraceResult RayTracer::trace(const Vec3& origin, const Vec3& direction) const {
    TraceResult result{HitType::NONE, 0.0, 0.0};

    Vec3 pos = origin;
    Vec3 vel = direction.normalized();

    for (int step = 0; step < max_steps; ++step) {
        double r = pos.length();


        if (r > escape_radius && pos.dot(vel) > 0.0) {
            if (enable_stars) {
                auto star = starfield(vel);
                if (star.first > 0.0) {
                    result.hit       = HitType::BACKGROUND_STAR;
                    result.intensity = star.first * 1.2;  
                    result.hue       = star.second;
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
