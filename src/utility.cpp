#include "utility.h"

n64 Align(n64 a_nData, n64 a_nAlignment)
{
	return (a_nData + a_nAlignment - 1) / a_nAlignment * a_nAlignment;
}

bool UMakeDir(const UString::value_type* a_pDirName)
{
	if (UMkdir(a_pDirName) != 0)
	{
		if (errno != EEXIST)
		{
			return false;
		}
	}
	return true;
}

bool UGetFileSize(const UString::value_type* a_pFileName, n64& a_nFileSize)
{
	Stat st;
	if (UStat(a_pFileName, &st) != 0)
	{
		a_nFileSize = 0;
		return false;
	}
	a_nFileSize = st.st_size;
	return true;
}

FILE* Fopen(const char* a_pFileName, const char* a_pMode)
{
	FILE* fp = fopen(a_pFileName, a_pMode);
	if (fp == nullptr)
	{
		printf("ERROR: open file %s failed\n\n", a_pFileName);
	}
	return fp;
}

#if SDW_PLATFORM == SDW_PLATFORM_WINDOWS
FILE* FopenW(const wchar_t* a_pFileName, const wchar_t* a_pMode)
{
	FILE* fp = _wfopen(a_pFileName, a_pMode);
	if (fp == nullptr)
	{
		wprintf(L"ERROR: open file %ls failed\n\n", a_pFileName);
	}
	return fp;
}
#endif

bool Seek(FILE* a_fpFile, n64 a_nOffset)
{
	if (fflush(a_fpFile) != 0)
	{
		return false;
	}
	int nFd = Fileno(a_fpFile);
	if (nFd == -1)
	{
		return false;
	}
	Fseek(a_fpFile, 0, SEEK_END);
	n64 nFileSize = Ftell(a_fpFile);
	if (nFileSize < a_nOffset)
	{
		n64 nOffset = Lseek(nFd, a_nOffset - 1, SEEK_SET);
		if (nOffset == -1)
		{
			return false;
		}
		fputc(0, a_fpFile);
		fflush(a_fpFile);
	}
	else
	{
		Fseek(a_fpFile, a_nOffset, SEEK_SET);
	}
	return true;
}

void CopyFile(FILE* a_fpDest, FILE* a_fpSrc, n64 a_nSrcOffset, n64 a_nSize)
{
	const n64 nBufferSize = 0x100000;
	u8* pBuffer = new u8[nBufferSize];
	Fseek(a_fpSrc, a_nSrcOffset, SEEK_SET);
	while (a_nSize > 0)
	{
		n64 nSize = a_nSize > nBufferSize ? nBufferSize : a_nSize;
		fread(pBuffer, 1, static_cast<size_t>(nSize), a_fpSrc);
		fwrite(pBuffer, 1, static_cast<size_t>(nSize), a_fpDest);
		a_nSize -= nSize;
	}
	delete[] pBuffer;
}

void PadFile(FILE* a_fpFile, n64 a_nPadSize, u8 a_uPadData)
{
	const n64 nBufferSize = 0x100000;
	u8* pBuffer = new u8[nBufferSize];
	memset(pBuffer, a_uPadData, nBufferSize);
	while (a_nPadSize > 0)
	{
		n64 nSize = a_nPadSize > nBufferSize ? nBufferSize : a_nPadSize;
		fwrite(pBuffer, 1, static_cast<size_t>(nSize), a_fpFile);
		a_nPadSize -= nSize;
	}
	delete[] pBuffer;
}

const UString& UGetModuleFileName()
{
	const u32 uMaxPath = 4096;
	static UString sFileName;
	sFileName.clear();
	sFileName.resize(uMaxPath, USTR('\0'));
	u32 uSize = 0;
#if SDW_PLATFORM == SDW_PLATFORM_WINDOWS
	uSize = GetModuleFileNameW(nullptr, &*sFileName.begin(), uMaxPath);
#elif SDW_PLATFORM == SDW_PLATFORM_MACOS
	char szPath[uMaxPath] = {};
	u32 uPathSize = uMaxPath;
	if (_NSGetExecutablePath(szPath, &uPathSize) != 0)
	{
		printf("ERROR: _NSGetExecutablePath error\n\n");
		sFileName.clear();
	}
	else if (realpath(szPath, &*sFileName.begin()) == nullptr)
	{
		printf("ERROR: realpath error\n\n");
		sFileName.clear();
	}
	uSize = strlen(sFileName.c_str());
#elif SDW_PLATFORM == SDW_PLATFORM_LINUX
	ssize_t nCount = readlink("/proc/self/exe", &*sFileName.begin(), uMaxPath);
	if (nCount == -1)
	{
		printf("ERROR: readlink /proc/self/exe error\n\n");
		sFileName.clear();
	}
	else
	{
		sFileName[nCount] = '\0';
	}
	uSize = strlen(sFileName.c_str());
#endif
	sFileName.erase(uSize);
	sFileName = Replace(sFileName, USTR('\\'), USTR('/'));
	return sFileName;
}

const UString& UGetModuleDirName()
{
	static UString sDirName = UGetModuleFileName();
	UString::size_type uPos = sDirName.rfind(USTR('/'));
	if (uPos != UString::npos)
	{
		sDirName.erase(uPos);
	}
	else
	{
		sDirName.clear();
	}
	return sDirName;
}

void SetLocale()
{
#if SDW_PLATFORM == SDW_PLATFORM_MACOS
	setlocale(LC_ALL, "en_US.UTF-8");
#else
	setlocale(LC_ALL, "");
#endif
}

n32 SToN32(const string& a_sString, int a_nRadix /* = 10 */)
{
	return static_cast<n32>(strtol(a_sString.c_str(), nullptr, a_nRadix));
}

#if SDW_COMPILER == SDW_COMPILER_MSC && SDW_COMPILER_VERSION < 1600
string WToU8(const wstring& a_sString)
{
	int nLength = WideCharToMultiByte(CP_UTF8, 0, a_sString.c_str(), -1, nullptr, 0, nullptr, nullptr);
	char* pTemp = new char[nLength];
	WideCharToMultiByte(CP_UTF8, 0, a_sString.c_str(), -1, pTemp, nLength, nullptr, nullptr);
	string sString = pTemp;
	delete[] pTemp;
	return sString;
}

string U16ToU8(const U16String& a_sString)
{
	return WToU8(a_sString);
}

wstring U8ToW(const string& a_sString)
{
	int nLength = MultiByteToWideChar(CP_UTF8, 0, a_sString.c_str(), -1, nullptr, 0);
	wchar_t* pTemp = new wchar_t[nLength];
	MultiByteToWideChar(CP_UTF8, 0, a_sString.c_str(), -1, pTemp, nLength);
	wstring sString = pTemp;
	delete[] pTemp;
	return sString;
}

wstring U16ToW(const U16String& a_sString)
{
	return a_sString;
}

U16String U8ToU16(const string& a_sString)
{
	return U8ToW(a_sString);
}

U16String WToU16(const wstring& a_sString)
{
	return a_sString;
}
#else
string WToU8(const wstring& a_sString)
{
	static wstring_convert<codecvt_utf8<wchar_t>> c_cvt_u8;
	return c_cvt_u8.to_bytes(a_sString);
}

string U16ToU8(const U16String& a_sString)
{
	static wstring_convert<codecvt_utf8_utf16<Char16_t>, Char16_t> c_cvt_u8_u16;
	return c_cvt_u8_u16.to_bytes(a_sString);
}

wstring U8ToW(const string& a_sString)
{
	static wstring_convert<codecvt_utf8<wchar_t>> c_cvt_u8;
	return c_cvt_u8.from_bytes(a_sString);
}

wstring U16ToW(const U16String& a_sString)
{
	return U8ToW(U16ToU8(a_sString));
}

U16String U8ToU16(const string& a_sString)
{
	static wstring_convert<codecvt_utf8_utf16<Char16_t>, Char16_t> c_cvt_u8_u16;
	return c_cvt_u8_u16.from_bytes(a_sString);
}

U16String WToU16(const wstring& a_sString)
{
	return U8ToU16(WToU8(a_sString));
}
#endif

#if SDW_PLATFORM == SDW_PLATFORM_WINDOWS
wstring AToW(const string& a_sString)
{
	int nLength = MultiByteToWideChar(CP_ACP, 0, a_sString.c_str(), -1, nullptr, 0);
	wchar_t* pTemp = new wchar_t[nLength];
	MultiByteToWideChar(CP_ACP, 0, a_sString.c_str(), -1, pTemp, nLength);
	wstring sString = pTemp;
	delete[] pTemp;
	return sString;
}
#else
wstring AToW(const string& a_sString)
{
	return U8ToW(a_sString);
}
#endif

static const int s_nFormatBufferSize = 4096;

string FormatV(const char* a_szFormat, va_list a_vaList)
{
	static char c_szBuffer[s_nFormatBufferSize] = {};
	vsnprintf(c_szBuffer, s_nFormatBufferSize, a_szFormat, a_vaList);
	return c_szBuffer;
}

wstring FormatV(const wchar_t* a_szFormat, va_list a_vaList)
{
	static wchar_t c_szBuffer[s_nFormatBufferSize] = {};
	vswprintf(c_szBuffer, s_nFormatBufferSize, a_szFormat, a_vaList);
	return c_szBuffer;
}

string Format(const char* a_szFormat, ...)
{
	va_list vaList;
	va_start(vaList, a_szFormat);
	string sFormatted = FormatV(a_szFormat, vaList);
	va_end(vaList);
	return sFormatted;
}

wstring Format(const wchar_t* a_szFormat, ...)
{
	va_list vaList;
	va_start(vaList, a_szFormat);
	wstring sFormatted = FormatV(a_szFormat, vaList);
	va_end(vaList);
	return sFormatted;
}
