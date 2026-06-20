#include "FakeWindow.h"

void FakeWindow::Update(float deltaTime, float mouseX, float mouseY, bool mousePressed) {
	if (isMinimized)return;

	if (mousePressed && !isDragging) {
		if (IsMouseInTitleBar(mouseX, mouseY) && isFocused) {
			isDragging = true;

			dragOffsetX = mouseX - x;
			dragOffsetY = mouseY - y;
		}
	}

	if (isDragging) {
		if (mousePressed) {
			x = mouseX - dragOffsetX;
			y = mouseY - dragOffsetY;
		}
		else {
			isDragging = false;
		}
	}
}