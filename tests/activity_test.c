#include "activity.h"
/* Minimal libc helper for the freestanding ARM emulator harness. */
void *memset(void *dst, int value, size_t size)
{
    volatile unsigned char *p = dst;
    for (size_t i = 0; i < size; ++i) p[i] = (unsigned char)value;
    return dst;
}
#define CHECK(x) do { if (!(x)) return 1000 + __LINE__; } while (0)
int activity_tests(void)
{
    struct activity s = {0};
    activity_step(&s, 0, false, false, 0);
    CHECK(s.state == ACTIVITY_STANDBY);
    activity_step(&s, 1000, true, false, 0);
    CHECK(s.state == ACTIVITY_SEARCHING);
    activity_step(&s, 45999, false, false, 0);
    CHECK(s.state == ACTIVITY_SEARCHING);
    activity_step(&s, 46000, false, false, 0);
    CHECK(s.state == ACTIVITY_STANDBY);
    activity_step(&s, 46001, true, true, 46001);
    CHECK(s.state == ACTIVITY_STANDBY); /* no immediate retry despite broadcast */
    activity_step(&s, 106000, false, false, 0);
    CHECK(s.state == ACTIVITY_STANDBY); /* needs new validated movement */
    activity_step(&s, 106001, true, false, 0);
    CHECK(s.state == ACTIVITY_SEARCHING);
    activity_step(&s, 107000, false, true, 107000);
    CHECK(s.state == ACTIVITY_FORWARDING);
    activity_step(&s, 112000, false, false, 107000);
    CHECK(s.state == ACTIVITY_FORWARDING); /* temporary HR loss: allow reconnection */
    activity_step(&s, 136999, true, false, 107000);
    CHECK(s.state == ACTIVITY_FORWARDING);
    activity_step(&s, 137000, false, false, 107000);
    CHECK(s.state == ACTIVITY_STANDBY);

    s = (struct activity){0};
    activity_step(&s, 1000, true, false, 0);
    activity_step(&s, 2000, false, true, 2000);
    activity_step(&s, 180999, false, true, 180999);
    CHECK(s.state == ACTIVITY_FORWARDING); /* coasting / red light */
    activity_step(&s, 181000, false, true, 181000);
    CHECK(s.state == ACTIVITY_STANDBY); /* HR alone must not keep a parked bike awake */
    s = (struct activity){0};
    activity_step(&s, 1, true, false, 0);
    activity_step(&s, 45001, false, true, 45001);
    CHECK(s.state == ACTIVITY_STANDBY); /* late notification cannot undo deadline */

    struct cadence_tracker t = {0};
    uint8_t cadence[8] = {0, 255, 255, 255, 0, 4, 10, 0};
    CHECK(!cadence_motion(&t, 122, cadence, 8, 0));
    CHECK(!cadence_motion(&t, 122, cadence, 8, 250)); /* repeated broadcasts != movement */
    cadence[5] = 8; cadence[6] = 11;
    CHECK(cadence_motion(&t, 122, cadence, 8, 1000)); /* 60 rpm */
    cadence[0] = 80;
    CHECK(!cadence_motion(&t, 122, cadence, 8, 1100)); /* common background page */
    cadence[0] = 0; cadence[6] = 12;
    CHECK(!cadence_motion(&t, 122, cadence, 8, 1200)); /* event time did not change */
    CHECK(!cadence_motion(&t, 123, cadence, 8, 1300)); /* speed-only sensor */
    CHECK(!cadence_motion(&t, 122, cadence, 7, 1400));
    CHECK(!cadence_motion(&t, 122, 0, 8, 1400));

    t = (struct cadence_tracker){0};
    uint8_t wrap[8] = {0, 0, 0, 0, 250, 255, 255, 255};
    CHECK(!cadence_motion(&t, 122, wrap, 8, 0));
    wrap[4] = 232; wrap[5] = 3; wrap[6] = 0; wrap[7] = 0;
    CHECK(cadence_motion(&t, 122, wrap, 8, 1000)); /* count and time rollover */
    wrap[4] = 0; wrap[5] = 8; wrap[6] = 1;
    CHECK(!cadence_motion(&t, 122, wrap, 8, 31000)); /* stale acquisition baseline */

    t = (struct cadence_tracker){0};
    uint8_t combined[8] = {0, 4, 1, 0, 0, 8, 30, 0};
    CHECK(!cadence_motion(&t, 121, combined, 8, 0));
    combined[5] = 12; combined[6] = 31;
    CHECK(!cadence_motion(&t, 121, combined, 8, 1000)); /* wheel motion != crank motion */
    combined[1] = 8; combined[2] = 2;
    CHECK(cadence_motion(&t, 121, combined, 8, 2000));
    combined[0] = 1; combined[2] = 3;
    CHECK(!cadence_motion(&t, 121, combined, 8, 2010)); /* impossible RPM */
    return 0;
}
