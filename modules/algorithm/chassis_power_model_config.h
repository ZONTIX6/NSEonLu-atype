#ifndef CHASSIS_POWER_MODEL_CONFIG_H
#define CHASSIS_POWER_MODEL_CONFIG_H

/* Current speed-backend calibration, shared with host validation tools.
 * x = abs(final DJI command) / 1000, n = abs(rotor rpm).
 * P_wheel = k0 + k1*x + k2*n + k3*x*n + k4*x*x + k5*n*n.
 * The divisor is a fitted command normalization, not a verified mA/A scale.
 * This legacy static model does not distinguish driving from braking and
 * has no validated dynamic domain. Values below are unchanged, not refitted.
 */
#define CHASSIS_SPEED_POWER_MODEL_INITIALIZER { \
    .k0 = 1.3123816, \
    .k1 = -0.25836727, \
    .k2 = 0.0007424524, \
    .k3 = 0.0024271158, \
    .k4 = 0.081198552, \
    .k5 = 3.890596e-08, \
    .current_conversion = 1000.0f, \
}

#endif
