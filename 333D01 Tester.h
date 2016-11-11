/**************************************************************************/
/* LabWindows/CVI User Interface Resource (UIR) Include File              */
/* Copyright (c) National Instruments 2014. All Rights Reserved.          */
/*                                                                        */
/* WARNING: Do not add to, delete from, or otherwise modify the contents  */
/*          of this include file.                                         */
/**************************************************************************/

#include <userint.h>

#ifdef __cplusplus
    extern "C" {
#endif

     /* Panels and Controls: */

#define  PANEL                            1       /* callback function: panelCB */
#define  PANEL_CB_TEST_SENSOR             2       /* control type: command, callback function: cbTestSensor */
#define  PANEL_GRAPH_TIME_CH2             3       /* control type: graph, callback function: (none) */
#define  PANEL_GRAPH_TIME_CH1             4       /* control type: graph, callback function: (none) */
#define  PANEL_GRAPH_SPEC_AVG             5       /* control type: graph, callback function: (none) */
#define  PANEL_GRAPH_SPEC                 6       /* control type: graph, callback function: (none) */
#define  PANEL_TXT_RESULTS                7       /* control type: textBox, callback function: (none) */
#define  PANEL_TXT_SETUP                  8       /* control type: textBox, callback function: (none) */
#define  PANEL_SENS_B_RES                 9       /* control type: numeric, callback function: cbSens */
#define  PANEL_SENS_A_RES                 10      /* control type: numeric, callback function: (none) */
#define  PANEL_SENS_B                     11      /* control type: numeric, callback function: cbSens */
#define  PANEL_SENS_A                     12      /* control type: numeric, callback function: cbSens */
#define  PANEL_CMD_SET_DEFAULT            13      /* control type: command, callback function: cbSetDefault */
#define  PANEL_SPLITTER_4                 14      /* control type: splitter, callback function: (none) */
#define  PANEL_SPLITTER_5                 15      /* control type: splitter, callback function: (none) */
#define  PANEL_SPLITTER_3                 16      /* control type: splitter, callback function: (none) */
#define  PANEL_SPLITTER_2                 17      /* control type: splitter, callback function: (none) */
#define  PANEL_SPLITTER                   18      /* control type: splitter, callback function: (none) */
#define  PANEL_TEXTMSG_5                  19      /* control type: textMsg, callback function: (none) */
#define  PANEL_TEXTMSG                    20      /* control type: textMsg, callback function: (none) */
#define  PANEL_NUM_FREQUENCY              21      /* control type: numeric, callback function: (none) */
#define  PANEL_NUM_TOLERANCE              22      /* control type: numeric, callback function: (none) */
#define  PANEL_NUM_AMPLITUDE              23      /* control type: numeric, callback function: (none) */
#define  PANEL_TEXTMSG_2                  24      /* control type: textMsg, callback function: (none) */
#define  PANEL_NUM_COMPLETION             25      /* control type: scale, callback function: (none) */
#define  PANEL_TEXTMSG_6                  26      /* control type: textMsg, callback function: (none) */
#define  PANEL_TEXTMSG_7                  27      /* control type: textMsg, callback function: (none) */
#define  PANEL_TEXTMSG_12                 28      /* control type: textMsg, callback function: (none) */
#define  PANEL_TEXTMSG_13                 29      /* control type: textMsg, callback function: (none) */
#define  PANEL_TEXTMSG_10                 30      /* control type: textMsg, callback function: (none) */
#define  PANEL_TEXTMSG_CH2_TYP            31      /* control type: textMsg, callback function: (none) */
#define  PANEL_TEXTMSG_CH1_TYP            32      /* control type: textMsg, callback function: (none) */
#define  PANEL_TEXTMSG_11                 33      /* control type: textMsg, callback function: (none) */
#define  PANEL_TEXTMSG_4                  34      /* control type: textMsg, callback function: (none) */
#define  PANEL_RING_UNITS                 35      /* control type: ring, callback function: (none) */
#define  PANEL_PASSFAIL                   36      /* control type: textButton, callback function: (none) */


     /* Control Arrays: */

          /* (no control arrays in the resource file) */


     /* Menu Bars, Menus, and Menu Items: */

          /* (no menu bars in the resource file) */


     /* Callback Prototypes: */

int  CVICALLBACK cbSens(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK cbSetDefault(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK cbTestSensor(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK panelCB(int panel, int event, void *callbackData, int eventData1, int eventData2);


#ifdef __cplusplus
    }
#endif
