#pragma region Includes
#include <stdio.h>
#include <windows.h>
#include "ServiceBase.h"
#include "WinService.h"
#pragma endregion

// Settings of the service

// Internal name of the service
#define SERVICE_NAME L"SampleWindowsService"

// Displayed name of the service
#define SERVICE_DISPLAY_NAME L"Sample Windows Service"

// Service start options.
#define SERVICE_START_TYPE SERVICE_DEMAND_START

// List of service dependencies
#define SERVICE_DEPENDENCIES L""

// The name of the account under which the service should run
#define SERVICE_ACCOUNT L"LocalSystem"

// The password to the service account name
#define SERVICE_PASSWORD NULL

/*
 *   Install the current application as a service to the local service
 *   control manager database. If the function fails to install the
 *   service, it prints the error  in the standard output stream
 *
 *   @param pszServiceName - the name of the service to be installed
 *   @param pszDisplayName - the display name of the service
 *   @param dwStartType - the service start option. This parameter can be one of
 *   the following values: SERVICE_AUTO_START, SERVICE_BOOT_START,
 *   SERVICE_DEMAND_START, SERVICE_DISABLED, SERVICE_SYSTEM_START.
 *   @param pszDependencies - a pointer to a double null-terminated array of null-
 *   separated names of services or load ordering groups that the system
 *   must start before this service.
 *   @param pszAccount - the name of the account under which the service runs.
 *   @param pszPassword - the password to the account name.
 */
void InstallService(PWSTR pszServiceName,
                    PWSTR pszDisplayName,
                    DWORD dwStartType,
                    PWSTR pszDependencies,
                    PWSTR pszAccount,
                    PWSTR pszPassword)
{
    wchar_t szPath[MAX_PATH];
    SC_HANDLE schSCManager = NULL;
    SC_HANDLE schService = NULL;

    if (GetModuleFileName(NULL, szPath, ARRAYSIZE(szPath)) == 0)
    {
        wprintf(L"GetModuleFileName failed w/err 0x%08lx\n", GetLastError());
        goto Cleanup;
    }

    // Open the local default service control manager database
    schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT | SC_MANAGER_CREATE_SERVICE);
    if (schSCManager == NULL)
    {
        wprintf(L"OpenSCManager failed w/err 0x%08lx\n", GetLastError());
        goto Cleanup;
    }

    // Install the service into SCM by calling CreateService
    schService = CreateService(
        schSCManager,              // SCManager database
        pszServiceName,            // Name of service
        pszDisplayName,            // Name to display
        SERVICE_QUERY_STATUS,      // Desired access
        SERVICE_WIN32_OWN_PROCESS, // Service type
        dwStartType,               // Service start type
        SERVICE_ERROR_NORMAL,      // Error control type
        szPath,                    // Service's binary
        NULL,                      // No load ordering group
        NULL,                      // No tag identifier
        pszDependencies,           // Dependencies
        pszAccount,                // Service running account
        pszPassword                // Password of the account
    );
    if (schService == NULL)
    {
        wprintf(L"CreateService failed w/err 0x%08lx\n", GetLastError());
        goto Cleanup;
    }

    wprintf(L"%s is installed.\n", pszServiceName);

Cleanup:
    // Centralized cleanup for all allocated resources.
    if (schSCManager)
    {
        CloseServiceHandle(schSCManager);
        schSCManager = NULL;
    }
    if (schService)
    {
        CloseServiceHandle(schService);
        schService = NULL;
    }
}

/*
 *   Stop and remove the service from the local service control
 *   manager database. If the function fails to uninstall the service,
 *   it prints the error in the standard output stream.
 *
 *   @param pszServiceName - the name of the service to be removed.
 */
void UninstallService(PWSTR pszServiceName)
{
    SC_HANDLE schSCManager = NULL;
    SC_HANDLE schService = NULL;
    SERVICE_STATUS ssSvcStatus = {};

    // Open the local default service control manager database
    schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (schSCManager == NULL)
    {
        wprintf(L"OpenSCManager failed w/err 0x%08lx\n", GetLastError());
        goto Cleanup;
    }

    // Open the service with delete, stop, and query status permissions
    schService = OpenService(schSCManager, pszServiceName, SERVICE_STOP | SERVICE_QUERY_STATUS | DELETE);
    if (schService == NULL)
    {
        wprintf(L"OpenService failed w/err 0x%08lx\n", GetLastError());
        goto Cleanup;
    }

    // Try to stop the service
    if (ControlService(schService, SERVICE_CONTROL_STOP, &ssSvcStatus))
    {
        wprintf(L"Stopping %s.", pszServiceName);
        Sleep(1000);

        while (QueryServiceStatus(schService, &ssSvcStatus))
        {
            if (ssSvcStatus.dwCurrentState == SERVICE_STOP_PENDING)
            {
                wprintf(L".");
                Sleep(1000);
            }
            else
                break;
        }

        if (ssSvcStatus.dwCurrentState == SERVICE_STOPPED)
        {
            wprintf(L"\n%s is stopped.\n", pszServiceName);
        }
        else
        {
            wprintf(L"\n%s failed to stop.\n", pszServiceName);
        }
    }

    // Now remove the service by calling DeleteService.
    if (!DeleteService(schService))
    {
        wprintf(L"DeleteService failed w/err 0x%08lx\n", GetLastError());
        goto Cleanup;
    }

    wprintf(L"%s is removed.\n", pszServiceName);

Cleanup:
    // Centralized cleanup for all allocated resources.
    if (schSCManager)
    {
        CloseServiceHandle(schSCManager);
        schSCManager = NULL;
    }
    if (schService)
    {
        CloseServiceHandle(schService);
        schService = NULL;
    }
}

/**
 *   Entrypoint for the application.
 *
 *   @param  argc: number of command line arguments
 *   @param  argv: array of command line arguments
 *   @return none
 */
int wmain(int argc, wchar_t *argv[])
{
    if ((argc > 1) && ((*argv[1] == L'-' || (*argv[1] == L'/'))))
    {
        if (_wcsicmp(L"install", argv[1] + 1) == 0)
        {
            // Install the service when the command is
            // "-install" or "/install".
            InstallService(
                const_cast<PWSTR>(SERVICE_NAME),         // Name of service
                const_cast<PWSTR>(SERVICE_DISPLAY_NAME), // Name to display
                SERVICE_START_TYPE,                      // Service start type
                const_cast<PWSTR>(SERVICE_DEPENDENCIES), // Dependencies
                const_cast<PWSTR>(SERVICE_ACCOUNT),      // Service running account
                SERVICE_PASSWORD                         // Password of the account
            );
        }
        else if (_wcsicmp(L"remove", argv[1] + 1) == 0)
        {
            // Uninstall the service when the command is
            // "-remove" or "/remove".
            UninstallService(const_cast<PWSTR>(SERVICE_NAME));
        }
    }
    else
    {
        wprintf(L"Parameters:\n");
        wprintf(L" -install  to install the service.\n");
        wprintf(L" -remove   to remove the service.\n");

        WinService service(const_cast<PWSTR>(SERVICE_NAME));
        if (!ServiceBase::Run(service))
        {
            wprintf(L"Service failed to run w/err 0x%08lx\n", GetLastError());
        }
    }

    return 0;
}