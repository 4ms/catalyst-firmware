#pragma once

#include "controls.hh"
#include "params.hh"
#include "seq_common.hh"

namespace Catalyst2::Sequencer::Ui
{
class Reset : public Usual {
	enum class State {
		Wait,
		Proceed,
	};
	State state;

public:
	using Usual::Usual;
	void Init() override {
		state = State::Wait;
		p.player.Stop();
	}
	void Update(Abstract *&interface) {
		switch (state) {
			using enum State;
			case Wait: {
				if (!c.button.play.is_high() && !c.button.shift.is_high()) {
					state = Proceed;
				}
				break;
			}
			case Proceed: {
				if (c.button.add.is_high() || c.button.fine.is_high() || c.button.morph.is_high() ||
					c.button.bank.is_high() || c.button.shift.is_high())
				{
					return;
				}
				for (auto &i : c.button.scene) {
					if (i.is_high()) {
						return;
					}
				}
				if (c.button.play.is_high()) {
					p.data = SeqMode::Data{};
					p.player.Stop();
					return;
				}
			}
		}

		interface = this;
	}

	void PaintLeds(const Model::Output::Buffer &outs) override {
		ClearEncoderLeds();
		ClearButtonLeds();
	}
};

} // namespace Catalyst2::Sequencer::Ui