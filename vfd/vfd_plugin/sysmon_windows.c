/* Windows system monitor for the VFD display music player plugin. */
/* Copyright 2013 Boris Gjenero. Released under the MIT license. */

#define __W32API_USE_DLLIMPORT__
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
/* #include <conio.h> */
#include <stdio.h>
#include <math.h>

#include <Tlhelp32.h>

#include <powrprof.h>

#ifdef _MSC_VER
#include "lrint.h"
#endif
#include "sysmon.h"

#ifdef __MINGW32__
ULONGLONG WINAPI GetTickCount64(void);
#endif

/* CPU */

/* This stuff is defined elsewhere in Microsoft header files.
 * Those will cause problems if included.
 */

#define SystemBasicInformation       0
#define SystemPerformanceInformation 2
#define SystemTimeInformation        3

#define Li2Double(x) ((double)((x).HighPart) * 4.294967296E9 + (double)((x).LowPart))

typedef struct
{
    DWORD   dwUnknown1;
    ULONG   uKeMaximumIncrement;
    ULONG   uPageSize;
    ULONG   uMmNumberOfPhysicalPages;
    ULONG   uMmLowestPhysicalPage;
    ULONG   uMmHighestPhysicalPage;
    ULONG   uAllocationGranularity;
    PVOID   pLowestUserAddress;
    PVOID   pMmHighestUserAddress;
    ULONG   uKeActiveProcessors;
    BYTE    bKeNumberProcessors;
    BYTE    bUnknown2;
    WORD    wUnknown3;
} SYSTEM_BASIC_INFORMATION;

typedef struct
{
    LARGE_INTEGER   liIdleTime;
    DWORD           dwSpare[76];
} SYSTEM_PERFORMANCE_INFORMATION;

typedef struct
{
    LARGE_INTEGER liKeBootTime;
    LARGE_INTEGER liKeSystemTime;
    LARGE_INTEGER liExpTimeZoneBias;
    ULONG         uCurrentTimeZoneId;
    DWORD         dwReserved;
} SYSTEM_TIME_INFORMATION;


// ntdll!NtQuerySystemInformation (NT specific!)
//
// The function copies the system information of the
// specified type into a buffer
//
// NTSYSAPI
// NTSTATUS
// NTAPI
// NtQuerySystemInformation(
//    IN UINT SystemInformationClass,    // information type
//    OUT PVOID SystemInformation,       // pointer to buffer
//    IN ULONG SystemInformationLength,  // buffer size in bytes
//    OUT PULONG ReturnLength OPTIONAL   // pointer to a 32-bit
//                                       // variable that receives
//                                       // the number of bytes
//                                       // written to the buffer
// );
typedef LONG (WINAPI *PROCNTQSI)(UINT,PVOID,ULONG,PULONG);

PROCNTQSI NtQuerySystemInformation;

SYSTEM_PERFORMANCE_INFORMATION SysPerfInfo;
SYSTEM_TIME_INFORMATION        SysTimeInfo;
SYSTEM_BASIC_INFORMATION       SysBaseInfo;
double                         dbIdleTime = 0.0;
double                         dbSystemTime;
LONG                           status;
LARGE_INTEGER                  liOldIdleTime;
LARGE_INTEGER                  liOldSystemTime;

#ifdef WITH_EXCLUSION
ULARGE_INTEGER pxCreat, pxExit, pxKern, pxUser, pxOKern, pxOUser;
        double dbPxTime;
HANDLE pxH = 0;
#endif

#ifdef WITH_EXCLUSION
HANDLE getexclp() {
    /* Take a snapshot of the running process list */
    HANDLE hSnapShot=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
    /* Prepare the struct to contains every process information */
    PROCESSENTRY32 *processInfo=malloc(sizeof(PROCESSENTRY32));
    processInfo->dwSize=sizeof(PROCESSENTRY32);

    DWORD processID = 0;
    BOOL bFound = FALSE;

    /* Iterate through processes to find ours... */
    while(Process32Next(hSnapShot,processInfo)!=FALSE) {
        /* store the Process ID */
        processID = processInfo->th32ProcessID;

        /* Is this the process we're looking for? */
        if(strlen(processInfo->szExeFile) >= 8 &&
           strncmp(processInfo->szExeFile, "FahCore_", 8) == 0) {
            bFound = TRUE;
            break;
        }
    }

    CloseHandle(hSnapShot);
    free(processInfo);

    if(bFound) {
        /* Obtain the Process handle */
        return OpenProcess(PROCESS_QUERY_INFORMATION,FALSE,processID);
    } else {
        return 0;
    }
}
#endif /* WITH_EXCLUSION */

static int CPUinit(void) {
    NtQuerySystemInformation = (PROCNTQSI)GetProcAddress(
                                          GetModuleHandle("ntdll"),
                                         "NtQuerySystemInformation"
                                         );

    if (!NtQuerySystemInformation)
        return -1;

    liOldIdleTime.QuadPart = 0;
    liOldSystemTime.QuadPart = 0;

    // get number of processors in the system
    status =
NtQuerySystemInformation(SystemBasicInformation,&SysBaseInfo,sizeof(SysBaseInfo),NULL);
    if (status != NO_ERROR)
        return -1;

#ifdef WITH_EXCLUSION
    pxH = getexclp();
#endif

    (void)sysmon_cpupercent();        /* First poll returns no info */

    return 0;
}

int sysmon_cpupercent(void) {
    int r;

    // get new system time
    status = NtQuerySystemInformation(SystemTimeInformation,&SysTimeInfo,sizeof(SysTimeInfo),0);
    if (status != NO_ERROR)
        return -1;

    // get new CPU's idle time
    status =  NtQuerySystemInformation(SystemPerformanceInformation,&SysPerfInfo,sizeof(SysPerfInfo),NULL);
    if (status != NO_ERROR)
        return -1;

#ifdef WITH_EXCLUSION
    // get excluded process' time
    if (pxH) {
        GetProcessTimes(pxH, (FILETIME *)(&pxCreat), (FILETIME *)(&pxExit),
                       (FILETIME *)(&pxKern), (FILETIME *)(&pxUser));
    }
#endif

    // if it's a first call - skip it
    if (liOldIdleTime.QuadPart != 0) {
        // CurrentValue = NewValue - OldValue
        dbIdleTime = Li2Double(SysPerfInfo.liIdleTime) -
                     Li2Double(liOldIdleTime);
        dbSystemTime = Li2Double(SysTimeInfo.liKeSystemTime) -
                       Li2Double(liOldSystemTime);

#ifdef WITH_EXCLUSION
        if (pxH) {
            dbPxTime = Li2Double(pxKern) - Li2Double(pxOKern) +
                       Li2Double(pxUser) - Li2Double(pxOUser);
            dbIdleTime = (dbIdleTime + dbPxTime) / dbSystemTime;
        } else
#endif
        {
            dbIdleTime = dbIdleTime / dbSystemTime;
        }


        // CurrentCpuUsage% = 100 - (CurrentCpuIdle * 100) / NumberOfProcessors
        dbIdleTime = 100.0 - dbIdleTime * 100.0 /
                     (double)SysBaseInfo.bKeNumberProcessors;
    }

    // store new CPU's idle and system time
    liOldIdleTime = SysPerfInfo.liIdleTime;
    liOldSystemTime = SysTimeInfo.liKeSystemTime;

#ifdef WITH_EXCLUSION
    if (pxH) {
        pxOKern = pxKern;
        pxOUser = pxUser;
    }
#endif

    r = lrint(dbIdleTime);
    if (r < 0) r = 0;
    else if (r > 100) r = 100;
    return r;
}

static void CPUquit() {
#ifdef WITH_EXCLUSION
  if (pxH) {
    CloseHandle(pxH);
  }
#endif
}

/*** Memory ***/

static MEMORYSTATUSEX statex;

int sysmon_memorypercent(void) {
    statex.dwLength = sizeof(statex);
    if (GlobalMemoryStatusEx(&statex)) {
        return statex.dwMemoryLoad;
    } else {
        return -1;
    }
}

/*** Wake time ***/

#ifndef _MSC_VER
ULONGLONG WINAPI GetTickCount64(void);
#endif

static unsigned long waketime;

static unsigned long getlastwaketime(void) {
    ULONGLONG lastwake;

    if (CallNtPowerInformation(LastWakeTime, NULL, 0, &lastwake, sizeof(lastwake)) == 0) {
        return (unsigned long)(lastwake/(1000000000/100));
    } else {
        return (unsigned long)(GetTickCount64()/1000);
    }
}

void sysmon_pmwake(void) {
    (void)sysmon_cpupercent();        /* Info may be wrong */
    waketime = getlastwaketime();
}

unsigned long sysmon_getawaketime(void) {
    return (unsigned long)(GetTickCount64()/1000 - waketime);
}

/*** Misc ***/

int sysmon_init(void) {
    waketime = getlastwaketime();
    return CPUinit();
}

void sysmon_quit(void) {
    CPUquit();
}

#if 0
int main() {
  printf("X%iX\n", pxH = getexclp());

  CPUinit();
  while (1) {
    sleep(1000);
    printf("%i\n", CPUpoll());
  }
}
#endif
