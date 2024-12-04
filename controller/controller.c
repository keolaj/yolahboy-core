#include <stdio.h>
#include <string.h>
#include "controller.h"


void print_controller(Controller c) {
	printf("CONTROLLER STATE:\nA: %d\nB: %d\nUP: %d\nDOWN: %d\nLEFT: %d\nRIGHT: %d\nSELECT: %d\nSTART: %d\n", c.a, c.b, c.up, c.down, c.left, c.right, c.select, c.start);
}

Controller* create_controller() {
	Controller* ret = malloc(sizeof(Controller));
	if (ret == NULL) {
		printf("Could not initialize controller!");
		return NULL;
	}
	memset(ret, 0, sizeof(Controller));
	return ret;
}

u8 joypad_return(Controller controller, u8 data) {
	bool select_buttons = data & (1 << 5);
	bool select_dpad = data & (1 << 4);
	u8 ret = 0b00001111;
	if (select_buttons && select_dpad) return 0x0F;
	if (!select_buttons) {
		ret |= 0b00100000;
		if (controller.start) ret &= ~(1 << 3);
		if (controller.select) ret &= ~(1 << 2);
		if (controller.b) ret &= ~(1 << 1);
		if (controller.a) ret &= ~(1);
	}
	if (!select_dpad) {
		ret |= 0b00010000;
		if (controller.down) ret &= ~(1 << 3);
		if (controller.up) ret &= ~(1 << 2);
		if (controller.left) ret &= ~(1 << 1);
		if (controller.right) ret &= ~(1);
	}
	return ret;
}

void destroy_controller(Controller* controller) {
	if (controller == NULL) return;
	free(controller);
}
