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
// ���� �˻�
void Common::GetFileList(CString path, vector<CString>& strArray)
{

	//�˻� Ŭ����
	CFileFind finder;

	//CFileFind�� ����, ���͸��� �����ϸ� TRUE �� ��ȯ��
	BOOL bWorking = finder.FindFile(path); //

	CString fileName;
	CString DirName;

	while (bWorking)
	{
		//���� ���� / ���� �� �����ϸ�ٸ� TRUE ��ȯ
		bWorking = finder.FindNextFile();
		//���� �϶�
		if (finder.IsArchived())
		{
			//������ �̸�
			CString _fileName = finder.GetFileName();

			// �������� �������� ����������� ����
			if (_fileName == _T(".") ||
				_fileName == _T("..") ||
				_fileName == _T("Thumbs.db")) continue;

			fileName = finder.GetFileTitle();
			strArray.push_back(fileName);

		}
	}
}
// ���� ���� ���� ���α׷��� ���丮 ��θ� ��ȯ
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
// �α� �ڵ鷯 ����
void Common::CreateLog(CListBox* handle)
{
	bool bFlag = false;
	m_logHandle = handle;
	//log ����

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
	fullLogMessage.Format(_T("[%s] %s\r\n"), strDate, formattedMessage);  // �α� ���ο� ���� ���� �߰�

	if (m_fWriteFile.m_hFile != CFile::hFileNull) {
		// CString�� ������ ANSI ���ڿ��� ��ȯ�Ͽ� ���Ͽ� ����
		CStringA strAnsi(fullLogMessage); // Unicode CString�� ANSI CStringA�� ��ȯ
		m_fWriteFile.Write(strAnsi.GetBuffer(), strAnsi.GetLength());
		m_fWriteFile.Flush(); // ���� ���۸� ���
	}

	// �α� �޽��� ����Ʈ
	const int maxRetries = 3;
	int retryCount = 0;
	bool messagePosted = false;

	if (CWnd* pMainWnd = AfxGetMainWnd()) 
	{  // �� �����찡 ��ȿ���� Ȯ��
		CString* pStr = new CString(fullLogMessage);
		while (retryCount < maxRetries && !messagePosted) 
		{
			if (pMainWnd->PostMessage(WM_ADDLOG, reinterpret_cast<WPARAM>(pStr), 0)) 
			{
				messagePosted = true;
			}
			else {
				retryCount++;
				Sleep(100); // 100ms ��� �� ��õ�
			}
		}
		if (!messagePosted) 
		{
			// �޽��� ���� ���н� ó��
			Common::GetInstance()->AddLog(0, _T("Failed to post message to main window after %d retries"), maxRetries);
			delete pStr; // �޸� ������ �����ϱ� ���� pStr ����
		}
	}
}


// =============================================================================
void Common::CreateFolder(CString csPath)
{
	// ������ ��ο� ������ �����ϴ� �Լ��Դϴ�.
	// ������('/')�� ���е� ��θ� �޾Ƽ� �� ��� ��ū�� �����ϰ� ������ �����մϴ�.

	CString csPrefix(_T("")), csToken(_T(""));
	int nStart = 0, nEnd;
	while ((nEnd = csPath.Find('/', nStart)) >= 0)
	{
		// �����÷� ���е� ��ο��� ��ū ����
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
	// ���α׷� ���� ��ο� ������ ���ڿ��� �����ϴ� �Լ�.
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
	// ���� ���� ��θ� �����ϴ� �Լ�.
	m_SettingFilePath = _Path;
}

// =============================================================================
CString Common::Getsetingfilepath()
{
	// ���� ���� ��θ� ��ȯ�ϴ� �Լ�.
	return m_SettingFilePath;
}

// =============================================================================
CString Common::getPathCanonicalize(LPCTSTR path)
{
	// ��θ� ����ȭ�ϴ� �Լ�.
	TCHAR canoPath[MAX_PATH] = { 0, };
	::PathCanonicalize(canoPath, path);

	return canoPath;
}

// =============================================================================
CString Common::GetRootPath()
{
	// ��Ʈ ��θ� �������� �Լ�.
	TCHAR path[MAX_PATH] = { 0, };
	::GetModuleFileName(NULL, path, MAX_PATH);

	CString modulePath = getPathCanonicalize(path);

	return modulePath.Left(modulePath.ReverseFind('\\'));
}

// =============================================================================
std::string Common::GetRootPathA()
{
	// ANSI ������ ��Ʈ ��θ� �������� �Լ�.
	char path[MAX_PATH] = { 0, };
	::GetModuleFileNameA(NULL, path, MAX_PATH);

	std::string::size_type pos = std::string(path).find_last_of("\\");

	return std::string(path).substr(0, pos);
}



// =============================================================================
// Unicode char* -> CString ��ȯ ���� 
// char* -> wchar* -> CString ������ ��ȯ �Ǿ�� ��
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
// Unicode CString -> char* ��ȯ ���� 
// CString -> wchar* -> char* ������ ��ȯ �Ǿ�� ��
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
//Multi-byte ȯ�� ������ �� �ڵ�� ���� ���� ��� ���� 
//unicode ȯ�� ���� Casting �� ���� ������ ���� LPCSTR ���ڷ� �Ѱܼ� ��� �ϸ� ������ �ɼ���?
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
	// ������ ��ΰ� �������� Ȯ��
	if (PathIsDirectoryA(folderPath.c_str()))
	{
		// ������ ���� ���� ShellExecute �Լ� ���
		HINSTANCE result = ShellExecuteA(NULL, "open", folderPath.c_str(), NULL, NULL, SW_SHOWNORMAL);

		// ��� Ȯ��
		if ((int)result > 32)
		{
			// ���������� ������ ������
			return true;
		}
		else
		{
			// ���� ���� ����
			return false;
		}
	}
	else
	{
		// ������ ��ΰ� ������ �ƴ�
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

			// ���͸��� �����Ǿ��ų� �̹� ������
			strLog.Format(_T("Create Directory \n[%s]"), CString(path.c_str()));
			AddLog(0, strLog);
			return true; // ��� ���� ����
		}
		else
		{
			// ���͸� ���� ���� �� ������ �۾��� �̰��� �߰�
			strLog.Format(_T("Folder already exists in the path \n[%s]"), CString(path.c_str()));
			AddLog(0, strLog);
		}
	}

	return false;
}