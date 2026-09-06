#pragma once

vec3 rgb_to_hsv(vec3 c) {
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));

    float d = q.x - min(q.w, q.y);
    float e = 1e-10;

    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

vec3 hsv_to_rgb (vec3 c){
    vec3 p = abs(fract(c.xxx + vec3(0.0, 2.0 / 3.0, 1.0 / 3.0)) * 6.0 - 3.0);
    return c.z * mix(vec3(1.0), clamp(p - 1.0, 0.0, 1.0), c.y);
}

float luma(vec3 c) { return dot(c, vec3(0.2126, 0.7152, 0.0722)); }

// smooth bell shaped weight around a center
float tonal_weight(float x, float center, float width) {
    float d = (x - center) / max(width, 0.0001);
    return exp(-0.5 * d * d);
}

// Lift Gamma Gain
vec3 apply_LGG(vec3 color, vec3 lift, vec3 gamma, vec3 gain) {
    color = max(color, vec3(0.0));

    // Lift
    color += lift;

    color = max(color, vec3(0.0));

    // Gamma
    color = pow(color, 1.0 / max(gamma, vec3(0.001)));

    // Gain
    color *= gain;

    return color;
}

// contrast around a pivot.
vec3 apply_contrast(vec3 color, float contrast, float pivot) { return pivot + (color - pivot) * contrast; }

// saturation
vec3 apply_saturation(vec3 color, float saturation) {
    float y = luma(color);

    return mix(vec3(y), color, saturation);
}

vec3 apply_color_wheels(vec3 color, vec3 shadow, vec3 midtone, vec3 highlight) {
    float y = clamp(luma(color), 0.0, 1.0);

    float shadows = tonal_weight(y, 0.18, 0.20);
    float mids = tonal_weight(y, 0.50, 0.25);
    float highs = tonal_weight(y, 0.82, 0.20);

    // Normalize
    float total = shadows + mids + highs;

    if (total > 0.0001) {
        shadows /= total;
        mids /= total;
        highs /= total;
    }

    color += shadow * shadows;
    color += midtone * mids;
    color += highlight * highs;

    return color;
}
