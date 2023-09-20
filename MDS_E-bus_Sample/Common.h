#pragma once

#define INIPath	Common::GetInstance()->SetProgramPath(_T("CameraParams.ini"))

class Common
{

private:
	Common();
	~Common();
	Common(const Common& other);
	static Common* instance;
public:
	static Common* GetInstance()
	{
		if (instance == nullptr)
			instance = new Common();
		return instance;
	}

public:
	void CreateLog(CListBox* handle);
	void AddLog(int verbosity, LPCTSTR lpszFormat, ...);
	void ClearLog();
	void CreateFolder(CString csPath);
	void GetFileList(CString path, vector<CString>& strArray);
	CString SetProgramPath(CString _str);
	CString ChangeBool(bool bValue);
	void Setsetingfilepath(CString _Path);
	CString Getsetingfilepath();
	CString GetRootPath();
	std::string GetRootPathA();
	void CreateHorizontalScroll();
	void SetAutoStartFlag(bool bFlag);
	bool GetAutoStartFlag();
public:

	CString uni_char_to_CString_Convert(char* data);
	char* CString_to_uni_char_Convert(CString data);
	CString str2CString(const char* _str);
	std::string CString2str(CString _str);
	LPCSTR CString_to_LPCSTR_Convert(CString data);
	uint32_t rgbPalette(double ratio);
public:
	bool m_bBufferCheck;
	bool m_bPointerCheck;
	bool m_bMarkerCheck;

private:
	bool m_bAutoStart;

private:
	static CString getPathCanonicalize(LPCTSTR path);

private:
	CRITICAL_SECTION m_csLock;
	CFile m_fWriteFile;
	CListBox* m_logHandle;

	CString m_SettingFilePath;
};

