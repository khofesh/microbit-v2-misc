#include "test.h"

#include <math.h>

#include "motion.h"

static const Thresholds level1 = {250, 25};
static const Thresholds level2 = {120, 15};
static const Thresholds level3 = {60, 8};

static Vec3 flat()
{
    Vec3 a = {0.0f, 0.0f, 1000.0f};
    return a;
}

static Vec3 offset(float delta)
{
    Vec3 a = {0.0f, 0.0f, 1000.0f + delta};
    return a;
}

struct Counts
{
    int moving;
    int still;
    int moved;

    Counts() : moving(0), still(0), moved(0) {}
};

static void feed(MotionState &state, const Thresholds &t, Vec3 a, Counts &counts)
{
    switch (motion_update(state, a, t))
    {
    case MOTION_MOVING:
        counts.moving++;
        break;
    case MOTION_STILL:
        counts.still++;
        break;
    case MOTION_MOVED:
        counts.moved++;
        break;
    default:
        break;
    }
}

static void testFlatAndStill()
{
    MotionState state;
    motion_reset(state, flat());
    Counts counts;

    for (int i = 0; i < 100; i++)
        feed(state, level2, flat(), counts);

    CHECK_EQ(counts.moving, 0);
    CHECK_EQ(counts.moved, 0);
    CHECK_EQ(counts.still, 1);
    CHECK(state.still);
}

static void testTwoSampleSpike()
{
    MotionState state;
    motion_reset(state, flat());
    Counts counts;

    for (int i = 0; i < 10; i++)
        feed(state, level2, flat(), counts);

    feed(state, level2, offset(300), counts);
    feed(state, level2, offset(300), counts);

    for (int i = 0; i < 20; i++)
        feed(state, level2, flat(), counts);

    CHECK_EQ(counts.moving, 0);
}

static void testThreeSampleSpike()
{
    MotionState state;
    motion_reset(state, flat());
    Counts counts;

    for (int i = 0; i < 10; i++)
        feed(state, level2, flat(), counts);

    for (int i = 0; i < 3; i++)
        feed(state, level2, offset(300), counts);

    CHECK_EQ(counts.moving, 1);
    CHECK(state.moving);
}

static void testMovingThenStill()
{
    MotionState state;
    motion_reset(state, flat());
    Counts counts;

    for (int i = 0; i < 10; i++)
        feed(state, level2, flat(), counts);

    for (int i = 0; i < 3; i++)
        feed(state, level2, offset(300), counts);

    CHECK_EQ(counts.moving, 1);

    for (int i = 0; i < 80; i++)
        feed(state, level2, flat(), counts);

    CHECK_EQ(counts.moved, 1);
    CHECK(!state.moving);
    CHECK(state.still);
}

static void testSlowTilt()
{
    MotionState state;
    motion_reset(state, flat());
    Counts counts;

    const float radiansPerSample = 0.15f * 3.14159265f / 180.0f;

    for (int i = 0; i < 200; i++)
    {
        float angle = radiansPerSample * (i + 1);
        Vec3 a = {1000.0f * sinf(angle), 0.0f, 1000.0f * cosf(angle)};
        feed(state, level2, a, counts);
    }

    CHECK_EQ(counts.moving, 0);
    CHECK_EQ(counts.moved, 1);
    CHECK(motion_tilt_deg(state) > 15.0f);
}

static void testSensitivityTap()
{
    for (uint8_t level = 1; level <= 3; level++)
    {
        MotionState state;
        motion_reset(state, flat());
        Counts counts;

        for (int i = 0; i < 10; i++)
            feed(state, motion_thresholds(level), flat(), counts);

        for (int i = 0; i < 3; i++)
            feed(state, motion_thresholds(level), offset(200), counts);

        if (level == 1)
            CHECK_EQ(counts.moving, 0);
        else
            CHECK_EQ(counts.moving, 1);
    }
}

static void testSensitivityTilt()
{
    MotionState state;

    // 24 degrees is above level 2 and 3, but below level 1.
    const float radiansPerSample = 0.15f * 3.14159265f / 180.0f;

    for (uint8_t level = 1; level <= 3; level++)
    {
        motion_reset(state, flat());
        Counts counts;

        for (int i = 0; i < 160; i++)
        {
            float angle = radiansPerSample * (i + 1);
            Vec3 a = {1000.0f * sinf(angle), 0.0f, 1000.0f * cosf(angle)};
            feed(state, motion_thresholds(level), a, counts);
        }

        if (level == 1)
            CHECK_EQ(counts.moved, 0);
        else
            CHECK_EQ(counts.moved, 1);
    }
}

static void testThresholdTable()
{
    CHECK_EQ(motion_thresholds(1).moveMg, 250);
    CHECK_EQ(motion_thresholds(1).tiltDeg, 25);
    CHECK_EQ(motion_thresholds(2).moveMg, 120);
    CHECK_EQ(motion_thresholds(2).tiltDeg, 15);
    CHECK_EQ(motion_thresholds(3).moveMg, 60);
    CHECK_EQ(motion_thresholds(3).tiltDeg, 8);

    CHECK_EQ(motion_thresholds(0).moveMg, 250);
    CHECK_EQ(motion_thresholds(9).moveMg, 60);
}

void testMotion()
{
    testFlatAndStill();
    testTwoSampleSpike();
    testThreeSampleSpike();
    testMovingThenStill();
    testSlowTilt();
    testSensitivityTap();
    testSensitivityTilt();
    testThresholdTable();
}
