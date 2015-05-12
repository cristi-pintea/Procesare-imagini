// dibview.cpp : implementation of the CDibView class
//
// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1998 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#include "stdafx.h"
#include "diblook.h"

#include <vector>
using std::vector;

#include "dibdoc.h"
#include "dibview.h"
#include "dibapi.h"
#include "mainfrm.h"

#include "HRTimer.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define BEGIN_PROCESSING() INCEPUT_PRELUCRARI()

#define END_PROCESSING(Title) SFARSIT_PRELUCRARI(Title)

#define INCEPUT_PRELUCRARI() \
	CDibDoc* pDocSrc=GetDocument();										\
	CDocTemplate* pDocTemplate=pDocSrc->GetDocTemplate();				\
	CDibDoc* pDocDest=(CDibDoc*) pDocTemplate->CreateNewDocument();		\
	BeginWaitCursor();													\
	HDIB hBmpSrc=pDocSrc->GetHDIB();									\
	HDIB hBmpDest = (HDIB)::CopyHandle((HGLOBAL)hBmpSrc);				\
	if ( hBmpDest==0 ) {												\
		pDocTemplate->RemoveDocument(pDocDest);							\
		return;															\
	}																	\
	BYTE* lpD = (BYTE*)::GlobalLock((HGLOBAL)hBmpDest);					\
	BYTE* lpS = (BYTE*)::GlobalLock((HGLOBAL)hBmpSrc);					\
	int iColors = DIBNumColors((char *)&(((LPBITMAPINFO)lpD)->bmiHeader)); \
	RGBQUAD *bmiColorsDst = ((LPBITMAPINFO)lpD)->bmiColors;	\
	RGBQUAD *bmiColorsSrc = ((LPBITMAPINFO)lpS)->bmiColors;	\
	BYTE * lpDst = (BYTE*)::FindDIBBits((LPSTR)lpD);	\
	BYTE * lpSrc = (BYTE*)::FindDIBBits((LPSTR)lpS);	\
	int dwWidth  = ::DIBWidth((LPSTR)lpS);\
	int dwHeight = ::DIBHeight((LPSTR)lpS);\
	int w=WIDTHBYTES(dwWidth*((LPBITMAPINFOHEADER)lpS)->biBitCount);	\
	HRTimer my_timer;	\
	my_timer.StartTimer();	\

#define BEGIN_SOURCE_PROCESSING \
	CDibDoc* pDocSrc=GetDocument();										\
	BeginWaitCursor();													\
	HDIB hBmpSrc=pDocSrc->GetHDIB();									\
	BYTE* lpS = (BYTE*)::GlobalLock((HGLOBAL)hBmpSrc);					\
	int iColors = DIBNumColors((char *)&(((LPBITMAPINFO)lpS)->bmiHeader)); \
	RGBQUAD *bmiColorsSrc = ((LPBITMAPINFO)lpS)->bmiColors;	\
	BYTE * lpSrc = (BYTE*)::FindDIBBits((LPSTR)lpS);	\
	int dwWidth  = ::DIBWidth((LPSTR)lpS);\
	int dwHeight = ::DIBHeight((LPSTR)lpS);\
	int w=WIDTHBYTES(dwWidth*((LPBITMAPINFOHEADER)lpS)->biBitCount);	\
	


#define END_SOURCE_PROCESSING	\
	::GlobalUnlock((HGLOBAL)hBmpSrc);								\
    EndWaitCursor();												\
/////////////////////////////////////////////////////////////////////////////


//---------------------------------------------------------------
#define SFARSIT_PRELUCRARI(Titlu)	\
	double elapsed_time_ms = my_timer.StopTimer();	\
	CString Title;	\
	Title.Format(_TEXT("%s - Proc. time = %.2f ms"), _TEXT(Titlu), elapsed_time_ms);	\
	::GlobalUnlock((HGLOBAL)hBmpDest);								\
	::GlobalUnlock((HGLOBAL)hBmpSrc);								\
    EndWaitCursor();												\
	pDocDest->SetHDIB(hBmpDest);									\
	pDocDest->InitDIBData();										\
	pDocDest->SetTitle((LPCTSTR)Title);									\
	CFrameWnd* pFrame=pDocTemplate->CreateNewFrame(pDocDest,NULL);	\
	pDocTemplate->InitialUpdateFrame(pFrame,pDocDest);	\

/////////////////////////////////////////////////////////////////////////////
// CDibView

IMPLEMENT_DYNCREATE(CDibView, CScrollView)

BEGIN_MESSAGE_MAP(CDibView, CScrollView)
	//{{AFX_MSG_MAP(CDibView)
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateEditCopy)
	ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, OnUpdateEditPaste)
	ON_MESSAGE(WM_DOREALIZE, OnDoRealize)
	ON_COMMAND(ID_PROCESSING_PARCURGERESIMPLA, OnProcessingParcurgereSimpla)
	//}}AFX_MSG_MAP

	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, CScrollView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CScrollView::OnFilePrintPreview)
	ON_COMMAND(ID_PROCESSING_CONVOLUTIE, &CDibView::OnProcessingConvolutie)
	ON_COMMAND(ID_PROCESSING_REDUCE, &CDibView::OnProcessingReduce)
	ON_COMMAND(ID_PROCESSING_DIFFERENCE, &CDibView::OnProcessingDifference)
	ON_COMMAND(ID_PROCESSING_DOWN, &CDibView::OnProcessingDown)
	ON_COMMAND(ID_PROCESSING_REDUCERESUCCESIVA, &CDibView::OnProcessingReduceresuccesiva)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDibView construction/destruction

CDibView::CDibView()
{
}

CDibView::~CDibView()
{
}

/////////////////////////////////////////////////////////////////////////////
// CDibView drawing

void CDibView::OnDraw(CDC* pDC)
{
	CDibDoc* pDoc = GetDocument();

	HDIB hDIB = pDoc->GetHDIB();
	if (hDIB != NULL)
	{
		LPSTR lpDIB = (LPSTR) ::GlobalLock((HGLOBAL) hDIB);
		int cxDIB = (int) ::DIBWidth(lpDIB);         // Size of DIB - x
		int cyDIB = (int) ::DIBHeight(lpDIB);        // Size of DIB - y
		::GlobalUnlock((HGLOBAL) hDIB);
		CRect rcDIB;
		rcDIB.top = rcDIB.left = 0;
		rcDIB.right = cxDIB;
		rcDIB.bottom = cyDIB;
		CRect rcDest;
		if (pDC->IsPrinting())   // printer DC
		{
			// get size of printer page (in pixels)
			int cxPage = pDC->GetDeviceCaps(HORZRES);
			int cyPage = pDC->GetDeviceCaps(VERTRES);
			// get printer pixels per inch
			int cxInch = pDC->GetDeviceCaps(LOGPIXELSX);
			int cyInch = pDC->GetDeviceCaps(LOGPIXELSY);

			//
			// Best Fit case -- create a rectangle which preserves
			// the DIB's aspect ratio, and fills the page horizontally.
			//
			// The formula in the "->bottom" field below calculates the Y
			// position of the printed bitmap, based on the size of the
			// bitmap, the width of the page, and the relative size of
			// a printed pixel (cyInch / cxInch).
			//
			rcDest.top = rcDest.left = 0;
			rcDest.bottom = (int)(((double)cyDIB * cxPage * cyInch)
					/ ((double)cxDIB * cxInch));
			rcDest.right = cxPage;
		}
		else   // not printer DC
		{
			rcDest = rcDIB;
		}
		::PaintDIB(pDC->m_hDC, &rcDest, pDoc->GetHDIB(),
			&rcDIB, pDoc->GetDocPalette());
	}
}

/////////////////////////////////////////////////////////////////////////////
// CDibView printing

BOOL CDibView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

/////////////////////////////////////////////////////////////////////////////
// CDibView commands


LRESULT CDibView::OnDoRealize(WPARAM wParam, LPARAM)
{
	ASSERT(wParam != NULL);
	CDibDoc* pDoc = GetDocument();
	if (pDoc->GetHDIB() == NULL)
		return 0L;  // must be a new document

	CPalette* pPal = pDoc->GetDocPalette();
	if (pPal != NULL)
	{
		CMainFrame* pAppFrame = (CMainFrame*) AfxGetApp()->m_pMainWnd;
		ASSERT_KINDOF(CMainFrame, pAppFrame);

		CClientDC appDC(pAppFrame);
		// All views but one should be a background palette.
		// wParam contains a handle to the active view, so the SelectPalette
		// bForceBackground flag is FALSE only if wParam == m_hWnd (this view)
		CPalette* oldPalette = appDC.SelectPalette(pPal, ((HWND)wParam) != m_hWnd);

		if (oldPalette != NULL)
		{
			UINT nColorsChanged = appDC.RealizePalette();
			if (nColorsChanged > 0)
				pDoc->UpdateAllViews(NULL);
			appDC.SelectPalette(oldPalette, TRUE);
		}
		else
		{
			TRACE0("\tSelectPalette failed in CDibView::OnPaletteChanged\n");
		}
	}

	return 0L;
}

void CDibView::OnInitialUpdate()
{
	CScrollView::OnInitialUpdate();
	ASSERT(GetDocument() != NULL);

	SetScrollSizes(MM_TEXT, GetDocument()->GetDocSize());
}


void CDibView::OnActivateView(BOOL bActivate, CView* pActivateView,
					CView* pDeactiveView)
{
	CScrollView::OnActivateView(bActivate, pActivateView, pDeactiveView);

	if (bActivate)
	{
		ASSERT(pActivateView == this);
		OnDoRealize((WPARAM)m_hWnd, 0);   // same as SendMessage(WM_DOREALIZE);
	}
}

void CDibView::OnEditCopy()
{
	CDibDoc* pDoc = GetDocument();
	// Clean clipboard of contents, and copy the DIB.

	if (OpenClipboard())
	{
		BeginWaitCursor();
		EmptyClipboard();
		SetClipboardData (CF_DIB, CopyHandle((HANDLE) pDoc->GetHDIB()) );
		CloseClipboard();
		EndWaitCursor();
	}
}



void CDibView::OnUpdateEditCopy(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(GetDocument()->GetHDIB() != NULL);
}


void CDibView::OnEditPaste()
{
	HDIB hNewDIB = NULL;

	if (OpenClipboard())
	{
		BeginWaitCursor();

		hNewDIB = (HDIB) CopyHandle(::GetClipboardData(CF_DIB));

		CloseClipboard();

		if (hNewDIB != NULL)
		{
			CDibDoc* pDoc = GetDocument();
			pDoc->ReplaceHDIB(hNewDIB); // and free the old DIB
			pDoc->InitDIBData();    // set up new size & palette
			pDoc->SetModifiedFlag(TRUE);

			SetScrollSizes(MM_TEXT, pDoc->GetDocSize());
			OnDoRealize((WPARAM)m_hWnd,0);  // realize the new palette
			pDoc->UpdateAllViews(NULL);
		}
		EndWaitCursor();
	}
}


void CDibView::OnUpdateEditPaste(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(::IsClipboardFormatAvailable(CF_DIB));
}

void CDibView::OnProcessingParcurgereSimpla() 
{
	// TODO: Add your command handler code here
	BEGIN_PROCESSING();

	// Makes a grayscale image by equalizing the R, G, B components from the LUT
	for (int k=0;  k < iColors ; k++)
		bmiColorsDst[k].rgbRed=bmiColorsDst[k].rgbGreen=bmiColorsDst[k].rgbBlue=k;

	//  Goes through the bitmap pixels and performs their negative	
	for (int i=0;i<dwHeight;i++)
		for (int j=0;j<dwWidth;j++)
		  {	
			lpDst[i*w+j]= 255 - lpSrc[i*w+j]; //makes image negative
	  }

	END_PROCESSING("Negativ imagine");
}



int *differenceMatrix;
void CDibView::OnProcessingConvolutie()
{
	BEGIN_PROCESSING();

	differenceMatrix = (int*)malloc(dwHeight*dwWidth*sizeof(int));
	int nucleu[5][5] = { { 1, 4, 6, 4, 1 }, { 4, 16, 24, 16, 4 }, { 6, 24, 36, 24, 6 }, { 4, 16, 24, 16, 4 }, { 1, 4, 6, 4, 1 } };
	for (int i = 2; i<dwHeight - 2; i++)
		for (int j = 2; j < dwWidth - 2; j++)
		{
			int pixel = lpSrc[i*w + j];
			int value = 0;
			for (int k = 0; k < 5; k++)
				for (int l = 0; l < 5; l++)
				{
					value += nucleu[k][l] * lpSrc[(i + k - 2)*w + (j + l - 2)];
				}
			lpDst[i*w + j] = value / 256;
			if (lpDst[i*w + j] - lpSrc[i*w + j] < 0)
			{
				differenceMatrix[i*w + j] = 0;
			}
			else
			{
				differenceMatrix[i*w + j] = lpDst[i*w + j] - lpSrc[i*w + j];
			}
		}
	END_PROCESSING("Convolutie");

}


void CDibView::OnProcessingReduce()
{
	BEGIN_PROCESSING();
	int height,width;
	int *temp = (int *)malloc(dwHeight*dwWidth*(sizeof(int)));
	int i, j, count = 0;
	CString info;
	
	for (i = 0; i < dwHeight; i++)
		for (j = 0; j < dwWidth; j++)
			lpDst[i*w + j] = 255;

	for (i = 0; i < dwHeight; i++)
		for (j = 0; j < dwWidth; j++)
			if (i % 2 == 0 && j % 2 == 0)
			{
				temp[count] = lpSrc[i*w + j];
				count++;
			}
	height = dwHeight / 2;
	width = dwWidth / 2;

	/*info.Format(_TEXT("%d,%d"), height, width);
	AfxMessageBox(info);*/

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			lpDst[i*w + j] = temp[i*width + j];


	END_PROCESSING("REDUCE");

}




void CDibView::OnProcessingDifference()
{
	BEGIN_PROCESSING();
	int nucleu[5][5] = { { 1, 4, 6, 4, 1 }, { 4, 16, 24, 16, 4 }, { 6, 24, 36, 24, 6 }, { 4, 16, 24, 16, 4 }, { 1, 4, 6, 4, 1 } };
	int *convoluteMatrix = (int*)malloc(dwHeight*dwWidth*sizeof(int));
	for (int i = 0; i<dwHeight - 0; i++)
		for (int j = 0; j < dwWidth - 0; j++)
		{
			int pixel = lpSrc[i*w + j];
			int value = 0;
			for (int k = 0; k < 5; k++)
				for (int l = 0; l < 5; l++)
				{
					value += nucleu[k][l] * lpSrc[(i + k - 2)*w + (j + l - 2)];
				}
			convoluteMatrix[i*w + j] = value / 256;
			if (convoluteMatrix[i*w + j] - lpSrc[i*w + j] < 0)
			{
				lpDst[i*w + j] = 0;
			}
			else
			{
				lpDst[i*w + j] = convoluteMatrix[i*w + j] - lpSrc[i*w + j];
			}
		}
	END_PROCESSING("Band-Pass");
}

int *intermediateMatrix;
int NR_OF_TIMES = 8;
int matrixHeight;
int matrixWidth;
void CDibView::OnProcessingDown()
{
	BEGIN_PROCESSING();
	intermediateMatrix = (int*)malloc(dwHeight*dwWidth*sizeof(int));
	int nucleu[5][5] = { { 1, 4, 6, 4, 1 }, { 4, 16, 24, 16, 4 }, { 6, 24, 36, 24, 6 }, { 4, 16, 24, 16, 4 }, { 1, 4, 6, 4, 1 } };

	//initializari
	matrixHeight = dwHeight;
	matrixWidth = dwWidth;
	vector<vector<vector<double> > > matrixes;

	matrixes.resize(NR_OF_TIMES);
	for (int i = 0; i < 7; i++)
	{
		matrixes[i].resize(matrixHeight);

		for (int j = 0; j < dwHeight; j++)
		{
			matrixes[i][j].resize(matrixWidth);
		}
	}


	for (int cnt = 0; cnt < 7; cnt++)
	{
	for (int i = 2; i<dwHeight - 2; i++)
		for (int j = 2; j < dwWidth - 2; j++)
		{
			int pixel = lpSrc[i*w + j];
			int value = 0;
			for (int k = 0; k < 5; k++)
				for (int l = 0; l < 5; l++)
				{
					if (cnt == 0)
					{
						value += nucleu[k][l] * lpSrc[(i + k - 2)*w + (j + l - 2)];
					}
					else
					{
						value += nucleu[k][l] * matrixes[cnt-1][i + k - 2][(j + l - 2)];
					}
				}
			matrixes[cnt][i][j] = value / 256;
		}

	}
	for (int i = 0; i < dwHeight; i++)
		for (int j = 0; j < dwWidth; j++)
		{
			lpDst[i*w + j] = matrixes[7][i][j];
		}
	END_PROCESSING("Down-Sampling");

}

int NR_OF_TIMES1 = 8;

void CDibView::OnProcessingReduceresuccesiva()
{
	CString info;
	BEGIN_PROCESSING();
	int init = 0;
	int height, width;
	int *temp = (int *)malloc(dwHeight*dwWidth*(sizeof(int)));
	int i, j, count = 0;
	
	vector<vector<double>> imagini;
	imagini.resize(NR_OF_TIMES1);

	//alocam spatiu pentru toate cle 8 imagini
	for (i = 0; i < 8; i++)
	{
		imagini[i].resize(dwHeight * dwWidth);
	}

	for (i = 0; i < dwHeight; i++)
		for (j = 0; j < dwWidth; j++)
			imagini[0][i*w + j] = lpSrc[i*w + j];

	
	height = dwHeight;
	width = dwWidth;

	for (int nr = 1; nr < 5; nr++)
	{
		count = 0;

		for (i = 0; i < dwHeight; i++)
			for (j = 0; j < dwWidth; j++)
				imagini[nr][i*w + j] = 255;

		for (i = 0; i < dwHeight; i++)
			for (j = 0; j < dwWidth; j++)
				if (i % 2 == 0 && j % 2 == 0)
				{
					imagini[nr][count] = imagini[nr - 1][i*w + j];
					count++;
				}
		height = height / 2;
		width = width / 2;
	}

	/*info.Format(_TEXT("%d,%d"), height, width);
	AfxMessageBox(info);*/

	for (i = 0; i < dwHeight; i++)
		for (j = 0; j < dwWidth; j++)
			lpDst[i*w + j] = 255;

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			lpDst[i*w + j] = imagini[4][i*width + j];

	END_PROCESSING("Micsorare");
}
