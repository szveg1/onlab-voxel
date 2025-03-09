#include "Color.h"

Color Color::interpolate(const Color& c1, const Color& c2, float t)
{
	Color result;
	result.r = static_cast<unsigned char>(c1.r + t * (c2.r - c1.r));
	result.g = static_cast<unsigned char>(c1.g + t * (c2.g - c1.g));
	result.b = static_cast<unsigned char>(c1.b + t * (c2.b - c1.b));
	return result;
}

Color Color::heightToColor(float height)
{
	Color lowColor = { 128, 128, 128 }; // Green
	Color midColor = { 0, 153, 51 }; // Brown
	Color highColor = { 255, 255, 255 }; // White

	if (height < 0.5f)
	{
		// Interpolate between lowColor and midColor
		return interpolate(lowColor, midColor, height / 0.5f);
	}
	else
	{
		// Interpolate between midColor and highColor
		return interpolate(midColor, highColor, (height - 0.5f) / 0.5f);
	}
}
