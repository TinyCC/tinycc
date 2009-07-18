/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within this package.
 */
#ifndef _WINSVC_
#define _WINSVC_

#ifndef WINADVAPI
#define WINADVAPI DECLSPEC_IMPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define SERVICES_ACTIVE_DATABASEW L"ServicesActive"
#define SERVICES_FAILED_DATABASEW L"ServicesFailed"

#define SERVICES_ACTIVE_DATABASEA "ServicesActive"
#define SERVICES_FAILED_DATABASEA "ServicesFailed"

#define SC_GROUP_IDENTIFIERW L'+'
#define SC_GROUP_IDENTIFIERA '+'

#ifdef UNICODE
#define SERVICES_ACTIVE_DATABASE SERVICES_ACTIVE_DATABASEW
#define SERVICES_FAILED_DATABASE SERVICES_FAILED_DATABASEW

#define SC_GROUP_IDENTIFIER SC_GROUP_IDENTIFIERW
#else
#define SERVICES_ACTIVE_DATABASE SERVICES_ACTIVE_DATABASEA
#define SERVICES_FAILED_DATABASE SERVICES_FAILED_DATABASEA

#define SC_GROUP_IDENTIFIER SC_GROUP_IDENTIFIERA
#endif

#define SERVICE_NO_CHANGE 0xffffffff

#define SERVICE_ACTIVE 0x00000001
#define SERVICE_INACTIVE 0x00000002
#define SERVICE_STATE_ALL (SERVICE_ACTIVE | SERVICE_INACTIVE)

#define SERVICE_CONTROL_STOP 0x00000001
#define SERVICE_CONTROL_PAUSE 0x00000002
#define SERVICE_CONTROL_CONTINUE 0x00000003
#define SERVICE_CONTROL_INTERROGATE 0x00000004
#define SERVICE_CONTROL_SHUTDOWN 0x00000005
#define SERVICE_CONTROL_PARAMCHANGE 0x00000006
#define SERVICE_CONTROL_NETBINDADD 0x00000007
#define SERVICE_CONTROL_NETBINDREMOVE 0x00000008
#define SERVICE_CONTROL_NETBINDENABLE 0x00000009
#define SERVICE_CONTROL_NETBINDDISABLE 0x0000000A
#define SERVICE_CONTROL_DEVICEEVENT 0x0000000B
#define SERVICE_CONTROL_HARDWAREPROFILECHANGE 0x0000000C
#define SERVICE_CONTROL_POWEREVENT 0x0000000D
#define SERVICE_CONTROL_SESSIONCHANGE 0x0000000E

#define SERVICE_STOPPED 0x00000001
#define SERVICE_START_PENDING 0x00000002
#define SERVICE_STOP_PENDING 0x00000003
#define SERVICE_RUNNING 0x00000004
#define SERVICE_CONTINUE_PENDING 0x00000005
#define SERVICE_PAUSE_PENDING 0x00000006
#define SERVICE_PAUSED 0x00000007

#define SERVICE_ACCEPT_STOP 0x00000001
#define SERVICE_ACCEPT_PAUSE_CONTINUE 0x00000002
#define SERVICE_ACCEPT_SHUTDOWN 0x00000004
#define SERVICE_ACCEPT_PARAMCHANGE 0x00000008
#define SERVICE_ACCEPT_NETBINDCHANGE 0x00000010
#define SERVICE_ACCEPT_HARDWAREPROFILECHANGE 0x00000020
#define SERVICE_ACCEPT_POWEREVENT 0x00000040
#define SERVICE_ACCEPT_SESSIONCHANGE 0x00000080

#define SC_MANAGER_CONNECT 0x0001
#define SC_MANAGER_CREATE_SERVICE 0x0002
#define SC_MANAGER_ENUMERATE_SERVICE 0x0004
#define SC_MANAGER_LOCK 0x0008
#define SC_MANAGER_QUERY_LOCK_STATUS 0x0010
#define SC_MANAGER_MODIFY_BOOT_CONFIG 0x0020

#define SC_MANAGER_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | SC_MANAGER_CONNECT | SC_MANAGER_CREATE_SERVICE | SC_MANAGER_ENUMERATE_SERVICE | SC_MANAGER_LOCK | SC_MANAGER_QUERY_LOCK_STATUS | SC_MANAGER_MODIFY_BOOT_CONFIG)

#define SERVICE_QUERY_CONFIG 0x0001
#define SERVICE_CHANGE_CONFIG 0x0002
#define SERVICE_QUERY_STATUS 0x0004
#define SERVICE_ENUMERATE_DEPENDENTS 0x0008
#define SERVICE_START 0x0010
#define SERVICE_STOP 0x0020
#define SERVICE_PAUSE_CONTINUE 0x0040
#define SERVICE_INTERROGATE 0x0080
#define SERVICE_USER_DEFINED_CONTROL 0x0100

#define SERVICE_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | SERVICE_QUERY_CONFIG | SERVICE_CHANGE_CONFIG | SERVICE_QUERY_STATUS | SERVICE_ENUMERATE_DEPENDENTS | SERVICE_START | SERVICE_STOP | SERVICE_PAUSE_CONTINUE | SERVICE_INTERROGATE | SERVICE_USER_DEFINED_CONTROL)

#define SERVICE_RUNS_IN_SYSTEM_PROCESS 0x00000001

#define SERVICE_CONFIG_DESCRIPTION 1
#define SERVICE_CONFIG_FAILURE_ACTIONS 2

  typedef struct _SERVICE_DESCRIPTIONA {
    LPSTR lpDescription;
  } SERVICE_DESCRIPTIONA,*LPSERVICE_DESCRIPTIONA;

  typedef struct _SERVICE_DESCRIPTIONW {
    LPWSTR lpDescription;
  } SERVICE_DESCRIPTIONW,*LPSERVICE_DESCRIPTIONW;

#ifdef UNICODE
  typedef SERVICE_DESCRIPTIONW SERVICE_DESCRIPTION;
  typedef LPSERVICE_DESCRIPTIONW LPSERVICE_DESCRIPTION;
#else
  typedef SERVICE_DESCRIPTIONA SERVICE_DESCRIPTION;
  typedef LPSERVICE_DESCRIPTIONA LPSERVICE_DESCRIPTION;
#endif

  typedef enum _SC_ACTION_TYPE {
    SC_ACTION_NONE = 0,SC_ACTION_RESTART = 1,SC_ACTION_REBOOT = 2,SC_ACTION_RUN_COMMAND = 3
  } SC_ACTION_TYPE;

  typedef struct _SC_ACTION {
    SC_ACTION_TYPE Type;
    DWORD Delay;
  } SC_ACTION,*LPSC_ACTION;

  typedef struct _SERVICE_FAILURE_ACTIONSA {
    DWORD dwResetPeriod;
    LPSTR lpRebootMsg;
    LPSTR lpCommand;
    DWORD cActions;
    SC_ACTION *lpsaActions;
  } SERVICE_FAILURE_ACTIONSA,*LPSERVICE_FAILURE_ACTIONSA;

  typedef struct _SERVICE_FAILURE_ACTIONSW {
    DWORD dwResetPeriod;
    LPWSTR lpRebootMsg;
    LPWSTR lpCommand;
    DWORD cActions;
    SC_ACTION *lpsaActions;
  } SERVICE_FAILURE_ACTIONSW,*LPSERVICE_FAILURE_ACTIONSW;

#ifdef UNICODE
  typedef SERVICE_FAILURE_ACTIONSW SERVICE_FAILURE_ACTIONS;
  typedef LPSERVICE_FAILURE_ACTIONSW LPSERVICE_FAILURE_ACTIONS;
#else
  typedef SERVICE_FAILURE_ACTIONSA SERVICE_FAILURE_ACTIONS;
  typedef LPSERVICE_FAILURE_ACTIONSA LPSERVICE_FAILURE_ACTIONS;
#endif

  DECLARE_HANDLE(SC_HANDLE);
  typedef SC_HANDLE *LPSC_HANDLE;

  DECLARE_HANDLE(SERVICE_STATUS_HANDLE);

  typedef enum _SC_STATUS_TYPE {
    SC_STATUS_PROCESS_INFO = 0
  } SC_STATUS_TYPE;

  typedef enum _SC_ENUM_TYPE {
    SC_ENUM_PROCESS_INFO = 0
  } SC_ENUM_TYPE;

  typedef struct _SERVICE_STATUS {
    DWORD dwServiceType;
    DWORD dwCurrentState;
    DWORD dwControlsAccepted;
    DWORD dwWin32ExitCode;
    DWORD dwServiceSpecificExitCode;
    DWORD dwCheckPoint;
    DWORD dwWaitHint;
  } SERVICE_STATUS,*LPSERVICE_STATUS;

  typedef struct _SERVICE_STATUS_PROCESS {
    DWORD dwServiceType;
    DWORD dwCurrentState;
    DWORD dwControlsAccepted;
    DWORD dwWin32ExitCode;
    DWORD dwServiceSpecificExitCode;
    DWORD dwCheckPoint;
    DWORD dwWaitHint;
    DWORD dwProcessId;
    DWORD dwServiceFlags;
  } SERVICE_STATUS_PROCESS,*LPSERVICE_STATUS_PROCESS;

  typedef struct _ENUM_SERVICE_STATUSA {
    LPSTR lpServiceName;
    LPSTR lpDisplayName;
    SERVICE_STATUS ServiceStatus;
  } ENUM_SERVICE_STATUSA,*LPENUM_SERVICE_STATUSA;

  typedef struct _ENUM_SERVICE_STATUSW {
    LPWSTR lpServiceName;
    LPWSTR lpDisplayName;
    SERVICE_STATUS ServiceStatus;
  } ENUM_SERVICE_STATUSW,*LPENUM_SERVICE_STATUSW;

#ifdef UNICODE
  typedef ENUM_SERVICE_STATUSW ENUM_SERVICE_STATUS;
  typedef LPENUM_SERVICE_STATUSW LPENUM_SERVICE_STATUS;
#else
  typedef ENUM_SERVICE_STATUSA ENUM_SERVICE_STATUS;
  typedef LPENUM_SERVICE_STATUSA LPENUM_SERVICE_STATUS;
#endif

  typedef struct _ENUM_SERVICE_STATUS_PROCESSA {
    LPSTR lpServiceName;
    LPSTR lpDisplayName;
    SERVICE_STATUS_PROCESS ServiceStatusProcess;
  } ENUM_SERVICE_STATUS_PROCESSA,*LPENUM_SERVICE_STATUS_PROCESSA;

  typedef struct _ENUM_SERVICE_STATUS_PROCESSW {
    LPWSTR lpServiceName;
    LPWSTR lpDisplayName;
    SERVICE_STATUS_PROCESS ServiceStatusProcess;
  } ENUM_SERVICE_STATUS_PROCESSW,*LPENUM_SERVICE_STATUS_PROCESSW;

#ifdef UNICODE
  typedef ENUM_SERVICE_STATUS_PROCESSW ENUM_SERVICE_STATUS_PROCESS;
  typedef LPENUM_SERVICE_STATUS_PROCESSW LPENUM_SERVICE_STATUS_PROCESS;
#else
  typedef ENUM_SERVICE_STATUS_PROCESSA ENUM_SERVICE_STATUS_PROCESS;
  typedef LPENUM_SERVICE_STATUS_PROCESSA LPENUM_SERVICE_STATUS_PROCESS;
#endif

  typedef LPVOID SC_LOCK;

  typedef struct _QUERY_SERVICE_LOCK_STATUSA {
    DWORD fIsLocked;
    LPSTR lpLockOwner;
    DWORD dwLockDuration;
  } QUERY_SERVICE_LOCK_STATUSA,*LPQUERY_SERVICE_LOCK_STATUSA;

  typedef struct _QUERY_SERVICE_LOCK_STATUSW {
    DWORD fIsLocked;
    LPWSTR lpLockOwner;
    DWORD dwLockDuration;
  } QUERY_SERVICE_LOCK_STATUSW,*LPQUERY_SERVICE_LOCK_STATUSW;

#ifdef UNICODE
  typedef QUERY_SERVICE_LOCK_STATUSW QUERY_SERVICE_LOCK_STATUS;
  typedef LPQUERY_SERVICE_LOCK_STATUSW LPQUERY_SERVICE_LOCK_STATUS;
#else
  typedef QUERY_SERVICE_LOCK_STATUSA QUERY_SERVICE_LOCK_STATUS;
  typedef LPQUERY_SERVICE_LOCK_STATUSA LPQUERY_SERVICE_LOCK_STATUS;
#endif

  typedef struct _QUERY_SERVICE_CONFIGA {
    DWORD dwServiceType;
    DWORD dwStartType;
    DWORD dwErrorControl;
    LPSTR lpBinaryPathName;
    LPSTR lpLoadOrderGroup;
    DWORD dwTagId;
    LPSTR lpDependencies;
    LPSTR lpServiceStartName;
    LPSTR lpDisplayName;
  } QUERY_SERVICE_CONFIGA,*LPQUERY_SERVICE_CONFIGA;

  typedef struct _QUERY_SERVICE_CONFIGW {
    DWORD dwServiceType;
    DWORD dwStartType;
    DWORD dwErrorControl;
    LPWSTR lpBinaryPathName;
    LPWSTR lpLoadOrderGroup;
    DWORD dwTagId;
    LPWSTR lpDependencies;
    LPWSTR lpServiceStartName;
    LPWSTR lpDisplayName;
  } QUERY_SERVICE_CONFIGW,*LPQUERY_SERVICE_CONFIGW;

#ifdef UNICODE
  typedef QUERY_SERVICE_CONFIGW QUERY_SERVICE_CONFIG;
  typedef LPQUERY_SERVICE_CONFIGW LPQUERY_SERVICE_CONFIG;
#else
  typedef QUERY_SERVICE_CONFIGA QUERY_SERVICE_CONFIG;
  typedef LPQUERY_SERVICE_CONFIGA LPQUERY_SERVICE_CONFIG;
#endif

  typedef VOID (WINAPI *LPSERVICE_MAIN_FUNCTIONW)(DWORD dwNumServicesArgs,LPWSTR *lpServiceArgVectors);
  typedef VOID (WINAPI *LPSERVICE_MAIN_FUNCTIONA)(DWORD dwNumServicesArgs,LPSTR *lpServiceArgVectors);

#ifdef UNICODE
#define LPSERVICE_MAIN_FUNCTION LPSERVICE_MAIN_FUNCTIONW
#else
#define LPSERVICE_MAIN_FUNCTION LPSERVICE_MAIN_FUNCTIONA
#endif

  typedef struct _SERVICE_TABLE_ENTRYA {
    LPSTR lpServiceName;
    LPSERVICE_MAIN_FUNCTIONA lpServiceProc;
  } SERVICE_TABLE_ENTRYA,*LPSERVICE_TABLE_ENTRYA;

  typedef struct _SERVICE_TABLE_ENTRYW {
    LPWSTR lpServiceName;
    LPSERVICE_MAIN_FUNCTIONW lpServiceProc;
  } SERVICE_TABLE_ENTRYW,*LPSERVICE_TABLE_ENTRYW;

#ifdef UNICODE
  typedef SERVICE_TABLE_ENTRYW SERVICE_TABLE_ENTRY;
  typedef LPSERVICE_TABLE_ENTRYW LPSERVICE_TABLE_ENTRY;
#else
  typedef SERVICE_TABLE_ENTRYA SERVICE_TABLE_ENTRY;
  typedef LPSERVICE_TABLE_ENTRYA LPSERVICE_TABLE_ENTRY;
#endif

  typedef VOID (WINAPI *LPHANDLER_FUNCTION)(DWORD dwControl);
  typedef DWORD (WINAPI *LPHANDLER_FUNCTION_EX)(DWORD dwControl,DWORD dwEventType,LPVOID lpEventData,LPVOID lpContext);

#ifdef UNICODE
#define ChangeServiceConfig ChangeServiceConfigW
#define ChangeServiceConfig2 ChangeServiceConfig2W
#define CreateService CreateServiceW
#define EnumDependentServices EnumDependentServicesW
#define EnumServicesStatus EnumServicesStatusW
#define EnumServicesStatusEx EnumServicesStatusExW
#define GetServiceKeyName GetServiceKeyNameW
#define GetServiceDisplayName GetServiceDisplayNameW
#define OpenSCManager OpenSCManagerW
#define OpenService OpenServiceW
#define QueryServiceConfig QueryServiceConfigW
#define QueryServiceConfig2 QueryServiceConfig2W
#define QueryServiceLockStatus QueryServiceLockStatusW
#define RegisterServiceCtrlHandler RegisterServiceCtrlHandlerW
#define RegisterServiceCtrlHandlerEx RegisterServiceCtrlHandlerExW
#define StartServiceCtrlDispatcher StartServiceCtrlDispatcherW
#define StartService StartServiceW
#else
#define ChangeServiceConfig ChangeServiceConfigA
#define ChangeServiceConfig2 ChangeServiceConfig2A
#define CreateService CreateServiceA
#define EnumDependentServices EnumDependentServicesA
#define EnumServicesStatus EnumServicesStatusA
#define EnumServicesStatusEx EnumServicesStatusExA
#define GetServiceKeyName GetServiceKeyNameA
#define GetServiceDisplayName GetServiceDisplayNameA
#define OpenSCManager OpenSCManagerA
#define OpenService OpenServiceA
#define QueryServiceConfig QueryServiceConfigA
#define QueryServiceConfig2 QueryServiceConfig2A
#define QueryServiceLockStatus QueryServiceLockStatusA
#define RegisterServiceCtrlHandler RegisterServiceCtrlHandlerA
#define RegisterServiceCtrlHandlerEx RegisterServiceCtrlHandlerExA
#define StartServiceCtrlDispatcher StartServiceCtrlDispatcherA
#define StartService StartServiceA
#endif

  WINADVAPI WINBOOL WINAPI ChangeServiceConfigA(SC_HANDLE hService,DWORD dwServiceType,DWORD dwStartType,DWORD dwErrorControl,LPCSTR lpBinaryPathName,LPCSTR lpLoadOrderGroup,LPDWORD lpdwTagId,LPCSTR lpDependencies,LPCSTR lpServiceStartName,LPCSTR lpPassword,LPCSTR lpDisplayName);
  WINADVAPI WINBOOL WINAPI ChangeServiceConfigW(SC_HANDLE hService,DWORD dwServiceType,DWORD dwStartType,DWORD dwErrorControl,LPCWSTR lpBinaryPathName,LPCWSTR lpLoadOrderGroup,LPDWORD lpdwTagId,LPCWSTR lpDependencies,LPCWSTR lpServiceStartName,LPCWSTR lpPassword,LPCWSTR lpDisplayName);
  WINADVAPI WINBOOL WINAPI ChangeServiceConfig2A(SC_HANDLE hService,DWORD dwInfoLevel,LPVOID lpInfo);
  WINADVAPI WINBOOL WINAPI ChangeServiceConfig2W(SC_HANDLE hService,DWORD dwInfoLevel,LPVOID lpInfo);
  WINADVAPI WINBOOL WINAPI CloseServiceHandle(SC_HANDLE hSCObject);
  WINADVAPI WINBOOL WINAPI ControlService(SC_HANDLE hService,DWORD dwControl,LPSERVICE_STATUS lpServiceStatus);
  WINADVAPI SC_HANDLE WINAPI CreateServiceA(SC_HANDLE hSCManager,LPCSTR lpServiceName,LPCSTR lpDisplayName,DWORD dwDesiredAccess,DWORD dwServiceType,DWORD dwStartType,DWORD dwErrorControl,LPCSTR lpBinaryPathName,LPCSTR lpLoadOrderGroup,LPDWORD lpdwTagId,LPCSTR lpDependencies,LPCSTR lpServiceStartName,LPCSTR lpPassword);
  WINADVAPI SC_HANDLE WINAPI CreateServiceW(SC_HANDLE hSCManager,LPCWSTR lpServiceName,LPCWSTR lpDisplayName,DWORD dwDesiredAccess,DWORD dwServiceType,DWORD dwStartType,DWORD dwErrorControl,LPCWSTR lpBinaryPathName,LPCWSTR lpLoadOrderGroup,LPDWORD lpdwTagId,LPCWSTR lpDependencies,LPCWSTR lpServiceStartName,LPCWSTR lpPassword);
  WINADVAPI WINBOOL WINAPI DeleteService(SC_HANDLE hService);
  WINADVAPI WINBOOL WINAPI EnumDependentServicesA(SC_HANDLE hService,DWORD dwServiceState,LPENUM_SERVICE_STATUSA lpServices,DWORD cbBufSize,LPDWORD pcbBytesNeeded,LPDWORD lpServicesReturned);
  WINADVAPI WINBOOL WINAPI EnumDependentServicesW(SC_HANDLE hService,DWORD dwServiceState,LPENUM_SERVICE_STATUSW lpServices,DWORD cbBufSize,LPDWORD pcbBytesNeeded,LPDWORD lpServicesReturned);
  WINADVAPI WINBOOL WINAPI EnumServicesStatusA(SC_HANDLE hSCManager,DWORD dwServiceType,DWORD dwServiceState,LPENUM_SERVICE_STATUSA lpServices,DWORD cbBufSize,LPDWORD pcbBytesNeeded,LPDWORD lpServicesReturned,LPDWORD lpResumeHandle);
  WINADVAPI WINBOOL WINAPI EnumServicesStatusW(SC_HANDLE hSCManager,DWORD dwServiceType,DWORD dwServiceState,LPENUM_SERVICE_STATUSW lpServices,DWORD cbBufSize,LPDWORD pcbBytesNeeded,LPDWORD lpServicesReturned,LPDWORD lpResumeHandle);
  WINADVAPI WINBOOL WINAPI EnumServicesStatusExA(SC_HANDLE hSCManager,SC_ENUM_TYPE InfoLevel,DWORD dwServiceType,DWORD dwServiceState,LPBYTE lpServices,DWORD cbBufSize,LPDWORD pcbBytesNeeded,LPDWORD lpServicesReturned,LPDWORD lpResumeHandle,LPCSTR pszGroupName);
  WINADVAPI WINBOOL WINAPI EnumServicesStatusExW(SC_HANDLE hSCManager,SC_ENUM_TYPE InfoLevel,DWORD dwServiceType,DWORD dwServiceState,LPBYTE lpServices,DWORD cbBufSize,LPDWORD pcbBytesNeeded,LPDWORD lpServicesReturned,LPDWORD lpResumeHandle,LPCWSTR pszGroupName);
  WINADVAPI WINBOOL WINAPI GetServiceKeyNameA(SC_HANDLE hSCManager,LPCSTR lpDisplayName,LPSTR lpServiceName,LPDWORD lpcchBuffer);
  WINADVAPI WINBOOL WINAPI GetServiceKeyNameW(SC_HANDLE hSCManager,LPCWSTR lpDisplayName,LPWSTR lpServiceName,LPDWORD lpcchBuffer);
  WINADVAPI WINBOOL WINAPI GetServiceDisplayNameA(SC_HANDLE hSCManager,LPCSTR lpServiceName,LPSTR lpDisplayName,LPDWORD lpcchBuffer);
  WINADVAPI WINBOOL WINAPI GetServiceDisplayNameW(SC_HANDLE hSCManager,LPCWSTR lpServiceName,LPWSTR lpDisplayName,LPDWORD lpcchBuffer);
  WINADVAPI SC_LOCK WINAPI LockServiceDatabase(SC_HANDLE hSCManager);
  WINADVAPI WINBOOL WINAPI NotifyBootConfigStatus(WINBOOL BootAcceptable);
  WINADVAPI SC_HANDLE WINAPI OpenSCManagerA(LPCSTR lpMachineName,LPCSTR lpDatabaseName,DWORD dwDesiredAccess);
  WINADVAPI SC_HANDLE WINAPI OpenSCManagerW(LPCWSTR lpMachineName,LPCWSTR lpDatabaseName,DWORD dwDesiredAccess);
  WINADVAPI SC_HANDLE WINAPI OpenServiceA(SC_HANDLE hSCManager,LPCSTR lpServiceName,DWORD dwDesiredAccess);
  WINADVAPI SC_HANDLE WINAPI OpenServiceW(SC_HANDLE hSCManager,LPCWSTR lpServiceName,DWORD dwDesiredAccess);
  WINADVAPI WINBOOL WINAPI QueryServiceConfigA(SC_HANDLE hService,LPQUERY_SERVICE_CONFIGA lpServiceConfig,DWORD cbBufSize,LPDWORD pcbBytesNeeded);
  WINADVAPI WINBOOL WINAPI QueryServiceConfigW(SC_HANDLE hService,LPQUERY_SERVICE_CONFIGW lpServiceConfig,DWORD cbBufSize,LPDWORD pcbBytesNeeded);
  WINADVAPI WINBOOL WINAPI QueryServiceConfig2A(SC_HANDLE hService,DWORD dwInfoLevel,LPBYTE lpBuffer,DWORD cbBufSize,LPDWORD pcbBytesNeeded);
  WINADVAPI WINBOOL WINAPI QueryServiceConfig2W(SC_HANDLE hService,DWORD dwInfoLevel,LPBYTE lpBuffer,DWORD cbBufSize,LPDWORD pcbBytesNeeded);
  WINADVAPI WINBOOL WINAPI QueryServiceLockStatusA(SC_HANDLE hSCManager,LPQUERY_SERVICE_LOCK_STATUSA lpLockStatus,DWORD cbBufSize,LPDWORD pcbBytesNeeded);
  WINADVAPI WINBOOL WINAPI QueryServiceLockStatusW(SC_HANDLE hSCManager,LPQUERY_SERVICE_LOCK_STATUSW lpLockStatus,DWORD cbBufSize,LPDWORD pcbBytesNeeded);
  WINADVAPI WINBOOL WINAPI QueryServiceObjectSecurity(SC_HANDLE hService,SECURITY_INFORMATION dwSecurityInformation,PSECURITY_DESCRIPTOR lpSecurityDescriptor,DWORD cbBufSize,LPDWORD pcbBytesNeeded);
  WINADVAPI WINBOOL WINAPI QueryServiceStatus(SC_HANDLE hService,LPSERVICE_STATUS lpServiceStatus);
  WINADVAPI WINBOOL WINAPI QueryServiceStatusEx(SC_HANDLE hService,SC_STATUS_TYPE InfoLevel,LPBYTE lpBuffer,DWORD cbBufSize,LPDWORD pcbBytesNeeded);
  WINADVAPI SERVICE_STATUS_HANDLE WINAPI RegisterServiceCtrlHandlerA(LPCSTR lpServiceName,LPHANDLER_FUNCTION lpHandlerProc);
  WINADVAPI SERVICE_STATUS_HANDLE WINAPI RegisterServiceCtrlHandlerW(LPCWSTR lpServiceName,LPHANDLER_FUNCTION lpHandlerProc);
  WINADVAPI SERVICE_STATUS_HANDLE WINAPI RegisterServiceCtrlHandlerExA(LPCSTR lpServiceName,LPHANDLER_FUNCTION_EX lpHandlerProc,LPVOID lpContext);
  WINADVAPI SERVICE_STATUS_HANDLE WINAPI RegisterServiceCtrlHandlerExW(LPCWSTR lpServiceName,LPHANDLER_FUNCTION_EX lpHandlerProc,LPVOID lpContext);
  WINADVAPI WINBOOL WINAPI SetServiceObjectSecurity(SC_HANDLE hService,SECURITY_INFORMATION dwSecurityInformation,PSECURITY_DESCRIPTOR lpSecurityDescriptor);
  WINADVAPI WINBOOL WINAPI SetServiceStatus(SERVICE_STATUS_HANDLE hServiceStatus,LPSERVICE_STATUS lpServiceStatus);
  WINADVAPI WINBOOL WINAPI StartServiceCtrlDispatcherA(CONST SERVICE_TABLE_ENTRYA *lpServiceStartTable);
  WINADVAPI WINBOOL WINAPI StartServiceCtrlDispatcherW(CONST SERVICE_TABLE_ENTRYW *lpServiceStartTable);
  WINADVAPI WINBOOL WINAPI StartServiceA(SC_HANDLE hService,DWORD dwNumServiceArgs,LPCSTR *lpServiceArgVectors);
  WINADVAPI WINBOOL WINAPI StartServiceW(SC_HANDLE hService,DWORD dwNumServiceArgs,LPCWSTR *lpServiceArgVectors);
  WINADVAPI WINBOOL WINAPI UnlockServiceDatabase(SC_LOCK ScLock);

#ifdef __cplusplus
}
#endif
#endif
