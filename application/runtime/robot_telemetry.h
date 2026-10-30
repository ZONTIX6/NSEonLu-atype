#ifndef ROBOT_TELEMETRY_H
#define ROBOT_TELEMETRY_H

/* Whole-vehicle collection and persistence scheduling, called by the 20 ms task. */
void RobotTelemetryInit(void);
void RobotTelemetryTask(void);

#endif
