

#include "accretion_disk.h"
#include <algorithm>

AccretionDisk::AccretionDisk(const BlackHole& bh, double outer_factor)
    : inner_radius(bh.isco)                      
    , outer_radius(outer_factor * bh.rs)          
    , normal{0.0, 1.0, 0.0}                      
{}

bool AccretionDisk::isInDisk(double r) const {
    return r >= inner_radius && r <= outer_radius;
}

double AccretionDisk::temperature(const Vec3& hit_pos) const {
    double r = std::sqrt(hit_pos.x * hit_pos.x + hit_pos.z * hit_pos.z);
    if (r <= inner_radius) return 0.0;

    double ratio = inner_radius / r;                          
    double t_profile = std::pow(r / inner_radius, -0.75)      
                     * std::pow(1.0 - std::sqrt(ratio), 0.25); 


    double phi = std::atan2(hit_pos.z, hit_pos.x);
    double spiral = 1.0 + 0.3 * std::cos(2.0 * phi - 0.8 * r);
    t_profile *= spiral;

    return std::clamp(t_profile, 0.0, 1.0);
}

double AccretionDisk::computeIntensity(const Vec3& hit_pos, double g_factor) const {
    double T = temperature(hit_pos);
    double I_emit = T * T * T * T;                             





    double g_vis = std::pow(g_factor, 2.5); 
    return g_vis * I_emit;
}

double AccretionDisk::computeFrequencyShift(const Vec3& hit_pos,
                                             const Vec3& photon_dir,
                                             const BlackHole& bh) const {
    double r = std::sqrt(hit_pos.x * hit_pos.x + hit_pos.z * hit_pos.z);
    if (r < 1e-8) return 1.0;  


    double g_grav = std::sqrt(std::max(1e-12, 1.0 - bh.rs / r));


    double v_orb = std::sqrt(bh.mass / r);


    Vec3 r_hat = Vec3{hit_pos.x, 0.0, hit_pos.z}.normalized();
    Vec3 v_dir = cross(normal, r_hat).normalized();   
    Vec3 v_vec = v_dir * v_orb;


    double v2    = v_vec.length_sq();
    double gamma = 1.0 / std::sqrt(std::max(1e-12, 1.0 - v2));
    double v_dot_k = dot(v_vec, photon_dir.normalized());
    double g_doppler = 1.0 / (gamma * (1.0 - v_dot_k));


    double g = g_grav * g_doppler;
    return std::clamp(g, 0.01, 10.0);
}
