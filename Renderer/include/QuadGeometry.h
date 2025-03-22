#pragma once
#include "Geometry.h"

class QuadGeometry : public Geometry
{
public:
	QuadGeometry();
	~QuadGeometry() override;
	void draw() override;
private:

};