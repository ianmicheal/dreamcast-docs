/*
 * POLYLINE.CPP
 * Polyline Component Object Chapter 8
 *
 * Implementation of the CPolyline class that we expose as a
 * component object.
 *
 * Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
 *
 * Kraig Brockschmidt, Microsoft
 * Internet  :  kraigb@microsoft.com
 * Compuserve:  >INTERNET:kraigb@microsoft.com
 */


#include "polyline.h"


/*
 * CPolyline:CPolyline
 * CPolyline::~CPolyline
 *
 * Constructor Parameters:
 *  pUnkOuter       LPUNKNOWN of the controlling unknown.
 *  pfnDestroy      PFNDESTROYED to call when an object is
 *                  destroyed.
 *  hInst           HINSTANCE of the application we're in.
 */

CPolyline::CPolyline(LPUNKNOWN pUnkOuter, PFNDESTROYED pfnDestroy
    , HINSTANCE hInst)
    {
    m_hWnd=NULL;
    m_hInst=hInst;

    m_cRef=0;
    m_pUnkOuter=pUnkOuter;
    m_pfnDestroy=pfnDestroy;
    m_fDirty=FALSE;

    m_pImpIPolyline=NULL;
    m_pImpIConnPtCont=NULL;

    m_pAdv=NULL;
    m_pConnPt=NULL;

    //CHAPTER8MOD
    m_pST  =NULL;
    m_cf   =0;
    m_clsID=CLSID_Polyline8;

    m_pIStorage=NULL;
    m_pIStream =NULL;

    m_pImpIPersistStorage=NULL;
    m_pImpIPersistStreamInit=NULL;
    //End CHAPTER8MOD

    return;
    }


CPolyline::~CPolyline(void)
    {
    //CHAPTER8MOD
    if (NULL!=m_pST)
        delete m_pST;

    DeleteInterfaceImp(m_pImpIPersistStreamInit);
    DeleteInterfaceImp(m_pImpIPersistStorage);
    ReleaseInterface(m_pIStream);
    ReleaseInterface(m_pIStorage);
    //End CHAPTER8MOD

    DeleteInterfaceImp(m_pImpIConnPtCont);
    DeleteInterfaceImp(m_pImpIPolyline);

    ReleaseInterface(m_pAdv);
    ReleaseInterface(m_pConnPt);
    return;
    }




/*
 * CPolyline::Init
 *
 * Purpose:
 *  Performs any intiailization of a CPolyline that's prone to
 *  failure that we also use internally before exposing the
 *  object outside this DLL.
 *
 * Parameters:
 *  None
 *
 * Return Value:
 *  BOOL            TRUE if the function is successful,
 *                  FALSE otherwise.
 */

BOOL CPolyline::Init(void)
    {
    LPUNKNOWN       pIUnknown=this;

    if (NULL!=m_pUnkOuter)
        pIUnknown=m_pUnkOuter;

    //CHAPTER8MOD
    m_pST=new CStringTable(m_hInst);

    if (!m_pST->Init(IDS_POLYLINEMIN, IDS_POLYLINEMAX))
        return FALSE;

    m_cf=RegisterClipboardFormat(PSZ(IDS_STORAGEFORMAT));

    m_pImpIPersistStorage=new CImpIPersistStorage(this, pIUnknown);

    if (NULL==m_pImpIPersistStorage)
        return FALSE;

    m_pImpIPersistStreamInit=new CImpIPersistStreamInit(this
        , pIUnknown);

    if (NULL==m_pImpIPersistStreamInit)
        return FALSE;
    //End CHAPTER8MOD

    m_pImpIPolyline=new CImpIPolyline(this, pIUnknown);

    if (NULL==m_pImpIPolyline)
        return FALSE;

    m_pImpIConnPtCont=new CImpIConnPtCont(this, pIUnknown);

    if (NULL==m_pImpIConnPtCont)
        return FALSE;

    m_pConnPt=new CConnectionPoint(this);

    if (NULL==m_pConnPt)
        return FALSE;

    m_pConnPt->AddRef();    //Reversed in destructor
    return TRUE;
    }







/*
 * CPolyline::QueryInterface
 * CPolyline::AddRef
 * CPolyline::Release
 */

STDMETHODIMP CPolyline::QueryInterface(REFIID riid, PPVOID ppv)
    {
    *ppv=NULL;

    if (IID_IUnknown==riid)
        *ppv=this;

    if (IID_IConnectionPointContainer==riid)
        *ppv=m_pImpIConnPtCont;

    //CHAPTER8MOD
    if (IID_IPolyline8==riid)
        *ppv=m_pImpIPolyline;

    if (IID_IPersistStorage==riid)
        *ppv=m_pImpIPersistStorage;

    if (IID_IPersist==riid || IID_IPersistStream==riid
        || IID_IPersistStreamInit==riid)
        *ppv=m_pImpIPersistStreamInit;
    //End CHAPTER8MOD

    if (NULL!=*ppv)
        {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
        }

    return ResultFromScode(E_NOINTERFACE);
    }


STDMETHODIMP_(ULONG) CPolyline::AddRef(void)
    {
    return ++m_cRef;
    }


STDMETHODIMP_(ULONG) CPolyline::Release(void)
    {
    if (0L!=--m_cRef)
        return m_cRef;

    if (NULL!=m_pfnDestroy)
        (*m_pfnDestroy)();

    delete this;
    return 0L;
    }







/*
 * CPolyline::RectConvertMappings
 *
 * Purpose:
 *  Converts the contents of a rectangle from device (MM_TEXT) or
 *  HIMETRIC to the other.
 *
 * Parameters:
 *  pRect           LPRECT containing the rectangle to convert.
 *  fToDevice       BOOL TRUE to convert from HIMETRIC to device,
 *                  FALSE to convert device to HIMETRIC.
 *
 * Return Value:
 *  None
 */

void CPolyline::RectConvertMappings(LPRECT pRect, BOOL fToDevice)
    {
    HDC      hDC;
    int      iLpx, iLpy;

    if (NULL==pRect)
        return;

    hDC=GetDC(NULL);
    iLpx=GetDeviceCaps(hDC, LOGPIXELSX);
    iLpy=GetDeviceCaps(hDC, LOGPIXELSY);
    ReleaseDC(NULL, hDC);

    if (fToDevice)
        {
        pRect->left=MulDiv(iLpx, pRect->left, HIMETRIC_PER_INCH);
        pRect->top =MulDiv(iLpy, pRect->top , HIMETRIC_PER_INCH);

        pRect->right =MulDiv(iLpx, pRect->right, HIMETRIC_PER_INCH);
        pRect->bottom=MulDiv(iLpy, pRect->bottom,HIMETRIC_PER_INCH);
        }
    else
        {
        pRect->left=MulDiv(pRect->left, HIMETRIC_PER_INCH, iLpx);
        pRect->top =MulDiv(pRect->top , HIMETRIC_PER_INCH, iLpy);

        pRect->right =MulDiv(pRect->right, HIMETRIC_PER_INCH, iLpx);
        pRect->bottom=MulDiv(pRect->bottom,HIMETRIC_PER_INCH, iLpy);
        }

    return;
    }
