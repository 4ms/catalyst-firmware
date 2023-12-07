#pragma once

namespace Catalyst2
{

template<typename SeqModeData, typename MacroModeData>
class SavedSettings {

public:
	template<typename DataT>
	bool read(DataT &data);

	template<typename DataT>
	bool write(DataT const &data);

	template<>
	bool read(SeqModeData &data) {
		return true;
	}

	template<>
	bool write(SeqModeData const &data) {
		return true;
	}

	template<>
	bool read(MacroModeData &data) {
		return true;
	}

	template<>
	bool write(MacroModeData const &data) {
		return true;
	}
};

} // namespace Catalyst2
