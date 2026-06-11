#ifndef OBLIBERRY_HEXCULLING_H
#define OBLIBERRY_HEXCULLING_H

#include "HexCoords.h"
#include "HexMath.h"
#include "../Renderer/Camera.h"

struct VisibleHexRange {
    int minQ;
    int maxQ;
    int minR;
    int maxR;

    static VisibleHexRange Calculate(
        const Camera &camera,
        float windowWidth,
        float windowHeight) {
        glm::vec2 tl = camera.MouseToWorld(
            0,
            0,
            windowWidth,
            windowHeight);

        glm::vec2 tr = camera.MouseToWorld(
            windowWidth,
            0,
            windowWidth,
            windowHeight);

        glm::vec2 bl = camera.MouseToWorld(
            0,
            windowHeight,
            windowWidth,
            windowHeight);

        glm::vec2 br = camera.MouseToWorld(
            windowWidth,
            windowHeight,
            windowWidth,
            windowHeight);

        HexCoords h0 = HexMath::PixelToHex(tl);
        HexCoords h1 = HexMath::PixelToHex(tr);
        HexCoords h2 = HexMath::PixelToHex(bl);
        HexCoords h3 = HexMath::PixelToHex(br);

        constexpr int Padding = 4;

        VisibleHexRange range;

        range.minQ = std::min({
                         h0.q,
                         h1.q,
                         h2.q,
                         h3.q
                     }) - Padding;

        range.maxQ = std::max({
                         h0.q,
                         h1.q,
                         h2.q,
                         h3.q
                     }) + Padding;

        range.minR = std::min({
                         h0.r,
                         h1.r,
                         h2.r,
                         h3.r
                     }) - Padding;

        range.maxR = std::max({
                         h0.r,
                         h1.r,
                         h2.r,
                         h3.r
                     }) + Padding;

        return range;
    }
};

#endif //OBLIBERRY_HEXCULLING_H
