// DEMO7_12.CPP 24-bit bitmap loading demo

// INCLUDES ///////////////////////////////////////////////

#define WIN32_LEAN_AND_MEAN  // just say no to MFC

#define INITGUID

#include <windows.h>   // include important windows stuff
#include <windowsx.h> 
#include <mmsystem.h>
#include <iostream.h> // include important C/C++ stuff
#include <conio.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h> 
#include <math.h>
#include <io.h>
#include <fcntl.h>

#pragma comment(lib, "ddraw.lib")
#include <ddraw.h> // include directdraw

// DEFINES ////////////////////////////////////////////////

// defines for windows 
#define WINDOW_CLASS_NAME "WINCLASS1"

// default screen size
#define SCREEN_WIDTH    640  // size of screen
#define SCREEN_HEIGHT   480
#define SCREEN_BPP      32   // bits per pixel

#define BITMAP_ID            0x4D42 // universal id for a bitmap
#define MAX_COLORS_PALETTE   256

// TYPES //////////////////////////////////////////////////////

// basic unsigned types
typedef unsigned short USHORT;
typedef unsigned short WORD;
typedef unsigned char  UCHAR;
typedef unsigned char  BYTE;

// container structure for bitmaps .BMP file
typedef struct BITMAP_FILE_TAG
        {
        BITMAPFILEHEADER bitmapfileheader;  // this contains the bitmapfile header
        BITMAPINFOHEADER bitmapinfoheader;  // this is all the info including the palette
        PALETTEENTRY     palette[256];      // we will store the palette here
        UCHAR            *buffer;           // this is a pointer to the data

        } BITMAP_FILE, *BITMAP_FILE_PTR;

// this will hold our little alien
// 这个就是异型的结构类型、它包括了3个帧的离屏表面、
// 它的位置
// 目前的帧率
typedef struct ALIEN_OBJ_TYP
        {
        LPDIRECTDRAWSURFACE7 frames[3]; // 3 frames of animation for complete walk cycle
        int x,y;                        // position of alien
        int velocity;                   // x-velocity
        int current_frame;              // current frame of animation
        int counter;                    // used to time animation
 
        } ALIEN_OBJ, *ALIEN_OBJ_PTR;


// PROTOTYPES  //////////////////////////////////////////////

int Flip_Bitmap(UCHAR *image, int bytes_per_line, int height);

int Load_Bitmap_File(BITMAP_FILE_PTR bitmap, char *filename);

int Unload_Bitmap_File(BITMAP_FILE_PTR bitmap);

int DDraw_Fill_Surface(LPDIRECTDRAWSURFACE7 lpdds,int color);

LPDIRECTDRAWSURFACE7 DDraw_Create_Surface(int width, int height, int mem_flags, int color_key);

int Scan_Image_Bitmap(BITMAP_FILE_PTR bitmap,     // bitmap file to scan image data from
                      LPDIRECTDRAWSURFACE7 lpdds, // surface to hold data
                      int cx, int cy);             // cell to scan image from

int DDraw_Draw_Surface(LPDIRECTDRAWSURFACE7 source, // source surface to draw
                      int x, int y,                 // position to draw at
                      int width, int height,        // size of source surface
                      LPDIRECTDRAWSURFACE7 dest,    // surface to draw the surface on
                      int transparent);          // transparency flag

int Bmp2Surface(LPDIRECTDRAWSURFACE7 lpdds, int SurfaceWidth, int SurfaceHeight);


// MACROS /////////////////////////////////////////////////

// tests if a key is up or down
#define KEYDOWN(vk_code) ((GetAsyncKeyState(vk_code) & 0x8000) ? 1 : 0)
#define KEYUP(vk_code)   ((GetAsyncKeyState(vk_code) & 0x8000) ? 0 : 1)

// initializes a direct draw struct
#define DDRAW_INIT_STRUCT(ddstruct) { memset(&ddstruct,0,sizeof(ddstruct)); ddstruct.dwSize=sizeof(ddstruct); }

// this builds a 16 bit color value in 5.5.5 format (1-bit alpha mode)
#define _RGB16BIT555(r,g,b) ((b & 31) + ((g & 31) << 5) + ((r & 31) << 10))

// this builds a 16 bit color value in 5.6.5 format (green dominate mode)
#define _RGB16BIT565(r,g,b) ((b & 31) + ((g & 63) << 5) + ((r & 31) << 11))

// this builds a 32 bit color value in A.8.8.8 format (8-bit alpha mode)
#define _RGB32BIT(a,r,g,b) ((b) + ((g) << 8) + ((r) << 16) + ((a) << 24))

// GLOBALS ////////////////////////////////////////////////

HWND      main_window_handle = NULL; // globally track main window
int       window_closed      = 0;    // tracks if window is closed
HINSTANCE hinstance_app      = NULL; // globally track hinstance

// directdraw stuff

LPDIRECTDRAW7         lpdd        = NULL;    // dd object
LPDIRECTDRAWSURFACE7  lpddsprimary = NULL;   // dd primary surface
LPDIRECTDRAWSURFACE7  lpddsback    = NULL;   // dd back surface
LPDIRECTDRAWSURFACE7  lpddsbackground = NULL;// this will hold the background image
LPDIRECTDRAWPALETTE   lpddpal      = NULL;   // a pointer to the created dd palette
LPDIRECTDRAWCLIPPER   lpddclipper  = NULL;   // dd clipper
PALETTEENTRY          palette[256];          // color palette
PALETTEENTRY          save_palette[256];     // used to save palettes
DDSURFACEDESC2        ddsd;                  // a direct draw surface description struct
DDBLTFX               ddbltfx;               // used to fill
DDSCAPS2              ddscaps;               // a direct draw surface capabilities struct
HRESULT               ddrval;                // result back from dd calls
DWORD                 start_clock_count = 0; // used for timing

BITMAP_FILE           bitmap;                // holds the bitmap

ALIEN_OBJ             aliens[3];             // 3 aliens, one on each level


char text_bitmapinfo[80];

char buffer[80];                             // general printing buffer

// FUNCTIONS ////////////////////////////////////////////////


int DDraw_Draw_Surface(LPDIRECTDRAWSURFACE7 source, // source surface to draw
                      int x, int y,                 // position to draw at
                      int width, int height,        // size of source surface
                      LPDIRECTDRAWSURFACE7 dest,    // surface to draw the surface on
                      int transparent = 1)          // transparency flag
{
	// draw a bob at the x,y defined in the BOB
	// on the destination surface defined in dest

	RECT dest_rect,   // the destination rectangle
		 source_rect; // the source rectangle                             

	// fill in the destination rect
	// 目标表面、也就是需要填充的表面
	dest_rect.left   = x;
	dest_rect.top    = y;
	dest_rect.right  = x+width-1;
	dest_rect.bottom = y+height-1;

	// fill in the source rect
	// 源表面、也就是需要拷贝数据的表面、这里将拷贝的表面
	// 由传递过来的高宽决定
	source_rect.left    = 0;
	source_rect.top     = 0;
	source_rect.right   = width-1;
	source_rect.bottom  = height-1;

	// test transparency flag
	// 设置是否为透明、要就必须设置色彩键
	if (transparent)
	   {
	   // enable color key blit
	   // blt to destination surface
	   if (FAILED(dest->Blt(&dest_rect, source,
						 &source_rect,(DDBLT_WAIT | DDBLT_KEYSRC),
						 NULL)))
			   return(0);

	   } // end if
	else
	   {
	   // perform blit without color key
	   // blt to destination surface
	   if (FAILED(dest->Blt(&dest_rect, source,
						 &source_rect,(DDBLT_WAIT),
						 NULL)))
			   return(0);

	   } // end if

	// return success
	return(1);

} // end DDraw_Draw_Surface


int Bmp2Surface(LPDIRECTDRAWSURFACE7 lpdds, int SurfaceWidth, int SurfaceHeight)
{
	// copy the bitmap image to the lpddsback buffer line by line
	// notice the 24 to 32 bit conversion pixel by pixel

	// 一个像素一个像素的逐粒拷贝至表面

	// lock the lpddsback surface
	lpdds->Lock(NULL,&ddsd, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT,NULL);

	// get video pointer to primary surfce
	DWORD *back_buffer = (DWORD *)ddsd.lpSurface;       

	// process each line and copy it into the lpddsback buffer
	if (ddsd.lPitch == SurfaceWidth)
	{
	   // copy memory from double buffer to lpddsback buffer
	   memcpy((void *)back_buffer, (void *)bitmap.buffer, SurfaceWidth*SurfaceHeight);
	}
	else
	{


		
		for (int index_y = 0; index_y < SurfaceHeight; index_y++)
		{
			for (int index_x = 0; index_x < SurfaceWidth; index_x++)
			{
				// get BGR values
				UCHAR blue  = (bitmap.buffer[index_y*SurfaceWidth*3 + index_x*3 + 0]),
					  green = (bitmap.buffer[index_y*SurfaceWidth*3 + index_x*3 + 1]),
					  red   = (bitmap.buffer[index_y*SurfaceWidth*3 + index_x*3 + 2]);

				// this builds a 32 bit color value in A.8.8.8 format (8-bit alpha mode)
				DWORD pixel = _RGB32BIT(0,red,green,blue);

				// write the pixel
				back_buffer[index_x + (index_y*ddsd.lPitch >> 2)] = pixel;

			} // end for index_x

		} // end for index_y

		
		
	}

	// now unlock the lpddsback surface
	if (FAILED(lpdds->Unlock(NULL)))
	   return(0);
}  // end Bmp2Surface


int Scan_Image_Bitmap(BITMAP_FILE_PTR bitmap,     // bitmap file to scan image data from
                      LPDIRECTDRAWSURFACE7 lpdds, // surface to hold data
                      int cx, int cy)             // cell to scan image from
{
// this function extracts a bitmap out of a bitmap file

UCHAR *source_ptr,   // working pointers
      *dest_ptr;

DDSURFACEDESC2 ddsd;  //  direct draw surface description 

// get the addr to destination surface memory

// set size of the structure
ddsd.dwSize = sizeof(ddsd);

lpdds->Lock(NULL,&ddsd, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT,NULL);

// compute position to start scanning bits from
/*
(0, 0)  =》 cx=1, cy=1
(1, 0)  =>  cx=dwWidth+2, cy=1
(2, 0)  =>  cx=2dwWeight + 3, cy = 1
*/
// 因为白线的缘故、一条线一个像素点、
// cx负责人物动画帧的起点x坐标、因为人物的左右总有两条白线
// cy负责人物动画帧的起点的y坐标、因为人物的顶头总有一条白色的线
// 所以总是从第二行开始、因为这里传入的cy恒为0、所以cy恒为1
cx = cx*(ddsd.dwWidth + 1) + 1;
cy = cy*(ddsd.dwHeight+1) + 1;


//gwidth  = ddsd.dwWidth;
//gheight = ddsd.dwHeight;


// dwWidth和dwHeight分别是指人物表面的高宽
// 而biWidth和biHeight分别是指人物动画帧位图的高宽
// extract bitmap data
// 为什么要加上biWidth呢、原因是要从第二行才开始录入动画帧表面、因为
// 图片的上方有一条一像素的白色、所以这就是总是要从第二行开始的原因了
// 为什么要加上1呢、因为最左边有一条白线、所以总是要加上1、过滤过它
// ptr => buffer + biWidth + 1
// ptr => buffer + biWidth + dwWidth + 2
// ptr => buffer + biWidth + 2dwWidth + 3

int ptr_offset = cy*bitmap->bitmapinfoheader.biWidth+cx;

source_ptr = bitmap->buffer + ptr_offset;




// assign a pointer to the memory surface for manipulation
dest_ptr = (UCHAR *)ddsd.lpSurface;

// iterate thru each scanline and copy bitmap
// 一行一行的把位图相应的动画帧拷贝到帧表面上
for (int index_y=0; index_y < ddsd.dwHeight; index_y++)
    {
    // copy next line of data to destination
	// 这个狠好理解、前面确定好要拷贝哪帧的位置后、便从这个source_ptr这个位置点、
	// 拷贝dwWidth个字节的位置
    memcpy(dest_ptr, source_ptr, ddsd.dwWidth);

    // advance pointers
	// 在这里、由于动画帧表面是自己创建的、所以其实lPitch就是dwWidth人物表面的宽度而已
	// 这里写lPitch主要也是为了安全而已

	// 虽然内容是已经拷贝到相应位置了、但莪们还是要为下一行的拷贝做准备、因为指针还未移动呢！
	// 
	// 动画帧表面、在拷贝下一行之前移动一整行、刚好就是动画帧的宽、这个比较好理解
	// 虽然位图表面要拷贝的不是一整行、而是其中的dwWidth个像素而已、但也依然要移动位图宽度个字节
	// 想想、把(x, y)移动到(x, y+1)刚好不就是当前行的下一行么、所以刚好也就是相差biWidth个位置
    dest_ptr   += (ddsd.lPitch); // (ddsd.dwWidth);
    source_ptr += bitmap->bitmapinfoheader.biWidth;
    } // end for index_y

// unlock the surface 
lpdds->Unlock(NULL);

// return success
return(1);

} // end Scan_Image_Bitmap


LPDIRECTDRAWSURFACE7 DDraw_Create_Surface(int width, int height, int mem_flags, int color_key = 0)
{
// this function creates an offscreen plain surface

DDSURFACEDESC2 ddsd;         // working description
LPDIRECTDRAWSURFACE7 lpdds;  // temporary surface
    
// 清空ddsd然后再设置width, and height来创建离屏表面
memset(&ddsd,0,sizeof(ddsd));
ddsd.dwSize  = sizeof(ddsd);
ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;

// set dimensions of the new bitmap surface
ddsd.dwWidth  =  width;
ddsd.dwHeight =  height;

// set surface to offscreen plain
ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | mem_flags;

// create the surface
if (FAILED(lpdd->CreateSurface(&ddsd,&lpdds,NULL)))
   return(NULL);

// 设置色彩键、也就是透明色、默认设置为黑色
// test if user wants a color key
if (color_key >= 0)
   {
   // set color key to color 0
   DDCOLORKEY color_key; // used to set color key
   color_key.dwColorSpaceLowValue  = 0;
   color_key.dwColorSpaceHighValue = 0;

   // now set the color key for source blitting
   lpdds->SetColorKey(DDCKEY_SRCBLT, &color_key);
   } // end if

// return surface
return(lpdds);

// return surface
return(lpdds);
} // end DDraw_Create_Surface


int DDraw_Fill_Surface(LPDIRECTDRAWSURFACE7 lpdds,int color)
{
DDBLTFX ddbltfx; // this contains the DDBLTFX structure

// clear out the structure and set the size field 
DDRAW_INIT_STRUCT(ddbltfx);

// set the dwfillcolor field to the desired color
ddbltfx.dwFillPixel = color; 

// ready to blt to surface
lpdds->Blt(NULL,       // ptr to dest rectangle
           NULL,       // ptr to source surface, NA            
           NULL,       // ptr to source rectangle, NA
           DDBLT_COLORFILL | DDBLT_WAIT,   // fill and wait                   
           &ddbltfx);  // ptr to DDBLTFX structure

// return success
return(1);
} // end DDraw_Fill_Surface


int Load_Bitmap_File(BITMAP_FILE_PTR bitmap, char *filename)
{
// this function opens a bitmap file and loads the data into bitmap

int file_handle,  // the file handle
    index;        // looping index

UCHAR   *temp_buffer = NULL; // used to convert 24 bit images to 16 bit
OFSTRUCT file_data;          // the file data information

// open the file if it exists
if ((file_handle = OpenFile(filename,&file_data,OF_READ))==-1)
   return(0);

// now load the bitmap file header
_lread(file_handle, &bitmap->bitmapfileheader,sizeof(BITMAPFILEHEADER));

// test if this is a bitmap file
if (bitmap->bitmapfileheader.bfType!=BITMAP_ID)
   {
   // close the file
   _lclose(file_handle);

   // return error
   return(0);
   } // end if

// now we know this is a bitmap, so read in all the sections

// first the bitmap infoheader

// now load the bitmap file header
_lread(file_handle, &bitmap->bitmapinfoheader,sizeof(BITMAPINFOHEADER));

// now load the color palette if there is one
if (bitmap->bitmapinfoheader.biBitCount == 8)
   {
   _lread(file_handle, &bitmap->palette,MAX_COLORS_PALETTE*sizeof(PALETTEENTRY));

   // now set all the flags in the palette correctly and fix the reversed 
   // BGR RGBQUAD data format
   for (index=0; index < MAX_COLORS_PALETTE; index++)
       {
       // reverse the red and green fields
       int temp_color                = bitmap->palette[index].peRed;
       bitmap->palette[index].peRed  = bitmap->palette[index].peBlue;
       bitmap->palette[index].peBlue = temp_color;
       
       // always set the flags word to this
       bitmap->palette[index].peFlags = PC_NOCOLLAPSE;
       } // end for index

    } // end if

// finally the image data itself
_lseek(file_handle,-(int)(bitmap->bitmapinfoheader.biSizeImage),SEEK_END);

// now read in the image, if the image is 8 or 16 bit then simply read it
// but if its 24 bit then read it into a temporary area and then convert
// it to a 16 bit image

if (bitmap->bitmapinfoheader.biBitCount==8 || bitmap->bitmapinfoheader.biBitCount==16 || 
    bitmap->bitmapinfoheader.biBitCount==24)
   {
   // delete the last image if there was one
   if (bitmap->buffer)
       free(bitmap->buffer);

   // allocate the memory for the image
   if (!(bitmap->buffer = (UCHAR *)malloc(bitmap->bitmapinfoheader.biSizeImage)))
      {
      // close the file
      _lclose(file_handle);

      // return error
      return(0);
      } // end if

   // now read it in
   _lread(file_handle,bitmap->buffer,bitmap->bitmapinfoheader.biSizeImage);

   } // end if
else
   {
   // serious problem
   return(0);

   } // end else

#if 0
// write the file info out 


sprintf(text_bitmapinfo, "\nfilename:%s \nsize=%d \nwidth=%d \nheight=%d \nbitsperpixel=%d \ncolors=%d \nimpcolors=%d",
        filename,
        bitmap->bitmapinfoheader.biSizeImage,
        bitmap->bitmapinfoheader.biWidth,
        bitmap->bitmapinfoheader.biHeight,
		bitmap->bitmapinfoheader.biBitCount,
        bitmap->bitmapinfoheader.biClrUsed,
        bitmap->bitmapinfoheader.biClrImportant);


#endif

// close the file
_lclose(file_handle);

// flip the bitmap
Flip_Bitmap(bitmap->buffer, 
            bitmap->bitmapinfoheader.biWidth*(bitmap->bitmapinfoheader.biBitCount/8), 
            bitmap->bitmapinfoheader.biHeight);

// return success
return(1);

} // end Load_Bitmap_File

///////////////////////////////////////////////////////////

int Unload_Bitmap_File(BITMAP_FILE_PTR bitmap)
{
// this function releases all memory associated with "bitmap"
if (bitmap->buffer)
   {
   // release memory
   free(bitmap->buffer);

   // reset pointer
   bitmap->buffer = NULL;

   } // end if

// return success
return(1);

} // end Unload_Bitmap_File

///////////////////////////////////////////////////////////

int Flip_Bitmap(UCHAR *image, int bytes_per_line, int height)
{
// this function is used to flip bottom-up .BMP images

UCHAR *buffer; // used to perform the image processing
int index;     // looping index

// allocate the temporary buffer
if (!(buffer = (UCHAR *)malloc(bytes_per_line*height)))
   return(0);

// copy image to work area
memcpy(buffer,image,bytes_per_line*height);

// flip vertically
for (index=0; index < height; index++)
    memcpy(&image[((height-1) - index)*bytes_per_line],
           &buffer[index*bytes_per_line], bytes_per_line);

// release the memory
free(buffer);

// return success
return(1);

} // end Flip_Bitmap

///////////////////////////////////////////////////////////////

LRESULT CALLBACK WindowProc(HWND hwnd, 
						    UINT msg, 
                            WPARAM wparam, 
                            LPARAM lparam)
{
// this is the main message handler of the system
PAINTSTRUCT		ps;		// used in WM_PAINT
HDC				hdc;	// handle to a device context
//char buffer[80];        // used to print strings

// what is the message 
switch(msg)
	{	
	case WM_CREATE: 
        {
		// do initialization stuff here
        // return success
		return(0);
		} break;
   
	case WM_PAINT: 
		{
		// simply validate the window 
   	    hdc = BeginPaint(hwnd,&ps);	 
        
        // end painting
        EndPaint(hwnd,&ps);

        // return success
		return(0);
   		} break;

	case WM_DESTROY: 
		{

		// kill the application, this sends a WM_QUIT message 
		PostQuitMessage(0);

        // return success
		return(0);
		} break;

	default:break;

    } // end switch

// process any messages that we didn't take care of 
return (DefWindowProc(hwnd, msg, wparam, lparam));

} // end WinProc

///////////////////////////////////////////////////////////

int Game_Main(void *parms = NULL, int num_parms = 0)
{
// this is the main loop of the game, do all your processing
// here

// lookup for proper walking sequence
// 动画帧、先播放站立动作、然后是摆左手、然后再恢复站立动作、然后是摆右手
// 如此重复播放
// 声明为静态、所以不会随着Game_Main的调用而重复初始化
static int animation_seq[4] = {0,1,0,2};

int index; // general looping variable


// make sure this isn't executed again
if (window_closed)
   return(0);

// for now test if user is hitting ESC and send WM_CLOSE
if (KEYDOWN(VK_ESCAPE))
   {
   PostMessage(main_window_handle,WM_CLOSE,0,0);
   window_closed = 1;
   } // end if


DDRAW_INIT_STRUCT(ddbltfx);

if(lpddsback->Blt(NULL, lpddsbackground, NULL, DDBLT_WAIT, &ddbltfx)) return 0;


// move objects around
// 这里是移动异形
for (index=0; index < 3; index++)
{
// move each object to the right at its given velocity
// 移动异形向右走
aliens[index].x++; // =aliens[index].velocity;

// 测试异形的位置是否已经到达了右边界已经完全看不着了
// 就把它又扔会左边从头走过来
// 因为x是左上角的坐标、所以x超过了屏幕宽度刚好完全看不见
// 而因为异形图片的人物宽度刚好为80、所以重置到左边时设置为-80
// test if off screen edge, and wrap around
if (aliens[index].x > SCREEN_WIDTH)
   aliens[index].x = - 80;

// animate bot
if (++aliens[index].counter >= (8 - aliens[index].velocity))
    {
    // reset counter
    aliens[index].counter = 0;

    // advance to next frame
	// 如果超过第三帧、则重置为第0帧、这里是由前面的结构所决定的、只有三个动作
	// 由于采用了先自增再比较的办法、所以条件这里current_frame的范围恒为1~4
	// 所谓的第零帧、也就是标准的站立动作、可以从位图图片得知、
    if (++aliens[index].current_frame > 3)
       aliens[index].current_frame = 0;

    } // end if

} // end for index


// draw all the bots
for (index=0; index < 3; index++)
{
/*
原型
int DDraw_Draw_Surface(LPDIRECTDRAWSURFACE7 source, // source surface to draw
					  int x, int y,                 // position to draw at
					  int width, int height,        // size of source surface
					  LPDIRECTDRAWSURFACE7 dest,    // surface to draw the surface on
					  int transparent = 1)          // transparency flag
*/
// draw objects
// 把相应的三个异形的每帧动作拷贝进缓冲表面中
// 其中animation_seq[aliens[index].current_frame]决定了产生哪些动作帧
// 在Game_Main函数就已经有定义了static int animation_seq[4] = {0,1,0,2};
// 这里current_frame的范围就是0~3
DDraw_Draw_Surface(aliens[index].frames[animation_seq[aliens[index].current_frame]], 
                   aliens[index].x, aliens[index].y,
                   72,80,
                   lpddsback);

} // end for index


while(FAILED(lpddsprimary->Flip(NULL, DDFLIP_WAIT)));

// 每秒30帧
Sleep(33);


// do nothing -- look at pretty picture

// return success or failure or your own return code here
return(1);

} // end Game_Main

////////////////////////////////////////////////////////////

int Game_Init(void *parms = NULL, int num_parms = 0)
{
// this is called once after the initial window is created and
// before the main event loop is entered, do all your initialization
// here

// create IDirectDraw interface 7.0 object and test for error
if (FAILED(DirectDrawCreateEx(NULL, (void **)&lpdd, IID_IDirectDraw7, NULL)))
   return(0);

// set cooperation to full screen
if (FAILED(lpdd->SetCooperativeLevel(main_window_handle, 
                                      DDSCL_FULLSCREEN | DDSCL_ALLOWMODEX | 
                                      DDSCL_EXCLUSIVE | DDSCL_ALLOWREBOOT)))
   return(0);

// set display mode to 640x480x24
if (FAILED(lpdd->SetDisplayMode(SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_BPP,0,0)))
   return(0);

// clear ddsd and set size
DDRAW_INIT_STRUCT(ddsd); 

// enable valid fields
ddsd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;

ddsd.dwBackBufferCount = 1;

//
// request primary surface
ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_COMPLEX | DDSCAPS_FLIP;

// create the primary surface
if (FAILED(lpdd->CreateSurface(&ddsd, &lpddsprimary, NULL)))
   return(0);

ddsd.ddsCaps.dwCaps = DDSCAPS_BACKBUFFER;

if(FAILED(lpddsprimary->GetAttachedSurface(&ddsd.ddsCaps, &lpddsback))) return 0;



// 把主屏和缓冲屏都填充为黑色初始化
DDraw_Fill_Surface(lpddsprimary, _RGB32BIT(0, 0,0,0));
DDraw_Fill_Surface(lpddsback, _RGB32BIT(0, 0,0,0));

// load the 24-bit image
char* bmp_wc = "WarCraft24.bmp";
char* bmp_b8 = "bitmap8b.bmp";
char* bmp_b24 = "bitmap24.bmp";
char* bmp_b24e = "bitmap24_edit.bmp";

char* bmp_mo24 = "mosaic-600x.bmp";
char* bmp_ni24 = "nightelf-640x.bmp";
char* bmp_alley24 = "alley8_24bit.bmp";




// 载入背景图片
if (!Load_Bitmap_File(&bitmap, bmp_ni24))
   return(0);


// 创建背景表面、但实际上不是直接用背景表面来显示的、而是拷贝去缓冲表面和人物动作混合
// 后才一次性打到显示表面
// 这里头两个参数是指在屏幕的高和宽、第二个是指表面建立的地点、0指在显存建立、其它表示在
// 系统内存建立、当然速度自然是在显存建立快了、最后一个参数是是否设置为色彩键、这里设定为-1
// 也就是不设定任何色彩过滤、因为这个是背景表面、所以不需要任何透明的色彩键
lpddsbackground = DDraw_Create_Surface(640,480,0,-1);


// 把bmp的内容拷贝至缓冲表面中
Bmp2Surface(lpddsbackground, SCREEN_WIDTH, SCREEN_HEIGHT);












// 从现在开始创建人物动作了
if (!Load_Bitmap_File(&bitmap, "Dedsp0_24bit.bmp"))
   return(0);


// seed random number generator
// GetTickCount是一个系统启动至今的毫秒数、
// 配合srandg来产生一个随机数
srand(GetTickCount());

// initialize all the aliens

// alien on level 1 of complex

//
//系统在调用rand（）之前都会自动调用srand（），如果用户在rand（）之前曾调用过srand（）给参数seed指定了一个值，
//那么rand（）就会将seed的值作为产生伪随机数的初始值；
//而如果用户在rand（）前没有调用过srand（），那么rand（）就会自动调用srand（1），即系统默认将1作为伪随机数的初始值。
//所以前面要调用一次srand来确保以下调用rand()的值会产生不同
//
aliens[0].x              = rand()%SCREEN_WIDTH;
aliens[0].y              = 116 - 72;                  
aliens[0].velocity       = 2+rand()%4;
aliens[0].current_frame  = 0;             
aliens[0].counter        = 0;       

// alien on level 2 of complex

aliens[1].x              = rand()%SCREEN_WIDTH;
aliens[1].y              = 246 - 72;                  
aliens[1].velocity       = 2+rand()%4;
aliens[1].current_frame  = 0;             
aliens[1].counter        = 0;  

// alien on level 3 of complex

aliens[2].x              = rand()%SCREEN_WIDTH;
aliens[2].y              = 382 - 72;                  
aliens[2].velocity       = 2+rand()%4;
aliens[2].current_frame  = 0;             
aliens[2].counter        = 0;  




// now load the bitmap containing the alien imagery
// then scan the images out into the surfaces of alien[0]
// and copy then into the other two, be careful of reference counts!

// 现在开始载入人物的动画帧图片了
if (!Load_Bitmap_File(&bitmap,"Dedsp0_24bit.bmp"))
   return(0);


// create each surface and load bits
// 初始化异形0号的三个动作帧表面、
// 其实、所有的异形动作帧表面都一样的、所以没有必要为每个异形都创建相应的动作帧、
// 所以其它异形的动作帧都指向这个异形0号的动作帧就可以了
for (int index = 0; index < 3; index++)
    {
    // create surface to hold image
    aliens[0].frames[index] = DDraw_Create_Surface(72,80,0);

    // now load bits...
    Scan_Image_Bitmap(&bitmap,                 // bitmap file to scan image data from
                      aliens[0].frames[index], // surface to hold data
                      index, 0);               // cell to scan image from    

    } // end for index

// unload the bitmap file, we no longer need it
Unload_Bitmap_File(&bitmap);


// now for the tricky part. There is no need to create more surfaces with the same
// data, so I'm going to copy the surface pointers member for member to each alien
// however, be careful, since the reference counts do NOT go up, you still only need
// to release() each surface once!

for (index = 0; index < 3; index++)
    aliens[1].frames[index] = aliens[2].frames[index] = aliens[0].frames[index];


// return success or failure or your own return code here
return(1);

} // end Game_Init

/////////////////////////////////////////////////////////////

int Game_Shutdown(void *parms = NULL, int num_parms = 0)
{
// this is called after the game is exited and the main event
// loop while is exited, do all you cleanup and shutdown here


// first the palette
if (lpddpal)
   {
   lpddpal->Release();
   lpddpal = NULL;
   } // end if


// now the lpddsbackground surface
if (lpddsbackground)
   {
   lpddsbackground->Release();
   lpddsbackground = NULL;
   } // end if

// now the lpddsback surface
if (lpddsback)
   {
   lpddsback->Release();
   lpddsback = NULL;
   } // end if


// now the primary surface
if (lpddsprimary)
   {
   lpddsprimary->Release();
   lpddsprimary = NULL;
   } // end if

// now blow away the IDirectDraw4 interface
if (lpdd)
   {
   lpdd->Release();
   lpdd = NULL;
   } // end if

// unload the bitmap file, we no longer need it
Unload_Bitmap_File(&bitmap);

// return success or failure or your own return code here
return(1);

} // end Game_Shutdown

// WINMAIN ////////////////////////////////////////////////

int WINAPI WinMain(	HINSTANCE hinstance,
					HINSTANCE hprevinstance,
					LPSTR lpcmdline,
					int ncmdshow)
{

WNDCLASSEX winclass; // this will hold the class we create
HWND	   hwnd;	 // generic window handle
MSG		   msg;		 // generic message
HDC        hdc;      // graphics device context

// first fill in the window class stucture
winclass.cbSize         = sizeof(WNDCLASSEX);
winclass.style			= CS_DBLCLKS | CS_OWNDC | 
                          CS_HREDRAW | CS_VREDRAW;
winclass.lpfnWndProc	= WindowProc;
winclass.cbClsExtra		= 0;
winclass.cbWndExtra		= 0;
winclass.hInstance		= hinstance;
winclass.hIcon			= LoadIcon(NULL, IDI_APPLICATION);
winclass.hCursor		= LoadCursor(NULL, IDC_ARROW); 
winclass.hbrBackground	= (HBRUSH)GetStockObject(BLACK_BRUSH);
winclass.lpszMenuName	= NULL;
winclass.lpszClassName	= WINDOW_CLASS_NAME;
winclass.hIconSm        = LoadIcon(NULL, IDI_APPLICATION);

// save hinstance in global
hinstance_app = hinstance;

// register the window class
if (!RegisterClassEx(&winclass))
	return(0);

// create the window
if (!(hwnd = CreateWindowEx(NULL,                  // extended style
                            WINDOW_CLASS_NAME,     // class
						    "DirectDraw 24-Bit Bitmap Loading", // title
						    WS_POPUP | WS_VISIBLE,
					 	    0,0,	  // initial x,y
						    SCREEN_WIDTH,SCREEN_HEIGHT,  // initial width, height
						    NULL,	  // handle to parent 
						    NULL,	  // handle to menu
						    hinstance,// instance of this application
						    NULL)))	// extra creation parms
return(0);

// save main window handle
main_window_handle = hwnd;

// initialize game here
Game_Init();

// enter main event loop
while(TRUE)
	{
    // test if there is a message in queue, if so get it
	if (PeekMessage(&msg,NULL,0,0,PM_REMOVE))
	   { 
	   // test if this is a quit
       if (msg.message == WM_QUIT)
           break;
	
	   // translate any accelerator keys
	   TranslateMessage(&msg);

	   // send the message to the window proc
	   DispatchMessage(&msg);
	   } // end if
    
       // main game processing goes here
       Game_Main();
       
	} // end while

// closedown game here
Game_Shutdown();

// return to Windows like this
return(msg.wParam);

} // end WinMain

///////////////////////////////////////////////////////////

