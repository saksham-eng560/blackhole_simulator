

#include "camera.h"

Camera::Camera(double distance, double theta, double phi, double fov_deg, double roll)
    : cam_distance(distance)
    , cam_theta(theta)
    , cam_phi(phi)
    , cam_roll(roll)
    , fov(fov_deg * M_PI / 180.0)
{}

Vec3 Camera::getPosition() const {
    double sin_t = std::sin(cam_theta);
    double cos_t = std::cos(cam_theta);
    return {
        cam_distance * sin_t * std::cos(cam_phi),
        cam_distance * cos_t,
        cam_distance * sin_t * std::sin(cam_phi)
    };
}

Vec3 Camera::generateRayDirection(double u, double v, double aspect_ratio) const {
    Vec3 pos     = getPosition();
    Vec3 forward = (-pos).normalized();          


    Vec3 world_up{0.0, 1.0, 0.0};
    if (std::abs(forward.dot(world_up)) > 0.999) {
        world_up = Vec3{0.0, 0.0, 1.0};
    }

    Vec3 right = cross(forward, world_up).normalized();
    Vec3 up    = cross(right, forward).normalized();

    if (cam_roll != 0.0) {
        double cos_r = std::cos(cam_roll);
        double sin_r = std::sin(cam_roll);
        Vec3 new_right = right * cos_r + up * sin_r;
        Vec3 new_up    = up * cos_r - right * sin_r;
        right = new_right;
        up = new_up;
    }

    double half_fov = std::tan(fov * 0.5);
    Vec3 dir = forward
             + right * (u * half_fov * aspect_ratio)
             + up    * (v * half_fov);
    return dir.normalized();
}

void Camera::rotateTheta(double delta) {
    cam_theta = std::clamp(cam_theta + delta, 0.05, M_PI - 0.05);
}

void Camera::rotateRoll(double delta) {
    cam_roll += delta;
}

void Camera::rotatePhi(double delta) {
    cam_phi += delta;

    if (cam_phi < 0.0)       cam_phi += 2.0 * M_PI;
    if (cam_phi >= 2.0 * M_PI) cam_phi -= 2.0 * M_PI;
}

void Camera::zoom(double factor) {
    cam_distance = std::max(5.0, std::min(200.0, cam_distance * factor));
}

void Camera::adjustFOV(double delta_deg) {
    double fov_deg = fov * 180.0 / M_PI;
    fov_deg = std::clamp(fov_deg + delta_deg, 10.0, 120.0);
    fov = fov_deg * M_PI / 180.0;
}
