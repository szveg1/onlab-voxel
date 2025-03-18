#pragma once
#include <cstdint>

struct Color
{
	unsigned char r, g, b, a;

	Color interpolate(const Color& c1, const Color& c2, float t);
	Color heightToColor(float height);
	uint32_t rgba() const;
};