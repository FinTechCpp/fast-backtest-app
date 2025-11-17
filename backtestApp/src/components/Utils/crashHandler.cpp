#include "components/Utils/crashHandler.h"
#include <QDir>
#include <QStandardPaths>
#include <QCoreApplication>

QFile* CrashHandler::s_crashLogFile = nullptr;
QTextStream* CrashHandler::s_crashLogStream = nullptr;

void CrashHandler::setupCrashHandler()
{
#ifdef _WIN32
    // Create crash log directory
    QString crashLogDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/crash_logs";
    QDir().mkpath(crashLogDir);
    
    QString crashLogPath = crashLogDir + "/crash_" + 
                          QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".log";
    
    s_crashLogFile = new QFile(crashLogPath);
    if (s_crashLogFile->open(QIODevice::WriteOnly | QIODevice::Text)) {
        s_crashLogStream = new QTextStream(s_crashLogFile);
        *s_crashLogStream << "=== Crash Log ===" << Qt::endl;
        *s_crashLogStream << "Application: " << QCoreApplication::applicationName() << Qt::endl;
        *s_crashLogStream << "Version: " << QCoreApplication::applicationVersion() << Qt::endl;
        *s_crashLogStream << "Start Time: " << QDateTime::currentDateTime().toString() << Qt::endl;
        *s_crashLogStream << Qt::endl;
        s_crashLogStream->flush();
        
        qDebug() << "Crash handler initialized. Log file:" << crashLogPath;
    }
    
    // Set up Windows exception handler
    SetUnhandledExceptionFilter(exceptionFilter);
#endif
}

void CrashHandler::logCrashInfo(const QString& message)
{
    if (s_crashLogStream) {
        *s_crashLogStream << "[" << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz") << "] " 
                         << message << Qt::endl;
        s_crashLogStream->flush();
    }
    
    qCritical() << message;
}

#ifdef _WIN32
LONG WINAPI CrashHandler::exceptionFilter(EXCEPTION_POINTERS* exceptionInfo)
{
    if (s_crashLogStream) {
        *s_crashLogStream << Qt::endl;
        *s_crashLogStream << "========================================" << Qt::endl;
        *s_crashLogStream << "UNHANDLED EXCEPTION DETECTED!" << Qt::endl;
        *s_crashLogStream << "Time: " << QDateTime::currentDateTime().toString() << Qt::endl;
        
        DWORD exceptionCode = exceptionInfo->ExceptionRecord->ExceptionCode;
        *s_crashLogStream << "Exception Code: 0x" << QString::number(exceptionCode, 16).toUpper() << Qt::endl;
        *s_crashLogStream << "Exception: " << getExceptionString(exceptionCode) << Qt::endl;
        
        void* exceptionAddress = exceptionInfo->ExceptionRecord->ExceptionAddress;
        *s_crashLogStream << "Exception Address: 0x" << QString::number((quintptr)exceptionAddress, 16) << Qt::endl;
        
        // Access violation details
        if (exceptionCode == EXCEPTION_ACCESS_VIOLATION || exceptionCode == EXCEPTION_IN_PAGE_ERROR) {
            ULONG_PTR* info = exceptionInfo->ExceptionRecord->ExceptionInformation;
            QString accessType = (info[0] == 0) ? "Read" : (info[0] == 1) ? "Write" : "Execute";
            *s_crashLogStream << "Access Type: " << accessType << Qt::endl;
            *s_crashLogStream << "Access Address: 0x" << QString::number(info[1], 16) << Qt::endl;
        }
        
        *s_crashLogStream << Qt::endl;
        *s_crashLogStream << "Stack trace:" << Qt::endl;
        
        // Capture stack trace
        CONTEXT* context = exceptionInfo->ContextRecord;
        STACKFRAME64 stackFrame;
        memset(&stackFrame, 0, sizeof(STACKFRAME64));
        
#ifdef _M_X64
        DWORD machineType = IMAGE_FILE_MACHINE_AMD64;
        stackFrame.AddrPC.Offset = context->Rip;
        stackFrame.AddrPC.Mode = AddrModeFlat;
        stackFrame.AddrFrame.Offset = context->Rbp;
        stackFrame.AddrFrame.Mode = AddrModeFlat;
        stackFrame.AddrStack.Offset = context->Rsp;
        stackFrame.AddrStack.Mode = AddrModeFlat;
#else
        DWORD machineType = IMAGE_FILE_MACHINE_I386;
        stackFrame.AddrPC.Offset = context->Eip;
        stackFrame.AddrPC.Mode = AddrModeFlat;
        stackFrame.AddrFrame.Offset = context->Ebp;
        stackFrame.AddrFrame.Mode = AddrModeFlat;
        stackFrame.AddrStack.Offset = context->Esp;
        stackFrame.AddrStack.Mode = AddrModeFlat;
#endif
        
        HANDLE process = GetCurrentProcess();
        HANDLE thread = GetCurrentThread();
        
        // Initialize symbol handler
        SymInitialize(process, NULL, TRUE);
        SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);
        
        int frameNum = 0;
        while (StackWalk64(machineType, process, thread, &stackFrame, context,
                          NULL, SymFunctionTableAccess64, SymGetModuleBase64, NULL)) {
            
            if (stackFrame.AddrPC.Offset == 0) break;
            
            *s_crashLogStream << QString("  Frame %1: 0x%2")
                .arg(frameNum)
                .arg(QString::number(stackFrame.AddrPC.Offset, 16)) << Qt::endl;
            
            // Get symbol info
            char symbolBuffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME];
            PSYMBOL_INFO symbol = (PSYMBOL_INFO)symbolBuffer;
            symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
            symbol->MaxNameLen = MAX_SYM_NAME;
            
            DWORD64 displacement = 0;
            if (SymFromAddr(process, stackFrame.AddrPC.Offset, &displacement, symbol)) {
                *s_crashLogStream << QString("    %1 + 0x%2")
                    .arg(symbol->Name)
                    .arg(QString::number(displacement, 16)) << Qt::endl;
                
                // Get line info
                IMAGEHLP_LINE64 line;
                line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
                DWORD lineDisplacement = 0;
                if (SymGetLineFromAddr64(process, stackFrame.AddrPC.Offset, &lineDisplacement, &line)) {
                    *s_crashLogStream << QString("    File: %1:%2")
                        .arg(line.FileName)
                        .arg(line.LineNumber) << Qt::endl;
                }
            }
            
            frameNum++;
            if (frameNum > 50) break; // Limit stack trace depth
        }
        
        SymCleanup(process);
        
        *s_crashLogStream << "========================================" << Qt::endl;
        s_crashLogStream->flush();
        
        // Generate minidump
        generateMiniDump(exceptionInfo);
    }
    
    return EXCEPTION_EXECUTE_HANDLER;
}

void CrashHandler::generateMiniDump(EXCEPTION_POINTERS* exceptionInfo)
{
    QString crashLogDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/crash_logs";
    QString dumpPath = crashLogDir + "/crash_" + 
                      QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".dmp";
    
    HANDLE dumpFile = CreateFileW(
        reinterpret_cast<LPCWSTR>(dumpPath.utf16()),
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    
    if (dumpFile != INVALID_HANDLE_VALUE) {
        MINIDUMP_EXCEPTION_INFORMATION dumpInfo;
        dumpInfo.ThreadId = GetCurrentThreadId();
        dumpInfo.ExceptionPointers = exceptionInfo;
        dumpInfo.ClientPointers = FALSE;
        
        MiniDumpWriteDump(
            GetCurrentProcess(),
            GetCurrentProcessId(),
            dumpFile,
            MiniDumpWithFullMemory,
            &dumpInfo,
            NULL,
            NULL
        );
        
        CloseHandle(dumpFile);
        
        if (s_crashLogStream) {
            *s_crashLogStream << "Minidump written to: " << dumpPath << Qt::endl;
            s_crashLogStream->flush();
        }
    }
}

QString CrashHandler::getExceptionString(DWORD code)
{
    switch (code) {
        case EXCEPTION_ACCESS_VIOLATION: return "Access Violation";
        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: return "Array Bounds Exceeded";
        case EXCEPTION_BREAKPOINT: return "Breakpoint";
        case EXCEPTION_DATATYPE_MISALIGNMENT: return "Datatype Misalignment";
        case EXCEPTION_FLT_DENORMAL_OPERAND: return "Float Denormal Operand";
        case EXCEPTION_FLT_DIVIDE_BY_ZERO: return "Float Divide by Zero";
        case EXCEPTION_FLT_INEXACT_RESULT: return "Float Inexact Result";
        case EXCEPTION_FLT_INVALID_OPERATION: return "Float Invalid Operation";
        case EXCEPTION_FLT_OVERFLOW: return "Float Overflow";
        case EXCEPTION_FLT_STACK_CHECK: return "Float Stack Check";
        case EXCEPTION_FLT_UNDERFLOW: return "Float Underflow";
        case EXCEPTION_ILLEGAL_INSTRUCTION: return "Illegal Instruction";
        case EXCEPTION_IN_PAGE_ERROR: return "In Page Error";
        case EXCEPTION_INT_DIVIDE_BY_ZERO: return "Integer Divide by Zero";
        case EXCEPTION_INT_OVERFLOW: return "Integer Overflow";
        case EXCEPTION_INVALID_DISPOSITION: return "Invalid Disposition";
        case EXCEPTION_NONCONTINUABLE_EXCEPTION: return "Noncontinuable Exception";
        case EXCEPTION_PRIV_INSTRUCTION: return "Privileged Instruction";
        case EXCEPTION_SINGLE_STEP: return "Single Step";
        case EXCEPTION_STACK_OVERFLOW: return "Stack Overflow";
        default: return "Unknown Exception";
    }
}
#endif
