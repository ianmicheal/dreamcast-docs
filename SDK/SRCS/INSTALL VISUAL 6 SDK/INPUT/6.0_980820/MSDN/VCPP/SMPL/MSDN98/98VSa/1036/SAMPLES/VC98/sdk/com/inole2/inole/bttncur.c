/*
 * BTTNCUR.C
 * Buttons & Cursors
 *
 * Public functions to generate different states of toolbar buttons
 * from a single bitmap.  States are normal, pressed, checked, and
 * disabled.
 *
 * Copyright (c)1992-1995 Microsoft Corporation, All Rights Reserved
 *
 * Kraig Brockschmidt, Microsoft
 * Internet  :  kraigb@microsoft.com
 * Compuserve:  >INTERNET:kraigb@microsoft.com
 */


#include "inoledll.h"

//Cache GDI objects to speed drawing.
HDC     g_hDCGlyphs    = NULL;
HDC     g_hDCMono      = NULL;
HBRUSH  g_hBrushDither = NULL;

//Standard images to use in case caller doesn't provide them
HBITMAP g_rghBmpStandardImages[3];

//Standard button colors.
const COLORREF g_crStandard[4]={ RGB(0, 0, 0)      //STDCOLOR_BLACK
                   , RGB(128, 128, 128)            //STDCOLOR_DKGRAY
                   , RGB(192, 192, 192)            //STDCOLOR_LTGRAY
                   , RGB(255, 255, 255)};          //STDCOLOR_WHITE


COLORREF g_crSys[5];



/*
 * Mapping from image identifier to button type (command/attribute).
 * Version 1.00 of this DLL has no attribute images defined, so
 * the code will only support three states for each command
 * button.  Any state is, however, valid for an application
 * defined image.
 */

UINT mpButtonType[TOOLIMAGE_MAX-TOOLIMAGE_MIN+1]=
        {
        BUTTONTYPE_COMMAND, BUTTONTYPE_COMMAND, BUTTONTYPE_COMMAND,
        BUTTONTYPE_COMMAND, BUTTONTYPE_COMMAND, BUTTONTYPE_COMMAND,
        BUTTONTYPE_COMMAND, BUTTONTYPE_COMMAND, BUTTONTYPE_COMMAND
        };


/*
 * UIToolConfigureForDisplay
 * Public API
 *
 * Purpose:
 *  Initializes the library to scale button images for the display
 *  type.  Without calling this function the library defaults to 96
 *  DPI (VGA).  By calling this function an application acknowledges
 *  that it must use the data returned from this function to
 *  configure itself for the display.
 *
 * Parameters:
 *  lpDD            LPTOOLDISPLAYDATA to fill with the display-
 *                  sensitive size values.
 *
 * Return Value:
 *  BOOL            TRUE if the sizes were obtained, FALSE otherwise.
 */

BOOL WINAPI UIToolConfigureForDisplay(LPTOOLDISPLAYDATA lpDD)
    {
    int         cy;
    HDC         hDC;


    if (NULL==lpDD || IsBadWritePtr(lpDD, sizeof(TOOLDISPLAYDATA)))
        return FALSE;

    /*
     * Determine the aspect ratio of the display we're currently
     * running on and calculate the necessary information.
     *
     * By retrieving the logical Y extent of the display driver, you
     * only have limited possibilities:
     *      LOGPIXELSY      Display
     *      ----------------------------------------
     *         48             CGA    (unsupported)
     *         72             EGA
     *         96             VGA
     *        120             8514/a (i.e. HiRes VGA)
     */

    hDC=GetDC(NULL);

    if (NULL==hDC)
        return FALSE;

    cy=GetDeviceCaps(hDC, LOGPIXELSY);
    ReleaseDC(NULL, hDC);

    /*
     * Instead of single comparisons, check ranges instead, so in
     * case we get something funky, we'll act reasonable.
     */
    if (72 >=cy)
        {
        lpDD->uDPI     =72;
        lpDD->cyBar    =CYBUTTONBAR72;
        lpDD->cxButton =TOOLBUTTON_STD72WIDTH;
        lpDD->cyButton =TOOLBUTTON_STD72HEIGHT;
        lpDD->cxImage  =TOOLBUTTON_STD72IMAGEWIDTH;
        lpDD->cyImage  =TOOLBUTTON_STD72IMAGEHEIGHT;
        lpDD->uIDImages=IDB_STANDARDIMAGES72;
        }
    else
        {
        if (72 < cy && 120 > cy)
            {
            lpDD->uDPI     =96;
            lpDD->cyBar    =CYBUTTONBAR96;
            lpDD->cxButton =TOOLBUTTON_STD96WIDTH;
            lpDD->cyButton =TOOLBUTTON_STD96HEIGHT;
            lpDD->cxImage  =TOOLBUTTON_STD96IMAGEWIDTH;
            lpDD->cyImage  =TOOLBUTTON_STD96IMAGEHEIGHT;
            lpDD->uIDImages=IDB_STANDARDIMAGES96;
            }
        else
            {
            lpDD->uDPI     =120;
            lpDD->cyBar    =CYBUTTONBAR120;
            lpDD->cxButton =TOOLBUTTON_STD120WIDTH;
            lpDD->cyButton =TOOLBUTTON_STD120HEIGHT;
            lpDD->cxImage  =TOOLBUTTON_STD120IMAGEWIDTH;
            lpDD->cyImage  =TOOLBUTTON_STD120IMAGEHEIGHT;
            lpDD->uIDImages=IDB_STANDARDIMAGES120;
            }
        }

    return TRUE;
    }








/*
 * ToolButtonInit
 * Internal
 *
 * Purpose:
 *  Initializes GDI objects for drawing images through
 *  UIToolButtonDraw. If the function fails, the function has
 *  already performed proper cleanup.
 *
 * Parameters:
 *  hInst           HINSTANCE of the DLL
 *
 * Return Value:
 *  BOOL            TRUE if initialization succeeded.  FALSE
 *                  otherwise.
 */

BOOL ToolButtonInit(HINSTANCE hInst)
    {
    UINT        i;

    //DC for BitBltting the image (the glyph)
    g_hDCGlyphs=CreateCompatibleDC(NULL);

    //Create a monochrome DC and a brush for doing pattern dithering.
    g_hDCMono=CreateCompatibleDC(NULL);

    g_hBrushDither=HBrushDitherCreate(GetSysColor(COLOR_BTNFACE)
        , GetSysColor(COLOR_BTNHIGHLIGHT));

    for (i=0; i < 3; i++)
        {
        g_rghBmpStandardImages[i]=LoadBitmap(hInst
            , MAKEINTRESOURCE(IDB_STANDARDIMAGESMIN+i));
        }

    if (NULL==g_hDCGlyphs || NULL==g_hDCMono
        || NULL==g_hBrushDither || NULL==g_rghBmpStandardImages[0])
        {
        //On failure, cleanup whatever might have been allocated.
        ToolButtonFree();
        return FALSE;
        }

    return TRUE;
    }





/*
 * ToolButtonFree
 * Internal
 *
 * Purpose:
 *  Free all GDI allocations made during initialization.
 *
 * Parameters:
 *  None
 *
 * Return Value:
 *  None
 */

void ToolButtonFree(void)
    {
    UINT        i;

    if (NULL!=g_hDCMono)
        DeleteDC(g_hDCMono);

    g_hDCMono=NULL;

    if (NULL!=g_hDCGlyphs)
        DeleteDC(g_hDCGlyphs);

    g_hDCGlyphs=NULL;

    if (NULL!=g_hBrushDither)
        DeleteObject(g_hBrushDither);

    g_hBrushDither=NULL;

    for (i=0; i < 3; i++)
        {
        if (NULL!=g_rghBmpStandardImages[i])
            DeleteObject(g_rghBmpStandardImages[i]);

        g_rghBmpStandardImages[i]=NULL;
        }

    return;
    }





/*
 * HBrushDitherCreate
 * Internal
 *
 * Purpose:
 *  Creates and returns a handle to a pattern brush created from
 *  an 8*8 monochrome pattern bitmap.  We use the button face and
 *  highlight colors to indicate the resulting colors of a PatBlt
 *  using this brush.
 *
 * Parameters:
 *  rgbFace         COLORREF of the button face color.
 *  rgbHilight      COLORREF of the button highlight color.
 *
 * Return Value:
 *  HBITMAP         Handle to the dither bitmap.
 */

HBRUSH HBrushDitherCreate(COLORREF rgbFace, COLORREF rgbHilight)
    {
    struct  //BITMAPINFO with 16 colors
        {
        BITMAPINFOHEADER    bmiHeader;
        RGBQUAD             bmiColors[16];
        } bmi;

    HBRUSH          hBrush=NULL;
    DWORD           patGray[8];
    HDC             hDC;
    HBITMAP         hBmp;
    static COLORREF rgbFaceOld   =0xFFFFFFFF;  //Initially impossible
    static COLORREF rgbHilightOld=0xFFFFFFFF;  //Initially impossible

    /*
     * If the colors haven't changed from last time, just return the
     * existing brush.
     */
    if (rgbFace==rgbFaceOld && rgbHilight==rgbHilightOld)
        return g_hBrushDither;

    rgbFaceOld=rgbFace;
    rgbHilightOld=rgbHilight;

    /*
     * We're going to create an 8*8 brush for PatBlt using the
     * button face color and button highlight color.  We use this
     * brush to affect the pressed state and the disabled state.
     */
    bmi.bmiHeader.biSize         = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth        = 8;
    bmi.bmiHeader.biHeight       = 8;
    bmi.bmiHeader.biPlanes       = 1;
    bmi.bmiHeader.biBitCount     = 1;
    bmi.bmiHeader.biCompression  = BI_RGB;
    bmi.bmiHeader.biSizeImage    = 0;
    bmi.bmiHeader.biXPelsPerMeter= 0;
    bmi.bmiHeader.biYPelsPerMeter= 0;
    bmi.bmiHeader.biClrUsed      = 0;
    bmi.bmiHeader.biClrImportant = 0;

    bmi.bmiColors[0].rgbBlue     = GetBValue(rgbFace);
    bmi.bmiColors[0].rgbGreen    = GetGValue(rgbFace);
    bmi.bmiColors[0].rgbRed      = GetRValue(rgbFace);
    bmi.bmiColors[0].rgbReserved = 0;

    bmi.bmiColors[1].rgbBlue     = GetBValue(rgbHilight);
    bmi.bmiColors[1].rgbGreen    = GetGValue(rgbHilight);
    bmi.bmiColors[1].rgbRed      = GetRValue(rgbHilight);
    bmi.bmiColors[1].rgbReserved = 0;

    //Create the byte array for CreateDIBitmap.
    patGray[6]=patGray[4]=patGray[2]=patGray[0]=0x5555AAAAL;
    patGray[7]=patGray[5]=patGray[3]=patGray[1]=0xAAAA5555L;

    //Create the bitmap
    hDC=GetDC(NULL);
    hBmp=CreateDIBitmap(hDC, &bmi.bmiHeader, CBM_INIT, patGray
                        , (LPBITMAPINFO)&bmi, DIB_RGB_COLORS);
    ReleaseDC(NULL, hDC);

    //Create the brush from the bitmap
    if (NULL!=hBmp)
        {
        hBrush=CreatePatternBrush(hBmp);
        DeleteObject(hBmp);
        }

    /*
     * If we could recreate a brush, clean up and make it the current
     * pattern.  Otherwise the best we can do it return the old one,
     * which will be colored wrong, but at least it works.
     */
    if (NULL!=hBrush)
        {
        if (NULL!=g_hBrushDither)
            DeleteObject(g_hBrushDither);

        g_hBrushDither=hBrush;
        }

    return g_hBrushDither;
    }





/*
 * UIToolButtonDraw
 * Public API
 *
 * Purpose:
 *  Draws the complete image of a toolbar-style button with a given
 *  image in the center and in a specific state.  The button is drawn
 *  on a specified hDC at a given location, so this function is
 *  useful on standard owner-draw buttons as well as on toolbar
 *  controls that have only one window but show images of multiple
 *  buttons.
 *
 * Parameters:
 *  hDC             HDC on which to draw.
 *  x, y            int coordinates at which to draw.
 *  dx, dy          int dimensions of the *button*, not the image.
 *  hBmp            HBITMAP from which to draw the image.
 *  bmx, bmy        int dimensions of each bitmap in hBmp.  If hBmp
 *                  is NULL then these are forced to the standard
 *                  sizes.
 *  iImage          int index to the image to draw in the button
 *  uStateIn        UINT containing the state index for the button
 *                  and the color control bits.
 *  pTDD            LPTOOLDISPLAYDATA containing display
 *                  configuration. Can be NULL if hBmp is non-NULL.
 *
 * Return Value:
 *  BOOL            TRUE if drawing succeeded, FALSE otherwise
 *                  meaning that hDC is NULL or hBmp is NULL and
 *                  iImage is not a valid index for a standard image.
 */

BOOL WINAPI UIToolButtonDraw(HDC hDC, int x, int y, int dx, int dy
    , HBITMAP hBmp, int bmx, int bmy, int iImage, UINT uStateIn
    , LPTOOLDISPLAYDATA pTDD)
    {
    UINT            uState=(UINT)LOBYTE((WORD)uStateIn);
    UINT            uColors=(UINT)HIBYTE((WORD)uStateIn
                        & PRESERVE_ALL);
    int             xOffsetGlyph, yOffsetGlyph;
    int             i, iSaveDC;
    HDC             hMemDC;
    HGDIOBJ         hObj;
    HBRUSH          hBR;
    HBITMAP         hBmpT;
    HBITMAP         hBmpMono;
    HBITMAP         hBmpMonoOrg;
    HBITMAP         hBmpSave=NULL;
    TOOLDISPLAYDATA tdd;

    if (NULL==hDC)
        return FALSE;

    if (NULL==pTDD)
        {
        pTDD=&tdd;
        UIToolConfigureForDisplay(pTDD);
        }

    /*
     * If we're given no image bitmap, then use the standard and
     * validate the image index.  We also enforce the standard
     * bitmap size and the size of the button (as requested by
     * User Interface designers).
     */
    if (NULL==hBmp && !(uState & BUTTONGROUP_BLANK))
        {
        i=pTDD->uIDImages-IDB_STANDARDIMAGESMIN;
        hBmp=g_rghBmpStandardImages[i];

        bmx=pTDD->cxImage;            //Force bitmap dimensions
        bmy=pTDD->cyImage;

        dx=pTDD->cxButton;            //Force button dimensions
        dy=pTDD->cyButton;

        if (iImage > TOOLIMAGE_MAX)
            return FALSE;

        /*
         * If we are using a standard command button, verify that
         * the state does not contain the LIGHTFACE group which
         * only applies to attribute buttons.
         */
        if (BUTTONTYPE_COMMAND==mpButtonType[iImage]
            && (uState & BUTTONGROUP_LIGHTFACE))
            return FALSE;
        }

    //Create a dithered bitmap.
    hBmpMono=CreateBitmap(dx-2, dy-2, 1, 1, NULL);

    if (NULL==hBmpMono)
        return FALSE;

    hBmpMonoOrg=(HBITMAP)SelectObject(g_hDCMono,  hBmpMono);


    //Save the DC state before we munge on it.
    iSaveDC=SaveDC(hDC);

    /*
     * Draw a button sans image.  This also fills g_crSys with the
     * system colors for us which has space for five colors.
     * We don't use the fifth, the frame color, in this function.
     */
    DrawBlankButton(hDC, x, y, dx, dy
        , (BOOL)(uState & BUTTONGROUP_DOWN), g_crSys);

    //Shift coordinates to account for the button's border
    x++;
    y++;
    dx-=2;
    dy-=2;

    /*
     * Determine the offset necessary to center the image but also
     * reflect the pushed-in state, which means just adding 1 to
     * the up state.
     */
    i=(uState & BUTTONGROUP_DOWN) ? 1 : 0;
    xOffsetGlyph=((dx-bmx) >> 1)+i;
    yOffsetGlyph=((dy-bmy) >> 1)+i;


    //Select given image into the glyph DC before calling MaskCreate
    if (NULL!=hBmp)
        hBmpSave=(HBITMAP)SelectObject(g_hDCGlyphs, hBmp);


    /*
     * Draw the face on the button.  If we have an up or [mouse]down
     * button then we can just draw it as-is.  For indeterminate,
     * disabled, or down disabled we have to gray the image and
     * possibly add a white shadow to it (disabled/down disabled).
     *
     * Also note that for the intermediate state we first draw the
     * normal up state, then proceed to add disabling looking
     * highlights.
     */

    //Up, mouse down, down, indeterminate
    if ((uState & BUTTONGROUP_ACTIVE)
        && !(uState & BUTTONGROUP_BLANK))
        {
        BOOL            fColorsSame=TRUE;

        /*
         * In here we pay close attention to the system colors.
         * Where the source image is black, we paint COLOR_BTNTEXT.
         * Where light gray, we paint COLOR_BTNFACE.  Where dark gray
         * we paint COLOR_BTNSHADOW, and where white we paint
         * COLOR_BTNHILIGHT.
         *
         * The uColors variable contains flags to prevent color
         * conversion.  To do a little optimization, we just do a
         * single BitBlt if we're preserving all colors or if no
         * colors are different than the standards, which is by far
         * the most common case.  Otherwise, cycle through the four
         * colors we can convert and do a BitBlt that converts it to
         * the system color.
         */

        //See what colors are different.
        for (i=STDCOLOR_BLACK; i<=STDCOLOR_WHITE; i++)
            fColorsSame &= (g_crSys[i]==g_crStandard[i]);

        if (PRESERVE_ALL==uColors || fColorsSame)
            {
            BitBlt(hDC, x+xOffsetGlyph, y+yOffsetGlyph, bmx, bmy
                , g_hDCGlyphs, iImage*bmx, 0, SRCCOPY);
            }
        else
            {
            /*
             * Cycle through hard-coded colors and create a mask that
             * has all regions of that color in white and all other
             * regions black.  Then we select a pattern brush of the
             * color to convert to: if we aren't converting the color
             * then we use a brush of the standard hard-coded color,
             * otherwise we use the actual system color.  The
             * ROP_DSPDxax means that anything that's 1's in the mask
             * get the pattern, anything that's 0 is unchanged in
             * the destination.
             *
             * To prevent too many Blts to the screen, we use an
             * intermediate bitmap and DC.
             */

            hMemDC=CreateCompatibleDC(hDC);

            //Make sure conversion of monochrome to color stays B&W
            SetTextColor(hMemDC, 0L);                 //mono 0's->0
            SetBkColor(hMemDC, (COLORREF)0x00FFFFFF); //mono 1's->1

            hBmpT=CreateCompatibleBitmap(hDC, bmx, bmy);
            SelectObject(hMemDC, hBmpT);

            //Copy the unmodified bitmap to the temporary bitmap
            BitBlt(hMemDC, 0, 0, bmx, bmy, g_hDCGlyphs, iImage*bmx
                , 0, SRCCOPY);

            for (i=STDCOLOR_BLACK; i<=STDCOLOR_WHITE; i++)
                {
                //Convert pixels of convert color to 1's in the mask
                SetBkColor(g_hDCGlyphs, g_crStandard[i]);
                BitBlt(g_hDCMono, 0, 0, bmx, bmy, g_hDCGlyphs, iImage*bmx
                    , 0, SRCCOPY);

                //Preserve or modify the color depending on the flag.
                hBR=CreateSolidBrush((uColors & (1 << i))
                    ? g_crStandard[i] : g_crSys[i]);

                if (NULL!=hBR)
                    {
                    hObj=SelectObject(hMemDC, hBR);

                    if (NULL!=hObj)
                        {
                        BitBlt(hMemDC, 0, 0, dx-1, dy-1, g_hDCMono, 0
                            , 0, ROP_DSPDxax);
                        SelectObject(hMemDC, hObj);
                        }

                    DeleteObject(hBR);
                    }
                }

            //Now put the final version on the display and clean up
            BitBlt(hDC, x+xOffsetGlyph, y+yOffsetGlyph, dx-1, dy-1
                   , hMemDC, 0, 0, SRCCOPY);

            DeleteDC(hMemDC);
            DeleteObject(hBmpT);

            }
        }


    //Disabled and indeterminate states (unless we're blank)
    if ((uState & BUTTONGROUP_DISABLED
        || ATTRIBUTEBUTTON_INDETERMINATE==uState)
        && !(uState & BUTTONGROUP_BLANK))
        {
        //Grayed state (up or down, no difference)
        MaskCreate(iImage, dx, dy, bmx, bmy, xOffsetGlyph
            , yOffsetGlyph, 0);

        //Make sure conversion of monochrome to color stays B&W
        SetTextColor(hDC, 0L);                     //0's in mono -> 0
        SetBkColor(hDC, (COLORREF)0x00FFFFFF);     //1's in mono -> 1

        //If we're disabled, up or down, draw the highlighted shadow.
        if (uState & BUTTONGROUP_DISABLED)
            {
            hBR=CreateSolidBrush(g_crSys[SYSCOLOR_HILIGHT]);

            if (NULL!=hBR)
                {
                hObj=SelectObject(hDC, hBR);

                if (NULL!=hObj)
                    {
                    //Draw hilight color where we have 0's in mask
                    BitBlt(hDC, x+1, y+1, dx-2, dy-2, g_hDCMono, 0, 0
                        , ROP_PSDPxax);
                    SelectObject(hDC, hObj);
                    }
                DeleteObject(hBR);
                }
            }

        //Draw the gray image.
        hBR=CreateSolidBrush(g_crSys[SYSCOLOR_SHADOW]);

        if (NULL!=hBR)
            {
            hObj=SelectObject(hDC, hBR);

            if (NULL!=hObj)
                {
                //Draw the shadow color where we have 0's in the mask
                BitBlt(hDC, x, y, dx-2, dy-2, g_hDCMono, 0, 0
                    , ROP_PSDPxax);
                SelectObject(hDC, hObj);
                }

            DeleteObject(hBR);
            }
        }

    //If the button is selected do the dither brush avoiding glyph
    if (uState & BUTTONGROUP_LIGHTFACE)
        {
        HBRUSH      hBRDither;

        /*
         * Get the dither brush.  This function will recreate it if
         * necessary or return the global one if the colors already
         * match.
         */
        hBRDither=HBrushDitherCreate(g_crSys[SYSCOLOR_FACE]
            , g_crSys[SYSCOLOR_HILIGHT]);
        hObj=SelectObject(hDC, hBRDither);

        if (NULL!=hObj)
            {
            /*
             * The mask we create now determines where the dithering
             * ends up.  In the down disabled state, we have to
             * preserve the highlighted shadow, so the mask we create
             * must have two masks of the original glyph, one of them
             * offset by one pixel in both x & y.  For the
             * indeterminate state, we have to mask all highlighted
             * areas.  The state passed to MaskCreate matters here
             * (we've used zero before).
             */
            MaskCreate(iImage, dx, dy, bmx, bmy
                       , xOffsetGlyph-1, yOffsetGlyph-1, uState);

            //Convert monochrome masks to B&W color bitmap in BitBlt.
            SetTextColor(hDC, 0L);
            SetBkColor(hDC, (COLORREF)0x00FFFFFF);

            /*
             * Only draw the dither brush where the mask is 1's.  For
             * the indeterminate state we have to not overdraw the
             * shadow highlight so we use dx-3, dy-3 instead of dx-1
             * and dy-1.  We do this whether or not we're blank.
             */
            i=(ATTRIBUTEBUTTON_INDETERMINATE==uState
               || BLANKBUTTON_INDETERMINATE==uState) ? 3 : 1;

            BitBlt(hDC, x+1, y+1, dx-i, dy-i, g_hDCMono, 0, 0
                , ROP_DSPDxax);
            SelectObject(hDC, hObj);
            }

        //DO NOT delete hBRDither!  It's a shared global.
        }

    //Cleanup g_hDCGlyphs:  Must do AFTER calling MaskCreate
    if (NULL!=hBmpSave)
        SelectObject(g_hDCGlyphs, hBmpSave);

    SelectObject(g_hDCMono,   hBmpMonoOrg);
    DeleteObject(hBmpMono);

    //Restore everything in the DC.
    RestoreDC(hDC, iSaveDC);
    return TRUE;
    }






/*
 * DrawBlankButton
 *
 * Purpose:
 *  Draws a button with no face using the current system colors in
 *  either an up or down state.
 *
 * Parameters:
 *  hDC             HDC on which to draw
 *  x, y            int coordinates where we start drawing
 *  dx,dy           int size of the button
 *  fDown           BOOL indicating the up or down state of the
 *                  button
 *  pcr             COLORREF * to five colors in which we store text,
 *                  shadow, face, highlight, and frame colors.  This
 *                  is a matter of convenience for the caller, since
 *                  we have to load these colors anyway we might as
 *                  well send them back.
 *
 * Return Value:
 *  None
 */

void DrawBlankButton(HDC hDC, int x, int y, int dx, int dy
    , BOOL fDown, COLORREF *pcr)
    {
    //Get the current system colors for buttons.
    pcr[0]=GetSysColor(COLOR_BTNTEXT);
    pcr[1]=GetSysColor(COLOR_BTNSHADOW);
    pcr[2]=GetSysColor(COLOR_BTNFACE);
    pcr[3]=GetSysColor(COLOR_BTNHIGHLIGHT);
    pcr[4]=GetSysColor(COLOR_WINDOWFRAME);

    //Draw the border around the button.
    PatB(hDC, x+1,    y,      dx-2, 1,    pcr[4]);
    PatB(hDC, x+1,    y+dy-1, dx-2, 1,    pcr[4]);
    PatB(hDC, x,      y+1,    1,    dy-2, pcr[4]);
    PatB(hDC, x+dx-1, y+1,    1,    dy-2, pcr[4]);

    //Shift coordinates to account for the border we just drew
    x++;
    y++;
    dx-=2;
    dy-=2;

    //Paint the interior grey as a default.
    PatB(hDC, x, y, dx, dy, pcr[2]);

    /*
     * Draw shadows and highlights.  The DOWN grouping that contains
     * down, mouse down, and down disabled are drawn depressed.  Up,
     * indeterminate, and disabled are drawn up.
     */

    if (fDown)
        {
        PatB(hDC, x, y, 1,  dy, pcr[1]);
        PatB(hDC, x, y, dx, 1,  pcr[1]);
        }
    else
        {
        //Normal button look.
        PatB(hDC, x, y, 1,    dy-1, pcr[3]);
        PatB(hDC, x, y, dx-1, 1,    pcr[3]);

        PatB(hDC, x+dx-1, y,      1,  dy, pcr[1]);
        PatB(hDC, x,      y+dy-1, dx, 1,  pcr[1]);

        PatB(hDC, x+1+dx-3, y+1,    1,    dy-2, pcr[1]);
        PatB(hDC, x+1,      y+dy-2, dx-2, 1,    pcr[1]);
        }

    return;
    }






/*
 * PatB
 * Internal
 *
 * Purpose:
 *  A more convenient PatBlt operation for drawing button borders and
 *  highlights.
 *
 * Parameters:
 *  hDC             HDC on which to paint.
 *  x, y            int coordinates at which to paint.
 *  dx, dy          int dimensions of rectangle to paint.
 *  rgb             COLORREF to use as the background color.
 *
 * Return Value:
 *  None
 */

void PatB(HDC hDC, int x, int y, int dx, int dy, COLORREF rgb)
    {
    RECT        rc;

    SetBkColor(hDC, rgb);
    SetRect(&rc, x, y, x+dx, y+dy);
    ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);
    }




/*
 * MaskCreate
 * Internal
 *
 * Purpose:
 *  Creates a monochrome mask bitmap of the given image at the given
 *  offset in the global g_hDCMono.  Anywhere in the image that you
 *  have the light gray (STDCOLOR_LTGRAY) or the white highlight
 *  (STDCOLOR_WHITE) you get get 1's.  All other pixels are 0's
 *
 * Parameters:
 *  iImage          UINT index of the image for which to create a
 *                  mask.
 *  dx, dy          int dimensions of the button.
 *  bmx, bmy        int dimensions of the bitmap to use.
 *  xOffset         int offset for x inside g_hDCMono where we paint.
 *  yOffset         int offset for y inside g_hDCMono where we paint.
 *  uState          UINT state of the image.  Special cases are made
 *                  for ATTRIBUTEBUTTON_DOWNDISABLED and
 *                  ATTRIBUTEBUTTON_INDETERMINATE.  In any case
 *                  where you do not want a special case, pass zero
 *                  here, regardless of the true button state.
 *
 * Return Value:
 *  None
 */

void MaskCreate(UINT iImage, int dx, int dy, int bmx, int bmy
    ,int xOffset, int yOffset, UINT uState)
    {
    //Initalize whole area with zeros
    PatBlt(g_hDCMono, 0, 0, dx, dy, WHITENESS);

    if (uState & BUTTONGROUP_BLANK)
        return;

    //Convert face colored pixels to 1's. all others to black.
    SetBkColor(g_hDCGlyphs, g_crStandard[STDCOLOR_LTGRAY]);
    BitBlt(g_hDCMono, xOffset, yOffset, bmx, bmy, g_hDCGlyphs
        , iImage*bmx, 0, SRCCOPY);

    //Indeterminate state, don't turn highlight's to 1's--leave black
    if (ATTRIBUTEBUTTON_INDETERMINATE!=uState)
        {
        //Convert highlight colored pixels to 1's, OR with previous.
        SetBkColor(g_hDCGlyphs, g_crStandard[STDCOLOR_WHITE]);
        BitBlt(g_hDCMono, xOffset, yOffset, bmx, bmy, g_hDCGlyphs
            , iImage*bmx, 0, SRCPAINT);
        }

    /*
     * For the down disabled state, AND this same mask with itself at
     * an offset of 1, which accounts for the highlight shadow.
     */
    if (ATTRIBUTEBUTTON_DOWNDISABLED==uState)
        BitBlt(g_hDCMono, 1, 1, dx-1, dy-1, g_hDCMono,  0, 0, SRCAND);

    return;
    }
