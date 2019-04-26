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

#include "utils.hpp"


int main( ) {
	SetConsoleTitle("assistance");
	tesseract::TessBaseAPI tess;

	if ( tess.Init( "./tessdata", "deu" ) ) {
		std::cout << "OCRTesseract: Could not initialize tesseract." << std::endl;
		std::cin.get( );
		return 0;
	}
	std::ofstream log( "ocr_log.txt", std::fstream::app );

	HWND hWnd = FindWindow( 0, "RAGE Multiplayer" );


	while ( hWnd ) {
		if ( !take_screenshot( hWnd ) )
			continue;

		// setup
		tess.SetPageSegMode( tesseract::PageSegMode::PSM_AUTO );
		tess.SetVariable( "save_best_choices", "T" );
		// read image
		auto pixs = pixRead( "screen.png" );
		pixs = pixConvertRGBToGray( pixs, 0.0f, 0.0f, 0.0f ); //grey "filter"
		pixs = pixScaleGrayLI( pixs, 5.5f, 5.5f ); // zoom
		if ( !pixs ) {
			continue;
		}

		// recognize
		tess.SetImage( pixs );
		tess.Recognize( 0 );
		std::cout << std::unique_ptr< char[] >( tess.GetUTF8Text( ) ).get( ) << std::endl;
		log << std::unique_ptr< char[] >( tess.GetUTF8Text( ) ).get( ) << std::endl;
		std::string str = std::unique_ptr< char[] >( tess.GetUTF8Text( ) ).get( );
		if ( string_contains( str, "voll" ) || string_contains( str, "explodiert" ) || string_contains( str, "falsch" ) ) {
			Beep( 300, 300 );
		}

		tess.Clear( );
		//pixWrite("newscreen.png", pixs, IFF_PNG);
		pixDestroy( &pixs );

		hWnd = FindWindow( 0, "RAGE Multiplayer" );
		std::this_thread::sleep_for( std::chrono::seconds( 5 ) );
	}
	std::cout << "Open RAGE Multiplayer!" << std::endl;
	std::cin.get( );


	return 1;
}
