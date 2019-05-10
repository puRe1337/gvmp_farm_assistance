#pragma once
#include <chrono>

class timer
{
	using milli = std::chrono::milliseconds;
	using time_point = std::chrono::high_resolution_clock::time_point;
public:
	using seconds = /*td::chrono::seconds;*/ std::chrono::duration< double, std::ratio< 1 > >;

	auto diff( ) const {
		return std::chrono::duration_cast< seconds >( std::chrono::high_resolution_clock::now( ) - m_time ).count( );
	}

	void reset( ) {
		m_time = std::chrono::high_resolution_clock::now( );
	}

private:
	time_point m_time;
};
