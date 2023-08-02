#include "controls.hh"

namespace Catalyst2
{

void run_hardware_test(Controls &controls)
{
	//
	controls.start();

	while (true) {
		// if (HAL_GetTick() > 10)
		// 	__BKPT();
		// auto slider = controls.read_slider();
		// if (slider > 4000)
		// 	controls.set_encoder_led(0, Colors::white);
		// else if (slider > 3000)
		// 	controls.set_encoder_led(1, Colors::white);
		// else if (slider > 2000)
		// 	controls.set_encoder_led(2, Colors::white);
		// else
		// 	controls.set_encoder_led(3, Colors::white);

		// controls.write_to_encoder_leds();
	}
}

} // namespace Catalyst2
