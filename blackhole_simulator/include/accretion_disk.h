
#pragma once

#include "vec3.h"
#include "black_hole.h"
#include <cmath>

class AccretionDisk {
public:
    double inner_radius;       
    double outer_radius;       
    Vec3   normal;             


    AccretionDisk(const BlackHole& bh, double outer_factor = 12.0);


    bool isInDisk(double r) const;


    double temperature(const Vec3& hit_pos) const;


    double computeIntensity(const Vec3& hit_pos, double g_factor) const;


    double computeFrequencyShift(const Vec3& hit_pos, const Vec3& photon_dir,
                                  const BlackHole& bh) const;
};
