/***************************************************************************
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
 *                                                                         *
 *   Part of the Free Heroes2 Engine:                                      *
 *   http://sourceforge.net/projects/fheroes2                              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#define _CRT_SECURE_NO_WARNINGS

#include <ctime>
#include <sstream>
#include <fstream>
#include <locale>


#include "SDL.h"
#include "system.h"

#if defined(WIN32)

#include <Windows.h>
#include <direct.h>
#endif


#if defined(WIN32) || defined(WIN64)
// Copied from linux libc sys/stat.h:
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#endif

#if !defined(WIN32)

#include <sys/stat.h>
#include <unistd.h>
#include <clocale>
#include <utility>

#endif

#if defined(__MINGW32CE__)
#undef Shell_NotifyIcon
extern "C" {
    BOOL WINAPI Shell_NotifyIcon(DWORD, PNOTIFYICONDATAW);
}

// wincommon/SDL_sysevents.c
extern HICON screen_icn;
extern HINSTANCE SDL_Instance;
extern HWND SDL_Window;
#endif

#if defined(__SYMBIAN32__)
#define SEPARATOR '\\'
#elif defined(__WIN32__)
#define SEPARATOR '\\'
#else
#define SEPARATOR '/'
#endif

#include "tools.h"

int System::MakeDirectory(const std::string& path)
{
#if defined(__SYMBIAN32__)
    return mkdir(path.c_str(), S_IRWXU);
#elif defined(WIN32)
    return _mkdir(path.c_str());
#else
    return mkdir(path.c_str(), S_IRWXU);
#endif
}

std::string System::ConcatePath(const std::string& str1, const std::string& str2)
{
    return std::string(str1 + SEPARATOR + str2);
}

std::string System::GetHomeDirectory(const std::string& prog)
{
    std::string res;

    if (GetEnvironment("HOME"))
        res = ConcatePath(GetEnvironment("HOME"), std::string(".").append(prog));
    else if (GetEnvironment("APPDATA"))
        res = ConcatePath(GetEnvironment("APPDATA"), prog);

    return res;
}

ListDirs System::GetDataDirectories(const std::string& prog)
{
    ListDirs dirs;

#if defined(ANDROID)
    const char* internal = SDL_AndroidGetInternalStoragePath();
    if(internal) dirs.push_back(System::ConcatePath(internal, prog));

    if(SDL_ANDROID_EXTERNAL_STORAGE_READ && SDL_AndroidGetExternalStorageState())
    {
    const char* external = SDL_AndroidGetExternalStoragePath();
    if(external) dirs.push_back(System::ConcatePath(external, prog));
    }

    dirs.push_back(System::ConcatePath("/storage/sdcard0", prog));
    dirs.push_back(System::ConcatePath("/storage/sdcard1", prog));
#endif

    return dirs;
}

ListFiles System::GetListFiles(const std::string& prog, const std::string& prefix, const std::string& filter)
{
    ListFiles res;

#if defined(ANDROID)
    VERBOSE(prefix << ", " << filter);

    // check assets
    StreamFile sf;
    if(sf.open("assets.list", "rb"))
    {
    std::list<std::string> rows = StringSplit(GetString(sf.getRaw(sf.size())), "\n");
    for(std::list<std::string>::const_iterator
        it = rows.begin(); it != rows.end(); ++it)
    if(prefix.empty() ||
        ((prefix.size() <= (*it).size() &&
        0 == prefix.compare((*it).substr(0, prefix.size())))))
    {
        if(filter.empty() ||
        (0 == filter.compare((*it).substr((*it).size() - filter.size(), filter.size()))))
        res.push_back(*it);
    }
    }

    ListDirs dirs = GetDataDirectories(prog);

    for(ListDirs::const_iterator
    it = dirs.begin(); it != dirs.end(); ++it)
    {
        res.ReadDir(prefix.size() ? System::ConcatePath(*it, prefix) : *it, filter, false);
    }
#endif
    return res;
}

std::string System::GetDirname(const std::string& str)
{
    if (str.empty())
        return str;
    const size_t pos = str.rfind(SEPARATOR);

    if (std::string::npos == pos)
        return std::string(".");
    if (pos == 0)
        return std::string("./");
    if (pos == str.size() - 1)
        return GetDirname(str.substr(0, str.size() - 1));
    return str.substr(0, pos);
}

std::string System::GetBasename(const std::string& str)
{
    if (str.empty())
        return str;
    const size_t pos = str.rfind(SEPARATOR);

    if (std::string::npos == pos ||
        pos == 0)
        return str;
    if (pos == str.size() - 1)
        return GetBasename(str.substr(0, str.size() - 1));
    return str.substr(pos + 1);
}

const char* System::GetEnvironment(const char* name)
{
#if defined(__MINGW32CE__) || defined(__MINGW32__)
    return SDL_getenv(name);
#else
    return getenv(name);
#endif
}

int System::SetEnvironment(const char* name, const char* value)
{
#if defined(WIN32)
    std::string str(std::string(name) + "=" + std::string(value));
    // SDL 1.2.12 (char *)
    return SDL_putenv(const_cast<char *>(str.c_str()));
#else
    return setenv(name, value, 1);
#endif
}

void System::SetLocale(int category, const char* locale)
{
#if defined(ANDROID)
    setlocale(category, locale);
#else
    setlocale(category, locale);
#endif
}

std::string System::GetMessageLocale(int length /* 1, 2, 3 */)
{
    std::string locname;
#if defined(WIN32)
    char* clocale = setlocale(LC_MONETARY, nullptr);
#elif defined(ANDROID)
    char* clocale = setlocale(LC_MESSAGES, nullptr);
#else
    char *clocale = std::setlocale(LC_MESSAGES, nullptr);
#endif

    if (clocale)
    {
        locname = StringLower(clocale);
        // 3: en_us.utf-8
        // 2: en_us
        // 1: en
        if (length < 3)
        {
            auto list = StringSplit(locname, length < 2 ? "_" : ".");
            return list.empty() ? locname : list.front();
        }
    }

    return locname;
}

int System::GetCommandOptions(int argc, const std::vector<std::string>& argv, const char* optstring)
{
#if defined(__MINGW32CE__)
    return -1;
#else
    return -1;

#endif
}

char* System::GetOptionsArgument()
{
#if defined(WIN32)
    return nullptr;
#else
    return optarg;
#endif
}

size_t System::GetMemoryUsage()
{
#if defined(__SYMBIAN32__)
    return 0;
#elif defined(__WIN32__)
    static MEMORYSTATUS ms;

    ZeroMemory(&ms, sizeof ms);
    ms.dwLength = sizeof(MEMORYSTATUS);
    GlobalMemoryStatus(&ms);

    return ms.dwTotalVirtual - ms.dwAvailVirtual;
#elif defined(__LINUX__)
    unsigned int size = 0;
    std::ostringstream os;
    os << "/proc/" << getpid() << "/statm";

    std::ifstream fs(os.str().c_str());
    if (fs.is_open())
    {
        fs >> size;
        fs.close();
    }

    return size * getpagesize();
#else
    return 0;
#endif
}

std::string System::GetTime()
{
    time_t raw;
    char buf[13] = {0};

    time(&raw);
    struct tm* tmi = localtime(&raw);

    strftime(buf, sizeof buf - 1, "%X", tmi);

    return std::string(buf);
}

bool System::IsFile(const std::string& name, bool writable)
{
    std::ifstream f(name.c_str());
    return f.good();
}

#ifdef WIN32
bool dirExists(const std::string& dirName_in)
{
    const DWORD ftyp = GetFileAttributesA(dirName_in.c_str());
    if (ftyp == INVALID_FILE_ATTRIBUTES)
        return false; //something is wrong with your path!

    // this is a directory!
    return ftyp & FILE_ATTRIBUTE_DIRECTORY;
}
#endif


bool System::IsDirectory(const std::string& name, bool writable)
{
#if defined (ANDROID)
    return writable ? 0 == access(name.c_str(), W_OK) : true;
#else
#ifdef WIN32
    return dirExists(name);
#else
    struct stat fs{};

    if (stat(name.c_str(), &fs) || !S_ISDIR(fs.st_mode))
        return false;

    return writable ? 0 == access(name.c_str(), W_OK) : S_IRUSR & fs.st_mode;

#endif
#endif
}

int System::Unlink(const std::string& file)
{
#ifdef WIN32
    return _unlink(file.c_str());
#else
    return unlink(file.c_str());
#endif
}

int System::CreateTrayIcon(bool fl)
{
#if defined(__MINGW32CE__) && defined(ID_ICON)
    NOTIFYICONDATA nid = {0};
    nid.cbSize =  sizeof(nid);
    nid.uID = ID_ICON;
    nid.hWnd = SDL_Window;

    if(fl)
    {
    nid.uFlags = NIF_ICON | NIF_MESSAGE;
    nid.uCallbackMessage = WM_USER;
    nid.hIcon = ::LoadIcon(SDL_Instance, MAKEINTRESOURCE(ID_ICON));
    return Shell_NotifyIcon(NIM_ADD, &nid);
    }

    return Shell_NotifyIcon(NIM_DELETE, &nid);
#endif
    return 0;
}

void System::PowerManagerOff(bool fl)
{
#if defined(__MINGW32CE__)
    // power manager control
    const wchar_t lpGlobalSubKeyPM[] = TEXT("System\\CurrentControlSet\\Control\\Power\\Timeouts");
    const wchar_t lpNamePM[] = TEXT("BattSuspendTimeout");
    static DWORD origValuePM = 0;

    HKEY   hKey;

    if(ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, lpGlobalSubKeyPM, 0, KEY_ALL_ACCESS, &hKey))
    {
        DWORD dwType = REG_DWORD;
    DWORD value = 0;

    // save orig value
    if(fl)
    {
            DWORD valueLen = sizeof(origValuePM);

            if(ERROR_SUCCESS == RegQueryValueEx(hKey, lpNamePM, 0, &dwType, (LPBYTE) &origValuePM, &valueLen))
        RegSetValueEx(hKey, lpNamePM, 0, dwType, (const BYTE*) &value, sizeof(value));
    }
    else
    if(origValuePM)
        RegSetValueEx(hKey, lpNamePM, 0, dwType, (const BYTE*) &origValuePM, sizeof(origValuePM));

        RegCloseKey(hKey);
    }

    HANDLE hEvent = CreateEvent(nullptr, FALSE, FALSE, TEXT("PowerManager/ReloadActivityTimeouts"));

    if(hEvent != nullptr)
    {
        SetEvent(hEvent);
        CloseHandle(hEvent);
    }

    // backlight control
    const wchar_t lpGlobalSubKeyBL[] = TEXT("\\ControlPanel\\Backlight");
    const wchar_t lpNameBL[] = TEXT("BatteryTimeout");
    static DWORD origValueBL = 0;

    if(ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, lpGlobalSubKeyBL, 0, KEY_ALL_ACCESS, &hKey))
    {
        DWORD dwType = REG_DWORD;
    DWORD value = 0;

    // save orig value
    if(fl)
    {
            DWORD valueLen = sizeof(origValueBL);

            if(ERROR_SUCCESS == RegQueryValueEx(hKey, lpNameBL, 0, &dwType, (LPBYTE) &origValueBL, &valueLen))
        RegSetValueEx(hKey, lpNameBL, 0, dwType, (const BYTE*) &value, sizeof(value));
    }
    else
    if(origValueBL)
        RegSetValueEx(hKey, lpNameBL, 0, dwType, (const BYTE*) &origValueBL, sizeof(origValueBL));

        RegCloseKey(hKey);
    }

    hEvent = CreateEvent(nullptr, FALSE, FALSE, TEXT("BackLightChangeEvent"));

    if(hEvent != nullptr)
    {
        SetEvent(hEvent);
        CloseHandle(hEvent);
    }
#endif
}

bool System::isRunning()
{
#if defined(__MINGW32CE__)
    SetEnvironment("DEBUG_VIDEO", "1");
    SetEnvironment("DEBUG_VIDEO_GAPI", "1");

    HWND hwnd = FindWindow(nullptr, L"SDL_app");

    if(hwnd)
    {
        ShowWindow(hwnd, SW_SHOW);
        SetForegroundWindow(hwnd);
    }

    return hwnd;
#endif

    return false;
}

int System::ShellCommand(const char* cmd)
{
    if (!cmd)
    {
        return -1;
    }
    return system(cmd);
}

int System::GetRenderFlags()
{
#if defined(__MINGW32CE__) || defined(__SYMBIAN32__)
    return SDL_SWSURFACE;
#endif
#if defined(__WIN32__) || defined(ANDROID)
    return SDL_HWSURFACE;
#endif
    return SDL_SWSURFACE;
}
