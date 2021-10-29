#ifndef _XINPUT_H
#define _XINPUT_H

#include "usb.h"

#define STACKSIZE	0x2000

extern	OSMesgQueue	n_dmaMessageQ,	n_rdpMessageQ;
extern	OSMesgQueue	n_rspMessageQ,	n_retraceMessageQ;
extern	OSMesgQueue	n_siMessageQ;

#endif