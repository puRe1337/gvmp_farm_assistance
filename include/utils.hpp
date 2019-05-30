//#define WIN64_LEAN_AND_MEAN
#pragma once
#include <Windows.h>
#include <atlbase.h>
#include <gdiplus.h>
#include <memory>
#include <vector>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include "globals.hpp"

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

static int GetEncoderClsid( const WCHAR* format, CLSID* pClsid ) {
	UINT num = 0; // number of image encoders
	UINT size = 0; // size of the image encoder array in bytes

	ImageCodecInfo* pImageCodecInfo = NULL;

	GetImageEncodersSize( &num, &size );
	if ( size == 0 )
		return -1; // Failure

	pImageCodecInfo = ( ImageCodecInfo* )( malloc( size ) );
	if ( pImageCodecInfo == NULL )
		return -1; // Failure

	GetImageEncoders( num, size, pImageCodecInfo );

	for ( UINT j = 0; j < num; ++j ) {
		if ( wcscmp( pImageCodecInfo[ j ].MimeType, format ) == 0 ) {
			*pClsid = pImageCodecInfo[ j ].Clsid;
			free( pImageCodecInfo );
			return j; // Success
		}
	}

	free( pImageCodecInfo );
	return -1; // Failure
}


struct gdiplus_init
{
	gdiplus_init( ) {
		GdiplusStartupInput inp;
		GdiplusStartupOutput outp{};
		if ( Ok != GdiplusStartup( &token_, &inp, &outp ) )
			throw std::runtime_error( "GdiplusStartup" );
	}

	~gdiplus_init( ) {
		GdiplusShutdown( token_ );
	}

private:
	ULONG_PTR token_{};
};


static bool take_screenshot( HWND hWnd, std::vector< uint8_t >& data, Rect rect ) {
	using namespace Gdiplus;
	gdiplus_init gdiplus;

	HDC desktopdc = GetDC( hWnd );
	HDC mydc = CreateCompatibleDC( desktopdc );

	RECT target_rect;
	GetClientRect( hWnd, &target_rect );
	int width = target_rect.right - target_rect.left;
	int height = target_rect.bottom - target_rect.top;

	HBITMAP mybmp = CreateCompatibleBitmap( desktopdc, width, height );
	HBITMAP oldbmp = ( HBITMAP )SelectObject( mydc, mybmp );
	BitBlt( mydc, 0, 0, width, height, desktopdc, 0, 0, SRCCOPY | CAPTUREBLT );
	SelectObject( mydc, oldbmp );

	std::unique_ptr< Bitmap > b( Bitmap::FromHBITMAP( mybmp, NULL ) );


	std::unique_ptr< Bitmap > ba( b->Clone( rect, b->GetPixelFormat( ) ) );
	CLSID encoderClsid;
	if ( ba && GetEncoderClsid( L"image/png", &encoderClsid ) != -1 ) {
		//Status stat = ba->Save( L"screen.png", &encoderClsid, NULL );

		CComPtr< IStream > istream;
		CreateStreamOnHGlobal( NULL, TRUE, &istream );
		Status stat = ba->Save( istream, &encoderClsid );
		if ( stat != Gdiplus::Status::Ok ) {
			// cleanup
			SelectObject( mydc, oldbmp );
			DeleteDC( mydc );
			ReleaseDC( hWnd, desktopdc );
			DeleteObject( mybmp );
			return false;
		}

		//get memory handle associated with istream
		HGLOBAL hg = NULL;
		GetHGlobalFromStream( istream, &hg );

		//copy IStream to buffer
		const auto bufsize = static_cast< int >( GlobalSize( hg ) );
		data.resize( bufsize );

		//lock & unlock memory
		const auto pImage = GlobalLock( hg );
		memcpy( &data[ 0 ], pImage, bufsize );
		GlobalUnlock( hg );
	}
	else {
		// cleanup
		SelectObject( mydc, oldbmp );
		DeleteDC( mydc );
		ReleaseDC( hWnd, desktopdc );
		DeleteObject( mybmp );
		return false;
	}

	// cleanup
	SelectObject( mydc, oldbmp );
	DeleteDC( mydc );
	ReleaseDC( hWnd, desktopdc );
	DeleteObject( mybmp );
	return true;
}

static COLORREF scan_color( HWND hWnd, int x, int y ) {
	using namespace Gdiplus;
	gdiplus_init gdiplus;

	HDC desktopdc = GetDC( hWnd );

	auto pixel = GetPixel( desktopdc, x, y );

	ReleaseDC( hWnd, desktopdc );
	return pixel;
}

static bool string_contains( const std::string& str, const std::string& comp ) {
	return str.find( comp ) != std::string::npos;
}

static std::vector< std::pair< int, int > > scan_for_image( const std::vector< uint8_t >& screen_data, const std::string& path ) {
	std::vector< std::pair< int, int > > return_vec{};

	cv::Mat img = cv::imdecode( screen_data, cv::IMREAD_COLOR );
	auto templ = cv::imread( path, cv::IMREAD_COLOR );

	int match_method = cv::TM_CCOEFF_NORMED;
	int result_cols = img.cols - templ.cols + 1;
	int result_rows = img.rows - templ.rows + 1;
	cv::Mat result;
	result.create( result_rows, result_cols, CV_32FC1 );

	cv::matchTemplate( img, templ, result, match_method );

	//normalize(result, result, 0, 1, cv::NORM_MINMAX, -1, cv::Mat());

	cv::threshold( result, result, 0.7, 1., cv::THRESH_BINARY );
	int count = 0;
	while ( 1 ) {
		double minVal;
		double maxVal;
		cv::Point minLoc;
		cv::Point maxLoc;
		cv::Point matchLoc;
		cv::minMaxLoc( result, &minVal, &maxVal, &minLoc, &maxLoc, cv::Mat( ) );
		matchLoc = maxLoc;
		auto min_thresh = ( minVal + 1e-6 ) * 1.5;
		//printf("%f\n", min_thresh);
		if ( maxVal >= min_thresh ) {
			return_vec.emplace_back( matchLoc.x + ( templ.cols / 2 ), matchLoc.y + ( templ.rows / 2 ) );
			cv::rectangle( img, matchLoc, cv::Point( matchLoc.x + templ.cols, matchLoc.y + templ.rows ), cv::Scalar::all( 0 ), 2, 8, 0 );
			cv::rectangle( result, matchLoc, cv::Point( matchLoc.x + templ.cols, matchLoc.y + templ.rows ), cv::Scalar::all( 0 ), 2, 8, 0 );
		}
		else
			break;
	}
	//cv::imshow( path, img );
	//cv::imshow("template", templ);
	//cv::waitKey( 1 );

	return return_vec;
}

static std::string get_ocr_text( tesseract::TessBaseAPI& tess, const std::vector< uint8_t >& image, bool scale ) {
	// read image
	auto pixs = pixReadMemPng( image.data( ), image.size( ) );
	//auto pixs = pixRead( "screen.png" );
	pixs = pixConvertRGBToGray( pixs, 0.0f, 0.0f, 0.0f ); //grey "filter"
	if ( scale )
		pixs = pixScaleGrayLI( pixs, 5.5f, 5.5f ); // zoom
	if ( !pixs ) {
		return {};
	}

	// recognize
	tess.SetImage( pixs );
	tess.Recognize( 0 );
	std::string str = std::unique_ptr< char[] >( tess.GetUTF8Text( ) ).get( );
	tess.Clear( );
	pixDestroy( &pixs );
	return str;
}

static void send_key_msg( HWND hWnd, uint32_t key ) {
	SendMessage( hWnd, WM_KEYDOWN, key, 0x390000 ); // KEY DOWN
	std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
	SendMessage( hWnd, WM_KEYUP, key, 0x390000 ); // KEY UP
}

static void send_opencar_msg( HWND hWnd ) {
	SendMessage( hWnd, WM_KEYDOWN, 0x58, 0x390000 ); // KEY X DOWN
	std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
	SendMessage( hWnd, WM_MOUSEMOVE, 0, MAKELPARAM(1170, 379) ); // move mouse
	std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
	SendMessage( hWnd, WM_KEYUP, 0x58, 0x390000 ); // KEY X UP
	std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
	//0x49 = I
	send_key_msg( hWnd, 0x49 );
}

static void send_closecar_msg( HWND hWnd ) {
	SendMessage( hWnd, WM_KEYDOWN, 0x58, 0x390000 ); // KEY X DOWN
	std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
	SendMessage( hWnd, WM_MOUSEMOVE, 0, MAKELPARAM(1170, 379) ); // move mouse
	std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
	SendMessage( hWnd, WM_KEYUP, 0x58, 0x390000 ); // KEY X UP
	std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
}

static void send_move_item_msg( HWND hWnd, POINT p, POINT p2 ) {
	SendMessage( hWnd, WM_MOUSEMOVE, 0, MAKELPARAM(p.x, p.y) ); //maus auf das item was eingelagert werden soll
	std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
	SendMessage( hWnd, WM_LBUTTONDOWN, 0, MAKELPARAM(p.x, p.y) ); //klick auf "item"
	std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
	SendMessage( hWnd, WM_MOUSEMOVE, 0, MAKELPARAM(p2.x, p2.y) ); //move to freie platz
	std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
	SendMessage( hWnd, WM_LBUTTONUP, 0, MAKELPARAM(p2.x, p2.y) );
}

static void send_mwheel_down_msg( HWND hWnd, POINT p ) {
	//0xff880000 down, 0x00780000 up
	SendMessage( hWnd, WM_MOUSEWHEEL, 0xff880000, MAKELPARAM(p.x ,p.y) );
}

static std::vector< std::pair< int, int > > scan_for_items( const std::vector< uint8_t >& screen_data, globals::item_definitions& found_item ) {
	found_item = globals::item_definitions::null;
	for ( auto i = 1; i < globals::item_definitions::max_def; i++ ) {
		auto item = scan_for_image( screen_data, globals::item_path.at( i ) );
		if ( !item.empty( ) ) {
			found_item = static_cast< globals::item_definitions >( i );
			return item;
		}
	}
	return {};
}
