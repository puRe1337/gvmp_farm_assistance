#pragma once

using time_p = std::chrono::high_resolution_clock::time_point;

namespace globals
{
	constexpr auto window_name = "RAGE Multiplayer";

	inline auto switch_state = false;
	inline time_p start{};


	inline std::vector< std::string > signal_list = {
		"voll", "explodiert", "falsch", "ausgegangen", "Kocher", "Farming beendet"
	};

	inline std::vector< std::string > compare_list = {
		"Du hast wohl etwas falsch", "gemacht, dein Methkocher", "ist explodliert! Das tat weh!", "Da sie keine Materialien", "mehr haben, ist der Kocher", "ausgegangen!"
	};

	enum item_definitions
	{
		null = 0,
		item_kroete,
		item_kroete2,
		item_zinkkohle,
		item_aramidfaser
	};

	inline std::vector< std::string > item_names = {
		"no item",
		"Kroete(n)",
		"Kroete(n)2",
		"Zinkkohle",
		"Aramidfaser(n)"
	};
}
