#include "stdafx.h"
#include "Common.h"

#include "MDS_E-bus_SampleDlg.h"

Common* Common::instance = nullptr;

Common::Common()
{
	m_bBufferCheck = false;
	m_bPointerCheck = false;
	m_bMarkerCheck = false;

	m_logHandle = NULL;
}

Common::~Common()
{
	if (instance != nullptr)
	{
		delete instance;
		instance = nullptr;
	}
}

// =============================================================================
CString Common::str2CString(const char* _str)
{
	CString str2;
	str2 = (CString)_str;

	return str2;

}

// =============================================================================
std::string Common::CString2str(CString _str)
{

	// Convert a TCHAR string to a LPCSTR
	CT2CA pszConvertedAnsiString(_str);

	// Construct a std::string using the LPCSTR input
	std::string s(pszConvertedAnsiString);

	return s;
}

// =============================================================================
// 파일 검색
void Common::GetFileList(CString path, vector<CString>& strArray)
{

	//검색 클래스
	CFileFind finder;

	//CFileFind는 파일, 디렉터리가 존재하면 TRUE 를 반환함
	BOOL bWorking = finder.FindFile(path); //

	CString fileName;
	CString DirName;

	while (bWorking)
	{
		//다음 파일 / 폴더 가 존재하면다면 TRUE 반환
		bWorking = finder.FindNextFile();
		//파일 일때
		if (finder.IsArchived())
		{
			//파일의 이름
			CString _fileName = finder.GetFileName();

			// 현재폴더 상위폴더 썸네일파일은 제외
			if (_fileName == _T(".") ||
				_fileName == _T("..") ||
				_fileName == _T("Thumbs.db")) continue;

			fileName = finder.GetFileTitle();
			strArray.push_back(fileName);

		}
	}
}

// =============================================================================
// 로그 핸들러 생성
void Common::CreateLog(CListBox* handle)
{
	bool bFlag = false;
	m_logHandle = handle;
	//log 생성

	TCHAR szPath[_MAX_PATH];
	GetModuleFileName(AfxGetApp()->m_hInstance, szPath, _MAX_PATH);
	CString strProgramDir = szPath;
	CFileStatus mStatus;
	int nLen = strProgramDir.ReverseFind('\\');
	if (nLen > 0)
		strProgramDir = strProgramDir.Left(nLen);

	CString path;
	path.Format(_T("%s\\SystemLog\\"), (LPCTSTR)strProgramDir);

	if (!PathIsDirectory(path))
	{
		SHCreateDirectoryEx(NULL, path, NULL);
	}

	CString sTime;
	CTime Time = CTime::GetCurrentTime();
	sTime.Format(_T("[System_log] %.4d_%0.2d_%.2d.txt"), Time.GetYear(), Time.GetMonth(), Time.GetDay());


	path += sTime;

	if (m_fWriteFile.Open(path, CFile::modeCreate | CFile::modeNoTruncate | CFile::modeWrite | CFile::shareDenyNone))
	{
		bFlag = InitializeCriticalSectionAndSpinCount(&m_csLock,2000);
		AddLog(1, _T("Create Log"));
	}

	AddLog(1, _T("Log System initialize start"));

}

// =============================================================================
void Common::ClearLog()
{
	m_logHandle->ResetContent();
}

// =============================================================================
void Common::CreateHorizontalScroll()
{
	CString str; CSize sz; int dx = 0;
	CDC* pDC = m_logHandle->GetDC();

	for (int i = 0; i < m_logHandle->GetCount(); i++)
	{
		m_logHandle->GetText(i, str);
		sz = pDC->GetTextExtent(str);

		if (sz.cx > dx)
			dx = sz.cx;
	}
	m_logHandle->ReleaseDC(pDC);

	if (m_logHandle->GetHorizontalExtent() < dx)
	{
		m_logHandle->SetHorizontalExtent(dx);
	}
}

// =============================================================================
void Common::AddLog(int verbosity, LPCTSTR lpszFormat, ...)
{
	SYSTEMTIME cur_time; // 현재 시간 정보를 저장할 구조체
	CString strDate; // 시간 정보를 포맷팅하여 저장할 문자열

	GetLocalTime(&cur_time); // 현재 시간 정보를 가져옴
	strDate.Format(_T("%02d:%02d:%02d:%03ld"), cur_time.wHour, cur_time.wMinute, cur_time.wSecond, cur_time.wMilliseconds);

	if (m_csLock.LockCount > -1) // 잠금 횟수를 확인하여 잠겨있지 않은 경우 반환
		return;

	CString logline; // 로그를 저장할 문자열

	// 로그 라인에서 불필요한 문자열 교체
	logline.Replace(_T(" \""), _T("."));

	EnterCriticalSection(&m_csLock); // 임계 영역 진입

	CString strWriteData; // 로그를 파일에 기록하기 위한 문자열 생성
	strWriteData.Format(_T("[%s]%s"), (LPCTSTR)strDate, lpszFormat); // 시간과 포맷 문자열을 합쳐서 로그 문자열 생성

	if (m_fWriteFile.m_hFile != CFile::hFileNull) // 파일 핸들이 유효한 경우
	{
		m_fWriteFile.SeekToEnd(); // 파일 끝으로 이동
		strWriteData = strWriteData + _T("\r\n"); // 로그 문자열에 개행 문자 추가
		m_fWriteFile.Write(strWriteData, strWriteData.GetLength() * 2); // 로그 문자열을 파일에 쓰기
	}
	m_fWriteFile.Flush(); // 파일 쓰기 버퍼 비우기

	LPTSTR  lpBuffer = strWriteData.GetBuffer(); // 문자열을 TCHAR 형태로 변환하여 버퍼 얻음
	strWriteData.ReleaseBuffer();

	HWND listHWnd = ::GetDlgItem(AfxGetMainWnd()->m_hWnd, IDC_LIST_LOG); // 리스트 박스의 핸들을 얻음

	Sleep(1); // 잠시 대기

	SendMessage(listHWnd, LB_ADDSTRING, 0, (LPARAM)lpBuffer); // 리스트 박스에 문자열 추가
	m_logHandle->SetTopIndex(m_logHandle->GetCount() - 1); // 리스트 박스 스크롤을 가장 아래로 이동
	CreateHorizontalScroll(); // 가로 스크롤 생성

	LeaveCriticalSection(&m_csLock); // 임계 영역 빠져나옴
}

// =============================================================================
void Common::CreateFolder(CString csPath)
{
	// 지정된 경로에 폴더를 생성하는 함수입니다.
	// 슬래시('/')로 구분된 경로를 받아서 각 경로 토큰을 추출하고 폴더를 생성합니다.

	CString csPrefix(_T("")), csToken(_T(""));
	int nStart = 0, nEnd;
	while ((nEnd = csPath.Find('/', nStart)) >= 0)
	{
		// 슬래시로 구분된 경로에서 토큰 추출
		CString csToken = csPath.Mid(nStart, nEnd - nStart);
		CreateDirectory(csPrefix + csToken, NULL);

		csPrefix += csToken;
		csPrefix += _T("/");
		nStart = nEnd + 1;
	}
	csToken = csPath.Mid(nStart);
	CreateDirectory(csPrefix + csToken, NULL);
}

// =============================================================================
CString Common::ChangeBool(bool bValue)
{
	return (bValue ? _T("1") : _T("0"));
}

// =============================================================================
CString Common::SetProgramPath(CString _str)
{
	// 프로그램 실행 경로에 지정된 문자열을 연결하는 함수.
	TCHAR path1[_MAX_DIR];
	GetModuleFileName(NULL, path1, sizeof path1);

	CString strPath = path1;
	CString rootpath, path, Lfilepath, Rfilepath;
	int i = strPath.ReverseFind('\\');
	strPath = strPath.Left(i);
	CString str;
	str.Format(_T("%s\\%s"), (LPCTSTR)strPath, _str);
	return str;
}

// =============================================================================
void Common::Setsetingfilepath(CString _Path)
{
	// 설정 파일 경로를 설정하는 함수.
	m_SettingFilePath = _Path;
}

// =============================================================================
CString Common::Getsetingfilepath()
{
	// 설정 파일 경로를 반환하는 함수.
	return m_SettingFilePath;
}

// =============================================================================
CString Common::getPathCanonicalize(LPCTSTR path)
{
	// 경로를 정규화하는 함수.
	TCHAR canoPath[MAX_PATH] = { 0, };
	::PathCanonicalize(canoPath, path);

	return canoPath;
}

// =============================================================================
CString Common::GetRootPath()
{
	// 루트 경로를 가져오는 함수.
	TCHAR path[MAX_PATH] = { 0, };
	::GetModuleFileName(NULL, path, MAX_PATH);

	CString modulePath = getPathCanonicalize(path);

	return modulePath.Left(modulePath.ReverseFind('\\'));
}

// =============================================================================
std::string Common::GetRootPathA()
{
	// ANSI 형식의 루트 경로를 가져오는 함수.
	char path[MAX_PATH] = { 0, };
	::GetModuleFileNameA(NULL, path, MAX_PATH);

	std::string::size_type pos = std::string(path).find_last_of("\\");

	return std::string(path).substr(0, pos);
}

// =============================================================================
// Unicode char* -> CString 변환 과정 
// char* -> wchar* -> CString 순서로 변환 되어야 함
CString Common::uni_char_to_CString_Convert(char* data)
{
	int len = 0;
	CString str;
	BSTR buf = SysAllocString(L"");

	// 1. char* to wchar * conversion
	len = MultiByteToWideChar(CP_ACP, 0, data, static_cast<int>(strlen(data)), buf, len);
	buf = SysAllocStringLen(NULL, len);
	MultiByteToWideChar(CP_ACP, 0, data, static_cast<int>(strlen(data)), buf, len);

	// 2. wchar_t* to CString conversion
	str.Format(_T("%s"), buf);

	return str;
}

// =============================================================================
// Unicode CString -> char* 변환 과정 
// CString -> wchar* -> char* 순서로 변환 되어야 함
char* Common::CString_to_uni_char_Convert(CString data)
{
	wchar_t* wchar_str;
	char* char_str;
	int char_str_len;

	// 1. CString to wchar * conversion
	wchar_str = data.GetBuffer(data.GetLength());
	char_str_len = WideCharToMultiByte(CP_ACP, 0, wchar_str, -1, NULL, 0, NULL, NULL);
	char_str = new char[char_str_len];

	// 2. wchar_t* to char* conversion
	WideCharToMultiByte(CP_ACP, 0, wchar_str, -1, char_str, char_str_len, 0, 0);

	//delete char_str;
	return char_str;
	
}

// =============================================================================
// unicode Casting : CString -> LPCSTR Casting
//CString -> LPCSTR
//LPCSTR lpcstr = (LPSTR)(LPCTSTR)data;
//WinExec( lpcstr, SW_HIDE );
//Multi-byte 환경 에서는 위 코드로 문제 없이 사용 가능 
//unicode 환경 에서 Casting 은 가능 하지만 실제 LPCSTR 인자로 넘겨서 사용 하면 문제가 될수도?
LPCSTR Common::CString_to_LPCSTR_Convert(CString data)
{
	CStringA strA(data);
	LPCSTR ptr = strA;
	
	return ptr;
}

// =============================================================================
uint32_t Common::rgbPalette(double ratio)
{
	// 비율을 정규화하여 6개의 영역에 맞게 조정.
	// 각 영역은 256 단위 길이다.
	int normalized = int(ratio * 256 * 6);

	// 위치에 대한 영역 검색.
	int region = normalized / 256;

	// 가장 가까운 영역의 시작 지점까지의 거리 검색.
	int x = normalized % 256;

	uint8_t r = 0, g = 0, b = 0;
	switch (region)
	{
		case 0: r = 255; g = 0;   b = 0;   g += x; break;
		case 1: r = 255; g = 255; b = 0;   r -= x; break;
		case 2: r = 0;   g = 255; b = 0;   b += x; break;
		case 3: r = 0;   g = 255; b = 255; g -= x; break;
		case 4: r = 0;   g = 0;   b = 255; r += x; break;
		case 5: r = 255; g = 0;   b = 255; b -= x; break;
	}
	return 0;
}

void Common::SetAutoStartFlag(bool bFlag)
{
	m_bAutoStart = bFlag;
}

bool Common::GetAutoStartFlag()
{
	return m_bAutoStart;
}

