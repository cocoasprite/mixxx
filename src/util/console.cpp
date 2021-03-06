
#include "console.h"

#include <stdio.h>
#include <QtDebug>

#ifdef __WINDOWS__
#include <windows.h>
#include <io.h> // Debug Console
typedef BOOL(WINAPI* pfSetCurrentConsoleFontEx)(HANDLE, BOOL, PCONSOLE_FONT_INFOEX);


Console::Console() {

    // Setup Windows console encoding
    // toLocal8Bit() returns the ANSI file encoding format
    // this does not necessarily match the OEM console encoding
    // https://www.microsoft.com/resources/msdn/goglobal/default.mspx
    // In case of a German Windows XP to 10 console encoding is cp850
    // where files encoding is cp1252
    // Qt has no solution for it https://bugreports.qt.io/browse/QTBUG-13303
    // http://stackoverflow.com/questions/1259084/what-encoding-code-page-is-cmd-exe-using
    // We try to change the console encoding to file encoding
    // For a German windows we expect
    // LOCALE_IDEFAULTANSICODEPAGE "1252" // ANSI Codepage used by Qt toLocal8Bit
    // LOCALE_IDEFAULTCODEPAGE "850" // OEM Codepage Console

    m_shouldResetCodePage = false;
    if(AttachConsole(ATTACH_PARENT_PROCESS)) {
        // we are started from a console porcess
        int fd;
        FILE *fp;

        fd = _open_osfhandle((long) GetStdHandle(STD_OUTPUT_HANDLE), 0);
        fp = _fdopen(fd, "w");
        *stdout = *fp;
        setvbuf(stdout, NULL, _IONBF, 0);

        fd = _open_osfhandle((long) GetStdHandle(STD_ERROR_HANDLE), 0);
        fp = _fdopen(fd, "w");
        *stderr = *fp;
        setvbuf(stderr, NULL, _IONBF, 0);

        // Save current code page
        m_oldCodePage = GetConsoleOutputCP();
        m_shouldResetCodePage = true;

        HMODULE kernel32_dll = LoadLibraryW(L"kernel32.dll");
        if (kernel32_dll) {
            pfSetCurrentConsoleFontEx pfSCCFX = (pfSetCurrentConsoleFontEx)GetProcAddress(kernel32_dll, "SetCurrentConsoleFontEx");
            if (pfSCCFX) {
                // Use a unicode font
                CONSOLE_FONT_INFOEX newFont;
                newFont.cbSize = sizeof newFont;
                newFont.nFont = 0;
                newFont.dwFontSize.X = 0;
                newFont.dwFontSize.Y = 14;
                newFont.FontFamily = FF_DONTCARE;
                newFont.FontWeight = FW_NORMAL;
                wcscpy_s(newFont.FaceName, L"Consolas");
                pfSCCFX(GetStdHandle(STD_OUTPUT_HANDLE), FALSE, &newFont);
            } else {
                // This happens on Windows XP
                qWarning() << "The console font may not support non ANSI characters." <<
                              "In case of character issues switch to font \"Lucida Console\"";
            }
        }

        // set console to the default ANSI Code Page
        UINT defaultCodePage;
        GetLocaleInfo(LOCALE_USER_DEFAULT,
                      LOCALE_RETURN_NUMBER | LOCALE_IDEFAULTANSICODEPAGE,
                      reinterpret_cast<LPWSTR>(&defaultCodePage),
                      sizeof(defaultCodePage));
        SetConsoleOutputCP(defaultCodePage);
    } else {
        // started by double click
        // no need to deal with a console
    }
}

Console::~Console() {
    // Reset Windows console to old code page
    // We need to stick with the unicode font since
    // changing back will destroy the console history
    if (m_shouldResetCodePage) {
        SetConsoleOutputCP(m_oldCodePage);
    }
}

#else // __WINDOWS__

// Nothing to do on non Windows targets
Console::Console() {
}

Console::~Console() {
}

#endif // __WINDOWS__
