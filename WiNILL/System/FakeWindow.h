#pragma once
// Gameplay bounds for the player's orbital field. This is not a UI surface.
class SpatialBounds {
public:
	float x, y;
	float width, height;

	SpatialBounds(float x, float y, float w, float h) :
		x(x), y(y), width(w), height(h) {}
};
