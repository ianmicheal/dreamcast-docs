/*
 * DOCUMENT.CPP
 * Patron Chapter 7
 *
 * Implementation of the CPatronDoc derivation of CDocument that
 * manages pages for us.
 *
 * Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
 *
 * Kraig Brockschmidt, Microsoft
 * Internet  :  kraigb@microsoft.com
 * Compuserve:  >INTERNET:kraigb@microsoft.com
 */


#include "patron.h"
#include <memory.h>
#include <dlgs.h>       //Pring Dlg button IDs


/*
 * CPatronDoc::CPatronDoc
 * CPatronDoc::~CPatronDoc
 *
 * Constructor Parameters:
 *  hInst           HINSTANCE of the application.
 *  pFR             PCFrame of the frame object.
 *  pAdv            PCDocumentAdviseSink to notify on events.
 */

CPatronDoc::CPatronDoc(HINSTANCE hInst, PCFrame pFR
    , PCDocumentAdviseSink pAdv)
    : CDocument(hInst, pFR, pAdv)
    {
    m_pPG=NULL;
    m_lVer=VERSIONCURRENT;
    //CHAPTER7MOD
    m_pIStorage=NULL;
    //End CHAPTER7MOD
    return;
    }


CPatronDoc::~CPatronDoc(void)
    {
    if (NULL!=m_pPG)
        delete m_pPG;

    //CHAPTER7MOD
    ReleaseInterface(m_pIStorage);
    //End CHAPTER7MOD

    return;
    }





/*
 * CPatronDoc::Init
 *
 * Purpose:
 *  Initializes an already created document window.  The client
 *  actually creates the window for us, then passes that here for
 *  further initialization.
 *
 * Parameters:
 *  pDI             PDOCUMENTINIT containing initialization
 *                  parameters.
 *
 * Return Value:
 *  BOOL            TRUE if the function succeeded, FALSE otherwise.
 */

BOOL CPatronDoc::Init(PDOCUMENTINIT pDI)
    {
    //Change the stringtable range to our customization.
    pDI->idsMin=IDS_DOCUMENTMIN;
    pDI->idsMax=IDS_DOCUMENTMAX;

    //Do default initialization
    if (!CDocument::Init(pDI))
        return FALSE;

    //Pages are created when we get a Load later.
    return TRUE;
    }






/*
 * CPatronDoc::FMessageHook
 *
 * Purpose:
 *  Processes WM_SIZE for the document so we can resize the Pages
 *  window.
 *
 * Parameters:
 *  <WndProc Parameters>
 *  pLRes           LRESULT * in which to store the return
 *                  value for the message.
 *
 * Return Value:
 *  BOOL            TRUE to prevent further processing,
 *                  FALSE otherwise.
 */

BOOL CPatronDoc::FMessageHook(HWND hWnd, UINT iMsg, WPARAM wParam
    , LPARAM lParam, LRESULT *pLRes)
    {
    UINT        dx, dy;
    RECT        rc;

    *pLRes=0;

    //Eat to prevent flickering
    if (WM_ERASEBKGND==iMsg)
        return TRUE;

    if (WM_SIZE==iMsg && NULL!=m_pPG)
        {
        dx=LOWORD(lParam);
        dy=HIWORD(lParam);

        if (SIZE_MINIMIZED!=wParam)
            {
            //Resize Pages window to fit the new document size.
            GetClientRect(hWnd, &rc);
            m_pPG->RectSet(&rc, FALSE);
            }
        }

    /*
     * We return FALSE even on WM_SIZE so we can let the default
     * procedure handle maximized MDI child windows appropriately.
     */
    return FALSE;
    }





/*
 * CPatronDoc::Clear
 *
 * Purpose:
 *  Sets all contents in the document back to defaults with no
 *  filename.
 *
 * Paramters:
 *  None
 *
 * Return Value:
 *  None
 */

void CPatronDoc::Clear(void)
    {
    //Completely reset the pages
    //CHAPTER7MOD
    if (NULL!=m_pPG)
        m_pPG->StorageSet(NULL, FALSE, FALSE);
    //End CHAPTER7MOD

    CDocument::Clear();
    m_lVer=VERSIONCURRENT;
    return;
    }





/*
 * CPatronDoc::Load
 *
 * Purpose:
 *  Loads a given document without any user interface overwriting
 *  the previous contents of the editor.
 *
 * Parameters:
 *  fChangeFile     BOOL indicating if we're to update the window
 *                  title and the filename from using this file.
 *  pszFile         LPTSTR to the filename to load.  Could be NULL
 *                  for an untitled document.
 *
 * Return Value:
 *  UINT            An error value from DOCERR_*
 */

UINT CPatronDoc::Load(BOOL fChangeFile, LPTSTR pszFile)
    {
    RECT        rc;
    //CHAPTER7MOD
    LPSTORAGE   pIStorage;
    HRESULT     hr;
    CLSID       clsID;
    DWORD       dwMode=STGM_TRANSACTED | STGM_READWRITE
                    | STGM_SHARE_EXCLUSIVE;

    if (NULL==pszFile)
        {
        //Create a new temp file.
        hr=StgCreateDocfile(NULL, dwMode | STGM_CREATE
            | STGM_DELETEONRELEASE, 0, &pIStorage);

        //Mark this our class since we check with ReadClassStg.
        if (SUCCEEDED(hr))
            WriteClassStg(pIStorage, CLSID_PatronPages);
        }
    else
        {
        hr=StgOpenStorage(pszFile, NULL, dwMode, NULL, 0, &pIStorage);
        }

    if (FAILED(hr))
        return DOCERR_COULDNOTOPEN;

    //Check if this is our type of file and exit if not.
    hr=ReadClassStg(pIStorage, &clsID);

    if (FAILED(hr) || CLSID_PatronPages!=clsID)
        {
        pIStorage->Release();
        return DOCERR_READFAILURE;
        }

    //End CHAPTER7MOD

    //Attempt to create our contained Pages window.
    m_pPG=new CPages(m_hInst);
    GetClientRect(m_hWnd, &rc);

    //CHAPTER7MOD
    if (!m_pPG->Init(m_hWnd, &rc, WS_CHILD | WS_VISIBLE
        , ID_PAGES, NULL))
        {
        pIStorage->Release();
        return DOCERR_READFAILURE;
        }

    if (!m_pPG->StorageSet(pIStorage, FALSE, (NULL==pszFile)))
        {
        pIStorage->Release();
        return DOCERR_READFAILURE;
        }

    m_pIStorage=pIStorage;
    Rename(pszFile);

    //Do initial setup if new file, otherwise Pages handles things.
    if (NULL==pszFile)
        {
        //Go initialize the Pages for the default printer.
        if (!PrinterSetup(NULL, TRUE))
            return DOCERR_COULDNOTOPEN;

        //Go create an initial page.
        m_pPG->PageInsert(0);
        }
    //End CHAPTER7MOD

    FDirtySet(FALSE);
    return DOCERR_NONE;
    }






//CHAPTER7MOD
/*
 * CPatronDoc::Save
 *
 * Purpose:
 *  Writes the file to a known filename, requiring that the user
 *  has previously used FileOpen or FileSaveAs in order to have
 *  a filename.
 *
 * Parameters:
 *  uType           UINT indicating the type of file the user
 *                  requested to save in the File Save As dialog.
 *  pszFile         LPTSTR under which to save.  If NULL, use the
 *                  current name.
 *
 * Return Value:
 *  UINT            An error value from DOCERR_*
 */

UINT CPatronDoc::Save(UINT uType, LPTSTR pszFile)
    {
    HRESULT     hr;
    LPSTORAGE   pIStorage;

    //Save or Save As with the same file is just a commit.
    if (NULL==pszFile
        || (NULL!=pszFile && 0==lstrcmpi(pszFile, m_szFile)))
        {
        WriteFmtUserTypeStg(m_pIStorage, m_cf
            , PSZ(IDS_CLIPBOARDFORMAT));

        //Insure pages are up to date.
        m_pPG->StorageUpdate(FALSE);

        //Commit everyting
        m_pIStorage->Commit(STGC_DEFAULT);

        FDirtySet(FALSE);
        return DOCERR_NONE;
        }

    /*
     * When we're given a name, open the storage, creating it new
     * if it does not exist or overwriting the old one.  Then CopyTo
     * from the current to the new, Commit the new, Release the old.
     */

    hr=StgCreateDocfile(pszFile, STGM_TRANSACTED | STGM_READWRITE
        | STGM_CREATE | STGM_SHARE_EXCLUSIVE, 0, &pIStorage);

    if (FAILED(hr))
        return DOCERR_COULDNOTOPEN;

    WriteClassStg(pIStorage, CLSID_PatronPages);
    WriteFmtUserTypeStg(pIStorage, m_cf, PSZ(IDS_CLIPBOARDFORMAT));

    //Insure all pages are up-to-date.
    m_pPG->StorageUpdate(TRUE);

    //This also copies the CLSID we stuff in here on file creation.
    hr=m_pIStorage->CopyTo(NULL, NULL, NULL, pIStorage);

    if (FAILED(hr))
        {
        SCODE       sc;

        pIStorage->Release();
        sc=GetScode(hr);

        /*
         * If we failed because of low memory, use IRootStorage
         * to switch into the new file.
         */
        if (E_OUTOFMEMORY==sc)
            {
            LPROOTSTORAGE        pIRoot;

            //Delete file we already created
            DeleteFile(pszFile);

            if (FAILED(m_pIStorage->QueryInterface
                (IID_IRootStorage, (PPVOID)&pIRoot)))
                return DOCERR_WRITEFAILURE;

           #ifdef WIN32ANSI
            OLECHAR     szTemp[MAX_PATH];

            MultiByteToWideChar(CP_ACP, 0, pszFile, -1, szTemp, MAX_PATH);
            hr=pIRoot->SwitchToFile(szTemp);
           #else
            hr=pIRoot->SwitchToFile(pszFile);
           #endif
            pIRoot->Release();

            if (FAILED(hr))
                return DOCERR_WRITEFAILURE;

            //If successful, the Commit below finishes the save.
            pIStorage=m_pIStorage;
            m_pIStorage->AddRef();    //Matches Release below
            }
        }

    pIStorage->Commit(STGC_DEFAULT);

    /*
     * Revert changes on the original storage.  If this was a temp
     * file, it's deleted since we used STGM_DELETEONRELEASE.
     */
    m_pIStorage->Release();

    //Make this new storage current
    m_pIStorage=pIStorage;
    m_pPG->StorageSet(pIStorage, TRUE, FALSE);

    FDirtySet(FALSE);
    Rename(pszFile);    //Update caption bar.

    return DOCERR_NONE;
    }
//End CHAPTER7MOD






/*
 * CPatronDoc::NewPage
 *
 * Purpose:
 *  Creates a new page in the document's pages control after the
 *  current page.
 *
 * Parameters:
 *  None
 *
 * Return Value:
 *  UINT            Index of the new page.
 */

UINT CPatronDoc::NewPage(void)
    {
    FDirtySet(TRUE);
    return m_pPG->PageInsert(0);
    }







/*
 * CPatronDoc::DeletePage
 *
 * Purpose:
 *  Deletes the current page from the document.
 *
 * Parameters:
 *  None
 *
 * Return Value:
 *  UINT            Index of the now current page.
 */

UINT CPatronDoc::DeletePage(void)
    {
    FDirtySet(TRUE);
    return m_pPG->PageDelete(0);
    }







/*
 * CPatronDoc::NextPage
 *
 * Purpose:
 *  Shows the next page in the pages window.
 *
 * Parameters:
 *  None
 *
 * Return Value:
 *  UINT            Index of the new page.
 */

UINT CPatronDoc::NextPage(void)
    {
    UINT        iPage;

    iPage=m_pPG->CurPageGet();
    return m_pPG->CurPageSet(++iPage);
    }







/*
 * CPatronDoc::PreviousPage
 *
 * Purpose:
 *  Shows the previous page in the pages window.
 *
 * Parameters:
 *  None
 *
 * Return Value:
 *  UINT            Index of the new page.
 */

UINT CPatronDoc::PreviousPage(void)
    {
    UINT        iPage;

    //If iPage is zero, then we wrap around to the end.
    iPage=m_pPG->CurPageGet();
    return m_pPG->CurPageSet(--iPage);
    }






/*
 * CPatronDoc::FirstPage
 *
 * Purpose:
 *  Shows the first page page in the pages window.
 *
 * Parameters:
 *  None
 *
 * Return Value:
 *  UINT            Index of the new page.
 */

UINT CPatronDoc::FirstPage(void)
    {
    return m_pPG->CurPageSet(0);
    }






/*
 * CPatronDoc::LastPage
 *
 * Purpose:
 *  Shows the last page in the pages window.
 *
 * Parameters:
 *  None
 *
 * Return Value:
 *  UINT            Index of the last page.
 */

UINT CPatronDoc::LastPage(void)
    {
    return m_pPG->CurPageSet(NOVALUE);
    }
