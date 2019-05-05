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

auto farm_state = false;
auto kill_process = false;
using time_p = std::chrono::high_resolution_clock::time_point;
time_p start{};

void farm_thread( HWND hwnd ) {
	while ( !kill_process ) {
		if ( GetAsyncKeyState( VK_F10 ) & 1 ) {
			farm_state = !farm_state;
			if ( farm_state )
				start = std::chrono::high_resolution_clock::now( );
			else
				fmt::print( "Farmen abgebrochen!\n" );
			std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
		}
		if ( farm_state ) {
			SendMessage( hwnd, WM_KEYDOWN, 0x45, 0x390000 );
			std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
			SendMessage( hwnd, WM_KEYUP, 0x45, 0x390000 );
			std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );
		}
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
	fmt::print( "Start by pressing F10!\n" );
	tesseract::TessBaseAPI tess;

	if ( tess.Init( "./tessdata", "deu" ) ) {
		fmt::print( "OCRTesseract: Could not initialize tesseract.\n" );
		std::cin.get( );
		return 0;
	}
	std::ofstream log( "ocr_log.txt", std::fstream::app );

	HWND hWnd = FindWindow( 0, "RAGE Multiplayer" );
	if ( !hWnd ) {
		fmt::print( "Open RAGE Multiplayer!\n" );
		std::cin.get( );
		return 0;
	}

	std::thread farm( farm_thread, hWnd );

	try {
		while ( hWnd ) {
			hWnd = FindWindow( 0, "RAGE Multiplayer" );
			if ( !hWnd )
				continue;

			std::vector< uint8_t > screen;
			if ( !take_screenshot( hWnd, screen ) )
				continue;

			// setup
			tess.SetPageSegMode( tesseract::PageSegMode::PSM_AUTO );
			tess.SetVariable( "save_best_choices", "T" );
			// read image
			auto pixs = pixReadMemPng( screen.data( ), screen.size( ) );
			//auto pixs = pixRead( "screen.png" );
			pixs = pixConvertRGBToGray( pixs, 0.0f, 0.0f, 0.0f ); //grey "filter"
			pixs = pixScaleGrayLI( pixs, 5.5f, 5.5f ); // zoom
			if ( !pixs ) {
				continue;
			}

			// recognize
			tess.SetImage( pixs );
			tess.Recognize( 0 );
			std::string str = std::unique_ptr< char[] >( tess.GetUTF8Text( ) ).get( );
			fmt::print( "[{}]\n", str );
			log << fmt::format( "[{}]\n", str );
			if ( !str.empty( ) ) {

				for ( auto s : signal_list ) {
					if ( string_contains( str, s ) ) {
						Beep( 300, 300 );
						farm_state = false;
						time_p end = std::chrono::high_resolution_clock::now( );
						fmt::print( "Gefarmt: {}s\n", std::chrono::duration_cast< std::chrono::seconds >( end - start ).count( ) );
					}
				}
				if ( string_contains( str, "Sie kochen nun Meth" ) ) {
					start = std::chrono::high_resolution_clock::now( );
					Beep( 300, 300 );
					fmt::print( "Kochen gestartet!\n" );
				}
				else if ( string_contains( str, "Meth kochen beendet!" ) ) {
					time_p end = std::chrono::high_resolution_clock::now( );
					fmt::print( "Gekocht: {}s\n", std::chrono::duration_cast< std::chrono::seconds >( end - start ).count( ) );
					Beep( 300, 300 );
					fmt::print( "Kochen beendet!\n" );
				}
				for ( auto s : compare_list ) {
					if ( string_contains( str, s ) ) {
						Beep( 300, 300 );
						farm_state = false;
						time_p end = std::chrono::high_resolution_clock::now( );
						fmt::print( "Gekocht: {}s\n", std::chrono::duration_cast< std::chrono::seconds >( end - start ).count( ) );
					}
				}
			}
			tess.Clear( );
			//pixWrite( "newscreen.png", pixs, IFF_PNG );
			pixDestroy( &pixs );

			std::this_thread::sleep_for( std::chrono::seconds( 3 ) );
		}
	}
	catch ( const std::exception& e ) {
		std::cerr << e.what( ) << std::endl;
	}


	kill_process = true;
	farm_state = false;
	farm.join( );
	tess.End( );
	CoUninitialize( );
	fmt::print( "Open RAGE Multiplayer!\n" );
	std::cin.get( );


	return 1;
}
