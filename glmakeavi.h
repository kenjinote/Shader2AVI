#include <vfw.h>
#include <mmsystem.h>

class CVideoSaver {
private:
    PAVIFILE aviFile;
    PAVISTREAM pstream,psCompressed;
    int vWidth, vHeight;
    bool isInited;
    char videoCodec[5];
    bool createAVI(LPCTSTR name, const int framesPerSecond, LPCTSTR title, bool showOptions,HWND hwnd);
public:
    CVideoSaver();
    ~CVideoSaver();    
    bool openAVI(LPCTSTR filename, const int width, const int height, const int framesPerSecond, LPCTSTR title, char *compression, bool showOptions,HWND hwnd);
    bool closeAVI();
    int saveFrame();
    bool isOpen();
    char* getCurrentCodec();
};



CVideoSaver::CVideoSaver()
{
    this->isInited=false;
}

CVideoSaver::~CVideoSaver()
{
	if (this->isInited) // if we do have a file open finish the avi!
	   closeAVI();
}

bool CVideoSaver::openAVI(LPCTSTR filename, const int width, const int height, const int framesPerSecond, LPCTSTR title, char *compression, bool showOptions,HWND hwnd)
{
    // our screen capture size
    this->vWidth=width;
    this->vHeight=height;
    
    strncpy(this->videoCodec,compression,4);

    // create the avi file   
    this->isInited=createAVI(filename, framesPerSecond, title, showOptions, hwnd);
    
    return this->isInited;
}

bool CVideoSaver::closeAVI()
{
    if (this->isInited)
	{
        // release the streams, and don't forget AVIFileExit()!
        AVIStreamRelease(psCompressed);
        AVIStreamRelease(pstream);
        AVIFileRelease(aviFile);
    	AVIFileExit();
    	this->isInited=false;
    	return true;
	}

    return false;
}

bool CVideoSaver::createAVI(LPCTSTR name, const int framesPerSecond, LPCTSTR title, bool showOptions,HWND hwnd)
{
	LONG hr;
	AVISTREAMINFO info;

	AVIFileInit();
	hr = AVIFileOpen( &aviFile, name, OF_CREATE | OF_WRITE | OF_SHARE_DENY_NONE, NULL );
	if( hr != 0 ){
		return false; // cannot open file
	}

	// set our video properties
    info.fccType = streamtypeVIDEO; // this stream is a video stream
	info.dwFlags = 0;
	info.dwCaps = 0;
	info.wPriority = 0;
	info.wLanguage = 0;
	info.dwScale = 1;	// An idea like suspicion
	info.dwRate = framesPerSecond; // our framerate
	info.dwStart = 0;	//	ALWAYS
	info.dwLength = 0; // nbImages;
	info.dwInitialFrames = 0; // NO SOUND in it
	info.dwSuggestedBufferSize = 0; // ADOBE PREMIERE will choose!
	info.dwQuality = (DWORD)-1; // Default value (good choice?)
	info.dwSampleSize = 0; // For video stream
	info.rcFrame.left = info.rcFrame.top = 0;
	info.rcFrame.right = vWidth;
	info.rcFrame.bottom = vHeight;
	info.dwEditCount = 0;
	info.dwFormatChangeCount = 0;
	lstrcpyn(info.szName, title, sizeof(info.szName)/sizeof(TCHAR));

    hr = AVIFileCreateStream( aviFile, &pstream, &info );
	if(hr!=0){
        AVIFileRelease(aviFile); // release on error
    	AVIFileExit();
        return false; // Can not create AVI stream
	}
	

    BITMAPINFOHEADER infoHeader;  // info header.
    infoHeader.biSize = sizeof(BITMAPINFOHEADER);
    infoHeader.biPlanes        = 1;
    infoHeader.biBitCount      = 24; // bits per pixel
    infoHeader.biCompression   = BI_RGB;
    infoHeader.biSizeImage     = 0;
    infoHeader.biXPelsPerMeter = 0;
    infoHeader.biYPelsPerMeter = 0;
    infoHeader.biClrUsed       = 0;
    infoHeader.biClrImportant  = 0;
    infoHeader.biWidth         = vWidth;
    infoHeader.biHeight        = vHeight;

    AVICOMPRESSOPTIONS opts; ZeroMemory(&opts,sizeof(opts)); // our default options
    

    // examples:
    // mmioFOURCC('D','I','B',' '); // Uncompressed
    // mmioFOURCC('C','V','I','D'); // Cinpak
    // mmioFOURCC('I','V','3','2'); // Intel video 3.2
    // mmioFOURCC('M','S','V','C'); // Microsoft video 1
    // mmioFOURCC('I','V','5','0'); // Intel video 5.0
    // mmioFOURCC('d','i','v','x'); // DivX
    // set the compression
    opts.fccHandler=mmioFOURCC( videoCodec[0],videoCodec[1],videoCodec[2],videoCodec[3]); // the codec we want to use
    
    AVICOMPRESSOPTIONS *aopts[1];
    aopts[0]=&opts;

    if (showOptions) // show the options dialog for compressors
    {
        BOOL res = (BOOL)AVISaveOptions(hwnd,0,1,&pstream,aopts); //0,1,&pstream
        if (!res)
            AVISaveOptionsFree(1,aopts);
        else
        {
            videoCodec[0]=(opts.fccHandler);            // get the selected codec name
            videoCodec[1]=(opts.fccHandler>>8);
            videoCodec[2]=(opts.fccHandler>>16);
            videoCodec[3]=(opts.fccHandler>>24);
        }
    }
    
    hr = AVIMakeCompressedStream(&psCompressed, pstream, &opts, NULL);
    if (hr==AVIERR_NOCOMPRESSOR || hr==AVIERR_UNSUPPORTED) // no compressor found or not suitable for our data
    {
        ZeroMemory(&opts,sizeof(opts));
        opts.fccHandler=mmioFOURCC('D','I','B',' '); // so try to use uncompressed!
        hr = AVIMakeCompressedStream(&psCompressed, pstream, &opts, NULL);
    }    
    if (hr != AVIERR_OK) {
        AVIStreamRelease(pstream); // release on eror
        AVIFileRelease(aviFile);
    	AVIFileExit();
        return false;
    }
    //↓これ�?コメントアウトしな�?�?inuxでエラーになる�?であえてした
    //AVISaveOptionsFree(1,aopts); // free our options
    hr = AVIStreamSetFormat(psCompressed, 0, &infoHeader, infoHeader.biSize);
    if (hr!=AVIERR_OK) {
        AVIStreamRelease(psCompressed); // release on error
        AVIStreamRelease(pstream);
        AVIFileRelease(aviFile);
    	AVIFileExit();
        return false;
    }

	return true;
}

int CVideoSaver::saveFrame()
{
    // if the avi file wasn't initialized correctly, return false, no need to do anything!
    if (!this->isInited) return false;    
    
    // This will hold our pixel data
    unsigned char *outputImage;
    
    int imgSize=vWidth * abs(vHeight) * 3;  // x*y*3bytes per pixel

    // Allocate the neccessary memory.
    try {
        outputImage = (unsigned char*)malloc(imgSize);
    } catch (...) {
        return 1;
    }

    // Clear the variable.
    memset(outputImage, 0, imgSize);

    // You use the glReadPixels() to read every pixel on the screen
    // that you specify.  You must use one less than each size.
    // This is the only part where opengl is used, you could of course change this function
    // to for example capture the desktop or whatever...
    glReadPixels(0, 0, vWidth, vHeight, GL_RGB, GL_UNSIGNED_BYTE, outputImage);
     
    // change RGB to BGR as this is the .bmp format!
    unsigned char tempColors;     // Use to change image from RGB to BGR.
    for (int index = 0; index < imgSize; index += 3)
    {
        tempColors = outputImage[index];
        outputImage[index] = outputImage[index + 2];
        outputImage[index + 2] = tempColors;
    }

	static int frame=-1;
	frame++;

    // write to avi
    if( AVIStreamWrite( psCompressed /*pstream*/, frame, 1, (LPBYTE)outputImage,
				imgSize, AVIIF_KEYFRAME, NULL, NULL ) != 0 ){
		return 2; // Can not write to AVI file
	}

    // Clear the allocated memory.
    free(outputImage);
	
	return 0;
}

bool CVideoSaver::isOpen()
{
    return this->isInited;
}

char* CVideoSaver::getCurrentCodec()
{
    return this->videoCodec;
}
