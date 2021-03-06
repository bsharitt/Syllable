// Copyright 1999 Jesper Hansen. Free for use under the Gnu Public License
//-----------------------------------------------------------------------------
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <posix/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
//#include <sys/ioctl.h>
extern "C"
{
    extern int ioctl(int __fd, unsigned long int __request, ...);

}
//-------------------------------------
//#include <interface/Bitmap.h>
//#include <support/ByteOrder.h>
//#include <support/Debug.h>
//-------------------------------------
#include "../driver/cpia.h"
#include "cpiacam.h"
//-----------------------------------------------------------------------------

//#define CHECKLOCK assert(  IsLocked()&&LockingThread()==find_thread(NULL) )
#define CHECKLOCK assert( (IsLocked())&&(GetOwner()==get_thread_id(NULL)) );

#define MAX_EXPOSURE (1*256*256+46*256+1)

template<class T> T _min( T a, T b ) { return a<b?a:b; }
template<class T> T _max( T a, T b ) { return a>b?a:b; }
template<class T> T clamp( T v, T l, T h ) { return v<l?l:v>h?h:v; }

#define  MAX_STREAMSIZE ((((2+352+352*2+1)*288)+4095)&(~4095))

//-----------------------------------------------------------------------------

CPiACam::CPiACam( const char *device ) :
	Locker( "CPiACam lock" )
{
    Lock();

    strcpy( fDevicePath, device );

    fVideoSize				= RES_CIF;
    fRoiLeft				= 0/8;
    fRoiRight				= 352/8;
    fRoiTop				= 0/4;
    fRoiBottom				= 288/4;
	
    fCompression			= COMPRESSION_AUTO;
    fCurrentSetCompression		= COMPRESSION_AUTO;
    fDecimation				= DECIMATION_ON;
    fCurrentSetDecimation		= DECIMATION_ON;
	
    fAutoExposure			= true;
    fExposure				= 1.0f;
    fGain				= 0;

    fAutoColorBalance			= true;
    fRedColorBalance			= 0.5f;
    fGreenColorBalance			= 0.5f;
    fBlueColorBalance			= 0.5f;

    
    fYThreshold				= 0.7f;
    fUVThreshold			= 0.7f;

    fBrightness				= 0.5f;
    fContrast				= 0.5f;
    fSaturation				= 0.5f;
	
    fFrameRate				= FRATE_30;
    fFrameDiv				= FDIV_1;
	
    fCompressTarget			= COMPTARGET_QUALITY;
    fCompressRate			= 15;
    fCompressQuality		= 0.5f;
	
    fApcorCorrect1			= 1;
    fApcorCorrect2			= 1;
    fApcorCorrect3			= 2;
    fApcorCorrect4			= 3;
    fApcorIntensity1		= 8;
    fApcorIntensity2		= 6;
    fApcorIntensity3		= 4;
    fApcorIntensity4		= 4;
    fVLOffset1				= 24;
    fVLOffset2				= 28;
    fVLOffset3				= 30;
    fVLOffset4				= 30;

    fFileHandle = open( fDevicePath, O_RDWR );
    if( fFileHandle < 0 )
    {
	fprintf( stderr, "Could not open device %s: %s\n", device, strerror(errno) );
	fCameraData = NULL;
    }
    else
    {
	if( Init() >= 0 )
	{
	    fCameraData = new uint8[MAX_STREAMSIZE];
	    assert( fCameraData != NULL );
	}
	else
	{
	    fCameraData = NULL;
	    close( fFileHandle );
	    fFileHandle = -1;
	}
    }
    Unlock();
}

CPiACam::~CPiACam()
{
    CHECKLOCK;
	
    if( fFileHandle >= 0 )
    {
	IOGotoLoPower();

	close( fFileHandle );
    }
    delete fCameraData;
}

//-----------------------------------------------------------------------------

status_t CPiACam::InitCheck() const
{
    return fFileHandle>=0 ? 0 : -1;
}

//-----------------------------------------------------------------------------

status_t CPiACam::Init()
{
    status_t status;
	
    for( int retrycnt=0; retrycnt<3; retrycnt++ )
    {
	if( retrycnt != 0 )
	{
	    fprintf( stderr, "CPiACam::Init(): Retry %d; resetting camera...\n", retrycnt );
	    IOResetCam();
	}

	status = IOStopDataStream();
	if( status < 0 )
	{
	    fprintf( stderr, "CPiACam::Init(): IOStopDataStream failed\n" );
	    continue;
	}

	  // Goto low power
	IOGotoLoPower();

	  // Get camera version
	status = IOGetVersion( &fFirmwareVersion, &fFirmwareRevision, &fVideoCompressorVersion, &fVideoCompressorRevision );
	if( status < 0 )
	{
	    fprintf( stderr, "CPiACam::Init(): IOGetVersion failed\n" );
	    PrintCameraStatus( "CPiACam::Init (IOGetVersion)" );
	    continue;
	}

	  // Get PNP id
	uint16 vendorid, productid, deviceid;
	status = IOGetPnpId( &vendorid, &productid, &deviceid );
	if( status < 0 )
	{
	    fprintf( stderr, "CPiACam::Init(): IOGetPnpId failed\n" );
	    PrintCameraStatus( "CPiACam::Init (IOGetPnpId)" );
	    continue;
	}

	  // Check status, if any modules is in a weird state, we retry and reset the camera.
	uint8 systemstate, vpctrlstate, streamstate, fatalerror, cmderror, debugflags, vpstatus, errorcode;
	status = IOGetCameraStatus( &systemstate, &vpctrlstate, &streamstate, &fatalerror, &cmderror, &debugflags, &vpstatus, &errorcode );
	if( status<0 || fatalerror!=0 || cmderror!=0 )
	{
	    fprintf( stderr, "CPiACam::Init(): first IOGetCameraStatus failed\n" );
	    PrintCameraStatus( "CPiACam::Init (first IOGetCameraStatus)" );
	    continue;
	}

	IOGotoHiPower();

	  // Recheck status
	status = IOGetCameraStatus( &systemstate, &vpctrlstate, &streamstate, &fatalerror, &cmderror, &debugflags, &vpstatus, &errorcode );
	if( status<0 || fatalerror!=0 || cmderror!=0 )
	{
	    fprintf( stderr, "CPiACam::Init(): second IOGetCameraStatus failed\n" );
	    PrintCameraStatus( "CPiACam::Init (second IOGetCameraStatus)" );
	    continue;
	}
		
//		IOStartDataStream();

//	IOSetVPDefaults();

	MidSetFormatAndROI();
	MidSetExposure();
	MidSetColorBalance();
	MidSetColorParms();
	MidSetSensorFPS();
	MidSetCompressionTarget();
	MidSetYUVThresh();
	MidSetApcor();
	MidSetVLOffset();
		
//		SetMaxGain( 2 );

	status = IOSetGrabMode( 1 );
	if( status < 0 )
	{
	    fprintf( stderr, "CPiACam::CaptureFrame(): could not set IOSetGrabMode\n" );
	    return status;
	}

	  // Recheck status again
	status = IOGetCameraStatus( &systemstate, &vpctrlstate, &streamstate, &fatalerror, &cmderror, &debugflags, &vpstatus, &errorcode );
	if( status<0 || fatalerror!=0 || cmderror!=0 )
	{
	    fprintf( stderr, "CPiACam::Init(): third IOGetCameraStatus failed\n" );
	    PrintCameraStatus( "CPiACam::Init (third IOGetCameraStatus)" );
	    continue;
	}
		
	return status;
    }

    return -1;
}

//-----------------------------------------------------------------------------

status_t CPiACam::Reset()
{
    CHECKLOCK;

    if( fFileHandle >= 0 )
	return Init();
    else
	return -1;
}

int CPiACam::GetFirmwareVersion()
{
	return fFirmwareVersion;
}

int CPiACam::GetFirmwareRevision()
{
	return fFirmwareRevision;
}

int CPiACam::GetVideoCompressorVersion()
{
	return fVideoCompressorVersion;
}

int CPiACam::GetVideoCompressorRevision()
{
	return fVideoCompressorRevision;
}

//-----------------------------------------------------------------------------

status_t CPiACam::SetVideoSize( videosize_t videosize, uint roileft, uint roitop, uint roiright, uint roibottom )
{
    CHECKLOCK;

    assert( roileft < roiright );
    assert( roitop < roibottom );

    uint width = videosize==RES_CIF ? 352 : 176;
    uint height = videosize==RES_CIF ? 288 : 144;

    roileft		= clamp( (roileft/8)*8,			0u, width ) / 8;
    roiright	= clamp( ((roiright+7)/8)*8,	0u, width ) / 8;
    roitop		= clamp( (roitop/4)*4,			0u, height ) / 4;
    roibottom	= clamp( ((roibottom+3)/4)*4,	0u, height ) / 4;
	
    if( videosize==fVideoSize && roileft==fRoiLeft && roiright==fRoiRight && roitop==fRoiTop && roibottom==fRoiBottom )
	return 0;

    fVideoSize	= videosize;
    fRoiLeft	= roileft;
    fRoiRight	= roiright;
    fRoiTop		= roitop;
    fRoiBottom	= roibottom;

    return MidSetFormatAndROI();
}

//-----------------------------------------------------------------------------

status_t CPiACam::EnableAutoExposure( bool autoexp )
{
    CHECKLOCK;
	
    if( fAutoExposure == autoexp )
	return 0;
	
    fAutoExposure = autoexp;

    return MidSetExposure();
}

status_t CPiACam::SetExposure( float exposure, int gain )
{
    CHECKLOCK;

    exposure = clamp( exposure, 0.0f, 1.0f );
    gain = clamp( gain, 0, 3 );
	
    if( exposure==fExposure && gain==fGain )
	return 0;
		
    fExposure = exposure;
    fGain = gain;

    return MidSetExposure();
}

status_t CPiACam::GetColorBalance( float *red, float *green, float *blue )
{
    CHECKLOCK;

    uint8 hwred;
    uint8 hwgreen;
    uint8 hwblue;
	
    status_t status = IOGetColorBalance( &hwred, &hwgreen, &hwblue );
    if( status >= 0 )
    {
	if( red ) *red = hwred/212.0f;
	if( green ) *green = hwgreen/212.0f;
	if( blue ) *blue = hwblue/212.0f;
    }

    return status;
}

status_t CPiACam::GetExposure( float *exposure, int *gain )
{
    CHECKLOCK;

    uint8 hwgain;
    uint8 hwfineexp;
    uint8 hwcoarseexplo;
    uint8 hwcoarseexphi;
	
    status_t status = IOGetExposure( &hwgain, &hwfineexp, &hwcoarseexplo, &hwcoarseexphi, NULL,NULL,NULL,NULL );
    if( status >= 0 )
    {
	if( exposure )
	{
	    int hwexp = hwcoarseexphi<<16 | hwcoarseexplo<<8 | hwfineexp;
	    *exposure = float(hwexp)/(MAX_EXPOSURE);
	}
	if( gain )
	{
	    *gain = hwgain;
	}
    }

    return status;
}

//-----------------------------------------------------------------------------

status_t CPiACam::EnableAutoColorBalance( bool autocolorbalance )
{
    CHECKLOCK;
	
    if( fAutoColorBalance == autocolorbalance )
	return 0;
	
    fAutoColorBalance = autocolorbalance;

    return MidSetColorBalance();
}

status_t CPiACam::SetColorBalance( float red, float green, float blue )
{
    CHECKLOCK;

    red = clamp( red, 0.0f, 1.0f );
    green = clamp( green, 0.0f, 1.0f );
    blue = clamp( blue, 0.0f, 1.0f );
    
    if( red==fRedColorBalance && green==fGreenColorBalance && blue==fBlueColorBalance )
	return 0;
		
    fRedColorBalance = red;
    fGreenColorBalance = green;
    fBlueColorBalance = blue;

    return MidSetColorBalance();
}

//-----------------------------------------------------------------------------

status_t CPiACam::SetColorParms( float brightness, float contrast, float saturation )
{
    CHECKLOCK;

    brightness = clamp( brightness, 0.0f, 1.0f );
    contrast = clamp( contrast, 0.0f, 1.0f );
    saturation = clamp( saturation, 0.0f, 1.0f );

    if( brightness==fBrightness && contrast==fContrast && saturation==fSaturation )
	return 0;

    fBrightness = brightness;
    fContrast = contrast;
    fSaturation = saturation;

    return MidSetColorParms();
}

//-----------------------------------------------------------------------------

status_t CPiACam::SetFrameRate( framerate_t rate, framedivide_t div )
{
    CHECKLOCK;

    if( rate==fFrameRate && div==fFrameDiv )
	return 0;

    fFrameRate = rate;
    fFrameDiv = div;

    return MidSetSensorFPS();
}

static const struct
{
    float                  framerate;
    CPiACam::framerate_t   rate;
    CPiACam::framedivide_t div;
}
gFramerateList[8] =
{
    { 30.0f/1.0f, CPiACam::FRATE_30, CPiACam::FDIV_1 },
    { 30.0f/2.0f, CPiACam::FRATE_30, CPiACam::FDIV_2 },
    { 30.0f/4.0f, CPiACam::FRATE_30, CPiACam::FDIV_4 },
    { 30.0f/8.0f, CPiACam::FRATE_30, CPiACam::FDIV_8 },
    { 25.0f/1.0f, CPiACam::FRATE_25, CPiACam::FDIV_1 },
    { 25.0f/2.0f, CPiACam::FRATE_25, CPiACam::FDIV_2 },
    { 25.0f/4.0f, CPiACam::FRATE_25, CPiACam::FDIV_4 },
    { 25.0f/8.0f, CPiACam::FRATE_25, CPiACam::FDIV_8 }
};

status_t CPiACam::SetFrameRate( float vFramerate )
{
    CHECKLOCK;

    int besti = 0;
    for( int i=1; i<8; i++ )
    {
	if( fabs(vFramerate-gFramerateList[i].framerate) <
	    fabs(vFramerate-gFramerateList[besti].framerate) )
	    besti = i;
    }
    SetFrameRate( gFramerateList[besti].rate, gFramerateList[besti].div );
    return 0;
}

//-----------------------------------------------------------------------------

status_t CPiACam::EnableCompression( compression_t compression, decimation_t decimation )
{
    CHECKLOCK;

    printf( "CPiACam::EnableCompression: %d\n", decimation );
    
    fCompression = compression;
    fDecimation = decimation;
	
      // The hardware is updated in the CaptureFrame() function.

    return 0;
}

status_t CPiACam::SetCompressionTarget( comprressiontarget_t target, int framerate, float quality )
{
    CHECKLOCK;
	
    framerate	= clamp( framerate, 0, 30 );
    quality		= clamp( quality, 0.0f, 1.0f );
	
    if( target==fCompressTarget && framerate==fCompressRate && quality==fCompressQuality )
	return 0;
	
    fCompressTarget		= target;
    fCompressRate		= framerate;
    fCompressQuality	= quality;

    return MidSetCompressionTarget();
}

status_t CPiACam::SetYUVTreshold( float ythreshold, float uvthreshold )
{
    CHECKLOCK;

    ythreshold = clamp( ythreshold, 0.0f, 1.0f );
    uvthreshold = clamp( uvthreshold, 0.0f, 1.0f );

    if( ythreshold==fYThreshold && uvthreshold==fUVThreshold )
	return 0;
	
    fYThreshold = ythreshold;
    fUVThreshold = uvthreshold;

    return MidSetYUVThresh();
}

status_t CPiACam::SetApcor(
	uint correct1, uint intensity1,
	uint correct2, uint intensity2,
	uint correct3, uint intensity3,
	uint correct4, uint intensity4 )
{
    CHECKLOCK;
	
    correct1 = clamp( correct1, 0u, 15u ); intensity1 = clamp( intensity1, 0u, 15u );
    correct2 = clamp( correct2, 0u, 15u ); intensity2 = clamp( intensity2, 0u, 15u );
    correct3 = clamp( correct3, 0u, 15u ); intensity3 = clamp( intensity3, 0u, 15u );
    correct4 = clamp( correct4, 0u, 15u ); intensity4 = clamp( intensity4, 0u, 15u );
	
    if( correct1==fApcorCorrect1 && correct2==fApcorCorrect2 && correct3==fApcorCorrect3 && correct4==fApcorCorrect4 &&
	intensity1==fApcorIntensity1 && intensity2==fApcorIntensity2 && intensity3==fApcorIntensity3 && intensity4==fApcorIntensity4 )
	return 0;
		
    fApcorCorrect1 = correct1; fApcorIntensity1 = intensity1;
    fApcorCorrect2 = correct2; fApcorIntensity2 = intensity2;
    fApcorCorrect3 = correct3; fApcorIntensity3 = intensity3;
    fApcorCorrect4 = correct4; fApcorIntensity4 = intensity4;
	
    return MidSetApcor();
}

status_t CPiACam::SetVLOffset( int offset1, int offset2, int offset3, int offset4 )
{
    CHECKLOCK;
	
    offset1 = clamp( offset1, -128, 127 );
    offset2 = clamp( offset2, -128, 127 );
    offset3 = clamp( offset3, -128, 127 );
    offset4 = clamp( offset4, -128, 127 );
	
    if( offset1==fVLOffset1 && offset2==fVLOffset2 && offset3==fVLOffset3 && offset4==fVLOffset4 )
	return 0;
		
    fVLOffset1 = offset1;
    fVLOffset2 = offset2;
    fVLOffset3 = offset3;
    fVLOffset4 = offset4;

    return MidSetVLOffset();
}


//-----------------------------------------------------------------------------

status_t CPiACam::MidSetExposure()
{
    status_t status;
	
    assert( fExposure>=0.0f && fExposure<=1.0f );
    assert( fGain>=0 && fGain<=3 );

    if( fAutoExposure )
    {
	status = IOSetExposure( 0, 2, 0, 2, 0,0,0,0,0,0,0,0 );
    }
    else
    {
	int exp = (int)(fExposure * MAX_EXPOSURE);
	uint8 fineexp = exp&0xff;
	uint8 coarseexplo = (exp>>8)&0xff;
	uint8 coarseexphi = (exp>>16)&0x01;

	status = IOSetExposure( 0, 3, 0, 2, 0,0,0,0,0,0,0,0 );
	if( status >= 0 )
	    status = IOSetExposure( 0, 1, 0, 2, fGain, fineexp,coarseexplo,coarseexphi,0,0,0,0 );
    }

    if( status < 0 )
	fprintf( stderr, "CPiACam::MidSetExposure(): failed\n" );

    return status;
}

status_t CPiACam::MidSetColorBalance()
{
#if 1
    status_t status;
	
    assert( fRedColorBalance>=0.0f && fRedColorBalance<=1.0f );
    assert( fGreenColorBalance>=0.0f && fGreenColorBalance<=1.0f );
    assert( fBlueColorBalance>=0.0f && fBlueColorBalance<=1.0f );

    if( fAutoColorBalance )
    {
	printf( "CB:AUTO\n" );
	status = IOSetColorBalance( 2, 0,0,0 );
    }
    else
    {
	printf( "CB:MAN\n" );
	status = IOSetColorBalance( 3, 0,0,0 );
	if( status >= 0 )
	    status = IOSetColorBalance( 1, fRedColorBalance*212, fGreenColorBalance*212, fBlueColorBalance*212 );
    }

    if( status < 0 )
	fprintf( stderr, "CPiACam::MidSetColorBalance(): failed\n" );

    return status;
#else
    return 0;
#endif
}

status_t CPiACam::MidSetColorParms()
{
#if 1
    status_t status;

    status = IOSetColorParms( fBrightness*128.0f, fContrast*128.0f, fSaturation*128.0f );
    if( status < 0 )
	fprintf( stderr, "CPiACam::MidSetColorParms(): failed\n" );

    return status;
#else
    return 0;
#endif
}

status_t CPiACam::MidSetSensorFPS()
{
    status_t status;

    status = IOSetSensorFPS( fFrameDiv, fFrameRate );
    if( status < 0 )
	fprintf( stderr, "CPiACam::MidSetSensorFPS(): failed\n" );

    return status;
}

status_t CPiACam::MidSetFormatAndROI()
{
    status_t status;

    status = IOSetFormat( fVideoSize, SUBSAMPLING_YUV422, 0 );
    if( status >= 0 )
	status = IOSetROI( fRoiLeft, fRoiRight, fRoiTop, fRoiBottom );
    if( status < 0 )
	fprintf( stderr, "CPiACam::MidSetFormatAndROI(): failed\n" );

    return status;
}

status_t CPiACam::MidSetCompressionTarget()
{
    status_t status;
	
    status = IOSetCompressionTarget( fCompressTarget, fCompressRate, (1.0f-fCompressQuality)*31.0f );
    if( status < 0 )
	fprintf( stderr, "CPiACam::MinSetCompressionTarget(): failed\n" );

    return status;
}

status_t CPiACam::MidSetYUVThresh()
{
    status_t status;

    status = IOSetYUVThresh( (1.0f-fYThreshold)*31.0f, (1.0f-fUVThreshold)*31.0f );
    if( status < 0 )
	fprintf( stderr, "CPiACam::IOSetYUVThresh(): failed\n" );

    return status;
}

status_t CPiACam::MidSetApcor()
{
#if 0
    status_t status;

    status = IOSetApcor( (fApcorCorrect1<<4)|fApcorIntensity1, (fApcorCorrect2<<4)|fApcorIntensity2, (fApcorCorrect3<<4)|fApcorIntensity3, (fApcorCorrect4<<4)|fApcorIntensity4 );
    if( status < 0 )
	fprintf( stderr, "CPiACam::MidSetApcor(): failed\n" );

    return status;
#else
    return 0;
#endif
}

status_t CPiACam::MidSetVLOffset()
{
#if 0
    status_t status;

    status = IOSetVLOffset( fVLOffset1, fVLOffset2, fVLOffset3, fVLOffset4 );
    if( status < 0 )
	fprintf( stderr, "CPiACam::MidSetVLOffset(): failed\n" );

    return status;
#else
    return 0;
#endif
}


//-----------------------------------------------------------------------------

status_t CPiACam::CaptureFrame( os::Bitmap *bitmap, bool delta )
{
    assert( bitmap != NULL );

    void *data = bitmap->LockRaster();
    status_t status =  CaptureFrame( data, bitmap->GetBytesPerRow(), bitmap->GetColorSpace(), delta );
    bitmap->UnlockRaster();
    return status;
}

status_t CPiACam::CaptureFrame( void *data, int bytesperline, color_space colorspace, bool delta )
{
    CHECKLOCK;
    status_t status;

    assert( data != NULL );
    assert( bytesperline > 0 );
    assert( (colorspace==CS_RGB32) /*|| (colorspace==CS_YCbCr422)*/ );

    compression_t setcompression = delta?fCompression:COMPRESSION_DISABLED;
    if( setcompression!=fCurrentSetCompression || fDecimation!=fCurrentSetDecimation )
    {
	IODiscardFrame();
//	printf( "SET DECIMATION: %d\n", fDecimation );
	IOSetCompression( setcompression, fDecimation );
	  // 
	MidSetCompressionTarget();
	MidSetYUVThresh();

//		IOSetGrabMode( 0 );
//		IOSetGrabMode( 1 );
	IODiscardFrame();
	fCurrentSetCompression = setcompression;
	fCurrentSetDecimation = fDecimation;
    }

    fCameraData[0] = 0;
    fCameraData[1] = 0;

#if 0 // USB
//	IOStartDataStream();

    status = ioctl( fFileHandle, 9999 );
    if( status != B_OK )
    {
	fprintf( stderr, "************************************************************\n" );
	return status;
    }

    status = IOSetGrabMode( 1 );
    if( status != B_OK )
    {
	fprintf( stderr, "CPiACam::CaptureFrame(): could not set IOSetGrabMode\n" );
	return status;
    }


    status = IOGrabFrame( 0 );
    if( status != B_OK )
    {
	fprintf( stderr, "CPiACam::CaptureFrame(): could not set IOGrabFrame\n" );
	return status;
    }

#else // PPP

//    printf( ">IOGrabFrame\n" );
    IOGrabFrame( fVideoSize==RES_CIF?120:60 );
//    printf( "<IOGrabFrame\n" );

#if 0
      // wait untill a frame is ready.
    bigtime_t starttime = system_time();
    while( 1 )
    {
	uint8 streamstate;
	status_t status = IOGetCameraStatus( NULL, NULL, &streamstate, NULL, NULL, NULL, NULL, NULL );
	if( status != B_NO_ERROR )
	{
	    fprintf( stderr, "CPiACam::CaptureFrame(): could not get camera state\n" );
	    return status;
	}
//		PrintCameraStatus( "CPiACam::CaptureFrameOK" );

	if( streamstate == STREAM_READY ) break;

	if( streamstate != STREAM_NOT_READY )
	{
	    fprintf( stderr, "CPiACam::CaptureFrame(): got unexpected stream state: %d\n", streamstate );
	    PrintCameraStatus( "CPiACam::CaptureFrame" );
	    return B_ERROR;
	}
		
	snooze( 3000 );
    }
    bigtime_t endtime = system_time();
    fprintf( stderr, "%Ld\n", endtime-starttime );
#else
      // I have mooved this command down into the kernel driver to speed things up a bit...
//    printf( ">IOWaitForFrame\n" );
    status = IOWaitForFrame();
//    printf( "<IOWaitForFrame\n" );
    if( status < 0 )
    {
	fprintf( stderr, "CPiACam::CaptureFrame(): waitforframe failed\n" );
	PrintCameraStatus( "CPiACam::CaptureFrame" );
	return status;
    }
#endif
#endif

      // download the image data:
    size_t actualread = 0;
//    printf( ">UploadStreamData\n" );
    UploadStreamData( MAX_STREAMSIZE, fCameraData, &actualread );
//    printf( "<UploadStreamData\n" );
//	PrintHex( fCameraData, actualread );

//	fprintf( stderr, "<%d>", (int)actualread ); fflush( stdout );

    if( actualread < 64 )
    {
	fprintf( stderr, "CPiACam::CaptureFrame(): could not read header (got %d bytes)\n", (int)actualread );
	return -1;
    }
#if 0
    for( int i=0; i<1024; i+=16 )
    {
	printf( "%04X:", i );
	for( int j=0; j<8; j++ ) printf( " %02X", fCameraData[i+j] );
	printf( " " );
	for( int j=8; j<16; j++ ) printf( " %02X", fCameraData[i+j] );
	printf( "\n" );
    }
#endif
    if( fCameraData[0]!=0x19 || fCameraData[1]!=0x68 )
    {
	fprintf( stderr, "CPiACam::CaptureFrame(): wrong magic\n" );
	return -1;
    }
	
      // Frame size
    if( fCameraData[16]!=RES_QCIF && fCameraData[16]!=RES_CIF )
    {
	fprintf( stderr, "CPiACam::CaptureFrame(): illegal resolution: %d\n", fCameraData[16] );
	return -1;
    }
    int framewidth  = fCameraData[16]==RES_CIF ? 352 : 176;
    int frameheight = fCameraData[16]==RES_CIF ? 288 : 144;
	
      // yuv subsampling
    if( fCameraData[17]!=SUBSAMPLING_YUV420 && fCameraData[17]!=SUBSAMPLING_YUV422 )
    {
	fprintf( stderr, "CPiACam::CaptureFrame(): illegal subsambling: %d\n", fCameraData[17] );
	return -1;
    }
    bool yuv422 = fCameraData[17]==SUBSAMPLING_YUV422;
	
      // Region of interest:
    int roileft   = fCameraData[24]*8;
    int roiright  = fCameraData[25]*8;
    int roitop    = fCameraData[26]*4;
    int roibottom = fCameraData[27]*4;
    if( (roileft<0 || roileft>framewidth) ||
	(roiright<0 || roiright>framewidth) ||
	(roitop<0 || roitop>frameheight) ||
	(roibottom<0 || roibottom>frameheight) ||
	(roileft >= roiright) ||
	(roitop >= roibottom) )
    {
	fprintf( stderr, "CPiACam::CaptureFrame(): illegal roi (l:%d r:%d t:%d b:%d)\n", roileft, roiright, roitop, roibottom );
	return -1;
    }

      // compression
    if( fCameraData[28]!=0 && fCameraData[28]!=1 )
    {
	fprintf( stderr, "CPiACam::CaptureFrame(): illegal compression: %d\n", fCameraData[28] );
	return -1;
    }
    bool compression = fCameraData[28]==1;

      // decimation
    if( fCameraData[29]!=0 && fCameraData[29]!=1 )
    {
	fprintf( stderr, "CPiACam::CaptureFrame(): illegal decimation: %d\n", fCameraData[29] );
	return -1;
    }
    int decimation = fCameraData[29]+1;
	
    uint8 *bitmapdata = (uint8*)data;
    uint8 *cameradata = fCameraData+64;

    const char *compmod = "";
	
    if( colorspace == CS_RGB32 )
    {
	if( bytesperline < framewidth*4 )
	{
	    fprintf( stderr, "CPiACam::CaptureFrame(): bytesperline<framewidth*4\n" );
	    return -1;
	}

	if( yuv422 )
	{
	    if( !compression )
	    {
		status = ProcessFrame_YUV422_RGB32_NoComp( cameradata,actualread-64, bitmapdata,bytesperline, roileft,roiright, roitop,roibottom, decimation );
		compmod = "ProcessFrame_YUV422_RGB32_NoComp";
	    }
	    else
	    {
		status = ProcessFrame_YUV422_RGB32_Comp( cameradata,actualread-64, bitmapdata,bytesperline, roileft,roiright, roitop,roibottom, decimation );
		compmod = "ProcessFrame_YUV422_RGB32_Comp";
	    }
	}
	else
	{
	    status = -1;
	    compmod = "Unsopported:ProcessFrame_YUV420_RGB32";
	}
    }
#if 0
    else if( colorspace == B_YCbCr422 )
    {
	if( bytesperline < framewidth*2 )
	{
	    fprintf( stderr, "CPiACam::CaptureFrame(): bytesperline<framewidth*2\n" );
	    return B_ERROR;
	}

	if( yuv422 )
	{
	    if( !compression )
	    {
		status = ProcessFrame_YUV422_YUV422_NoComp( cameradata,actualread-64, bitmapdata,bytesperline, roileft,roiright, roitop,roibottom, decimation );
		compmod = "ProcessFrame_YUV422_YUV422_NoComp";
	    }
	    else
	    {
		status = ProcessFrame_YUV422_YUV422_Comp( cameradata,actualread-64, bitmapdata,bytesperline, roileft,roiright, roitop,roibottom, decimation );
		compmod = "ProcessFrame_YUV422_YUV422_Comp";
	    }
	}
	else
	{
	    status = B_ERROR;
	    compmod = "Unsopported:ProcessFrame_YUV420_YUV422";
	}
    }
#endif
    else
    {
	status = -1;
	compmod = "Unsopported:ProcessFrame_YUV420";
    }
	
    if( status < 0 )
    {
	fprintf( stderr, "CPiACam::CaptureFrame(): could not decompress data: %s\n", compmod );
	return status;
    }

    return 0;
}

//-----------------------------------------------------------------------------

status_t CPiACam::DoIOCTL( cpiacmd *cmd )
{
    status_t status;

    int retrycnt = 1;
    do
    {
	status = ioctl( fFileHandle, CPIA_COMMAND, cmd );
	if( status >= 0 )
	    break;
	fprintf( stderr, "Retrying ioctl module:%d proc:%d retry:%d\n", (int)cmd->module, (int)cmd->proc, retrycnt );
    }
    while( retrycnt++<3 );

    if( status < 0 )
	fprintf( stderr, "Failed ioctl module:%d proc:%d\n", (int)cmd->module, (int)cmd->proc );

    return status;
}

void CPiACam::PrintCameraStatus( const char *string )
{
    static const char *module[8] = { "CPIA", "SYSTEM", "INT_CTRL", "PROCESS", "XXX_COM", "VP_CTRL", "CAPTURE", "DEBUG" };

    uint8 systemstate, vpctrlstate, streamstate, fatalerror, cmderror, debugflags, vpstatus, errorcode;
    IOGetCameraStatus( &systemstate, &vpctrlstate, &streamstate, &fatalerror, &cmderror, &debugflags, &vpstatus, &errorcode );

    if( fatalerror!=0 || cmderror!=0 )
    {
	fprintf( stderr, "Camera status:(%s)\n", string );
	fprintf( stderr, "    System state:  %02X\n", systemstate );
	fprintf( stderr, "    VPCtrl state:  %02X\n", vpctrlstate );
	fprintf( stderr, "    Stream state:  %02X\n", streamstate );
	fprintf( stderr, "    Fatal error:   %02X", fatalerror );
	if( fatalerror )
	    for( int i=0; i<8; i++ )
		if( fatalerror&(1<<i) ) fprintf( stderr, " %s", module[i] );
	printf( "\n" );
	fprintf( stderr, "    Command error: %02X", cmderror );
	if( cmderror )
	    for( int i=0; i<8; i++ )
		if( cmderror&(1<<i) ) fprintf( stderr, " %s", module[i] );
	printf( "\n" );
	fprintf( stderr, "    Debug flags:   %02X\n", debugflags );
	fprintf( stderr, "    VP status:     %02X\n", vpstatus );
	fprintf( stderr, "    Error code:    %02X\n", errorcode );
    }
}

status_t CPiACam::IOResetCam()
{
    status_t status = ioctl( fFileHandle, CPIA_RESET );
    return status;
}

status_t CPiACam::IOWaitForFrame()
{
    status_t status = ioctl( fFileHandle, CPIA_WAITFORFRAME );
    return status;
}

status_t CPiACam::IOStopDataStream()
{
    status_t status = ioctl( fFileHandle, CPIA_STOPSTREAM );
    return status;
}

status_t CPiACam::IOStartDataStream()
{
    status_t status = ioctl( fFileHandle, CPIA_STARTSTREAM );
    return status;
}


status_t CPiACam::UploadStreamData( size_t maxlen, void *buffer, size_t *actualread )
{
#if 1
    size_t readlen = read( fFileHandle, buffer,  maxlen );
    if( readlen < 0 )
    {
	*actualread = 0;
	return readlen;
    }
    *actualread = readlen;
    return 0;
#else

    size_t bufsize = (maxlen + B_PAGE_SIZE-1) & ~(B_PAGE_SIZE-1);
    void *buf;
    area_id buf_id = create_area( "buf", (void**)&buf, B_ANY_ADDRESS, bufsize, B_CONTIGUOUS, B_READ_AREA | B_WRITE_AREA );
//--
    size_t readlen = read( fFileHandle, buf,  maxlen );
    if( readlen < 0 )
    {
	*actualread = 0;
	delete_area( buf_id );
	return readlen;
    }
    *actualread = readlen;
//--
    delete_area( buf_id );

    return 0;
#endif
}

