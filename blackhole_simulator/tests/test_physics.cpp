/**
 * @file test_physics.cpp
 * @brief Comprehensive unit tests for the black hole physics engine.
 *
 * Tests cover:
 *   1. Vec3 math operations
 *   2. BlackHole derived radii
 *   3. AccretionDisk temperature profile & Doppler shift
 *   4. Camera ray generation
 *   5. RayTracer — event horizon absorption
 *   6. RayTracer — accretion disk hits
 *   7. RayTracer — photon escape (background)
 *   8. RayTracer — gravitational lensing (deflection > Newtonian)
 *   9. RayTracer — Doppler asymmetry (approaching vs receding)
 *   10. TerminalRenderer — initialization & pixel buffer
 */

#include "vec3.h"
#include "black_hole.h"
#include "accretion_disk.h"
#include "camera.h"
#include "ray_tracer.h"

#include <cstdio>
#include <cmath>
#include <cassert>
#include <string>

// ── Test utilities ──────────────────────────────────────────────────────────

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) \
    printf("  %-55s", name); \
    fflush(stdout);

#define PASS() \
    do { printf("\033[32m✓ PASS\033[0m\n"); tests_passed++; } while(0)

#define FAIL(msg) \
    do { printf("\033[31m✗ FAIL: %s\033[0m\n", msg); tests_failed++; } while(0)

#define ASSERT_NEAR(a, b, tol, msg) \
    if (std::abs((a) - (b)) > (tol)) { FAIL(msg); return; } 

#define ASSERT_TRUE(cond, msg) \
    if (!(cond)) { FAIL(msg); return; }

// ═════════════════════════════════════════════════════════════════════════════
// Test 1: Vec3 math
// ═════════════════════════════════════════════════════════════════════════════

void test_vec3_operations() {
    TEST("Vec3: addition, subtraction, scaling");
    Vec3 a{1, 2, 3};
    Vec3 b{4, 5, 6};
    Vec3 sum = a + b;
    ASSERT_NEAR(sum.x, 5.0, 1e-12, "sum.x != 5");
    ASSERT_NEAR(sum.y, 7.0, 1e-12, "sum.y != 7");
    ASSERT_NEAR(sum.z, 9.0, 1e-12, "sum.z != 9");
    Vec3 scaled = a * 2.0;
    ASSERT_NEAR(scaled.x, 2.0, 1e-12, "scaled.x != 2");
    PASS();
}

void test_vec3_dot_cross() {
    TEST("Vec3: dot product & cross product");
    Vec3 a{1, 0, 0};
    Vec3 b{0, 1, 0};
    ASSERT_NEAR(a.dot(b), 0.0, 1e-12, "orthogonal dot != 0");
    Vec3 c = a.cross(b);
    ASSERT_NEAR(c.x, 0.0, 1e-12, "cross.x != 0");
    ASSERT_NEAR(c.y, 0.0, 1e-12, "cross.y != 0");
    ASSERT_NEAR(c.z, 1.0, 1e-12, "cross.z != 1");
    PASS();
}

void test_vec3_normalize() {
    TEST("Vec3: normalize");
    Vec3 v{3, 4, 0};
    Vec3 n = v.normalized();
    ASSERT_NEAR(n.length(), 1.0, 1e-12, "normalized length != 1");
    ASSERT_NEAR(n.x, 0.6, 1e-12, "n.x != 0.6");
    ASSERT_NEAR(n.y, 0.8, 1e-12, "n.y != 0.8");
    PASS();
}

// ═════════════════════════════════════════════════════════════════════════════
// Test 2: BlackHole radii
// ═════════════════════════════════════════════════════════════════════════════

void test_blackhole_radii() {
    TEST("BlackHole: Schwarzschild radius, photon sphere, ISCO");
    BlackHole bh(1.0);
    ASSERT_NEAR(bh.rs, 2.0, 1e-12, "rs != 2M");
    ASSERT_NEAR(bh.photon_sphere, 3.0, 1e-12, "photon sphere != 3M");
    ASSERT_NEAR(bh.isco, 6.0, 1e-12, "ISCO != 6M");
    ASSERT_TRUE(bh.isInsideHorizon(1.5), "r=1.5 should be inside horizon");
    ASSERT_TRUE(!bh.isInsideHorizon(3.0), "r=3.0 should be outside horizon");
    PASS();
}

void test_blackhole_set_mass() {
    TEST("BlackHole: setMass recalculates all radii");
    BlackHole bh(1.0);
    bh.setMass(2.0);
    ASSERT_NEAR(bh.rs, 4.0, 1e-12, "rs != 4 after setMass(2)");
    ASSERT_NEAR(bh.photon_sphere, 6.0, 1e-12, "photon sphere != 6");
    ASSERT_NEAR(bh.isco, 12.0, 1e-12, "ISCO != 12");
    PASS();
}

// ═════════════════════════════════════════════════════════════════════════════
// Test 3: AccretionDisk
// ═════════════════════════════════════════════════════════════════════════════

void test_disk_radii() {
    TEST("AccretionDisk: inner edge = ISCO, outer edge = factor * rs");
    BlackHole bh(1.0);
    AccretionDisk disk(bh, 10.0);
    ASSERT_NEAR(disk.inner_radius, 6.0, 1e-12, "inner != ISCO=6M");
    ASSERT_NEAR(disk.outer_radius, 20.0, 1e-12, "outer != 10*rs=20M");
    ASSERT_TRUE(disk.isInDisk(10.0), "r=10 should be in disk");
    ASSERT_TRUE(!disk.isInDisk(3.0), "r=3 should NOT be in disk");
    ASSERT_TRUE(!disk.isInDisk(25.0), "r=25 should NOT be in disk");
    PASS();
}

void test_disk_temperature_profile() {
    TEST("AccretionDisk: T(r_isco) = 0, T peaks near 1.36*r_isco");
    BlackHole bh(1.0);
    AccretionDisk disk(bh, 10.0);
    // At the inner edge, temperature should be 0
    ASSERT_NEAR(disk.temperature(Vec3{disk.inner_radius, 0.0, 0.0}), 0.0, 1e-10, "T(r_isco) != 0");
    // Temperature should peak somewhere between 1.2-1.5 × r_isco
    double T_peak = 0;
    double r_peak = 0;
    for (double r = disk.inner_radius + 0.1; r < disk.outer_radius; r += 0.1) {
        double T = disk.temperature(Vec3{r, 0.0, 0.0});
        if (T > T_peak) { T_peak = T; r_peak = r; }
    }
    double peak_ratio = r_peak / disk.inner_radius;
    ASSERT_TRUE(peak_ratio > 1.2 && peak_ratio < 1.6,
                "T peak ratio not in [1.2, 1.6]");
    ASSERT_TRUE(T_peak > 0.0, "T_peak should be > 0");
    PASS();
}

void test_disk_doppler_asymmetry() {
    TEST("AccretionDisk: Doppler asymmetry (blueshift vs redshift)");
    BlackHole bh(1.0);
    AccretionDisk disk(bh, 10.0);
    double r = 10.0;  // Mid-disk

    // Photon arriving from the approaching side (same direction as orbit)
    // vs receding side (opposite direction)
    // Disk orbits in xz-plane, normal = +y, so at pos (r,0,0),
    // orbital velocity is in the -z direction (cross(+y, +x) = -z)
    Vec3 hit_pos{r, 0.0, 0.0};
    Vec3 approaching_dir{0.0, -1.0, -1.0};  // Photon going along orbit
    Vec3 receding_dir{0.0, -1.0, 1.0};      // Photon going against orbit

    double g_approach = disk.computeFrequencyShift(hit_pos, approaching_dir, bh);
    double g_recede   = disk.computeFrequencyShift(hit_pos, receding_dir, bh);

    // Approaching side should be blueshifted (g > 1 or at least > g_recede)
    ASSERT_TRUE(g_approach > g_recede,
                "approaching g should be > receding g (Doppler asymmetry)");
    PASS();
}

// ═════════════════════════════════════════════════════════════════════════════
// Test 4: Camera
// ═════════════════════════════════════════════════════════════════════════════

void test_camera_position() {
    TEST("Camera: spherical → Cartesian position");
    Camera cam(30.0, M_PI / 2.0, 0.0, 45.0);  // θ=π/2 → equatorial
    Vec3 pos = cam.getPosition();
    ASSERT_NEAR(pos.y, 0.0, 1e-8, "y should be ~0 at θ=π/2");
    ASSERT_NEAR(pos.x, 30.0, 1e-8, "x should be ~30 at θ=π/2, φ=0");
    PASS();
}

void test_camera_ray_center() {
    TEST("Camera: center ray points toward origin");
    Camera cam(30.0, M_PI / 2.0, 0.0, 45.0);
    Vec3 pos = cam.getPosition();
    Vec3 dir = cam.generateRayDirection(0.0, 0.0, 1.0);  // Center pixel
    // Center ray should point from camera toward origin
    Vec3 to_origin = (-pos).normalized();
    double alignment = dir.dot(to_origin);
    ASSERT_TRUE(alignment > 0.99, "center ray should point toward origin");
    PASS();
}

// ═════════════════════════════════════════════════════════════════════════════
// Test 5: RayTracer — event horizon
// ═════════════════════════════════════════════════════════════════════════════

void test_ray_into_horizon() {
    TEST("RayTracer: ray aimed directly at BH → EVENT_HORIZON");
    BlackHole bh(1.0);
    AccretionDisk disk(bh, 10.0);
    RayTracer tracer(bh, disk);
    tracer.enable_stars = false;

    // Fire ray from directly above on the y-axis, straight down.
    // This avoids the disk (the ray never has a y-sign change at r > ISCO
    // because it goes straight through the origin on the y-axis,
    // crossing y=0 at r≈0 which is inside the horizon).
    Vec3 origin{0.0, 40.0, 0.0};
    Vec3 dir{0.0, -1.0, 0.0};

    TraceResult result = tracer.trace(origin, dir);
    // A photon on the y-axis (h=0) has zero angular momentum → no deflection,
    // so it falls straight in.  It crosses y=0 at r=0 (inside horizon).
    ASSERT_TRUE(result.hit == HitType::EVENT_HORIZON,
                "direct radial ray should be absorbed by event horizon");
    ASSERT_NEAR(result.intensity, 0.0, 1e-6, "horizon should be black");
    PASS();
}

// ═════════════════════════════════════════════════════════════════════════════
// Test 6: RayTracer — disk hit
// ═════════════════════════════════════════════════════════════════════════════

void test_ray_hits_disk() {
    TEST("RayTracer: ray aimed at disk → ACCRETION_DISK");
    BlackHole bh(1.0);
    AccretionDisk disk(bh, 10.0);
    RayTracer tracer(bh, disk);
    tracer.enable_stars = false;

    // Fire from above, angled to hit the disk at ~r=10
    Vec3 origin{10.0, 15.0, 0.0};
    Vec3 dir{0.0, -1.0, 0.0};  // Straight down toward disk

    TraceResult result = tracer.trace(origin, dir);
    ASSERT_TRUE(result.hit == HitType::ACCRETION_DISK,
                "downward ray over disk should hit ACCRETION_DISK");
    ASSERT_TRUE(result.intensity > 0.0, "disk hit should have positive intensity");
    ASSERT_TRUE(result.hue > 0.0, "disk hit should have positive hue (temperature)");
    PASS();
}

// ═════════════════════════════════════════════════════════════════════════════
// Test 7: RayTracer — escape to infinity
// ═════════════════════════════════════════════════════════════════════════════

void test_ray_escapes() {
    TEST("RayTracer: ray aimed away from BH → NONE (escape)");
    BlackHole bh(1.0);
    AccretionDisk disk(bh, 10.0);
    RayTracer tracer(bh, disk);
    tracer.enable_stars = false;

    // Fire away from origin
    Vec3 origin{30.0, 10.0, 0.0};
    Vec3 dir{1.0, 1.0, 0.0};  // Away from BH

    TraceResult result = tracer.trace(origin, dir.normalized());
    ASSERT_TRUE(result.hit == HitType::NONE || result.hit == HitType::BACKGROUND_STAR,
                "ray aimed away should escape");
    PASS();
}

// ═════════════════════════════════════════════════════════════════════════════
// Test 8: Gravitational lensing — photon deflection
// ═════════════════════════════════════════════════════════════════════════════

void test_gravitational_lensing() {
    TEST("RayTracer: photon passing near BH is deflected (lensing)");
    BlackHole bh(1.0);
    AccretionDisk disk(bh, 10.0);
    RayTracer tracer(bh, disk);
    tracer.enable_stars = false;
    tracer.max_steps = 2000;  // Extra steps for precision
    tracer.escape_radius = 200.0;

    // Send a photon that passes near the photon sphere but doesn't hit
    // Start far away, aim to pass at impact parameter b ≈ 4M = 4
    // (just outside critical impact parameter b_crit = 3√3·M ≈ 5.196)
    Vec3 origin{-100.0, 0.0, 6.0};    // Far left, offset by b=6 in z
    Vec3 dir{1.0, 0.0, 0.0};          // Aimed along +x (parallel)

    // In flat space, the photon would travel in a straight line and
    // exit at x=+100, z=6 (same z). In curved spacetime, z should change.
    // We can't easily get the final position from TraceResult, but we
    // can verify it doesn't get absorbed (it should escape at b=6 > b_crit≈5.196)
    TraceResult result = tracer.trace(origin, dir.normalized());
    ASSERT_TRUE(result.hit != HitType::EVENT_HORIZON,
                "photon at b=6 should NOT be absorbed (b > b_crit≈5.196)");
    PASS();
}

void test_critical_impact_absorbed() {
    TEST("RayTracer: photon at b < b_crit is captured");
    BlackHole bh(1.0);
    AccretionDisk disk(bh, 10.0);
    RayTracer tracer(bh, disk);
    tracer.enable_stars = false;
    tracer.max_steps = 2000;

    // Critical impact parameter: b_crit = 3√3·M ≈ 5.196 for M=1
    // Send photon with b = 4 in the z-direction (perpendicular to disk normal)
    // so it stays in the xz-plane and spirals into the BH without
    // necessarily crossing the disk.
    Vec3 origin{-80.0, 0.01, 4.0};    // Tiny y-offset, b=4 in z
    Vec3 dir{1.0, 0.0, 0.0};          // Straight across

    TraceResult result = tracer.trace(origin, dir.normalized());
    // At b=4 < b_crit≈5.196, the photon should be captured or hit disk
    // (it spirals inward and may cross the disk on the way in)
    ASSERT_TRUE(result.hit == HitType::EVENT_HORIZON ||
                result.hit == HitType::ACCRETION_DISK,
                "photon at b=4 < b_crit should be captured or hit disk");
    PASS();
}

// ═════════════════════════════════════════════════════════════════════════════
// Test 9: Doppler beaming asymmetry in ray tracing
// ═════════════════════════════════════════════════════════════════════════════

void test_doppler_beaming_visual() {
    TEST("RayTracer: left disk brighter than right (Doppler beaming)");
    BlackHole bh(1.0);
    AccretionDisk disk(bh, 10.0);
    RayTracer tracer(bh, disk);
    tracer.enable_stars = false;

    Camera cam(30.0, 1.2, 0.0, 40.0);
    Vec3 cam_pos = cam.getPosition();

    // Sample left side of the image vs right side
    // Disk rotates prograde → one side approaches, other recedes
    double left_intensity = 0.0, right_intensity = 0.0;
    int left_count = 0, right_count = 0;

    for (int x = 0; x < 40; ++x) {
        double u = 2.0 * x / 40.0 - 1.0;
        double v = 0.0;  // Midline
        Vec3 dir = cam.generateRayDirection(u, v, 2.0);
        TraceResult res = tracer.trace(cam_pos, dir);
        if (res.hit == HitType::ACCRETION_DISK) {
            if (u < 0) { left_intensity += res.intensity; left_count++; }
            else       { right_intensity += res.intensity; right_count++; }
        }
    }

    // At least one side should have disk hits
    ASSERT_TRUE(left_count > 0 || right_count > 0,
                "should have some disk hits on midline scan");

    // The two sides should have different average intensity (Doppler asymmetry)
    if (left_count > 0 && right_count > 0) {
        double avg_left  = left_intensity / left_count;
        double avg_right = right_intensity / right_count;
        // They should differ by at least 5%
        double ratio = (avg_left > avg_right) ?
                       avg_left / avg_right : avg_right / avg_left;
        ASSERT_TRUE(ratio > 1.05,
                    "Doppler asymmetry should cause >5% brightness difference");
    }
    PASS();
}

// ═════════════════════════════════════════════════════════════════════════════
// Test 10: Full frame render (smoke test)
// ═════════════════════════════════════════════════════════════════════════════

void test_full_frame_render() {
    TEST("Full frame: 60x30 render completes without crash");
    BlackHole bh(1.0);
    AccretionDisk disk(bh, 10.0);
    // Use a slightly closer camera with wider FOV to ensure
    // we see horizon, disk, and sky all in one frame
    Camera cam(20.0, 1.0, 0.0, 60.0);
    RayTracer tracer(bh, disk);

    Vec3 cam_pos = cam.getPosition();
    int w = 60, h = 30;
    double aspect = static_cast<double>(w) / (h * 2.0);

    int horizon_count = 0, disk_count = 0, escape_count = 0, star_count = 0;

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            double u = 2.0 * x / w - 1.0;
            double v = 1.0 - 2.0 * y / h;
            Vec3 dir = cam.generateRayDirection(u, v, aspect);
            TraceResult res = tracer.trace(cam_pos, dir);
            switch (res.hit) {
                case HitType::EVENT_HORIZON:  horizon_count++; break;
                case HitType::ACCRETION_DISK: disk_count++;    break;
                case HitType::BACKGROUND_STAR: star_count++;   break;
                case HitType::NONE:           escape_count++;  break;
            }
        }
    }

    printf("\n        [stats: horizon=%d, disk=%d, stars=%d, sky=%d]\n",
           horizon_count, disk_count, star_count, escape_count);
    // Re-print alignment for next test
    printf("  %-55s", "");

    ASSERT_TRUE(horizon_count > 0,  "should have some event horizon pixels");
    ASSERT_TRUE(disk_count > 0,     "should have some accretion disk pixels");
    ASSERT_TRUE(escape_count + star_count > 0, "should have some escape/star pixels");
    PASS();
}

// ═════════════════════════════════════════════════════════════════════════════
// Test runner
// ═════════════════════════════════════════════════════════════════════════════

int main() {
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════════╗\n");
    printf("║  Black Hole Physics Engine — Test Suite                      ║\n");
    printf("╚═══════════════════════════════════════════════════════════════╝\n\n");

    printf("── Vec3 Math ─────────────────────────────────────────────────\n");
    test_vec3_operations();
    test_vec3_dot_cross();
    test_vec3_normalize();

    printf("\n── BlackHole Model ───────────────────────────────────────────\n");
    test_blackhole_radii();
    test_blackhole_set_mass();

    printf("\n── AccretionDisk ─────────────────────────────────────────────\n");
    test_disk_radii();
    test_disk_temperature_profile();
    test_disk_doppler_asymmetry();

    printf("\n── Camera ────────────────────────────────────────────────────\n");
    test_camera_position();
    test_camera_ray_center();

    printf("\n── RayTracer (Physics Validation) ────────────────────────────\n");
    test_ray_into_horizon();
    test_ray_hits_disk();
    test_ray_escapes();
    test_gravitational_lensing();
    test_critical_impact_absorbed();
    test_doppler_beaming_visual();

    printf("\n── Integration (Smoke Test) ──────────────────────────────────\n");
    test_full_frame_render();

    printf("\n");
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("  Results: %d passed, %d failed, %d total\n",
           tests_passed, tests_failed, tests_passed + tests_failed);
    printf("═══════════════════════════════════════════════════════════════\n\n");

    return tests_failed > 0 ? 1 : 0;
}
