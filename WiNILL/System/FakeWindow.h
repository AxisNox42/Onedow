#pragma once
#include <string>

class FakeWindow {
public:
	int id;
	std::string title;

	float x, y;
	float width, height;
	float titleBarHeight;

	bool isFocused;
	bool isDragging;
	bool isMinimized;

	float dragOffsetX;
	float dragOffsetY;

	FakeWindow(int id, std::string title, float x, float y, float w, float h) :
		id(id), title(title), x(x), y(y), width(w), height(h),
		titleBarHeight(31.0f), isFocused(false), isDragging(false), isMinimized(false),
		dragOffsetX(0.0f), dragOffsetY(0.0f) {}

	void Update(float deltaTime, float mouseX, float mouseY, bool mousePressed);

	bool IsMouseInTitleBar(float mouseX, float mouseY) {
		return (mouseX >= x && mouseX <= x + width &&
			mouseY >= y && mouseY <= y + titleBarHeight);
	}

	bool IsMouseInWindow(float mouseX, float mouseY) {
		if (isMinimized) return IsMouseInTitleBar(mouseX, mouseY);
		return (mouseX >= x && mouseX <= x + width &&
			mouseY >= y && mouseY <= y + titleBarHeight);
	}
};