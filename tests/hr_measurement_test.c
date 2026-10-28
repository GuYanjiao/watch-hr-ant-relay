#include "hr_measurement.h"
#define CHECK(x) do { if (!(x)) return __LINE__; } while (0)
int run_tests(void)
{
    struct hr_measurement out;
    const uint8_t normal[] = {0, 72};
    CHECK(hr_parse(normal, sizeof(normal), &out));
    CHECK(out.bpm == 72 && !out.contact_lost && out.rr_count == 0);
    const uint8_t wide[] = {1, 200, 0};
    CHECK(hr_parse(wide, sizeof(wide), &out) && out.bpm == 200);
    const uint8_t too_high[] = {1, 0, 1};
    CHECK(!hr_parse(too_high, sizeof(too_high), &out));
    const uint8_t rr[] = {0x1e, 80, 0x34, 0x12, 0, 4, 0, 3};
    CHECK(hr_parse(rr, sizeof(rr), &out));
    CHECK(out.bpm == 80 && out.rr_count == 2 && out.rr_ticks == 1792 && !out.contact_lost);
    const uint8_t lost[] = {4, 80};
    CHECK(hr_parse(lost, sizeof(lost), &out) && out.contact_lost);
    const uint8_t unsupported_contact[] = {2, 80};
    CHECK(hr_parse(unsupported_contact, sizeof(unsupported_contact), &out) && !out.contact_lost);
    const uint8_t zero[] = {0, 0};
    CHECK(hr_parse(zero, sizeof(zero), &out) && out.bpm == 0);
    const uint8_t bad_wide[] = {1, 72};
    CHECK(!hr_parse(bad_wide, sizeof(bad_wide), &out));
    const uint8_t bad_energy[] = {8, 72, 1};
    CHECK(!hr_parse(bad_energy, sizeof(bad_energy), &out));
    const uint8_t no_rr[] = {16, 72};
    CHECK(!hr_parse(no_rr, sizeof(no_rr), &out));
    const uint8_t odd_rr[] = {16, 72, 0};
    CHECK(!hr_parse(odd_rr, sizeof(odd_rr), &out));
    const uint8_t zero_rr[] = {16, 72, 0, 0};
    CHECK(!hr_parse(zero_rr, sizeof(zero_rr), &out));
    const uint8_t extra[] = {0, 72, 1};
    CHECK(!hr_parse(extra, sizeof(extra), &out));
    CHECK(!hr_parse(normal, 1, &out));
    CHECK(!hr_parse(normal, 0, &out));
    CHECK(!hr_parse(0, 2, &out));
    CHECK(!hr_parse(normal, 2, 0));
    /* Reject every truncated prefix of the energy + RR packet, except the
       complete first interval (six bytes), which is a valid shorter packet. */
    for (unsigned i = 0; i < sizeof(rr); i++) {
        CHECK(hr_parse(rr, i, &out) == (i == 6));
    }
    return 0;
}
