#pragma once

#include <QString>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QDebug>

#ifdef _WIN32
#include <windows.h>
#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")
#endif

class CrashHandler
{
public:
    static void setupCrashHandler();
    static void logCrashInfo(const QString& message);

private:
#ifdef _WIN32
    static LONG WINAPI exceptionFilter(EXCEPTION_POINTERS* exceptionInfo);
    static void generateMiniDump(EXCEPTION_POINTERS* exceptionInfo);
    static QString getExceptionString(DWORD code);
#endif
    
    static QFile* s_crashLogFile;
    static QTextStream* s_crashLogStream;
};
