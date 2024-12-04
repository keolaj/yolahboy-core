#pragma once

#include "../global_definitions.h"

Controller* create_controller();
void print_controller(Controller c);
u8 joypad_return(Controller controller, u8 data);
void joypad_write(Controller controller, u8 data);

void destroy_controller(Controller* controller);
