#include "ultra64.h"
#include "bcp.h"

#include "xinput.h"
#include "graph.h"
#include "usb.h"


#define DMA_QUEUE_SIZE	200
//#define PRINTF		osSyncPrintf
//#define IDE_PRINT

extern u32 __osBbIsBb;
extern u32 __osBbHackFlags;

/*
 * Thread and stack structures
 */
char   bootStack[STACKSIZE] __attribute__ ((aligned (8)));

static OSThread idleThread;
static char     idleThreadStack[STACKSIZE] __attribute__ ((aligned (8)));

static OSThread mainThread;
static char     mainThreadStack[STACKSIZE] __attribute__ ((aligned (8)));


/*
 * Message queues and message buffers used by this app 
 */
static OSMesg           PiMessages[DMA_QUEUE_SIZE], dmaMessageBuf;
static OSMesgQueue      PiMessageQ, dmaMessageQ;

static OSMesg           SiMessages[DMA_QUEUE_SIZE];
static OSMesgQueue      SiMessageQ;

/*
 * Local variables and routines
 */

static void		idleproc(char *);
static void		mainproc(char *);

void
boot(void)
{
    /* Init the video PLL */
    __osBbVideoPllInit(OS_TV_NTSC);

    osInitialize();
    osCreateThread(&idleThread, 1, (void(*)(void *))idleproc, (void *)0,
                   idleThreadStack+STACKSIZE, 8);
    osStartThread(&idleThread);
}

static OSMesgQueue	retraceMessageQ, siMessageQ;;
static OSMesg		dummyMessage, retraceMessageBuf;
OSMesg      siMessageBuf;

static u16 cfb[320*240] __attribute__((aligned(64)));
static void clear(u16 bg) {
    int i;
    for (i = 0; i < 320*240; ++i) {
            cfb[i] = bg;
    }
}

extern struct usbdevfuncs rdbs_dev_funcs;

static void
idleproc(char *argv)		/* priority 8 */
{
    osCreateViManager(OS_PRIORITY_VIMGR);
    osViSetMode(&osViModeTable[OS_VI_NTSC_LPN1]);

    /*
     * Start PI Mgr for access to cartridge - start before the debugger
     */
    osCreatePiManager((OSPri) OS_PRIORITY_PIMGR, &PiMessageQ, PiMessages,
            DMA_QUEUE_SIZE);

    osCreateMesgQueue(&dmaMessageQ, &dmaMessageBuf, 1);
    osCreateMesgQueue(&SiMessageQ, SiMessages, DMA_QUEUE_SIZE);
    osSetEventMesg(OS_EVENT_SI, &SiMessageQ, (OSMesg)DMA_QUEUE_SIZE);
    
    rdbs_dev_funcs.reset_ep0 = xinput_reset_ep0;
    rdbs_dev_funcs.query = xinput_query;
    rdbs_dev_funcs.get_desc = xinput_ch9GetDescription;
    rdbs_dev_funcs.initeps = xinput_init_data_eps;
    rdbs_dev_funcs.stall_ep = xinput_stall_data_eps;
    // hook and init usb
    osBbUsbInit();
    
    /*
     * The main thread's priority must be the same or lower than the original
     * idle's thread priority. This allows the idle thread to change its
     * priority to 0 before the main thread starts execution.
     */
    osCreateThread(&mainThread, 3, (void(*)(void *))mainproc, argv,
           mainThreadStack+STACKSIZE/8, (OSPri)7);
    osStartThread(&mainThread);

    osSetThreadPri(0, OS_PRIORITY_IDLE);
    for(;;);                                    /* idle thread */
}

#define KEY(x) ((key & x) != 0)

#define LX 85.0f
#define LXN 90.0f
#define LY 90.0f
#define LYN 90.0f

static void d_pad_test(void) 
{
    OSContStatus     sdata[MAXCONTROLLERS];
    OSContPad        ctrl_data[MAXCONTROLLERS];
    char pattern, joy[32];
    int key, down[4], color, i;

    osSetEventMesg(OS_EVENT_SI, &siMessageQ, (OSMesg)NULL);
    osContInit(&siMessageQ, &pattern, &sdata[0]);

    osWritebackDCacheAll();

    for (key=0; key<4; key++) down[key] = 0;
    for (; ;) {
        signed int lx, ly, rx, ry;
            
        tx_buf.type = 0x00;
        tx_buf.size = 0x14;
        
        osContStartReadData(&siMessageQ);
        osRecvMesg(&siMessageQ, NULL, OS_MESG_BLOCK);
        osContGetReadData(ctrl_data);

        for (i=key=0; i<MAXCONTROLLERS; i++) {
             if (!(ctrl_data[i].errno &CONT_NO_RESPONSE_ERROR))
                 key |= ctrl_data[i].button;
        }

        sprintf(joy, "(%d, %d)     ", ctrl_data[0].stick_x, ctrl_data[0].stick_y);
        printstr(gray, 15, 7, joy);

        tx_buf.buts0 = 0;
        tx_buf.buts1 = 0;
        
        tx_buf.buts0 |= KEY(U_JPAD) << 0;
        tx_buf.buts0 |= KEY(D_JPAD) << 1;
        tx_buf.buts0 |= KEY(L_JPAD) << 2;
        tx_buf.buts0 |= KEY(R_JPAD) << 3;
        
        tx_buf.buts0 |= KEY(START_BUTTON) << 4;
        
        tx_buf.buts1 |= KEY(L_TRIG) << 0;
        tx_buf.buts1 |= KEY(R_TRIG) << 1;
        
        tx_buf.buts1 |= KEY(A_BUTTON) << 4;
        tx_buf.buts1 |= KEY(B_BUTTON) << 5;
        
        tx_buf.ltrig = KEY(Z_TRIG) * 0xFF;
        tx_buf.rtrig = 0;
        
        if (ctrl_data[0].stick_x > 0) lx = (int)((float)ctrl_data[0].stick_x * (32767 / LX));
        else lx = (int)((float)ctrl_data[0].stick_x * (32767 / LXN));
        
        if (ctrl_data[0].stick_y > 0) ly = (int)((float)ctrl_data[0].stick_y * (32767 / LY));
        else ly = (int)((float)ctrl_data[0].stick_y * (32767 / LYN));
        
        if (lx > 32767) lx = 32767;
        if (lx < -32768) lx = -32768;
        if (ly > 32767) ly = 32767;
        if (ly < -32768) ly = -32768;
        tx_buf.lx_low = USB_uint_16_low((short)lx);
        tx_buf.lx_high = USB_uint_16_high((short)lx);
        tx_buf.ly_low = USB_uint_16_low((short)ly);
        tx_buf.ly_high = USB_uint_16_high((short)ly);
        
        rx = KEY(R_CBUTTONS) * 0x7FFF - KEY(L_CBUTTONS) * 0x7FFF;
        ry = KEY(U_CBUTTONS) * 0x7FFF - KEY(D_CBUTTONS) * 0x7FFF;
        tx_buf.rx_low = USB_uint_16_low(rx);
        tx_buf.rx_high = USB_uint_16_high(rx);
        tx_buf.ry_low = USB_uint_16_low(ry);
        tx_buf.ry_high = USB_uint_16_high(ry);
        
        osWritebackDCacheAll();

    }

    return;
}

static void 
mainproc(char *argv)		{
    __osBbHackFlags = 0;
    osCreateMesgQueue(&siMessageQ, &siMessageBuf, 1);
    osSetEventMesg(OS_EVENT_SI, &siMessageQ, dummyMessage);

    osCreateMesgQueue(&retraceMessageQ, &retraceMessageBuf, 1);
    osViSetEvent(&retraceMessageQ, dummyMessage, 1);
    osViBlack(1);
    osViSwapBuffer(cfb);
    clear(0x0);
    osViBlack(0);
    printstr(white, 12, 3, "Waiting for USB...");
    osWritebackDCacheAll();
    osViSwapBuffer(cfb);

    d_pad_test();

    for (;;);
}
