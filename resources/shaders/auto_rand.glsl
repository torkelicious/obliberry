
#pragma once

#ifndef TIME_UNIFORM_
#define TIME_UNIFORM_
uniform float u_Time;                // seconds since engine start
#endif

#ifndef RESOLUTION_UNIFORM_
#define RESOLUTION_UNIFORM_
uniform vec2 u_Resolution;           // render target size in px
#endif

#include <rand.glsl>

// different value per coordinate , per frame
float auto_rand(vec2 coord) {
    return rand(coord + vec2(fract(u_Time * 61.0), fract(u_Time * 83.0)));
}
