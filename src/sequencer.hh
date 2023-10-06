#pragma once
#include "channelmode.hh"
#include "channelvalue.hh"
#include "clockdivider.hh"
#include "conf/model.hh"
#include "randompool.hh"
#include "util/countzip.hh"
#include <array>
#include <cstdint>

namespace Catalyst2
{

struct Sequencer {
	enum class PlayMode : uint8_t {
		Sequential,
		PingPong,
		Random,
		NumPlayModes,
	};
	using SequenceId = uint8_t;
	using SeqData = std::array<Channel, Model::MaxSeqSteps>;
	struct Sequence {
		SeqData data;
		uint8_t length = Model::SeqStepsPerPage;
		uint8_t start_offset = 0;
		ChannelMode channelmode;
	};
	struct Player {
		ClockDivider clockdivider;
		PlayMode playmode = PlayMode::Sequential;
		uint8_t counter = 0;
		uint8_t step = 0;
		bool pingpong = true;
	};

	std::array<Sequence, Model::NumChans> sequence;
	std::array<Player, Model::NumChans> player;
	SequenceId cur_chan = 0;
	uint8_t cur_page = Model::SeqPages;

	ClockDivider &ClockDiv()
	{
		return player[cur_chan].clockdivider;
	}

	void IncStep(uint8_t step, int32_t inc, bool fine)
	{
		if (step >= Model::MaxSeqSteps)
			return;

		sequence[cur_chan].data[step].Inc(inc, fine, GetChanMode(cur_chan).IsGate());
	}

	bool IsChanSelected()
	{
		return cur_chan < Model::NumChans;
	}

	void SelChan(SequenceId chan)
	{
		if (chan >= Model::NumChans || chan == cur_chan) {
			cur_chan = Model::NumChans;
			return;
		}

		cur_chan = chan;
	}

	void DeselChan()
	{
		cur_chan = Model::NumChans;
	}

	SequenceId GetSelChan()
	{
		return cur_chan;
	}

	void SelPage(uint8_t page)
	{
		if (page == cur_page) {
			DeselPage();
			return;
		}

		cur_page = page;
	}

	void DeselPage()
	{
		cur_page = Model::SeqPages;
	}

	uint8_t GetSelPage()
	{
		return cur_page;
	}

	bool IsPageSelected()
	{
		return cur_page < Model::SeqPages;
	}

	bool GetClock() const
	{
		return clock_;
	}

	void Step()
	{
		clock_ = !clock_;

		for (auto [i, s, p] : countzip(sequence, player)) {
			p.clockdivider.Update();
			if (!p.clockdivider.Step())
				continue;

			p.counter += 1;
			const auto l = p.playmode == PlayMode::PingPong ? s.length - 1 : s.length;
			if (p.counter >= l) {
				p.counter = 0;
				p.pingpong = !p.pingpong;
			}

			auto step = p.counter + s.start_offset;
			step %= Model::MaxSeqSteps;

			switch (p.playmode) {
				case PlayMode::Sequential:
					p.step = step;
					break;
				case PlayMode::PingPong: {
					if (!p.pingpong)
						p.step = step;
					else
						p.step = (s.length + s.start_offset - 1) - p.counter;
					break;
				}
				case PlayMode::Random:
					p.step = (RandomPool::GetRandomVal(i, step) % s.length) + s.start_offset;
					break;
				default:
					break;
			}
		}
	}

	void Reset()
	{
		for (auto [i, s, p] : countzip(sequence, player)) {
			p.counter = 0;
			p.clockdivider.Reset();
			p.pingpong = false;
			p.step = s.start_offset;
		}
	}

	void AdjLength(int32_t dir)
	{
		int temp = sequence[cur_chan].length;
		temp += dir;
		temp = std::clamp<int32_t>(temp, 1, Model::MaxSeqSteps);
		sequence[cur_chan].length = temp;
	}

	void AdjStartOffset(int32_t dir)
	{
		int temp = sequence[cur_chan].start_offset;
		temp += dir;
		temp = std::clamp<int32_t>(temp, 0, Model::MaxSeqSteps - 1);
		sequence[cur_chan].start_offset = temp;
	}

	void AdjPlayMode(int32_t dir)
	{
		auto t = static_cast<int32_t>(player[cur_chan].playmode);
		t += dir;
		t = std::clamp<int32_t>(t, 0, static_cast<int32_t>(PlayMode::NumPlayModes) - 1);
		player[cur_chan].playmode = static_cast<PlayMode>(t);
	}

	PlayMode GetPlayMode()
	{
		return player[cur_chan].playmode;
	}

	void ResetLength(SequenceId seq)
	{
		sequence[seq].length = Model::SeqStepsPerPage;
	}

	void ResetStartOffset(SequenceId seq)
	{
		sequence[seq].start_offset = 0;
	}

	uint8_t GetLength()
	{
		return sequence[cur_chan].length;
	}

	uint8_t GetStartOffset()
	{
		return sequence[cur_chan].start_offset;
	}

	uint8_t GetStep(SequenceId seq)
	{
		return player[seq].step;
	}

	uint8_t GetStepPage(SequenceId seq)
	{
		return GetStep(seq) / Model::SeqPages;
	}

	ChannelValue::type GetStepValue(SequenceId seq, uint8_t step)
	{
		return sequence[seq].data[step].val;
	}

	ChannelMode GetChanMode(SequenceId seq)
	{
		return sequence[seq].channelmode;
	}

	void SetChanMode(SequenceId seq, ChannelMode mode)
	{
		sequence[seq].channelmode = mode;
	}

	void IncChanMode(SequenceId seq, int32_t dir)
	{
		sequence[seq].channelmode.Inc(dir);
	}

private:
	bool clock_ = false;
};

} // namespace Catalyst2
