#ifndef WDT_H
#define	WDT_H

#include "Common/LibDef.h"
#include "Project_Cfg.h"

public void SYS_SoftReset(void);
public void WDT_Tasks(void *);
public void WDT_Enable(void);
public void WDT_Disable(void);

#endif