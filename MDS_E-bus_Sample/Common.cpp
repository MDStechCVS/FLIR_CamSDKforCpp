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
// 현재 실행 중인 프로그램의 디렉토리 경로를 반환
// =============================================================================
CString Common::GetProgramDirectory()
{
	TCHAR szPath[_MAX_PATH];
	GetModuleFileName(AfxGetApp()->m_hInstance, szPath, _MAX_PATH);
	CString strProgramDir = szPath;
	int nLen = strProgramDir.ReverseFind('\\');

	if (nLen > 0)
		strProgramDir = strProgramDir.Left(nLen);

	return strProgramDir;
}

// =============================================================================
// 로그 핸들러 생성
void Common::CreateLog(CListBox* handle)
{
	bool bFlag = false;
	m_logHandle = handle;
	//log 생성

	CString path;
	path.Format(_T("%s\\SystemLog\\"), (LPCTSTR)GetProgramDirectory());

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
	SYSTEMTIME cur_time;
	CString strDate;
	CString formattedMessage;
	va_list args;

	GetLocalTime(&cur_time);
	strDate.Format(_T("%02d:%02d:%02d"), cur_time.wHour, cur_time.wMinute, cur_time.wSecond);

	va_start(args, lpszFormat);
	formattedMessage.FormatV(lpszFormat, args);
	va_end(args);

	CString fullLogMessage;
	fullLogMessage.Format(_T("[%s] %s\r\n"), strDate, formattedMessage);  // 로그 라인에 개행 문자 추가

	if (m_fWriteFile.m_hFile != CFile::hFileNull) {
		// CString의 내용을 ANSI 문자열로 변환하여 파일에 쓰기
		CStringA strAnsi(fullLogMessage); // Unicode CString을 ANSI CStringA로 변환
		m_fWriteFile.Write(strAnsi.GetBuffer(), strAnsi.GetLength());
		m_fWriteFile.Flush(); // 파일 버퍼를 비움
	}

	// 로그 메시지 포스트
	const int maxRetries = 3;
	int retryCount = 0;
	bool messagePosted = false;

	if (CWnd* pMainWnd = AfxGetMainWnd()) 
	{  // 주 윈도우가 유효한지 확인
		CString* pStr = new CString(fullLogMessage);
		while (retryCount < maxRetries && !messagePosted) 
		{
			if (pMainWnd->PostMessage(WM_ADDLOG, reinterpret_cast<WPARAM>(pStr), 0)) 
			{
				messagePosted = true;
			}
			else {
				retryCount++;
				Sleep(100); // 100ms 대기 후 재시도
			}
		}
		if (!messagePosted) 
		{
			// 메시지 전송 실패시 처리
			Common::GetInstance()->AddLog(0, _T("Failed to post message to main window after %d retries"), maxRetries);
			delete pStr; // 메모리 누수를 방지하기 위해 pStr 삭제
		}
	}
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



void Common::SetAutoStartFlag(bool bFlag)
{
	m_bAutoStart = bFlag;
}

bool Common::GetAutoStartFlag()
{
	return m_bAutoStart;
}

bool Common::OpenFolder(const std::string& folderPath)
{
	// 지정된 경로가 폴더인지 확인
	if (PathIsDirectoryA(folderPath.c_str()))
	{
		// 폴더를 열기 위해 ShellExecute 함수 사용
		HINSTANCE result = ShellExecuteA(NULL, "open", folderPath.c_str(), NULL, NULL, SW_SHOWNORMAL);

		// 결과 확인
		if ((int)result > 32)
		{
			// 성공적으로 폴더를 열었음
			return true;
		}
		else
		{
			// 폴더 열기 실패
			return false;
		}
	}
	else
	{
		// 지정된 경로가 폴더가 아님
		return false;
	}
}

// =============================================================================
bool Common::CreateDirectoryRecursively(const std::string& path)
{
	if (!path.empty())
	{
		CString strLog;
		if (SHCreateDirectoryExA(nullptr, path.c_str(), nullptr) == ERROR_SUCCESS)
		{

			// 디렉터리가 생성되었거나 이미 존재함
			strLog.Format(_T("Create Directory \n[%s]"), CString(path.c_str()));
			AddLog(0, strLog);
			return true; // 경로 생성 성공
		}
		else
		{
			// 디렉터리 생성 실패 시 수행할 작업을 이곳에 추가
			strLog.Format(_T("Folder already exists in the path \n[%s]"), CString(path.c_str()));
			AddLog(0, strLog);
		}
	}

	return false;
}