//#define WIN64_LEAN_AND_MEAN
#pragma once
#include <Windows.h>
#include <atlbase.h>
#include <gdiplus.h>
#include <memory>
#include <vector>

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
