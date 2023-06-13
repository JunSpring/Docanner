
// DocannerDlg.h: 헤더 파일
//
#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>
#include <vector>

using namespace cv;
using namespace std;


#pragma once

class Image
{
public:
	CImage originImg;
	CImage convertedImg;

	CString name;

	bool converted;

	Mat img;
	Mat draw;

	Image()
	{
		converted = false;
	}
};


// CDocannerDlg 대화 상자
class CDocannerDlg : public CDialogEx
{
// 생성입니다.
public:
	CDocannerDlg(CWnd* pParent = nullptr);	// 표준 생성자입니다.

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DOCANNER_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다.


// 구현입니다.
protected:
	HICON m_hIcon;
	

	// 생성된 메시지 맵 함수
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:

// 변수
public:
	Image images[100];

	int selectIndex = -1;
	int imagesCount;

	CString fileName;
	//CImage cresult;

// 함수
public:
	void MatToCImage(const Mat& mat, CImage& image);
	void cimageDraw(CImage& image, CStatic& cstatic);
	bool getPath(CString& cstring);
	bool addImg(int& index);
	bool convert(const int& index);
	CListBox m_list;
	CStatic m_after;
	CStatic m_before;
	afx_msg void OnBnClickedConvert();
	afx_msg void OnBnClickedAdd();
	afx_msg void OnBnClickedLoad();
	afx_msg void OnBnClickedConvertPdf();
};
