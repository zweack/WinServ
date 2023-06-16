#pragma once

#include "ServiceBase.h"


class WinService : public ServiceBase
{
public:
    WinService(PWSTR pszServiceName,
        BOOL fCanStop = TRUE,
        BOOL fCanShutdown = TRUE,
        BOOL fCanPauseContinue = FALSE);
    virtual ~WinService(void);

protected:
    virtual void OnStart(DWORD dwArgc, LPWSTR* pszArgv);
    virtual void OnStop();
    void ServiceWorkerThread(void);

private:
    BOOL m_fStopping;
    HANDLE m_hStoppedEvent;
};