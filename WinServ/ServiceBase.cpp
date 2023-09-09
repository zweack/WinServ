#pragma region Includes
#include "ServiceBase.h"
#include <assert.h>
#include <strsafe.h>
#pragma endregion

#pragma region Static Members

// Initialize the singleton service instance.
ServiceBase *ServiceBase::s_service = NULL;

/**
 *   Register the executable for a service with the Service Control
 *   Manager (SCM). After you call Run(ServiceBase), the SCM issues a Start
 *   command, which results in a call to the OnStart method in the service.
 *   This method blocks until the service has stopped.
 *
 *   @param service the reference to a ServiceBase object. It will become the
 *   singleton service instance of this service application.
 *
 *   @return If the function succeeds, the return value is TRUE. If the
 *   function fails, the return value is FALSE. To get extended error
 *   information, call GetLastError.
 */
BOOL ServiceBase::Run(ServiceBase &service)
{
    s_service = &service;

    SERVICE_TABLE_ENTRY serviceTable[] =
        {
            {service.m_name, ServiceMain},
            {NULL, NULL}};

    // Connects the main thread of a service process to the service control
    // manager, which causes the thread to be the service control dispatcher
    // thread for the calling process. This call returns when the service has
    // stopped. The process should simply terminate when the call returns.
    return StartServiceCtrlDispatcher(serviceTable);
}

/**
 *   Entry point for the service. It registers the handler function
 *   for the service and starts the service.
 *
 *   @param dwArgc: number of command line arguments
 *   @param lpszArgv: array of command line arguments
 */
void WINAPI ServiceBase::ServiceMain(DWORD dwArgc, PWSTR *pszArgv)
{
    assert(s_service != NULL);

    // Register the handler function for the service
    s_service->m_statusHandle = RegisterServiceCtrlHandler(
        s_service->m_name, ServiceCtrlHandler);
    if (s_service->m_statusHandle == NULL)
    {
        throw GetLastError();
    }

    // Start the service.
    s_service->Start(dwArgc, pszArgv);
}

/**
 *   This function is called by the SCM whenever a control code is
 *   sent to the service.
 *
 *   @param dwCtrlCode: the control code. This parameter can be one of the
 *   following values:
 *
 *     SERVICE_CONTROL_CONTINUE
 *     SERVICE_CONTROL_INTERROGATE
 *     SERVICE_CONTROL_NETBINDADD
 *     SERVICE_CONTROL_NETBINDDISABLE
 *     SERVICE_CONTROL_NETBINDREMOVE
 *     SERVICE_CONTROL_PARAMCHANGE
 *     SERVICE_CONTROL_PAUSE
 *     SERVICE_CONTROL_SHUTDOWN
 *     SERVICE_CONTROL_STOP
 *
 *   This parameter can also be a user-defined control code ranges from 128
 *   to 255.
 */
void WINAPI ServiceBase::ServiceCtrlHandler(DWORD dwCtrl)
{
    switch (dwCtrl)
    {
    case SERVICE_CONTROL_STOP:
        s_service->Stop();
        break;
    case SERVICE_CONTROL_PAUSE:
        s_service->Pause();
        break;
    case SERVICE_CONTROL_CONTINUE:
        s_service->Continue();
        break;
    case SERVICE_CONTROL_SHUTDOWN:
        s_service->Shutdown();
        break;
    case SERVICE_CONTROL_INTERROGATE:
        break;
    default:
        break;
    }
}

#pragma endregion

#pragma region Service Constructor and Destructor

/**
 *   The constructor of ServiceBase. It initializes a new instance
 *   of the ServiceBase class. The optional parameters (fCanStop,
 *   fCanShutdown and fCanPauseContinue) allow you to specify whether the
 *   service can be stopped, paused and continued, or be notified when system
 *   shutdown occurs.
 *
 *   @param pszServiceName: the name of the service
 *   @param fCanStop: the service can be stopped
 *   @param fCanShutdown: the service is notified when system shutdown occurs
 *   @param fCanPauseContinue: the service can be paused and continued
 */
ServiceBase::ServiceBase(PWSTR pszServiceName,
                         BOOL fCanStop,
                         BOOL fCanShutdown,
                         BOOL fCanPauseContinue)
{
    // Service name must be a valid string and cannot be NULL.
    m_name = (pszServiceName == NULL) ? const_cast<PWSTR>(L"") : pszServiceName;

    m_statusHandle = NULL;

    // The service runs in its own process.
    m_status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;

    // The service is starting.
    m_status.dwCurrentState = SERVICE_START_PENDING;

    // The accepted commands of the service.
    DWORD dwControlsAccepted = 0;
    if (fCanStop)
        dwControlsAccepted |= SERVICE_ACCEPT_STOP;
    if (fCanShutdown)
        dwControlsAccepted |= SERVICE_ACCEPT_SHUTDOWN;
    if (fCanPauseContinue)
        dwControlsAccepted |= SERVICE_ACCEPT_PAUSE_CONTINUE;
    m_status.dwControlsAccepted = dwControlsAccepted;

    m_status.dwWin32ExitCode = NO_ERROR;
    m_status.dwServiceSpecificExitCode = 0;
    m_status.dwCheckPoint = 0;
    m_status.dwWaitHint = 0;
}

/**
 *   The virtual destructor of ServiceBase.
 */
ServiceBase::~ServiceBase(void)
{
}

#pragma endregion

#pragma region Service Start, Stop, Pause, Continue, and Shutdown

/*
 *   This function starts the service. It calls the OnStart virtual
 *   function in which you can specify the actions to take when the service
 *   starts. If an error occurs during the startup, the error will be logged
 *   in the Application event log, and the service will be stopped.
 *
 *
 *   @param dwArgc   - number of command line arguments
 *   @param lpszArgv - array of command line arguments
 */
void ServiceBase::Start(DWORD dwArgc, PWSTR *pszArgv)
{
    try
    {
        // Tell SCM that the service is starting.
        SetServiceStatus(SERVICE_START_PENDING);

        // Perform service-specific initialization.
        OnStart(dwArgc, pszArgv);

        // Tell SCM that the service is started.
        SetServiceStatus(SERVICE_RUNNING);
    }
    catch (DWORD dwError)
    {
        // Log the error.
        WriteErrorLogEntry(L"Service Start", dwError);

        // Set the service status to be stopped.
        SetServiceStatus(SERVICE_STOPPED, dwError);
    }
    catch (...)
    {
        // Log the error.
        WriteEventLogEntry(L"Service failed to start.", EVENTLOG_ERROR_TYPE);

        // Set the service status to be stopped.
        SetServiceStatus(SERVICE_STOPPED);
    }
}

/**
 *   When implemented in a derived class, executes when a Start
 *   command is sent to the service by the SCM or when the operating system
 *   starts (for a service that starts automatically). Specifies actions to
 *   take when the service starts. Be sure to periodically call
 *   ServiceBase::SetServiceStatus() with SERVICE_START_PENDING if the
 *   procedure is going to take long time. You may also consider spawning a
 *   new thread in OnStart to perform time-consuming initialization tasks.
 *
 *   @param dwArgc   - number of command line arguments
 *   @param lpszArgv - array of command line arguments
 */
void ServiceBase::OnStart(DWORD dwArgc, PWSTR *pszArgv)
{
}

/**
 *   This function stops the service. It calls the OnStop virtual
 *   function in which you can specify the actions to take when the service
 *   stops. If an error occurs, the error will be logged in the Application
 *   event log, and the service will be restored to the original state.
 */
void ServiceBase::Stop()
{
    DWORD dwOriginalState = m_status.dwCurrentState;
    try
    {
        // Tell SCM that the service is stopping.
        SetServiceStatus(SERVICE_STOP_PENDING);

        // Perform service-specific stop operations.
        OnStop();

        // Tell SCM that the service is stopped.
        SetServiceStatus(SERVICE_STOPPED);
    }
    catch (DWORD dwError)
    {
        // Log the error.
        WriteErrorLogEntry(L"Service Stop", dwError);

        // Set the orginal service status.
        SetServiceStatus(dwOriginalState);
    }
    catch (...)
    {
        // Log the error.
        WriteEventLogEntry(L"Service failed to stop.", EVENTLOG_ERROR_TYPE);

        // Set the orginal service status.
        SetServiceStatus(dwOriginalState);
    }
}

/**
 *   When implemented in a derived class, executes when a Stop
 *   command is sent to the service by the SCM. Specifies actions to take
 *   when a service stops running. Be sure to periodically call
 *   ServiceBase::SetServiceStatus() with SERVICE_STOP_PENDING if the
 *   procedure is going to take long time.
 */
void ServiceBase::OnStop()
{
}

/**
 *   This function pauses the service if the service supports pause
 *   and continue. It calls the OnPause virtual function in which you can
 *   specify the actions to take when the service pauses. If an error occurs,
 *   the error will be logged in the Application event log, and the service
 *   will become running.
 *
 *   @param: none
 */
void ServiceBase::Pause()
{
    try
    {
        // Tell SCM that the service is pausing.
        SetServiceStatus(SERVICE_PAUSE_PENDING);

        // Perform service-specific pause operations.
        OnPause();

        // Tell SCM that the service is paused.
        SetServiceStatus(SERVICE_PAUSED);
    }
    catch (DWORD dwError)
    {
        // Log the error.
        WriteErrorLogEntry(L"Service Pause", dwError);

        // Tell SCM that the service is still running.
        SetServiceStatus(SERVICE_RUNNING);
    }
    catch (...)
    {
        // Log the error.
        WriteEventLogEntry(L"Service failed to pause.", EVENTLOG_ERROR_TYPE);

        // Tell SCM that the service is still running.
        SetServiceStatus(SERVICE_RUNNING);
    }
}

/**
 *   When implemented in a derived class, executes when a Pause
 *   command is sent to the service by the SCM. Specifies actions to take
 *   when a service pauses.
 *
 *   @param: none
 */
void ServiceBase::OnPause()
{
}

/**
 *   This function resumes normal functioning after being paused if
 *   the service supports pause and continue. It calls the OnContinue virtual
 *   function in which you can specify the actions to take when the service
 *   continues. If an error occurs, the error will be logged in the
 *   Application event log, and the service will still be paused.
 *
 *   @param: none
 */
void ServiceBase::Continue()
{
    try
    {
        // Tell SCM that the service is resuming.
        SetServiceStatus(SERVICE_CONTINUE_PENDING);

        // Perform service-specific continue operations.
        OnContinue();

        // Tell SCM that the service is running.
        SetServiceStatus(SERVICE_RUNNING);
    }
    catch (DWORD dwError)
    {
        // Log the error.
        WriteErrorLogEntry(L"Service Continue", dwError);

        // Tell SCM that the service is still paused.
        SetServiceStatus(SERVICE_PAUSED);
    }
    catch (...)
    {
        // Log the error.
        WriteEventLogEntry(L"Service failed to resume.", EVENTLOG_ERROR_TYPE);

        // Tell SCM that the service is still paused.
        SetServiceStatus(SERVICE_PAUSED);
    }
}

/**
 *   When implemented in a derived class, OnContinue runs when a
 *   Continue command is sent to the service by the SCM. Specifies actions to
 *   take when a service resumes normal functioning after being paused.
 *
 *   @param: none
 */
void ServiceBase::OnContinue()
{
}

/**
 *   This function executes when the system is shutting down. It
 *   calls the OnShutdown virtual function in which you can specify what
 *   should occur immediately prior to the system shutting down. If an error
 *   occurs, the error will be logged in the Application event log.
 *
 *   @param: none
 */
void ServiceBase::Shutdown()
{
    try
    {
        // Perform service-specific shutdown operations.
        OnShutdown();

        // Tell SCM that the service is stopped.
        SetServiceStatus(SERVICE_STOPPED);
    }
    catch (DWORD dwError)
    {
        // Log the error.
        WriteErrorLogEntry(L"Service Shutdown", dwError);
    }
    catch (...)
    {
        // Log the error.
        WriteEventLogEntry(L"Service failed to shut down.", EVENTLOG_ERROR_TYPE);
    }
}

/**
 *   When implemented in a derived class, executes when the system
 *   is shutting down. Specifies what should occur immediately prior to the
 *   system shutting down.
 *
 *   @param: none
 */
void ServiceBase::OnShutdown()
{
}

#pragma endregion

#pragma region Helper Functions

/**
 *   This function sets the service status and reports the status to
 *   the SCM.
 *
 *   @param dwCurrentState - the state of the service
 *   @param dwWin32ExitCode - error code to report
 *   @param dwWaitHint - estimated time for pending operation, in milliseconds
 */
void ServiceBase::SetServiceStatus(DWORD dwCurrentState,
                                   DWORD dwWin32ExitCode,
                                   DWORD dwWaitHint)
{
    static DWORD dwCheckPoint = 1;

    // Fill in the SERVICE_STATUS structure of the service.

    m_status.dwCurrentState = dwCurrentState;
    m_status.dwWin32ExitCode = dwWin32ExitCode;
    m_status.dwWaitHint = dwWaitHint;

    m_status.dwCheckPoint =
        ((dwCurrentState == SERVICE_RUNNING) ||
         (dwCurrentState == SERVICE_STOPPED))
            ? 0
            : dwCheckPoint++;

    // Report the status of the service to the SCM.
    ::SetServiceStatus(m_statusHandle, &m_status);
}

/**
 *   Log a message to the Application event log.
 *
 *   @param pszMessage: string message to be logged.
 *   @param wType: the type of event to be logged. The parameter can be
 *   one ofthe following values.
 *
 *     EVENTLOG_SUCCESS
 *     EVENTLOG_AUDIT_FAILURE
 *     EVENTLOG_AUDIT_SUCCESS
 *     EVENTLOG_ERROR_TYPE
 *     EVENTLOG_INFORMATION_TYPE
 *     EVENTLOG_WARNING_TYPE
 */
void ServiceBase::WriteEventLogEntry(const wchar_t pszMessage[], WORD wType)
{
    HANDLE hEventSource = NULL;
    LPCWSTR lpszStrings[2] = {NULL, NULL};

    hEventSource = RegisterEventSource(NULL, m_name);
    if (hEventSource)
    {
        lpszStrings[0] = m_name;
        lpszStrings[1] = pszMessage;

        ReportEvent(hEventSource, // Event log handle
                    wType,        // Event type
                    0,            // Event category
                    0,            // Event identifier
                    NULL,         // No security identifier
                    2,            // Size of lpszStrings array
                    0,            // No binary data
                    lpszStrings,  // Array of strings
                    NULL          // No binary data
        );

        DeregisterEventSource(hEventSource);
    }
}

/**
 *   Log an error message to the Application event log.
 *
 *   @param pszFunction - the function that gives the error
 *   @param dwError - the error code
 */
void ServiceBase::WriteErrorLogEntry(const wchar_t pszFunction[], DWORD dwError)
{
    wchar_t szMessage[260];
    StringCchPrintf(szMessage, ARRAYSIZE(szMessage),
                    L"%s failed w/err 0x%08lx", pszFunction, dwError);
    WriteEventLogEntry(szMessage, EVENTLOG_ERROR_TYPE);
}

#pragma endregion