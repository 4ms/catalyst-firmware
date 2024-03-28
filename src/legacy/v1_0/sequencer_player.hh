#pragma once

#include "conf/model.hh"
#include "queue.hh"
#include "random.hh"
#include "sequence_phaser.hh"
#include "sequencer_settings.hh"
#include "song_mode.hh"
#include "util/math.hh"
#include <array>
#include <cstdlib>

namespace Catalyst2::Legacy::V1_0::Sequencer::Player
{
struct Data {
	Random::Sequencer::Steps::Data randomsteps;
	bool Validate() const {
		return randomsteps.Validate();
	}
};
} // namespace Catalyst2::Legacy::V1_0::Sequencer::Player
