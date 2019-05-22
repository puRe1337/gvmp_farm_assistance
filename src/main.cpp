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
#include "globals.hpp"


int main( ) {
	SetConsoleTitle( "assistance" );
	if ( IsValidCodePage( 65001 ) )
		SetConsoleOutputCP( 65001 );
	if ( FAILED( CoInitialize( NULL ) ) )
		return 0;
	fmt::print( "F11 = Auto switch!\n" );
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

	HWND hWnd = FindWindow( 0, globals::window_name );
	if ( !hWnd ) {
		fmt::print( "Open {}!\n", globals::window_name );
		std::cin.get( );
		return 0;
	}


	timer t;
	t.reset( );
	timer t2;
	t2.reset( );
	timer t3;
	t3.reset( );
	auto t3_reset_once = true;
	try {
		while ( 1 ) {
			hWnd = FindWindow( 0, globals::window_name );
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

			if ( GetAsyncKeyState( VK_F11 ) & 1 ) {
				globals::switch_state = !globals::switch_state;
				if ( globals::switch_state )
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

					for ( auto s : globals::signal_list ) {
						if ( string_contains( str, s ) ) {
							Beep( 300, 300 );
							time_p end = std::chrono::high_resolution_clock::now( );
							fmt::print( "Gefarmt: {}s\n", std::chrono::duration_cast< timer::seconds >( end - globals::start ).count( ) );
						}
					}
					if ( string_contains( str, "Farming gestartet" ) ) {
						//Farmen gestartet
						globals::start = std::chrono::high_resolution_clock::now( );
						Beep( 300, 300 );
						fmt::print( "Farmen gestartet!\n" );
					}
					else if ( string_contains( str, "Sie kochen nun Meth" ) ) {
						//Kochen gestartet
						globals::start = std::chrono::high_resolution_clock::now( );
						Beep( 300, 300 );
						fmt::print( "Kochen gestartet!\n" );
					}
					else if ( string_contains( str, "Meth kochen beendet!" ) ) {
						//Kochen beendet
						time_p end = std::chrono::high_resolution_clock::now( );
						fmt::print( "Gekocht: {}s\n", std::chrono::duration_cast< timer::seconds >( end - globals::start ).count( ) );
						Beep( 300, 300 );
						fmt::print( "Kochen beendet!\n" );
					}
					else if ( string_contains( str, "Inventar ist voll" ) || string_contains( str, "Farming beendet" ) ) {
						//Farmen beendet
						fmt::print( "Inventar voll, �ffne Kofferraum\n" );
						stop_farming( hWnd );
					}
					for ( auto s : globals::compare_list ) {
						if ( string_contains( str, s ) ) {
							Beep( 300, 300 );
							time_p end = std::chrono::high_resolution_clock::now( );
							fmt::print( "Gekocht: {}s\n", std::chrono::duration_cast< timer::seconds >( end - globals::start ).count( ) );
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

			if ( globals::switch_state ) {
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
						auto found_item = globals::item_definitions::null;
						//printf( "Leere Slots Rucksack: %llu\n", scan_for_image( rucksack, R"(D:\cpp_projects\gvmp_farm_assistance\blank.png)" ).size( ) );
						//printf( "Leere Slots Kofferraum: %llu\n", scan_for_image( koferraum, R"(D:\cpp_projects\gvmp_farm_assistance\blank.png)" ).size( ) );
						auto item = scan_for_image( rucksack, "./img/Kroeten.png" );
						found_item = globals::item_kroete;
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

							send_move_item_msg( hWnd, p, p2 );
						}
						else if ( !item.empty( ) && koffer.empty( ) ) {
							//0xff880000 down, 0x00780000 up
							send_mwheel_down_msg( hWnd, { 1161, 496 } );
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
	tess.Clear( );
	tess.End( );
	CoUninitialize( );
	fmt::print( "Open {}!\n", globals::window_name );
	std::cin.get( );


	return 1;
}
