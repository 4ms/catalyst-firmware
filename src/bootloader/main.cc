#include "bootloader.hh"
#include "conf/flash_layout.hh"
#include "conf/rcc_conf.hh"
#include "debug.hh"
#include "drivers/system.hh"
#include "drivers/system_clocks.hh"
#include "system_target.hh"

namespace Catalyst2::Bootloader
{
struct System {
	System() {
		mdrivlib::System::SetVectorTable(BootloaderFlashAddr);
		mdrivlib::SystemClocks::init_clocks(osc_conf, clk_conf, rcc_periph_conf);

		Debug::Pin0{};

		SystemTarget::init();
	}
};

} // namespace Catalyst2::Bootloader

void main() {

	Catalyst2::Bootloader::System system_init;
	Catalyst2::Bootloader::GateBootloader bootloader;

	if (bootloader.check_enter_bootloader())
		bootloader.run();

	mdrivlib::System::reset_buses();
	mdrivlib::System::reset_RCC();
	mdrivlib::System::jump_to(Catalyst2::AppFlashAddr);
	while (true) {
		__NOP();
	}
}
