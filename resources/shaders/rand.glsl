// seeded random function
#pragma once

uint hash(uint x)
{
    x ^= x >> 16;
    x *= 0x7feb352du;
    x ^= x >> 15;
    x *= 0x846ca68bu;
    x ^= x >> 16;
    return x;
}

// rand from vec2 seed
float rand(vec2 p)
{
    uvec2 v = floatBitsToUint(p);
    uint h = hash(v.x ^ hash(v.y));
    return float(h) / 4294967296.0;
}
