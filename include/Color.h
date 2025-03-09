#pragma once

struct Color
{
	unsigned char r, g, b;

	Color interpolate(const Color& c1, const Color& c2, float t);
	Color heightToColor(float height);
};