
// DocannerDlg.cpp: 구현 파일
//

#include "pch.h"
#include "framework.h"
#include "Docanner.h"
#include "DocannerDlg.h"
#include "afxdialogex.h"
#include <atlimage.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CString ExtractFileName(const CString& fullPath)
{
	int lastBackslashIndex = fullPath.ReverseFind('\\');
	if (lastBackslashIndex != -1)
	{
		return fullPath.Right(fullPath.GetLength() - lastBackslashIndex - 1);
	}

	return fullPath; // 역슬래시가 없는 경우 전체 문자열을 반환합니다.
}

bool SaveImageAsPNG(const CString& filePath, const CImage& image)
{
	// PNG 파일로 저장
	if (SUCCEEDED(image.Save(filePath, Gdiplus::ImageFormatPNG)))
	{
		return true;
	}

	return false;
}

bool SaveCImageAsPNG(const CString& filePath, const CImage& image)
{
	// CImage를 32비트 ARGB 형식으로 변환
	CImage convertedImage;
	if (!convertedImage.Create(image.GetWidth(), image.GetHeight(), 32))
	{
		return false;
	}
	image.AlphaBlend(convertedImage.GetDC(), 0, 0, image.GetWidth(), image.GetHeight(), 0, 0, image.GetWidth(), image.GetHeight(), 255);

	return SaveImageAsPNG(filePath, convertedImage);
}

// Mat 객체를 CImage 객체로 변환하는 함수
void CDocannerDlg::MatToCImage(const Mat& mat, CImage& image)
{
	// Mat 객체에서 이미지 데이터 가져오기
	int w = mat.cols;
	int h = mat.rows;
	int channels = mat.channels();
	int stride = w * channels;
	Mat temp(h, w, CV_8UC3);
	if (channels == 1)
	{
		cvtColor(mat, temp, COLOR_GRAY2BGR);
	}
	else if (channels == 3)
	{
		cvtColor(mat, temp, COLOR_BGR2RGB);
	}
	else if (channels == 4)
	{
		cvtColor(mat, temp, COLOR_BGRA2RGB);
	}
	else
	{
		// 예외 처리: 지원하지 않는 채널 수
		throw invalid_argument("Unsupported number of channels");
	}

	// CImage 객체에 이미지 데이터 설정
	image.Create(w, h, 24);
	BYTE* dstData = (BYTE*)image.GetBits();
	BYTE* srcData = (BYTE*)temp.data;
	int dstStride = image.GetPitch();
	int srcStride = temp.step;
	for (int y = 0; y < h; y++)
	{
		memcpy(dstData + y * dstStride, srcData + y * srcStride, stride);
	}
}

void CDocannerDlg::cimageDraw(CImage& image, CStatic& cstatic)
{
	CRect rect;//픽쳐 컨트롤의 크기를 저장할 CRect 객체
	cstatic.GetWindowRect(rect);//GetWindowRect를 사용해서 픽쳐 컨트롤의 크기를 받는다.
	CDC* dc; //픽쳐 컨트롤의 DC를 가져올  CDC 포인터
	dc = cstatic.GetDC(); //픽쳐 컨트롤의 DC를 얻는다.

	image.StretchBlt(dc->m_hDC, 0, 0, rect.Width(), rect.Height(), SRCCOPY);//이미지를 픽쳐 컨트롤 크기로 조정
	ReleaseDC(dc);//DC 해제
}

bool CDocannerDlg::getPath(CString& cstring)
{
	CString str = _T("All files(*.*)|*.*|"); // 모든 파일 표시
	// _T("Excel 파일 (*.xls, *.xlsx) |*.xls; *.xlsx|"); 와 같이 확장자를 제한하여 표시할 수 있음
	CFileDialog dlg(TRUE, _T("*.dat"), NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, str, this);

	if (dlg.DoModal() == IDOK)
	{
		cstring = dlg.GetPathName();
		// 파일 경로를 가져와 사용할 경우, Edit Control에 값 저장
		return true;
	}
	return false;
}

bool CDocannerDlg::addImg(int& index)
{
	if (getPath(fileName))
	{
		string path = string(CT2CA(fileName));

		images[index].img = imread(path);

		MatToCImage(images[index].img, images[index].originImg);

		images[index].draw = images[index].img.clone();

		index++;
		return true;
	}
	return false;
}

bool CDocannerDlg::convert(const int& index)
{
	if (!images[index].converted)
	{
		// Gray 스케일 변환 및 캐니 엣지 검출
		Mat gray, edged;
		cvtColor(images[index].img, gray, COLOR_BGR2GRAY);
		GaussianBlur(gray, gray, Size(3, 3), 0);
		Canny(gray, edged, 75, 200);
		/*imshow(winName, edged);
		waitKey(0);*/

		// 컨투어 찾기
		vector<vector<Point>> cnts;
		vector<Vec4i> hierarchy;
		findContours(edged.clone(), cnts, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

		cout << cnts.size() << endl;
		drawContours(images[index].draw, cnts, -1, Scalar(0, 255, 0), 3);
		/*imshow(winName, draw);
		waitKey(0);*/

		// 컨투어들 중에 영역 크기순으로 정렬
		sort(cnts.begin(), cnts.end(), [](const vector<Point>& c1, const vector<Point>& c2) {
			return contourArea(c1) > contourArea(c2);
			});

		// 영역이 가장 큰 컨투어부터 근사 컨투어 단순화
		vector<Point> verticles;
		for (const auto& c : cnts) {
			double peri = arcLength(c, true);
			approxPolyDP(c, verticles, 0.08 * peri, true);
			if (verticles.size() == 4) { // 근사한 꼭짓점이 4개면 중지
				break;
			}
		}

		// N * 1 * 2 배열을 4 * 2 크기로 조정
		vector<Point2f> pts(4);
		for (int i = 0; i < 4; ++i) {
			pts[i] = verticles[i];
			circle(images[index].draw, pts[i], 25, Scalar(0, 255, 0), -1);
		}
		/*imshow(winName, draw);
		waitKey(0);*/

		// 좌표 4개 중 상하좌우 좌표 찾기
		Point2f topLeft = pts[0];
		Point2f topRight = pts[1];
		Point2f bottomRight = pts[2];
		Point2f bottomLeft = pts[3];

		// 변환 전 4개의 좌표
		Point2f pts1[4] = { topLeft, topRight, bottomRight, bottomLeft };

		// 변환 후 영상에 사용할 서류의 폭과 높이
		float w1 = abs(bottomRight.x - bottomLeft.x);
		float w2 = abs(topRight.x - topLeft.x);
		float h1 = abs(topRight.y - bottomRight.y);
		float h2 = abs(topLeft.y - bottomLeft.y);
		float width = max(w1, w2);
		float height = max(h1, h2);

		Point2f pts2[4] = { Point2f(width - 1, 0), Point2f(0, 0), Point2f(0, height - 1), Point2f(width - 1, height - 1) };

		Mat mtrx = getPerspectiveTransform(pts1, pts2);
		Mat result;
		warpPerspective(images[index].img, result, mtrx, Size(width, height)); // 원근 변환 적용

		MatToCImage(result, images[index].convertedImg);

		/*imshow("result", result);
		waitKey(0);
		destroyAllWindows();*/

		images[index].converted = true;
		return true;
	}

	return false;
}

// 응용 프로그램 정보에 사용되는 CAboutDlg 대화 상자입니다.

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

// 구현입니다.
protected:
	DECLARE_MESSAGE_MAP()
public:
//	afx_msg void OnPaint();
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_DOCANNER_DIALOG)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
//	ON_WM_PAINT()
END_MESSAGE_MAP()


// CDocannerDlg 대화 상자



CDocannerDlg::CDocannerDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DOCANNER_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CDocannerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST, m_list);
	DDX_Control(pDX, IDC_AFTER, m_after);
	DDX_Control(pDX, IDC_PICTURE, m_before);
}

BEGIN_MESSAGE_MAP(CDocannerDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_DESTROY()
	ON_WM_CREATE()
	ON_BN_CLICKED(IDC_CONVERT, &CDocannerDlg::OnBnClickedConvert)
	ON_BN_CLICKED(IDC_ADD, &CDocannerDlg::OnBnClickedAdd)
	ON_BN_CLICKED(IDC_LOAD, &CDocannerDlg::OnBnClickedLoad)
	ON_BN_CLICKED(IDC_CONVERT_PDF, &CDocannerDlg::OnBnClickedConvertPdf)
END_MESSAGE_MAP()


// CDocannerDlg 메시지 처리기

BOOL CDocannerDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 시스템 메뉴에 "정보..." 메뉴 항목을 추가합니다.

	// IDM_ABOUTBOX는 시스템 명령 범위에 있어야 합니다.
	// ASSERT((IDD_DOCANNER_DIALOG & 0xFFF0) == IDD_DOCANNER_DIALOG);
	ASSERT(IDD_DOCANNER_DIALOG < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDD_DOCANNER_DIALOG);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDD_DOCANNER_DIALOG, strAboutMenu);
		}
	}

	// 이 대화 상자의 아이콘을 설정합니다.  응용 프로그램의 주 창이 대화 상자가 아닐 경우에는
	//  프레임워크가 이 작업을 자동으로 수행합니다.
	SetIcon(m_hIcon, TRUE);			// 큰 아이콘을 설정합니다.
	SetIcon(m_hIcon, FALSE);		// 작은 아이콘을 설정합니다.

	// TODO: 여기에 추가 초기화 작업을 추가합니다.

	imagesCount = 0;

	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}

void CDocannerDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDD_DOCANNER_DIALOG)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 대화 상자에 최소화 단추를 추가할 경우 아이콘을 그리려면
//  아래 코드가 필요합니다.  문서/뷰 모델을 사용하는 MFC 애플리케이션의 경우에는
//  프레임워크에서 이 작업을 자동으로 수행합니다.

void CDocannerDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 그리기를 위한 디바이스 컨텍스트입니다.

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 클라이언트 사각형에서 아이콘을 가운데에 맞춥니다.
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 아이콘을 그립니다.
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// 사용자가 최소화된 창을 끄는 동안에 커서가 표시되도록 시스템에서
//  이 함수를 호출합니다.
HCURSOR CDocannerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CDocannerDlg::OnBnClickedConvert()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	if (selectIndex >= 0 && !images[selectIndex].converted && convert(selectIndex))
	{
		CRect rect;//픽쳐 컨트롤의 크기를 저장할 CRect 객체
		m_after.GetWindowRect(rect);//GetWindowRect를 사용해서 픽쳐 컨트롤의 크기를 받는다.
		CDC* dc; //픽쳐 컨트롤의 DC를 가져올  CDC 포인터
		dc = m_after.GetDC(); //픽쳐 컨트롤의 DC를 얻는다.

		images[selectIndex].convertedImg.StretchBlt(dc->m_hDC, 0, 0, rect.Width(), rect.Height(), SRCCOPY);//이미지를 픽쳐 컨트롤 크기로 조정
		ReleaseDC(dc);//DC 해제
	}
}


void CDocannerDlg::OnBnClickedAdd()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	addImg(imagesCount);
	m_list.AddString(ExtractFileName(fileName));
	images[imagesCount - 1].name = ExtractFileName(fileName);
}


void CDocannerDlg::OnBnClickedLoad()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	int index = m_list.GetCurSel();

	Invalidate();
	UpdateWindow();

	if (index >= 0)
	{
		selectIndex = index;

		CRect rect;//픽쳐 컨트롤의 크기를 저장할 CRect 객체
		m_before.GetWindowRect(rect);//GetWindowRect를 사용해서 픽쳐 컨트롤의 크기를 받는다.
		CDC* dc; //픽쳐 컨트롤의 DC를 가져올  CDC 포인터
		dc = m_before.GetDC(); //픽쳐 컨트롤의 DC를 얻는다.

		images[index].originImg.StretchBlt(dc->m_hDC, 0, 0, rect.Width(), rect.Height(), SRCCOPY);//이미지를 픽쳐 컨트롤 크기로 조정
		ReleaseDC(dc);//DC 해제

		if (images[index].converted)
		{
			CRect rect;//픽쳐 컨트롤의 크기를 저장할 CRect 객체
			m_after.GetWindowRect(rect);//GetWindowRect를 사용해서 픽쳐 컨트롤의 크기를 받는다.
			CDC* dc; //픽쳐 컨트롤의 DC를 가져올  CDC 포인터
			dc = m_after.GetDC(); //픽쳐 컨트롤의 DC를 얻는다.

			images[index].convertedImg.StretchBlt(dc->m_hDC, 0, 0, rect.Width(), rect.Height(), SRCCOPY);//이미지를 픽쳐 컨트롤 크기로 조정
			ReleaseDC(dc);//DC 해제
		}
	}
}


void CDocannerDlg::OnBnClickedConvertPdf()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	CString dir;
	dir.Format(L"convertedImg/converted_%s", images[selectIndex].name);
	SaveImageAsPNG(dir, images[selectIndex].convertedImg);
	AfxMessageBox(_T("PNG file saved successfully!"), MB_OK | MB_ICONINFORMATION);
}
