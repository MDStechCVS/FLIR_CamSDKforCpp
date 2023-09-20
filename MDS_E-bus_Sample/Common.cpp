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

// =============================================================================
// �α� �ڵ鷯 ����
void Common::CreateLog(CListBox* handle)
{
	bool bFlag = false;
	m_logHandle = handle;
	//log ����

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
	SYSTEMTIME cur_time; // ���� �ð� ������ ������ ����ü
	CString strDate; // �ð� ������ �������Ͽ� ������ ���ڿ�

	GetLocalTime(&cur_time); // ���� �ð� ������ ������
	strDate.Format(_T("%02d:%02d:%02d:%03ld"), cur_time.wHour, cur_time.wMinute, cur_time.wSecond, cur_time.wMilliseconds);

	if (m_csLock.LockCount > -1) // ��� Ƚ���� Ȯ���Ͽ� ������� ���� ��� ��ȯ
		return;

	CString logline; // �α׸� ������ ���ڿ�

	// �α� ���ο��� ���ʿ��� ���ڿ� ��ü
	logline.Replace(_T(" \""), _T("."));

	EnterCriticalSection(&m_csLock); // �Ӱ� ���� ����

	CString strWriteData; // �α׸� ���Ͽ� ����ϱ� ���� ���ڿ� ����
	strWriteData.Format(_T("[%s]%s"), (LPCTSTR)strDate, lpszFormat); // �ð��� ���� ���ڿ��� ���ļ� �α� ���ڿ� ����

	if (m_fWriteFile.m_hFile != CFile::hFileNull) // ���� �ڵ��� ��ȿ�� ���
	{
		m_fWriteFile.SeekToEnd(); // ���� ������ �̵�
		strWriteData = strWriteData + _T("\r\n"); // �α� ���ڿ��� ���� ���� �߰�
		m_fWriteFile.Write(strWriteData, strWriteData.GetLength() * 2); // �α� ���ڿ��� ���Ͽ� ����
	}
	m_fWriteFile.Flush(); // ���� ���� ���� ����

	LPTSTR  lpBuffer = strWriteData.GetBuffer(); // ���ڿ��� TCHAR ���·� ��ȯ�Ͽ� ���� ����
	strWriteData.ReleaseBuffer();

	HWND listHWnd = ::GetDlgItem(AfxGetMainWnd()->m_hWnd, IDC_LIST_LOG); // ����Ʈ �ڽ��� �ڵ��� ����

	Sleep(1); // ��� ���

	SendMessage(listHWnd, LB_ADDSTRING, 0, (LPARAM)lpBuffer); // ����Ʈ �ڽ��� ���ڿ� �߰�
	m_logHandle->SetTopIndex(m_logHandle->GetCount() - 1); // ����Ʈ �ڽ� ��ũ���� ���� �Ʒ��� �̵�
	CreateHorizontalScroll(); // ���� ��ũ�� ����

	LeaveCriticalSection(&m_csLock); // �Ӱ� ���� ��������
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

// =============================================================================
uint32_t Common::rgbPalette(double ratio)
{
	// ������ ����ȭ�Ͽ� 6���� ������ �°� ����.
	// �� ������ 256 ���� ���̴�.
	int normalized = int(ratio * 256 * 6);

	// ��ġ�� ���� ���� �˻�.
	int region = normalized / 256;

	// ���� ����� ������ ���� ���������� �Ÿ� �˻�.
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

