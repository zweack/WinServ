# WinServ
WinServ is a boilerplate for creating windows service in C++.

## Requirement
- Visual Studio 2019 or higher.
- Microsoft Visual C++
## Quick Start
### Define Service Identity
Open the code in Visual Studio. Go to entry point script at `WinServ/EntryPoint.cpp` and define following as per your service requirement:
```
SERVICE_NAME L"<your-service-name>"

SERVICE_DISPLAY_NAME L"<display name of your service>"

SERVICE_START_TYPE <service startup options>

SERVICE_DEPENDENCIES L"<list of service dependencies>"

SERVICE_ACCOUNT L"<service account name>"
```

### Update Service Logic

Go to `ServiceWorkerThread()` in `WinServ/WinService.cpp` and add your service logic. By default it simply prints "WinServ is running" every 50 seconds. 

### Update Service Startup and Termination (Optional)
If you want to execute any code when service starts or stops, you can add it in `OnStart()` and `OnStop()` function in `WinServ/WinService.cpp`

### Build
Build the project in Visual Studio and obtain the executable `WinServ.exe`.

Install Service

```
WinServ.exe -install
```
Now you can go to Service utility and start/stop your service or you can use sc utility in Command Prompt.

Uninstall Service

```
WinServ.exe -remove
```

## Logging
Windows has a utility [Event Viewer](https://www.windowscentral.com/software-apps/windows-11/how-to-get-started-with-event-viewer-on-windows-11) which is a legacy tool designed to aggregate event logs from apps and system components into an easily digestible structure. This service will log any error or output in event viewer. You can use syntax below to add logs in Event Viewer:
```
WriteEventLogEntry(L"Sample log message", EVENTLOG_INFORMATION_TYPE);
```
Following log types are supported:
```
EVENTLOG_SUCCESS                
EVENTLOG_ERROR_TYPE             
EVENTLOG_WARNING_TYPE           
EVENTLOG_INFORMATION_TYPE       
EVENTLOG_AUDIT_SUCCESS          
EVENTLOG_AUDIT_FAILURE 
```

## Contributing
This project welcomes contributions and suggestions. Please feel free to create a PR, report an issue or put up a feature request.

## License
WinServe is licensed under the MIT License.