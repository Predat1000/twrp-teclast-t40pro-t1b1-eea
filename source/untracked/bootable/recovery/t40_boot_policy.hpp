#pragma once
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <zlib.h>

// This planner never writes a device. It validates this tablet's observed
// boot-control layout and predicts the exact IBootControl 1.1 update.
namespace t40 {
inline uint32_t Load32(const uint8_t* p) {
    return uint32_t(p[0]) | (uint32_t(p[1]) << 8) |
           (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
}
inline void Store32(uint8_t* p, uint32_t v) {
    for (unsigned i = 0; i < 4; ++i) p[i] = uint8_t(v >> (8 * i));
}
inline bool PrepareMisc(const std::vector<uint8_t>& before, unsigned target,
                        unsigned current, std::vector<uint8_t>* expected,
                        std::string* error) {
    auto fail = [&](const char* why) { *error = why; return false; };
    constexpr size_t offset = 2048;
    if (before.size() != 1048576 || target > 1 || current > 1)
        return fail("unexpected misc size or slot");
    const uint8_t* c = before.data() + offset;
    if (std::memcmp(c + 4, "BCAB", 4) || c[8] != 1 || (c[9] & 7) != 2)
        return fail("unexpected boot-control format");
    if (c[0] != '_' || c[1] != 'a' + current || c[2] || c[3])
        return fail("boot-control current suffix disagrees with kernel");
    if (Load32(c + 28) != uint32_t(crc32(0, c, 28)))
        return fail("boot-control CRC mismatch");
    if (!(c[12] & 15) || (!(c[12] & 0x70) && !(c[12] & 0x80)) || (c[13] & 1))
        return fail("Android A rescue slot is not healthy");
    if (before[32768] != 2 || Load32(before.data() + 32769) != 0x56740ab0 ||
        before[32773] != 0)
        return fail("unexpected or active Virtual A/B state");
    *expected = before;
    uint8_t* e = expected->data() + offset;
    for (unsigned i = 0; i < 2; ++i) {
        if (i == target) {
            e[12 + 2*i] = (e[12 + 2*i] & 0x80) | 15 | (6 << 4);
            if (target != current) e[13 + 2*i] &= ~1;
        } else if ((e[12 + 2*i] & 15) >= 15) {
            e[12 + 2*i] = (e[12 + 2*i] & 0xf0) | 14;
        }
    }
    Store32(e + 28, uint32_t(crc32(0, e, 28)));
    return true;
}
}  // namespace t40
