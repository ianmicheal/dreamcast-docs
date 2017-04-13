/*
 * TENANT.CPP
 * Patron Chapter 17
 *
 * Implementation of the CTentant class which holds information
 * for a single object on a page.  It maintains position, references
 * to data, and a storage.
 *
 * Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
 *
 * Kraig Brockschmidt, Microsoft
 * Internet  :  kraigb@microsoft.com
 * Compuserve:  >INTERNET:kraigb@microsoft.com
 */


#include "patron.h"


/*
 * CTenant::CTenant
 * CTenant::~CTenant
 *
 * Constructor Parameters:
 *  dwID            DWORD identifier for this page.
 *  hWnd            HWND of the pages window.
 *  pPG             PCPages to the parent structure.
 */

CTenant::CTenant(DWORD dwID, HWND hWnd, PCPages pPG)
    {
    m_hWnd=hWnd;
    m_dwID=dwID;

    m_fInitialized=0;
    m_pIStorage=NULL;
    m_cOpens=0;

    m_pObj=NULL;
    m_pPG =pPG;
    m_clsID=CLSID_NULL;
    m_fSetExtent=FALSE;

    //CHAPTER17MOD
    m_cRef=0;
    m_pIOleObject=NULL;
    m_pIViewObject2=NULL;
    m_grfMisc=0;

    m_pImpIOleClientSite=NULL;
    m_pImpIAdviseSink=NULL;

    m_fRepaintEnabled=TRUE;
    //End CHAPTER17MOD
    return;
    }


CTenant::~CTenant(void)
    {
    //CHAPTER17MOD
    //Object pointers cleaned up in Close.

    DeleteInterfaceImp(m_pImpIAdviseSink);
    DeleteInterfaceImp(m_pImpIOleClientSite);
    //End CHAPTER17MOD

    return;
    }




//CHAPTER17MOD
/*
 * CTenant::QueryInterface
 * CTenant::AddRef
 * CTenant::Release
 *
 * Purpose:
 *  IUnknown members for CTenant object.
 */

STDMETHODIMP CTenant::QueryInterface(REFIID riid, PPVOID ppv)
    {
    *ppv=NULL;

    if (IID_IUnknown==riid)
        *ppv=this;

    if (IID_IOleClientSite==riid)
        *ppv=m_pImpIOleClientSite;

    if (IID_IAdviseSink==riid)
        *ppv=m_pImpIAdviseSink;

    if (NULL!=*ppv)
        {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
        }

    return ResultFromScode(E_NOINTERFACE);
    }


STDMETHODIMP_(ULONG) CTenant::AddRef(void)
    {
    return ++m_cRef;
    }

STDMETHODIMP_(ULONG) CTenant::Release(void)
    {
    if (0!=--m_cRef)
        return m_cRef;

    delete this;
    return 0;
    }

//End CHAPTER17MOD





/*
 * CTenant::GetID
 *
 * Return Value:
 *  DWORD           dwID field in this tenant.
 */

DWORD CTenant::GetID(void)
    {
    return m_dwID;
    }



/*
 * CTenant::GetStorageName
 *
 * Parameters:
 *  pszName         LPOLESTR to a buffer in which to store the storage
 *                  name for this tenant.
 *
 * Return Value:
 *  UINT            Number of characters stored.
 */

UINT CTenant::GetStorageName(LPOLESTR pszName)
    {
   #ifdef WIN32ANSI
    char        szTemp[32];
    UINT        cch;

    cch=wsprintf(szTemp, "Tenant %lu", m_dwID);
    MultiByteToWideChar(CP_ACP, 0, szTemp, -1, pszName, 32);
    return cch;
   #else
    return wsprintf(pszName, TEXT("Tenant %lu"), m_dwID);
   #endif
    }



//CHAPTER17MOD
/*
 * CTenant::StorageGet
 *
 * Purpose:
 *  Returns the IStorage pointer maintained by this tenant,
 *  AddRef'd of course.
 *
 * Parameters:
 *  ppStg           LPSTORAGE * in which to return the pointer.
 *
 * Return Value:
 *  None
 */

void CTenant::StorageGet(LPSTORAGE *ppStg)
    {
    if (NULL==ppStg)
        return;

    *ppStg=m_pIStorage;

    if (NULL!=*ppStg)
        (*ppStg)->AddRef();

    return;
    }
//End CHAPTER17MOD




/*
 * CTenant::Create
 *
 * Purpose:
 *  Creates a new tenant of the given CLSID, which can be either a
 *  static bitmap or metafile or any compound document object.
 *
 * Parameters:
 *  tType           TENANTTYPE to create, either a static metafile,
 *                  bitmap, or some kind of compound document object
 *                  This determines which OleCreate* call we use.
 *  pvType          LPVOID providing the relevant pointer from which
 *                  to create the tenant, depending on iType.
 *  pFE             LPFORMATETC specifying the type of renderings
 *                  to use.
 *  pptl            PPOINTL in which we store offset coordinates.
 *  pszl            LPSIZEL where this object should store its
 *                  lometric extents.
 *  pIStorage       LPSTORAGE of the page we live in.  We have to
 *                  create another storage in this for the tenant.
 *  ppo             PPATRONOBJECT containing placement data.
 *  dwData          DWORD with extra data, sensitive to iType.
 *
 * Return Value:
 *  UINT            A CREATE_* value depending on what we
 *                  actually do.
 */

UINT CTenant::Create(TENANTTYPE tType, LPVOID pvType
    , LPFORMATETC pFE, PPOINTL pptl, LPSIZEL pszl
    , LPSTORAGE pIStorage, PPATRONOBJECT ppo, DWORD dwData)
    {
    HRESULT             hr;
    LPUNKNOWN           pObj;
    UINT                uRet=CREATE_GRAPHICONLY;
    //CHAPTER17MOD
    //Some things moves to ObjectInitialize
    //End CHAPTER17MOD

    if (NULL==pvType || NULL==pIStorage)
        return CREATE_FAILED;

    //Fail if this is called for an already living tenant.
    if (m_fInitialized)
        return CREATE_FAILED;

    m_fInitialized=TRUE;

    //Create a new storage for this tenant.
    if (!Open(pIStorage))
        return CREATE_FAILED;

    /*
     * Get the placement info if it's here.  We either have a non-
     * NULL PPATRONOBJECT in ppo or we have to use default
     * placement and retrieve the size from the object itself.
     */
    pszl->cx=0;
    pszl->cy=0;

    if (NULL!=ppo)
        {
        *pFE=ppo->fe;
        *pptl=ppo->ptl;
        *pszl=ppo->szl;     //Could be 0,0 , so we ask object

        uRet=CREATE_PLACEDOBJECT;
        }

    hr=ResultFromScode(E_FAIL);

    //Now create an object based specifically for the type.
    switch (tType)
        {
        case TENANTTYPE_NULL:
            break;

        case TENANTTYPE_STATIC:
            /*
             * We could use OleCreateStaticFromData here which does
             * pretty much what we're doing below.  However, it does
             * not allow us to control whether we paste a bitmap or
             * a metafile--it uses metafile first, bitmap second.
             * For this reason we'll use code developed in Chapter
             * 11's FreeLoader to affect the paste.
             */
            hr=CreateStatic((LPDATAOBJECT)pvType, pFE, &pObj);
            break;

        //CHAPTER17MOD
        case TENANTTYPE_EMBEDDEDOBJECT:
            hr=OleCreate(*((LPCLSID)pvType), IID_IUnknown
                , OLERENDER_DRAW, NULL, NULL, m_pIStorage
                , (PPVOID)&pObj);
            break;

        case TENANTTYPE_EMBEDDEDFILE:
            hr=OleCreateFromFile(CLSID_NULL, (LPTSTR)pvType
                , IID_IUnknown, OLERENDER_DRAW, NULL, NULL
                , m_pIStorage, (PPVOID)&pObj);
            break;

        case TENANTTYPE_EMBEDDEDOBJECTFROMDATA:
            hr=OleCreateFromData((LPDATAOBJECT)pvType, IID_IUnknown
                , OLERENDER_DRAW, NULL, NULL, m_pIStorage
                , (PPVOID)&pObj);
            break;
        //End CHAPTER17MOD

        default:
            break;
        }

    //If creation didn't work, get rid of the element Open created.
    if (FAILED(hr))
        {
        Destroy(pIStorage);
        return CREATE_FAILED;
        }

    //CHAPTER17MOD
    //We don't get the size if PatronObject data was seen already.
    if (!ObjectInitialize(pObj, pFE, dwData))
        {
        Destroy(pIStorage);
        return CREATE_FAILED;
        }

    if (0==pszl->cx && 0==pszl->cy)
        {
        SIZEL   szl;

        //Try to get the real size of the object, default to 2"*2"
        SETSIZEL((*pszl), 2*LOMETRIC_PER_INCH, 2*LOMETRIC_PER_INCH);
        hr=ResultFromScode(E_FAIL);

        //Try IViewObject2 first, then IOleObject as a backup.
        if (NULL!=m_pIViewObject2)
            {
            hr=m_pIViewObject2->GetExtent(m_fe.dwAspect, -1, NULL
                , &szl);
            }
        else
            {
            if (NULL!=m_pIOleObject)
                hr=m_pIOleObject->GetExtent(m_fe.dwAspect, &szl);
            }

        if (SUCCEEDED(hr))
            {
            //Convert HIMETRIC to our LOMETRIC mapping
            SETSIZEL((*pszl), szl.cx/10, szl.cy/10);
            }
        }
    //End CHAPTER17MOD

    return uRet;
    }






/*
 * CTenant::Load
 *
 * Purpose:
 *  Recreates the object living in this tenant in place of calling
 *  FCreate.  This is used in loading as opposed to new creation.
 *
 * Parameters:
 *  pIStorage       LPSTORAGE of the page we live in.
 *  pti             PTENTANTINFO containing persistent information.
 *                  The ID value in this structure is ignored.
 *
 * Return Value:
 *  BOOL            TRUE if successful, FALSE otherwise.
 */

BOOL CTenant::Load(LPSTORAGE pIStorage, PTENANTINFO pti)
    {
    HRESULT         hr;
    LPUNKNOWN       pObj;
    //CHAPTER17MOD
    DWORD           dwState=TENANTSTATE_DEFAULT;
    //End CHAPTER17MOD

    if (NULL==pIStorage || NULL==pti)
        return FALSE;

    //CHAPTER17MOD
    /*
     * If we already initialized once, clean up, releasing
     * everything before we attempt to reload.  This happens
     * when using the Convert Dialog.
     */
    if (m_fInitialized)
        {
        //Preserve all states except open
        dwState=(m_dwState & ~TENANTSTATE_OPEN);
        m_cRef++;   //Prevent accidental closure

        //This should release all holds on our IStorage as well.
        if (NULL!=m_pIViewObject2)
            {
            m_pIViewObject2->SetAdvise(m_fe.dwAspect, 0, NULL);
            ReleaseInterface(m_pIViewObject2);
            }

        ReleaseInterface(m_pIOleObject);
        ReleaseInterface(m_pObj);

        m_pIStorage=NULL;   //We'll have already released this.
        m_cRef--;           //Match safety increment above.
        }
    //End CHAPTER14MOD

    m_fInitialized=TRUE;

    //Open the storage for this tenant.
    if (!Open(pIStorage))
        return FALSE;

    hr=OleLoad(m_pIStorage, IID_IUnknown, NULL, (PPVOID)&pObj);

    if (FAILED(hr))
        {
        Destroy(pIStorage);
        return FALSE;
        }

    //CHAPTER17MOD
    m_fSetExtent=pti->fSetExtent;
    ObjectInitialize(pObj, &pti->fe, NULL);

    //Restore the original state before reloading.
    m_dwState=dwState;
    //End CHAPTER17MOD

    RectSet(&pti->rcl, FALSE, FALSE);
    return TRUE;
    }




/*
 * CTenant::GetInfo
 *
 * Purpose:
 *  Retrieved a TENANTINFO structure for this tenant.
 *
 * Parameters:
 *  pti             PTENANTINFO structure to fill
 *
 * Return Value:
 *  None
 */

void CTenant::GetInfo(PTENANTINFO pti)
    {
    if (NULL!=pti)
        {
        pti->dwID=m_dwID;
        pti->rcl=m_rcl;
        pti->fe=m_fe;
        pti->fSetExtent=m_fSetExtent;
        }

    return;
    }




//CHAPTER17MOD
/*
 * CTenant::ObjectInitialize
 * (Protected)
 *
 * Purpose:
 *  Performs operations necessary after creating an object or
 *  reloading one from storage.
 *
 * Parameters:
 *  pObj            LPUNKNOWN of the object in this tenant.
 *  pFE             LPFORMATETC describing the graphic here.
 *  dwData          DWORD extra data.  If pFE->dwAspect==
 *                  DVASPECT_ICON then this is the iconic metafile.
 *
 * Return Value:
 *  BOOL            TRUE if the function succeeded, FALSE otherwise.
 */

BOOL CTenant::ObjectInitialize(LPUNKNOWN pObj, LPFORMATETC pFE
    , DWORD dwData)
    {
    HRESULT         hr;
    LPPERSIST       pIPersist=NULL;
    DWORD           dw;
    PCDocument      pDoc;
    TCHAR           szFile[CCHPATHMAX];

    if (NULL==pObj || NULL==pFE)
        return FALSE;

    m_pObj=pObj;
    m_fe=*pFE;
    m_fe.ptd=NULL;
    m_dwState=TENANTSTATE_DEFAULT;

    /*
     * Determine the type:  Static or Embedded.  If Static,
     * this will have CLSID_Picture_Metafile or CLSID_Picture_Dib.
     * Otherwise it's embedded.  Later we'll add a case for links.
     */
    m_tType=TENANTTYPE_EMBEDDEDOBJECT;

    if (SUCCEEDED(pObj->QueryInterface(IID_IPersist
        , (PPVOID)&pIPersist)))
        {
        CLSID   clsid=CLSID_NULL;

        hr=pIPersist->GetClassID(&clsid);

        //If we don't have a CLSID, default to static
        if (FAILED(hr) || CLSID_Picture_Metafile==clsid
            || CLSID_Picture_Dib==clsid)
            m_tType=TENANTTYPE_STATIC;

        pIPersist->Release();
        }


    m_pIViewObject2=NULL;
    hr=pObj->QueryInterface(IID_IViewObject2
        , (PPVOID)&m_pIViewObject2);

    if (FAILED(hr))
        return FALSE;

    m_pIViewObject2->SetAdvise(m_fe.dwAspect, 0, m_pImpIAdviseSink);

    //We need an IOleObject most of the time, so get one here.
    m_pIOleObject=NULL;
    hr=pObj->QueryInterface(IID_IOleObject
         , (PPVOID)&m_pIOleObject);

    /*
     * Follow up object creation with advises and so forth.  If
     * we cannot get IOleObject here, then we know we can't do
     * any IOleObject actions from here on--object is static.
     */
    if (FAILED(hr))
        return TRUE;

    /*
     * Get the MiscStatus bits and check for OLEMISC_ONLYICONIC.
     * If set, force dwAspect in m_fe to DVASPECT_ICON so we
     * remember to draw it properly and do extents right.
     */
    m_pIOleObject->GetMiscStatus(m_fe.dwAspect, &m_grfMisc);

    if (OLEMISC_ONLYICONIC & m_grfMisc)
        m_fe.dwAspect=DVASPECT_ICON;

    /*
     * We could pass m_pImpIOleClientSite in an OleCreate* call, but
     * since this function could be called after OleLoad, we still
     * need to do this here, so it's always done here...
     */
    m_pIOleObject->SetClientSite(m_pImpIOleClientSite);
    m_pIOleObject->Advise(m_pImpIAdviseSink, &dw);

    OleSetContainedObject(m_pIOleObject, TRUE);

    /*
     * For IOleObject::SetHostNames we need the application name
     * and the document name (which is passed in the object
     * parameter).  The design of Patron doesn't give us nice
     * structured access to the name of the document we're in, so
     * I grab the parent of the Pages window (the document) and
     * send it DOCM_PDOCUMENT which returns us the pointer.
     * Roundabout, but it works.
     */

    pDoc=(PCDocument)SendMessage(GetParent(m_hWnd), DOCM_PDOCUMENT
        , 0, 0L);

    if (NULL!=pDoc)
        pDoc->FilenameGet(szFile, CCHPATHMAX);
    else
        szFile[0]=0;

    NotifyOfRename(szFile, NULL);


    /*
     * If we're creating an iconic aspect object and we have
     * an object from the Insert Object dialog, then we need to
     * store that iconic presentation in the cache, handled
     * with the utility function INOLE_SwitchDisplayAspect.  In
     * this case dwData is a handle to the metafile containing
     * the icon.  If dwData is NULL then we depend on the
     * server to provide the aspect, in which case we need
     * a view advise.
     */

    if (DVASPECT_ICON & m_fe.dwAspect)
        {
        DWORD           dw=DVASPECT_CONTENT;
        IAdviseSink    *pSink;

        pSink=(NULL==dwData) ? NULL : m_pImpIAdviseSink;

        INOLE_SwitchDisplayAspect(m_pIOleObject, &dw
            , DVASPECT_ICON, (HGLOBAL)(UINT)dwData, FALSE
            , (NULL!=dwData), pSink, NULL);
        }

    return TRUE;
    }
//End CHAPTER17MOD




/*
 * CTenant::Open
 *
 * Purpose:
 *  Retrieves the IStorage associated with this tenant.  The
 *  IStorage is owned by the tenant and thus the tenant always
 *  holds a reference count.
 *
 *  If the storage is already open for this tenant, then this
 *  function will AddRef it; therefore the caller must always
 *  match an Open with a Close.
 *
 * Parameters:
 *  pIStorage       LPSTORAGE above this tenant (which has its
 *                  own storage).
 *
 * Return Value:
 *  BOOL            TRUE if opening succeeds, FALSE otherwise.
 */

BOOL CTenant::Open(LPSTORAGE pIStorage)
    {
    HRESULT     hr=NOERROR;
    DWORD       dwMode=STGM_TRANSACTED | STGM_READWRITE
                    | STGM_SHARE_EXCLUSIVE;
    OLECHAR     szTemp[32];

    if (NULL==m_pIStorage)
        {
        if (NULL==pIStorage)
            return FALSE;

        /*
         * Attempt to open the storage under this ID.  If there is
         * none, then create it.  In either case we end up with an
         * IStorage that we either save in pPage or release.
         */

        GetStorageName(szTemp);
        hr=pIStorage->OpenStorage(szTemp, NULL, dwMode, NULL, 0
            , &m_pIStorage);

        if (FAILED(hr))
            {
            hr=pIStorage->CreateStorage(szTemp, dwMode, 0, 0
                , &m_pIStorage);
            }
        }
    else
        m_pIStorage->AddRef();

    if (FAILED(hr))
        return FALSE;

    m_cOpens++;

    //CHAPTER17MOD
    //Create these if we don't have them already.
    if (NULL==m_pImpIOleClientSite && NULL==m_pImpIAdviseSink)
        {
        m_pImpIOleClientSite=new CImpIOleClientSite(this, this);
        m_pImpIAdviseSink=new CImpIAdviseSink(this, this);

        if (NULL==m_pImpIOleClientSite || NULL==m_pImpIAdviseSink)
            return FALSE;
        }
    //End CHAPTER17MOD

    return TRUE;
    }




/*
 * CTenant::Close
 *
 * Purpose:
 *  Possibly commits the storage, then releases it reversing the
 *  reference count from Open.  If the reference on the storage
 *  goes to zero, the storage is forgotten.  However, the object we
 *  contain is still held and as long as it's active the storage
 *  remains alive.
 *
 * Parameters:
 *  fCommit         BOOL indicating if we're to commit.
 *
 * Return Value:
 *  None
 */

void CTenant::Close(BOOL fCommit)
    {
    if (fCommit)
        Update();

    if (NULL!=m_pIStorage)
        {
        m_pIStorage->Release();

        /*
         * We can't use a zero reference count to know when to NULL
         * this since other things might have AddRef'd the storage.
         */
        //CHAPTER17MOD
        if (0==--m_cOpens)
            {
            m_pIStorage=NULL;

            //Close the object saving if necessary
            if (NULL!=m_pIOleObject)
                {
                m_pIOleObject->Close(OLECLOSE_SAVEIFDIRTY);
                ReleaseInterface(m_pIOleObject);
                }

            //Release all other held pointers
            if (NULL!=m_pIViewObject2)
                {
                m_pIViewObject2->SetAdvise(m_fe.dwAspect, 0, NULL);
                ReleaseInterface(m_pIViewObject2);
                }

            //We know we only hold one ref from Create or Load
            ReleaseInterface(m_pObj);
            }
        //End CHAPTER17MOD
        }

    return;
    }




/*
 * CTenant::Update
 *
 * Purpose:
 *  Forces a common on the page if it's open.
 *
 * Parameters:
 *  None
 *
 * Return Value:
 *  BOOL            TRUE if the object is open, FALSE otherwise.
 */

BOOL CTenant::Update(void)
    {
    LPPERSISTSTORAGE    pIPS;

    if (NULL!=m_pIStorage)
        {
        /*
         * We need to OleSave again because we might have changed
         * the size or position of this tenant.  We also need to
         * save the rectangle on the page, since that's not known
         * to OLE.
         */
        m_pObj->QueryInterface(IID_IPersistStorage, (PPVOID)&pIPS);

        //This fails for static objects...so we improvise if so
        if (FAILED(OleSave(pIPS, m_pIStorage, TRUE)))
            {
            //This is essentially what OleSave does.
            WriteClassStg(m_pIStorage, m_clsID);
            pIPS->Save(m_pIStorage, TRUE);
            }

        pIPS->SaveCompleted(NULL);
        pIPS->Release();

        m_pIStorage->Commit(STGC_DEFAULT);
        }

    return FALSE;
    }





/*
 * CTenant::Destroy
 *
 * Purpose:
 *  Removes this page from the given storage.  The caller should
 *  eventually delete this CTenant object to free the object herein.
 *  Nothing is committed when being destroyed.
 *
 * Parameters:
 *  pIStorage       LPSTORAGE contianing this page on which to call
 *                  DestroyElement
 *
 * Return Value:
 *  None
 */

void CTenant::Destroy(LPSTORAGE pIStorage)
    {
    OLECHAR     szTemp[32];

    if (NULL!=pIStorage)
        {
        //CHAPTER17MOD
        if (NULL!=m_pIOleObject)
            m_pIOleObject->Close(OLECLOSE_NOSAVE);
        //End CHAPTER17MOD

        if (NULL!=m_pIStorage)
            {
            //Remove all reference/open counts on this storage.
            while (0!=m_cOpens)
                {
                m_pIStorage->Release();
                m_cOpens--;
                }
            }

        GetStorageName(szTemp);
        pIStorage->DestroyElement(szTemp);

        m_pIStorage=NULL;
        }

    return;
    }




/*
 * CTenant::Select
 *
 * Purpose:
 *  Selects or deselects the tenant.
 *
 * Parameters:
 *  fSelect         BOOL indicating the new state of the tenant.
 *
 * Return Value:
 *  None
 */

void CTenant::Select(BOOL fSelect)
    {
    BOOL        fWasSelected;
    DWORD       dwState;
    RECT        rc;
    HDC         hDC;

    fWasSelected=(BOOL)(TENANTSTATE_SELECTED & m_dwState);

    //Nothing to do when there's no change.
    if (fWasSelected==fSelect)
        return;

    dwState=m_dwState & ~TENANTSTATE_SELECTED;
    m_dwState=dwState | ((fSelect) ? TENANTSTATE_SELECTED : 0);

    /*
     * Draw sizing handles to show the selection state.  We convert
     * things to MM_TEXT since that's what this function expects.
     */

    RECTFROMRECTL(rc, m_rcl);
    RectConvertMappings(&rc, NULL, TRUE);
    OffsetRect(&rc, -(int)m_pPG->m_xPos, -(int)m_pPG->m_yPos);

    hDC=GetDC(m_hWnd);

    UIDrawHandles(&rc, hDC, UI_HANDLES_INSIDE
        | UI_HANDLES_NOBORDER | UI_HANDLES_USEINVERSE
        , CXYHANDLE, !fWasSelected);

    ReleaseDC(m_hWnd, hDC);

    if (fSelect)
        m_pPG->m_fDirty=TRUE;

    return;
    }




//CHAPTER17MOD
/*
 * CTenant::ShowAsOpen
 *
 * Purpose:
 *  Draws or removes the hatch pattern over an object.
 *
 * Parameters:
 *  fOpen           BOOL indicating the open state of this tenant.
 *
 * Return Value:
 *  None
 */

void CTenant::ShowAsOpen(BOOL fOpen)
    {
    BOOL        fWasOpen;
    DWORD       dwState;
    RECT        rc;
    HDC         hDC;

    fWasOpen=(BOOL)(TENANTSTATE_OPEN & m_dwState);

    dwState=m_dwState & ~TENANTSTATE_OPEN;
    m_dwState=dwState | ((fOpen) ? TENANTSTATE_OPEN : 0);

    //If this was not open, then just hatch, otherwise repaint.
    if (!fWasOpen && fOpen)
        {
        RECTFROMRECTL(rc, m_rcl);
        RectConvertMappings(&rc, NULL, TRUE);
        OffsetRect(&rc, -(int)m_pPG->m_xPos, -(int)m_pPG->m_yPos);

        hDC=GetDC(m_hWnd);
        UIDrawShading(&rc, hDC, UI_SHADE_FULLRECT, 0);
        ReleaseDC(m_hWnd, hDC);
        }

    if (fWasOpen && !fOpen)
        Repaint();

    return;
    }





/*
 * CTenant::ShowYourself
 *
 * Purpose:
 *  Function that really just implements IOleClientSite::ShowObject.
 *  Here we first check if the tenant is fully visible, and if so,
 *  then nothing needs to happen.  Otherwise, if the upper left
 *  corner of the tenant is in the upper left visible quadrant of
 *  the window, we'll also consider ourselves done.  Otherwise
 *  we'll put the upper left corner of the object at the upper left
 *  corner of the window.
 *
 * Parameters:
 *  None
 *
 * Return Value:
 *  None
 */

void CTenant::ShowYourself(void)
    {
    RECTL       rcl;
    RECT        rc;
    POINT       pt1, pt2;

    //Scrolling deals in device units; get our rectangle in those.
    RectGet(&rcl, TRUE);

    //Get the window rectangle offset for the current scroll pos.
    GetClientRect(m_hWnd, &rc);
    OffsetRect(&rc, m_pPG->m_xPos, m_pPG->m_yPos);

    //Check if the object is already visible. (macro in bookguid.h)
    SETPOINT(pt1, (int)rcl.left,  (int)rcl.top);
    SETPOINT(pt2, (int)rcl.right, (int)rcl.bottom);

    if (PtInRect(&rc, pt1) && PtInRect(&rc, pt2))
        return;

    //Check if the upper left is within the upper left quadrant
    if (((int)rcl.left > rc.left
        && (int)rcl.left < ((rc.right+rc.left)/2))
        && ((int)rcl.top > rc.top
        && (int)rcl.top < ((rc.bottom+rc.top)/2)))
        return;

    //These are macros in INC\BOOK1632.H
    SendScrollPosition(m_hWnd, WM_HSCROLL, rcl.left-8);
    SendScrollPosition(m_hWnd, WM_VSCROLL, rcl.top-8);
    return;
    }



/*
 * CTenant::AddVerbMenu
 *
 * Purpose:
 *  Creates the variable verb menu item for the object in this
 *  tenant.
 *
 * Parmeters:
 *  hMenu           HMENU on which to add items.
 *  iPos            UINT position on that menu to add items.
 *
 * Return Value:
 *  None
 */

void CTenant::AddVerbMenu(HMENU hMenu, UINT iPos)
    {
    HMENU       hMenuTemp;
    LPOLEOBJECT pObj=m_pIOleObject;

    //If we're static, say we have no object.
    if (TENANTTYPE_STATIC==m_tType)
        pObj=NULL;

    OleUIAddVerbMenu(pObj, NULL, hMenu, iPos, IDM_VERBMIN
        , IDM_VERBMAX, TRUE, IDM_EDITCONVERT, &hMenuTemp);

    return;
    }




/*
 * CTenant::TypeGet
 *
 * Purpose:
 *  Returns the type of this tenant
 *
 * Parameters:
 *  None
 *
 * Return Value:
 *  TENANTTYPE      Type of the tenant.
 */

TENANTTYPE CTenant::TypeGet(void)
    {
    return m_tType;
    }






/*
 * CTenant::CopyEmbeddedObject
 *
 * Purpose:
 *  Copies an embedded object to the given data object (via SetData,
 *  assuming this is a data transfer object for clipboard/drag-drop)
 *  if that's what we're holding.
 *
 * Parameters:
 *  pIDataObject    LPDATAOBJECT in which to store the copy.
 *  pFE             LPFORMATETC into which to copy CFSTR_EMBEDDEDOBJECT
 *                  if we put that in the data object.
 *  pptl            PPOINTL to the pick point (NULL outside of
 *                  drag-drop);
 *
 * Return Value:
 *  None
 */

void CTenant::CopyEmbeddedObject(LPDATAOBJECT pIDataObject
    , LPFORMATETC pFE, PPOINTL pptl)
    {
    LPPERSISTSTORAGE    pIPS;
    STGMEDIUM           stm;
    FORMATETC           fe;
    HRESULT             hr;
    UINT                cf;
    POINTL              ptl;
    SIZEL               szl;

    //Can only copy embeddings.
    if (TENANTTYPE_EMBEDDEDOBJECT!=m_tType || NULL==m_pIOleObject)
        return;

    if (NULL==pptl)
        {
        SETPOINTL(ptl, 0, 0);
        pptl=&ptl;
        }

    /*
     * Create CFSTR_EMBEDDEDOBJECT.  This is simply an IStorage with
     * a copy of the embedded object in it.  The not-so-simple part
     * is getting an IStorage to stuff it in.  For this operation
     * we'll use a temporary compound file.
     */

    stm.pUnkForRelease=NULL;
    stm.tymed=TYMED_ISTORAGE;
    hr=StgCreateDocfile(NULL, STGM_TRANSACTED | STGM_READWRITE
        | STGM_CREATE| STGM_SHARE_EXCLUSIVE | STGM_DELETEONRELEASE
        , 0, &stm.pstg);

    if (FAILED(hr))
        return;

    m_pObj->QueryInterface(IID_IPersistStorage, (PPVOID)&pIPS);

    if (NOERROR==pIPS->IsDirty())
        {
        OleSave(pIPS, stm.pstg, FALSE);
        pIPS->SaveCompleted(NULL);
        }
    else
        m_pIStorage->CopyTo(0, NULL, NULL, stm.pstg);

    pIPS->Release();

    //stm.pstg now has a copy, so stuff it away.
    cf=RegisterClipboardFormat(CFSTR_EMBEDDEDOBJECT);
    SETDefFormatEtc(fe, cf, TYMED_ISTORAGE);

    if (SUCCEEDED(pIDataObject->SetData(&fe, &stm, TRUE)))
        *pFE=fe;
    else
        ReleaseStgMedium(&stm);

    stm.tymed=TYMED_HGLOBAL;

    /*
     * You want to make sure that if this object is iconic, that you
     * create the object descriptor with DVASPECT_ICON instead of
     * the more typical DVASPECT_CONTENT.  Also remember that
     * the pick point is in HIMETRIC.
     */
    XformSizeInPixelsToHimetric(NULL, (LPSIZEL)pptl, (LPSIZEL)&ptl);

    SETSIZEL(szl, (10*(m_rcl.right-m_rcl.left))
        , (10 * (m_rcl.bottom-m_rcl.top)));

    stm.hGlobal=INOLE_ObjectDescriptorFromOleObject
        (m_pIOleObject, m_fe.dwAspect, ptl, &szl);

    cf=RegisterClipboardFormat(CFSTR_OBJECTDESCRIPTOR);
    SETDefFormatEtc(fe, cf, TYMED_HGLOBAL);

    if (FAILED(pIDataObject->SetData(&fe, &stm, TRUE)))
        ReleaseStgMedium(&stm);

    return;
    }





/*
 * CTenant::NotifyOfRename
 *
 * Purpose:
 *  Instructs the tenant that the document was saved under a
 *  different name.  In order to keep the right compound document
 *  user interface, this tenant needs to tell its object through
 *  IOleObject::SetHostNames.
 *
 * Parameters:
 *  pszFile         LPTSTR of filename.
 *  pvReserved      LPVOID reserved for future use.
 *
 * Return Value:
 *  None
 */

void CTenant::NotifyOfRename(LPTSTR pszFile, LPVOID pvReserved)
    {
    TCHAR       szObj[40];
    TCHAR       szApp[40];

    if (NULL==m_pIOleObject)
        return;

    if (TEXT('\0')==*pszFile)
        {
        LoadString(m_pPG->m_hInst, IDS_UNTITLED, szObj
            , sizeof(szObj));
        }
    else
        {
        GetFileTitle(pszFile, szObj, sizeof(szObj));

       #ifndef WIN32
        //Force filenames to uppercase in DOS versions.
        AnsiUpper(szObj);
       #endif
        }

    LoadString(m_pPG->m_hInst, IDS_CAPTION, szApp, sizeof(szApp));
   #ifdef WIN32ANSI
    OLECHAR     szObjW[40], szAppW[40];

    MultiByteToWideChar(CP_ACP, 0, szObj, -1, szObjW, 40);
    MultiByteToWideChar(CP_ACP, 0, szApp, -1, szAppW, 40);
    m_pIOleObject->SetHostNames(szAppW, szObjW);
   #else
    m_pIOleObject->SetHostNames(szApp, szObj);
   #endif
    return;
    }

//End CHAPTER17MOD





/*
 * CTenant::Activate
 *
 * Purpose:
 *  Activates a verb on the object living in the tenant.  Does
 *  nothing for static objects.
 *
 * Parameters:
 *  iVerb           LONG of the verb to execute.
 *
 * Return Value:
 *  BOOL            TRUE if the object changed due to this verb
 *                  execution.
 */

BOOL CTenant::Activate(LONG iVerb)
    {
    //CHAPTER17MOD
    RECT        rc, rcH;
    CHourglass *pHour;
    SIZEL       szl;

    //Can't activate statics.
    if (TENANTTYPE_STATIC==m_tType || NULL==m_pIOleObject)
        {
        MessageBeep(0);
        return FALSE;
        }

    RECTFROMRECTL(rc, m_rcl);
    RectConvertMappings(&rc, NULL, TRUE);
    XformRectInPixelsToHimetric(NULL, &rc, &rcH);

    pHour=new CHourglass;

    //Get the server running first, then do a SetExtent, then show it
    OleRun(m_pIOleObject);

    if (m_fSetExtent)
        {
        SETSIZEL(szl, rcH.right-rcH.left, rcH.top-rcH.bottom);
        m_pIOleObject->SetExtent(m_fe.dwAspect, &szl);
        m_fSetExtent=FALSE;
        }

    m_pIOleObject->DoVerb(iVerb, NULL, m_pImpIOleClientSite, 0
        , m_hWnd, &rcH);

    delete pHour;

    //If object changes, IAdviseSink::OnViewChange will see it.
    return FALSE;
    //End CHAPTER17MOD
    }






/*
 * CTenant::Draw
 *
 * Purpose:
 *  Draws the tenant in its rectangle on the given hDC.  We assume
 *  the DC is already set up for the mapping mode in which our
 *  rectangle is expressed, since the Page we're in tells us both
 *  the rect and the hDC.
 *
 * Parameters:
 *  hDC             HDC in which to draw.  Could be a metafile,
 *                  memory DC, screen, or printer.
 *  ptd             DVTARGETDEVICE * describing the device.
 *  hIC             HDC holding an information context (printing).
 *  xOff, yOff      int offsets for the page in lometric
 *  fNoColor        BOOL indicating if we should do B & W
 *  fPrinter        BOOL indicating if we should render for a
 *                  printer.
 *
 * Return Value:
 *  None
 */

void CTenant::Draw(HDC hDC, DVTARGETDEVICE *ptd, HDC hIC
    , int xOff, int yOff, BOOL fNoColor, BOOL fPrinter)
    {
    HRESULT         hr;
    RECT            rc;
    RECTL           rcl;
    UINT            uMM;

    //CHAPTER17MOD
    //We hold IViewObject2 all the time now, so no QueryInterface
    //End CHAPTER17MOD

    RECTFROMRECTL(rc, m_rcl);
    OffsetRect(&rc, -xOff, -yOff);
    RECTLFROMRECT(rcl, rc);

    //CHAPTER17MOD
    //Repaint erases the rectangle to insure full object cleanup
    if (!fNoColor && !fPrinter)
        {
        COLORREF    cr;
        cr=SetBkColor(hDC, GetSysColor(COLOR_WINDOW));
        ExtTextOut(hDC, rc.left, rc.top, ETO_OPAQUE, &rc, NULL
            , 0, NULL);
        SetBkColor(hDC, cr);
        }

    //We have to use Draw since we have a target device and IC.
    hr=m_pIViewObject2->Draw(m_fe.dwAspect, -1, NULL, ptd, hIC, hDC
        , &rcl, NULL, NULL, 0);

    //End CHAPTER17MOD

    /*
     * If Draw failed, then perhaps it couldn't work for the device,
     * so try good old OleDraw as a last resort.  The code will
     * generally be OLE_E_BLANK.
     */
    if (FAILED(hr))
        OleDraw(m_pObj, m_fe.dwAspect, hDC, &rc);

    //CHAPTER17MOD
    if (!fPrinter
        && (TENANTSTATE_SELECTED | TENANTSTATE_OPEN) & m_dwState)
        {
        /*
         * Draw sizing handles to show the selection state.  We
         * convert things to MM_TEXT since that's what this
         * function expects.
         */
        RectConvertMappings(&rc, NULL, TRUE);
        uMM=SetMapMode(hDC, MM_TEXT);

        if (TENANTSTATE_SELECTED & m_dwState)
            {
            UIDrawHandles(&rc, hDC, UI_HANDLES_INSIDE
                | UI_HANDLES_NOBORDER | UI_HANDLES_USEINVERSE
                , CXYHANDLE, TRUE);
            }

        if (TENANTSTATE_OPEN & m_dwState)
            UIDrawShading(&rc, hDC, UI_SHADE_FULLRECT, 0);

        uMM=SetMapMode(hDC, uMM);
        }
    //End CHAPTER17MOD

    return;
    }





/*
 * CTenant::Repaint
 * CTenant::Invalidate
 *
 * Purpose:
 *  Repaints the tenant where it lies or invalidates its area
 *  for later repainting.
 *
 * Parameters:
 *  None
 *
 * Return Value:
 *  None
 */

void CTenant::Repaint(void)
    {
    RECT        rc;
    HDC         hDC;

    //CHAPTER17MOD
    /*
     * We might be asked to repaint from
     * IOleClientSite::OnShowWindow after we've switched pages if
     * our server was running. This check on m_cOpens prevents that.
     */
    if (0==m_cOpens || !m_fRepaintEnabled)
        return;
    //End CHAPTER17MOD

    hDC=GetDC(m_hWnd);
    SetRect(&rc, m_pPG->m_xPos, m_pPG->m_yPos, 0, 0);
    RectConvertMappings(&rc, NULL, FALSE);

    SetMapMode(hDC, MM_LOMETRIC);
    Draw(hDC, NULL, NULL, rc.left, rc.top, FALSE, FALSE);

    ReleaseDC(m_hWnd, hDC);
    return;
    }


void CTenant::Invalidate(void)
    {
    RECTL       rcl;
    RECT        rc;

    RectGet(&rcl, TRUE);
    RECTFROMRECTL(rc, rcl);

    OffsetRect(&rc, -(int)m_pPG->m_xPos, -(int)m_pPG->m_yPos);
    InvalidateRect(m_hWnd, &rc, FALSE);

    return;
    }




//CHAPTER17MOD
/*
 * CTenant::ObjectClassFormatAndIcon
 *
 * Purpose:
 *  Helper function for CPage::ConvertObject to retrieve necessary
 *  information about the object.
 *
 * Parameters:
 *  pClsID          LPCLSID in which to store the CLSID.
 *  pwFormat        LPWORD in which to store the clipboard format
 *                  used.
 *  ppszType        LPTSTR * in which to return a pointer to a
 *                  type string.
 *  phMetaIcon      HGLOBAL * in which to return the metafile
 *                  icon currently in use.
 *
 * Return Value:
 *  None
 */

void CTenant::ObjectClassFormatAndIcon(LPCLSID pClsID
    , LPWORD pwFormat, LPTSTR *ppszType, HGLOBAL *phMetaIcon
    , LPTSTR *ppszLabel)
    {
    HRESULT         hr;
    TCHAR           szType[128];
    LPDATAOBJECT    pIDataObject;
    FORMATETC       fe;
    STGMEDIUM       stm;

    if (TENANTTYPE_EMBEDDEDOBJECT!=m_tType || NULL==m_pIOleObject)
        return;

    if (NULL==pClsID || NULL==pwFormat || NULL==ppszType
        || NULL==phMetaIcon)
        return;


    /*
     * For embedded objects get the real CLSID of the object and
     * its format string.  If this fails then we can try to ask
     * the object, or we can look in the registry.
     */

    hr=ReadClassStg(m_pIStorage, pClsID);

    if (FAILED(hr))
        {
        hr=m_pIOleObject->GetUserClassID(pClsID);

        if (FAILED(hr))
            *pClsID=CLSID_NULL;
        }


    hr=ReadFmtUserTypeStg(m_pIStorage, pwFormat, ppszType);

    if (FAILED(hr))
        {
        *pwFormat=0;
        *ppszType=NULL;

        if (INOLE_GetUserTypeOfClass(*pClsID, 0, szType
            , sizeof(szType)))
            {
            *ppszType=INOLE_CopyString(szType);
            }
        }

    /*
     * Try to get the AuxUserType from the registry, using
     * the short version (registered under AuxUserType\2).
     * If that fails, just copy *ppszType.
     */
    *ppszLabel=NULL;

    if (INOLE_GetUserTypeOfClass(*pClsID, 2, szType
        , sizeof(szType)))
        {
        *ppszLabel=INOLE_CopyString(szType);
        }
    else
        *ppszLabel=INOLE_CopyString(*ppszType);

    //Get the icon for this thing, if we're iconic.
    *phMetaIcon=NULL;

    hr=m_pObj->QueryInterface(IID_IDataObject
        , (PPVOID)&pIDataObject);

    if (SUCCEEDED(hr))
        {
        SETFormatEtc(fe, CF_METAFILEPICT, DVASPECT_ICON, NULL
            , TYMED_MFPICT, -1);
        hr=pIDataObject->GetData(&fe, &stm);
        pIDataObject->Release();

        if (SUCCEEDED(hr))
            *phMetaIcon=stm.hGlobal;
        else
            *phMetaIcon=OleGetIconOfClass(*pClsID, NULL, TRUE);
        }

    return;
    }




/*
 * CTenant::SwitchOrUpdateAspect
 *
 * Purpose:
 *  Switches between DVASPECT_CONTENT and DVASPECT_ICON
 *
 * Parameters:
 *  hMetaIcon       HGLOBAL to the new icon if we're changing the
 *                  icon or switching to DVASPECT_ICON.  NULL to
 *                  change back to content.
 *  fPreserve       BOOL indicating if we're to preserve the old
 *                  aspect after changing.
 *
 * Return Value:
 *  BOOL            TRUE if anything changed, FALSE otherwise.
 */

BOOL CTenant::SwitchOrUpdateAspect(HGLOBAL hMetaIcon
    , BOOL fPreserve)
    {
    HRESULT     hr;
    DWORD       dwAspect;
    BOOL        fUpdate=FALSE;

    //Nothing to do if we're content already and there's no icon.
    if (NULL==hMetaIcon && DVASPECT_CONTENT==m_fe.dwAspect)
        return FALSE;

    //If we're iconic already, just cache the new icon
    if (NULL!=hMetaIcon && DVASPECT_ICON==m_fe.dwAspect)
        hr=INOLE_SetIconInCache(m_pIOleObject, hMetaIcon);
    else
        {
        //Otherwise, switch between iconic and content.
        dwAspect=(NULL==hMetaIcon) ? DVASPECT_CONTENT : DVASPECT_ICON;

        /*
         * Switch between aspects, where dwAspect has the new one
         * and m_fe.dwAspect will be changed in the process.
         */
        hr=INOLE_SwitchDisplayAspect(m_pIOleObject
            , &m_fe.dwAspect, dwAspect, hMetaIcon, !fPreserve
            , TRUE, m_pImpIAdviseSink, &fUpdate);

        if (SUCCEEDED(hr))
            {
            //Update MiscStatus for the new aspect
            m_pIOleObject->GetMiscStatus(m_fe.dwAspect, &m_grfMisc);

            if (fUpdate)
                m_pIOleObject->Update();    //This repaints.
            }
        }

    //If we switched, update our extents.
    if (SUCCEEDED(hr))
        {
        SIZEL       szl;

        m_pIOleObject->GetExtent(m_fe.dwAspect, &szl);

        if (0 > szl.cy)
            szl.cy=-szl.cy;

        //Convert HIMETRIC absolute units to our LOMETRIC mapping
        if (0!=szl.cx && 0!=szl.cy)
            SETSIZEL(szl, szl.cx/10, -szl.cy/10);

        Invalidate();                   //Remove old aspect
        SizeSet(&szl, FALSE, FALSE);    //Change size
        Repaint();                      //Paint the new one
        }

    return SUCCEEDED(hr);
    }



/*
 * CTenant::EnableRepaint
 *
 * Purpose:
 *  Toggles whether the Repaint function does anything.  This
 *  is used during conversion/emulation of an object to disable
 *  repaints until the new object can be given the proper extents.
 *
 * Parameters:
 *  fEnable         TRUE to enable repaints, FALSE to disable.
 *
 * Return Value:
 *  None
 */

void CTenant::EnableRepaint(BOOL fEnable)
    {
    m_fRepaintEnabled=fEnable;
    return;
    }
//End CHAPTER17MOD








/*
 * CTenant::ObjectGet
 *
 * Purpose:
 *  Retrieves the LPUNKNOWN of the object in use by this tenant
 *
 * Parameters:
 *  ppUnk           LPUNKNOWN * in which to return the object
 *                  pointer.
 *
 * Return Value:
 *  None
 */

void CTenant::ObjectGet(LPUNKNOWN *ppUnk)
    {
    if (NULL!=ppUnk)
        {
        *ppUnk=m_pObj;
        m_pObj->AddRef();
        }

    return;
    }





/*
 * CTenant::FormatEtcGet
 *
 * Purpose:
 *  Retrieves the FORMATETC in use by this tenant
 *
 * Parameters:
 *  pFE             LPFORMATETC in which to store the information.
 *  fPresentation   BOOL indicating if we want the real format or
 *                  that of the presentation.
 *
 * Return Value:
 *  None
 */

void CTenant::FormatEtcGet(LPFORMATETC pFE, BOOL fPresentation)
    {
    if (NULL!=pFE)
        {
        *pFE=m_fe;

        //CHAPTER17MOD
        //If there is no format, use metafile (for embedded objects)
        if (fPresentation || 0==pFE->cfFormat)
            {
            //Don't mess with dwAspect; might be icon or content.
            pFE->cfFormat=CF_METAFILEPICT;
            pFE->tymed=TYMED_MFPICT;
            }
        //End CHAPTER17MOD
        }

    return;
    }





/*
 * CTenant::SizeGet
 * CTenant::SizeSet
 * CTenant::RectGet
 * CTenant::RectSet
 *
 * Purpose:
 *  Returns or sets the size/position of the object contained here.
 *
 * Parameters:
 *  pszl/prcl       LPSIZEL (Size) or LPRECTL (Rect) with the
 *                  extents of interest.  In Get situations,
 *                  this will receive the extents; in Set it
 *                  contains the extents.
 *  fDevice         BOOL indicating that pszl/prcl is expressed
 *                  in device units.  Otherwise it's LOMETRIC.
 *  fInformObj      (Set Only) BOOL indicating if we need to inform
 *                  the object all.
 *
 * Return Value:
 *  None
 */

void CTenant::SizeGet(LPSIZEL pszl, BOOL fDevice)
    {
    if (!fDevice)
        {
        pszl->cx=m_rcl.right-m_rcl.left;
        pszl->cy=m_rcl.bottom-m_rcl.top;
        }
    else
        {
        RECT        rc;

        SetRect(&rc, (int)(m_rcl.right-m_rcl.left)
            , (int)(m_rcl.bottom-m_rcl.top), 0, 0);

        RectConvertMappings(&rc, NULL, TRUE);

        pszl->cx=(long)rc.left;
        pszl->cy=(long)rc.top;
        }

    return;
    }

//CHAPTER17MOD
void CTenant::SizeSet(LPSIZEL pszl, BOOL fDevice, BOOL fInformObj)
//End CHAPTER17MOD
    {
    SIZEL           szl;

    if (!fDevice)
        {
        szl=*pszl;
        m_rcl.right =pszl->cx+m_rcl.left;
        m_rcl.bottom=pszl->cy+m_rcl.top;
        }
    else
        {
        RECT        rc;

        SetRect(&rc, (int)pszl->cx, (int)pszl->cy, 0, 0);
        RectConvertMappings(&rc, NULL, FALSE);

        m_rcl.right =(long)rc.left+m_rcl.left;
        m_rcl.bottom=(long)rc.top+m_rcl.top;

        SETSIZEL(szl, (long)rc.left, (long)rc.top);
        }


    //Tell OLE that this object was resized.
    //CHAPTER17MOD
    if (NULL!=m_pIOleObject && fInformObj)
        {
        HRESULT     hr;
        BOOL        fRun=FALSE;

        //Convert our LOMETRIC into HIMETRIC by *=10
        szl.cx*=10;
        szl.cy*=-10;    //Our size is stored negative.

        /*
         * If the MiscStatus bit of OLEMISC_RECOMPOSEONRESIZE
         * is set, then we need to run the object before calling
         * SetExtent to make sure it has a real chance to
         * re-render the object.  We have to update and close
         * the object as well after this happens.
         */

        if (OLEMISC_RECOMPOSEONRESIZE & m_grfMisc)
            {
            if (!OleIsRunning(m_pIOleObject))
                {
                OleRun(m_pIOleObject);
                fRun=TRUE;
                }
            }

        hr=m_pIOleObject->SetExtent(m_fe.dwAspect, &szl);

        /*
         * If the object is not running and it does not have
         * RECOMPOSEONRESIZE, then SetExtent fails.  Make
         * sure that we call SetExtent again (by just calling
         * SizeSet here again) when we next run the object.
         */
        if (SUCCEEDED(hr))
            {
            m_fSetExtent=FALSE;

            if (fRun)
                {
                m_pIOleObject->Update();
                m_pIOleObject->Close(OLECLOSE_SAVEIFDIRTY);
                }
            }
        else
            {
            if (OLE_E_NOTRUNNING==GetScode(hr))
                m_fSetExtent=TRUE;
            }
        }
    //End CHAPTER17MOD

    return;
    }


void CTenant::RectGet(LPRECTL prcl, BOOL fDevice)
    {
    if (!fDevice)
        *prcl=m_rcl;
    else
        {
        RECT        rc;

        RECTFROMRECTL(rc, m_rcl);
        RectConvertMappings(&rc, NULL, TRUE);
        RECTLFROMRECT(*prcl, rc);
        }

    return;
    }


//CHAPTER17MOD
void CTenant::RectSet(LPRECTL prcl, BOOL fDevice, BOOL fInformObj)
//End CHAPTER17MOD
    {
    SIZEL   szl;
    LONG    cx, cy;

    cx=m_rcl.right-m_rcl.left;
    cy=m_rcl.bottom-m_rcl.top;

    if (!fDevice)
        m_rcl=*prcl;
    else
        {
        RECT        rc;

        RECTFROMRECTL(rc, *prcl);
        RectConvertMappings(&rc, NULL, FALSE);
        RECTLFROMRECT(m_rcl, rc);
        }

    /*
     * Tell ourselves that the size changed, if it did.  SizeSet
     * will call IOleObject::SetExtent for us.
     */
    if ((m_rcl.right-m_rcl.left)!=cx || (m_rcl.bottom-m_rcl.top)!=cy)
        {
        SETSIZEL(szl, m_rcl.right-m_rcl.left, m_rcl.bottom-m_rcl.top);
        //CHAPTER17MOD
        SizeSet(&szl, FALSE, fInformObj);
        //End CHAPTER17MOD
        }

    return;
    }







/*
 * CTenant::CreateStatic
 * (Protected)
 *
 * Purpose:
 *  Creates a new static bitmap or metafile object for this tenant
 *  using a freeloading method allowing us to specify exactly which
 *  type of data we want to paste since OleCreateStaticFromData
 *  doesn't.
 *
 * Parameters:
 *  pIDataObject    LPDATAOBJECT from which to paste.
 *  pFE             LPFORMATETC describing the format to paste.
 *  ppObj           LPUNKNOWN * into which we store the
 *                  object pointer.
 *
 * Return Value:
 *  HRESULT         NOERROR on success, error code otherwise.
 */

HRESULT CTenant::CreateStatic(LPDATAOBJECT pIDataObject
    , LPFORMATETC pFE, LPUNKNOWN *ppObj)
    {
    HRESULT             hr;
    STGMEDIUM           stm;
    LPUNKNOWN           pIUnknown;
    LPOLECACHE          pIOleCache;
    LPPERSISTSTORAGE    pIPersistStorage;
    CLSID               clsID;

    *ppObj=NULL;

    //Try to get the data desired as specified in pFE->cfFormat
    hr=pIDataObject->GetData(pFE, &stm);

    if (FAILED(hr))
        return hr;

    //Create the object to handle this data.
    if (CF_METAFILEPICT==pFE->cfFormat)
        clsID=CLSID_Picture_Metafile;
    else
        clsID=CLSID_Picture_Dib;

    hr=CreateDataCache(NULL, clsID, IID_IUnknown
        , (PPVOID)&pIUnknown);

    if (FAILED(hr))
        {
        ReleaseStgMedium(&stm);
        return hr;
        }

    m_clsID=clsID;

    //Stuff the data into the object
    pIUnknown->QueryInterface(IID_IPersistStorage
        , (PPVOID)&pIPersistStorage);
    pIPersistStorage->InitNew(m_pIStorage);

    //Now that we have the cache object, shove the data into it.
    pIUnknown->QueryInterface(IID_IOleCache, (PPVOID)&pIOleCache);
    pIOleCache->Cache(pFE, ADVF_PRIMEFIRST, NULL);

    hr=pIOleCache->SetData(pFE, &stm, TRUE);
    pIOleCache->Release();

    //Insure there is a persistent copy on the disk
    WriteClassStg(m_pIStorage, m_clsID);
    pIPersistStorage->Save(m_pIStorage, TRUE);
    pIPersistStorage->SaveCompleted(NULL);
    pIPersistStorage->Release();

    //The cache owns this now.
    ReleaseStgMedium(&stm);

    if (FAILED(hr))
        pIUnknown->Release();
    else
        *ppObj=pIUnknown;

    return hr;
    }
