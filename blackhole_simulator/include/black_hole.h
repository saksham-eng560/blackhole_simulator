
#pragma once

class BlackHole {
public:
    double mass;            
    double rs;              
    double photon_sphere;   
    double isco;            


    explicit BlackHole(double mass = 1.0)
        : mass(mass)
        , rs(2.0 * mass)
        , photon_sphere(3.0 * mass)
        , isco(6.0 * mass)
    {}


    void setMass(double m) {
        mass = m;
        rs = 2.0 * m;
        photon_sphere = 3.0 * m;
        isco = 6.0 * m;
    }


    bool isInsideHorizon(double r) const { return r <= rs; }


    bool isInsidePhotonSphere(double r) const { return r <= photon_sphere; }
};
