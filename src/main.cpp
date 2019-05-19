#include <iostream>
#include <memory>

#include <Windows.h>
#include <gdiplus.h>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "gdiplus.lib")

#include <allheaders.h> // leptonica main header for image io
#include <baseapi.h> // tesseract main header
#include <thread>
#include <fstream>

#include <utils.hpp>
#include <timer.hpp>
#include <fmt/format.h>


constexpr auto window_name = "RAGE Multiplayer";

auto farm_state = false;
auto switch_state = false;
auto kill_process = false;
using time_p = std::chrono::high_resolution_clock::time_point;
time_p start{};

void farm_thread( HWND hwnd ) {
	while ( !kill_process ) {
		if ( farm_state ) {
			SendMessage( hwnd, WM_KEYDOWN, 0x45, 0x390000 );
			std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
			SendMessage( hwnd, WM_KEYUP, 0x45, 0x390000 );
			std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );
		}
		std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
	}
}

std::vector< std::string > signal_list = {
	"voll", "explodiert", "falsch", "ausgegangen", "Kocher"
};

std::vector< std::string > compare_list = {
	"Du hast wohl etwas falsch", "gemacht, dein Methkocher", "ist explodliert! Das tat weh!", "Da sie keine Materialien", "mehr haben, ist der Kocher", "ausgegangen!"
};

int main( ) {
	SetConsoleTitle( "assistance" );
	if ( IsValidCodePage( 65001 ) )
		SetConsoleOutputCP( 65001 );
	if ( FAILED( CoInitialize( NULL ) ) )
		return 0;
	fmt::print( "F10 = Start, F11 = Auto switch!\n" );
	tesseract::TessBaseAPI tess;

	if ( tess.Init( "./tessdata", "deu" ) ) {
		fmt::print( "OCRTesseract: Could not initialize tesseract.\n" );
		std::cin.get( );
		return 0;
	}
	// setup
	tess.SetPageSegMode( tesseract::PageSegMode::PSM_AUTO );
	tess.SetVariable( "save_best_choices", "T" );
	tess.SetVariable( "debug_file", "tesseract.log" );
	std::ofstream log( "ocr_log.txt", std::fstream::app );

	HWND hWnd = FindWindow( 0, window_name );
	if ( !hWnd ) {
		fmt::print( "Open {}!\n", window_name );
		std::cin.get( );
		return 0;
	}

	std::thread farm( farm_thread, hWnd );

	timer t;
	t.reset( );
	timer t2;
	t2.reset( );
	timer t3;
	t3.reset( );
	auto t3_reset_once = true;
	try {
		while ( 1 ) {
			hWnd = FindWindow( 0, window_name );
			if ( !hWnd ) {
				if ( t3_reset_once ) {
					t3_reset_once = false;
					t3.reset( );
				}
				if ( t3.diff( ) >= ( 60.0 * 10.0 ) ) //break if window has not been found since 15 minutes
					break;
				continue;
			}
			t3.reset( ); //reset if "window_name" found
			if ( !t3_reset_once )
				t3_reset_once = true;

			if ( GetAsyncKeyState( VK_F10 ) & 1 ) {
				farm_state = !farm_state;
				if ( farm_state )
					start = std::chrono::high_resolution_clock::now( );
				else
					fmt::print( "Farmen abgebrochen!\n" );
				std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
			}
			if ( GetAsyncKeyState( VK_F11 ) & 1 ) {
				switch_state = !switch_state;
				if ( switch_state )
					fmt::print( "Auto Switch start!\n" );
				else
					fmt::print( "Auto Switch abgebrochen!\n" );
				std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
			}

			if ( t.diff( ) >= 3 ) {
				static Rect rect_farm = { 30, 30, 240, 140 };
				std::vector< uint8_t > screen;
				if ( !take_screenshot( hWnd, screen, rect_farm ) )
					continue;
				auto str = get_ocr_text( tess, screen, true );
				fmt::print( "[{}]\n", str );
				log << fmt::format( "[{}]\n", str );
				if ( !str.empty( ) ) {

					for ( auto s : signal_list ) {
						if ( string_contains( str, s ) ) {
							Beep( 300, 300 );
							farm_state = false;
							time_p end = std::chrono::high_resolution_clock::now( );
							fmt::print( "Gefarmt: {}s\n", std::chrono::duration_cast< timer::seconds >( end - start ).count( ) );
						}
					}
					if ( string_contains( str, "Sie kochen nun Meth" ) ) {
						start = std::chrono::high_resolution_clock::now( );
						Beep( 300, 300 );
						fmt::print( "Kochen gestartet!\n" );
					}
					else if ( string_contains( str, "Meth kochen beendet!" ) ) {
						time_p end = std::chrono::high_resolution_clock::now( );
						fmt::print( "Gekocht: {}s\n", std::chrono::duration_cast< timer::seconds >( end - start ).count( ) );
						Beep( 300, 300 );
						fmt::print( "Kochen beendet!\n" );
					}
					else if ( string_contains( str, "Inventar ist voll" ) ) {
						fmt::print( "Inventar voll, �ffne Kofferraum\n" );
						send_opencar_msg( hWnd );
					}
					for ( auto s : compare_list ) {
						if ( string_contains( str, s ) ) {
							Beep( 300, 300 );
							farm_state = false;
							time_p end = std::chrono::high_resolution_clock::now( );
							fmt::print( "Gekocht: {}s\n", std::chrono::duration_cast< timer::seconds >( end - start ).count( ) );
						}
					}
				}
				t.reset( );
			}
			if ( t2.diff( ) >= 10 ) {
				auto col = scan_color( hWnd, 960, 670 );
				if ( GetRValue(col) == 255 && GetGValue(col) == 151 && GetBValue(col) == 0 ) {
					static Rect rect_afk = { 765, 365, 390, 380 };
					std::vector< uint8_t > screen;
					if ( !take_screenshot( hWnd, screen, rect_afk ) )
						continue;

					std::string str = get_ocr_text( tess, screen, true );
					if ( !str.empty( ) ) {
						if ( string_contains( str, "Bist du noch da" ) || string_contains( str, "ICH BIN NOCH DA" ) || string_contains( str, "anwesend" ) ) {
							fmt::print( "AFK-Check gefunden!\n" );
							Beep( 500, 500 );

							POINT p = { 960, 670 };
							ClientToScreen( hWnd, &p );
							SendMessage( hWnd, WM_LBUTTONDOWN, 0, MAKELPARAM(p.x, p.y) );
							SendMessage( hWnd, WM_LBUTTONUP, 0, MAKELPARAM(p.x, p.y) );
						}
					}

				}
				t2.reset( );
			}

			if ( switch_state ) {
				auto col1 = scan_color( hWnd, 500, 300 );
				auto col2 = scan_color( hWnd, 1020, 300 );
				auto check1 = GetRValue(col1) >= 254 && GetGValue(col1) >= 254 && GetBValue(col1) >= 254;
				auto check2 = GetRValue(col2) >= 254 && GetGValue(col2) >= 254 && GetBValue(col2) >= 254;
				if ( check1 && check2 ) {
					static Rect rect_rucksack = { 453, 250, 484, 552 };
					std::vector< uint8_t > rucksack;
					if ( !take_screenshot( hWnd, rucksack, rect_rucksack ) )
						continue;
					static Rect rect_kofferraum = { 977, 250, 483, 552 };
					std::vector< uint8_t > koferraum;
					if ( !take_screenshot( hWnd, koferraum, rect_kofferraum ) )
						continue;


					static Rect rect_ocr_rucksack = { 477, 300, 213, 54 };
					std::vector< uint8_t > ocr_rucksack;
					if ( !take_screenshot( hWnd, ocr_rucksack, rect_ocr_rucksack ) )
						continue;

					static Rect rect_ocr_kofferraum = { 1010, 300, 213, 54 };
					std::vector< uint8_t > ocr_kofferaum;
					if ( !take_screenshot( hWnd, ocr_kofferaum, rect_ocr_kofferraum ) )
						continue;

					auto str = get_ocr_text( tess, ocr_kofferaum, false );
					auto str2 = get_ocr_text( tess, ocr_rucksack, false );

					if ( string_contains( str, "Kofferraum" ) && string_contains( str2, "Rucksack" ) ) {

						//printf( "Leere Slots Rucksack: %llu\n", scan_for_image( rucksack, R"(D:\cpp_projects\gvmp_farm_assistance\blank.png)" ).size( ) );
						//printf( "Leere Slots Kofferraum: %llu\n", scan_for_image( koferraum, R"(D:\cpp_projects\gvmp_farm_assistance\blank.png)" ).size( ) );
						auto item = scan_for_image( rucksack, "./img/Kroeten.png" );
						if ( item.empty( ) ) {
							item = scan_for_image( rucksack, "./img/Kroeten2.png" );
							if ( item.empty( ) )
								item = scan_for_image( rucksack, "./img/Zinkkohle.png" );
						}
						auto koffer = scan_for_image( koferraum, "./img/blank.png" );
						if ( !item.empty( ) && !koffer.empty( ) ) {
							auto [x, y] = item.at( 0 );
							printf( "Smartphone: %d %d\n", x, y );

							auto [x2, y2] = koffer.at( 0 );
							printf( "Freier Platz Kofferraum: %d %d\n", x2, y2 );

							POINT p = { x + 435, y + 250 };
							ClientToScreen( hWnd, &p );

							POINT p2 = { x2 + 977, y2 + 250 };
							ClientToScreen( hWnd, &p2 );

							SendMessage( hWnd, WM_MOUSEMOVE, 0, MAKELPARAM(p.x, p.y) ); //maus auf das item was eingelagert werden soll
							std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
							SendMessage( hWnd, WM_LBUTTONDOWN, 0, MAKELPARAM(p.x, p.y) ); //klick auf "item"
							std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
							SendMessage( hWnd, WM_MOUSEMOVE, 0, MAKELPARAM(p2.x, p2.y) ); //move to freie platz
							std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
							SendMessage( hWnd, WM_LBUTTONUP, 0, MAKELPARAM(p2.x, p2.y) );
						}
						else if ( !item.empty( ) && koffer.empty( ) ) {
							//0xff880000 down, 0x00780000 up
							SendMessage( hWnd, WM_MOUSEWHEEL, 0xff880000, MAKELPARAM(1161, 496) );
						}
					}
				}
			}

			std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
		}
	}
	catch ( const std::exception& e ) {
		std::cerr << e.what( ) << std::endl;
	}


	kill_process = true;
	farm_state = false;
	farm.join( );
	tess.Clear( );
	tess.End( );
	CoUninitialize( );
	fmt::print( "Open {}!\n", window_name );
	std::cin.get( );


	return 1;
}
