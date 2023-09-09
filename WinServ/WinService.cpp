
#define _CRT_SECURE_NO_WARNINGS

#pragma region Includes
#include <Windows.h>
#include <wininet.h>
#include "WinService.h"
#include "ThreadPool.h"
#pragma endregion

#pragma comment(lib, "wininet.lib")

#define BUFLEN 2048 // Max length of buffer

WinService::WinService(PWSTR pszServiceName,
                       BOOL fCanStop,
                       BOOL fCanShutdown,
                       BOOL fCanPauseContinue)
    : ServiceBase(pszServiceName, fCanStop, fCanShutdown, fCanPauseContinue)
{
    m_fStopping = FALSE;

    // Create a manual-reset event that is not signaled at first to indicate
    // the stopped signal of the service.
    m_hStoppedEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (m_hStoppedEvent == NULL)
    {
        throw GetLastError();
    }
}

WinService::~WinService(void)
{
    if (m_hStoppedEvent)
    {
        CloseHandle(m_hStoppedEvent);
        m_hStoppedEvent = NULL;
    }
}

/**
 *   Executes when a Start command is sent to the service by the SCM or
 *   when the operating system starts (for a service that starts automatically).
 *
 *   @param dwArgc   - number of command line arguments
 *   @param lpszArgv - array of command line arguments
 */
void WinService::OnStart(DWORD dwArgc, LPWSTR *lpszArgv)
{
    // Log a service start message to the Application log.
    WriteEventLogEntry(L"SampleWindowsService is started", EVENTLOG_INFORMATION_TYPE);

    // Queue the main service function for execution in a worker thread.
    ThreadPool::QueueWorkItem(&WinService::ServiceWorkerThread, this);
}

/**
 *   Performs the main function of the service. It runs on a thread pool worker thread.
 */
void WinService::ServiceWorkerThread(void)
{
    DWORD dwFlags;
    BOOL bConnected = InternetGetConnectedState(&dwFlags, 0);
    BOOL isFirstTime = TRUE;

    // Periodically check if the service is stopping.
    while (!m_fStopping)
    {
        WriteEventLogEntry(L"WinServ is running",
                           EVENTLOG_INFORMATION_TYPE);
        Sleep(50000);
    }

    // Signal the stopped event.
    SetEvent(m_hStoppedEvent);
}

/**
 *   Executes when a Stop command is sent to the service by SCM. It specifies actions
 *   to take when a service stops running.
 */
void WinService::OnStop()
{
    // In this example, OnStop logs a service-stop message to the application log and
    // waits for the finish of the main service function.
    WriteEventLogEntry(L"SampleWindowsService stopped",
                       EVENTLOG_INFORMATION_TYPE);

    // Indicate that the service is stopping and wait for the finish of the
    m_fStopping = TRUE;
    if (WaitForSingleObject(m_hStoppedEvent, INFINITE) != WAIT_OBJECT_0)
    {
        throw GetLastError();
    }
}