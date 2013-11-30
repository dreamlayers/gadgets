#ifndef _SYSMON_H_
#define _SYSMON_H_

int sysmon_init(void);
int sysmon_cpupercent(void);
int sysmon_memorypercent(void);
void sysmon_pmwake(void);
unsigned long sysmon_getawaketime(void);
void sysmon_quit();

#ifdef __linux
void pm_upower_init(void);
#endif

#endif
