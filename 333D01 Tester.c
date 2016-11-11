//==============================================================================
//
// Title:		333D01 Tester
// Purpose:		This LabWindows Application was developed to be used for a functional
//				test of the 333D01. It uses the Windows WaveIn functions to acquire
//				time data. The data rate for this application was chosen to be
//				functional with Windows XP.  It is a fixed purpose application
//				so many aspects are hard coded.
//
// Created on:	4/1/2014 at 2:16:25 PM by tclary
// Copyright 2014:	TMS. For use supporting the 333D01 USB Sensor
//
//
//	Free to use to support interfacing to the 333D01 sensor
//
// Programmed in National Instruments LabWindows/CVI 2012 Service Pack 1
//
//==============================================================================

//==============================================================================
// Include files

#include <windows.h>
#include <analysis.h>
#include <time.h>
#include <mmsystem.h>
#include <ansi_c.h>
#include <cvirte.h>		
#include <userint.h>
#include <string.h>
#include "333D01 Tester.h"
#include "toolbox.h"

//==============================================================================
// Constants

//==============================================================================
// Types

//==============================================================================
// Static global variables

static int panelHandle = 0;

//==============================================================================
// Static functions

//==============================================================================
// Global variables
WAVEFORMATEX wfmt;
MMRESULT mmstatus;
HWAVEIN waveInHandle;

int	gState;
enum {IDLE, RUNNING};

#define WAVE_BUFFER_COUNT	30
#define MAX_SAMPLES 16384
#define NSAMPLES 2048
#define NUM_CHANS 2
#define SAMPLE_RATE 8000
#define TEST_FREQUENCY 100	// Frequency to test
#define RESOLUTION_BITS 16

#define G_IN_SI	(9.80665)

// Counts / (m/s^2) - determined by averaging lot of 50 sensors from January 2014
#define SENS_CH_A_NOM	(35933)
#define SENS_CH_B_NOM	(70375)	

#define SENS_CH_A_TYP	(36000)	// Just rounded nominal to whole 1000
#define SENS_CH_B_TYP	(70000)	// Rounded to whole 1000

WAVEHDR waveheader[WAVE_BUFFER_COUNT];

static int gAvgIteration;
static double gdAvgFreqData[NUM_CHANS][NSAMPLES/2];

int gQWaveHandle;

#define G_UNIT_RMS 0
#define G_UNIT_PEAK 1
int gG_units;
double gYAXIS_Scale;

#define RMS_TO_PEAK	(0.707106781)

static void CVICALLBACK cbProcessTimeData (CmtTSQHandle queueHandle, unsigned int event, int value, void *callbackData);

int gIntTypical[NUM_CHANS] = {SENS_CH_A_TYP, SENS_CH_B_TYP};	// Typical expected values 
float gInt32SampleScaleFactor[NUM_CHANS];	// Factor to scale integer sample in 32 bits to float in units - includes conversion from integer
float gInt32SampleMvScaleFactor[NUM_CHANS];	// Factor to convert to Volts

double gWinRatioW2H;
#define WIN_RATIO_W2H (gWinRatioW2H)
#define CONVERT_TO_SENS 604.85911

 
//
// LabWindows main function
//
int main (int argc, char *argv[])
{
	int error = 0;
	char buffer[256];
	int ch;
	int width, height;
	int typical_value_prompt[] = {PANEL_TEXTMSG_CH1_TYP, PANEL_TEXTMSG_CH2_TYP};
	/* initialize and load resources */
	nullChk (InitCVIRTE (0, argv, 0));
	errChk (panelHandle = LoadPanel (0, "333D01 Tester.uir", PANEL));

	/* Get initial panel size */
	GetPanelAttribute ( PANEL, ATTR_WIDTH, &width);
	GetPanelAttribute ( PANEL, ATTR_HEIGHT, &height);
	gWinRatioW2H = (double) width / (double) height;

	for(ch=0; ch < NUM_CHANS; ch++) {
		sprintf(buffer,"%d", gIntTypical[ch]);
		SetCtrlVal ( PANEL, typical_value_prompt[ch], buffer);
	}
	
	/* Create queue */
	errChk (CmtNewTSQ (WAVE_BUFFER_COUNT*2, sizeof(void *), 0, &gQWaveHandle));
	error = CmtInstallTSQCallback (gQWaveHandle, EVENT_TSQ_ITEMS_IN_QUEUE, 1, cbProcessTimeData, NULL, CmtGetCurrentThreadID(), NULL);

//
// I am hardcoding the setup but dump it out for convenience
//
	sprintf(buffer,"Resolution   : 16 bits\nSample rate: %d\nBlock Size   : %d\nWindow        : Max Flattop\nAverages    : %d"
		,SAMPLE_RATE,NSAMPLES,	WAVE_BUFFER_COUNT);
	SetCtrlVal ( PANEL, PANEL_TXT_SETUP, buffer);
	SetCtrlAttribute(PANEL, PANEL_NUM_COMPLETION, ATTR_MAX_VALUE, WAVE_BUFFER_COUNT);	// Update iteration
	
	/* display the panel and run the user interface */
	errChk (DisplayPanel (panelHandle));
	errChk (RunUserInterface ());

Error:
	/* clean up */
	if (panelHandle > 0)
		DiscardPanel (panelHandle);
	return 0;
}

//==============================================================================
// Global functions
int CVICALLBACK cbSetDefault (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			SetCtrlVal(panelHandle, PANEL_SENS_A, SENS_CH_A_NOM);
			SetCtrlVal(panelHandle, PANEL_SENS_B, SENS_CH_B_NOM);
			break;
	}
	return 0;
}

//==============================================================================
// 
// Converts from our previous sensitivity in mV / g to Counts / (m/s^2)
//
//==============================================================================
int CVICALLBACK cbSens (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	unsigned int sensitivity;
	switch (event)
	{
		case EVENT_COMMIT:
			GetCtrlVal(panelHandle, control, &sensitivity);
			if (sensitivity < 200) {
				 sensitivity = (double) sensitivity * (double) CONVERT_TO_SENS + 0.5;
				 SetCtrlVal(panelHandle, control, sensitivity);
			}
			break;
	}
	return 0;
}


//==============================================================================
// 
// This call back function gets called as each audio buffer is filled.
//	It also gets called when the sound acquisition gets cancelled.
//	This is a Windows callback. The other callbacks are part of LabWindows
//
//==============================================================================
void CALLBACK waveInProc(HWAVEIN hwi,UINT uMsg,DWORD dwInstance,DWORD dw1,DWORD dw2)
{
	WAVEHDR *buffer;
	int status;

	
	if (uMsg == MM_WIM_OPEN) { // Opened
	} else if (uMsg == MM_WIM_CLOSE) { // Closed
	} else if (uMsg == MM_WIM_DATA) {
		buffer = (WAVEHDR *)dw1;
		if ((buffer->dwFlags & WHDR_DONE) != 0) {
			status = CmtWriteTSQData (gQWaveHandle, (void *)&buffer, 1, 0, NULL);			// Thread safe queue - LabWindows
		}
	}

}

//==============================================================================
//
// Display and calculate time data, spectrum, and average
//	This is called when there are items in the gQWaveHandle queue
//	Multiple queue items may exist per callback to this routine should iterate.
//
//==============================================================================
static void CVICALLBACK cbProcessTimeData (CmtTSQHandle queueHandle, unsigned int event, int value, void *callbackData) {
	WAVEHDR *buffer;
	int block_size;
	int status;
	signed char *bptr;
	union {
		int i;
		struct {
			char low;
			char mid;
			short high;
		} conv;
	} cast;

	static int time_plot_handle[NUM_CHANS];
	static int freq_plot_handle[NUM_CHANS];
	static int freq_avg_plot_handle[NUM_CHANS];

	static double time_data[NUM_CHANS][NSAMPLES];
	static double freq_data[NUM_CHANS][NSAMPLES/2];
	static double freq_data_scaled[NUM_CHANS][NSAMPLES/2];
	
	const int plot_colors[NUM_CHANS] = { VAL_GREEN, VAL_RED };
	const int time_plots[NUM_CHANS] = {PANEL_GRAPH_TIME_CH1, PANEL_GRAPH_TIME_CH2};
	const char *time_data_legend[NUM_CHANS] = {"Ch 1/A/Left", "Ch 2/B/Right"};
	double min=10000.0f, max=-10000.0f;
	double time_plot_y_max, time_plot_y_min;
	const int results_display[NUM_CHANS] = { PANEL_SENS_A_RES, PANEL_SENS_B_RES };
	double sensitivity, sensitivity_ms;

	int ch;
	int pnt;
	double yGain;
	double df;
	WindowConst windowConstants;
	char unitString[32];
	char string[256];
	double inputAccel;
	int test_frequency;
	int itolerance;
	double tolerance;
	int fail;

	while ((status = CmtReadTSQData (gQWaveHandle, &buffer, 1, 0, 0)) > 0) {

		// If block is not full, just return - may need to do something more
		if (buffer->dwBytesRecorded != buffer->dwBufferLength) {
			continue;
		}
	
		// XP does not support 24 bit data or resampling to 24 bits at 8KHz so lets try 16 bits
		if (wfmt.wBitsPerSample == 16) {
			block_size = buffer->dwBytesRecorded / 4;	// 3 bytes per sample and 2 channels.  Just hard code it
			bptr = buffer->lpData;
			for(pnt=0; pnt < block_size; pnt++) {
				cast.conv.low = 0;
				cast.conv.mid = *bptr++;
				cast.conv.high = *bptr++;
				time_data[0][pnt] = (double) cast.i * gInt32SampleScaleFactor[0]; // Scale to floating Engineering units
				if (time_data[0][pnt] < min) min = time_data[0][pnt];
				if (time_data[0][pnt] > max) max = time_data[0][pnt];
				cast.conv.low = 0;
				cast.conv.mid = *bptr++;
				cast.conv.high = *bptr++;
				time_data[1][pnt] = (double) cast.i * gInt32SampleScaleFactor[1]; // Scale to floating Engineering units 
				if (time_data[1][pnt] < min) min = time_data[1][pnt];
				if (time_data[1][pnt] > max) max = time_data[1][pnt];
			}
		} else if (wfmt.wBitsPerSample == 24) {
			block_size = buffer->dwBytesRecorded / 6;	// 3 bytes per sample and 2 channels.  Just hard code it
			bptr = buffer->lpData;
			for(pnt=0; pnt < block_size; pnt++) {
				cast.conv.low = *bptr++;
				cast.conv.mid = *bptr++;
				cast.conv.high = *bptr++;
				time_data[0][pnt] = (double) cast.i * gInt32SampleScaleFactor[0]; // Scale to floating Engineering units
				if (time_data[0][pnt] < min) min = time_data[0][pnt];
				if (time_data[0][pnt] > max) max = time_data[0][pnt];
				cast.conv.low = *bptr++;
				cast.conv.mid = *bptr++;
				cast.conv.high = *bptr++;
				time_data[1][pnt] = (double) cast.i * gInt32SampleScaleFactor[1]; // Scale to floating Engineering units 
				if (time_data[1][pnt] < min) min = time_data[1][pnt];
				if (time_data[1][pnt] > max) max = time_data[1][pnt];
			}
		} else {
			return;
		}

		if (max < 0.01) {
			time_plot_y_max = 0.01f;
			time_plot_y_min = -0.01f;
	
		} else {
			max *= gYAXIS_Scale;
			time_plot_y_max = (float)((int) ((max + 0.2f) * 10.0f))/10.0f;
			min *= gYAXIS_Scale;
			time_plot_y_min = (float)((int) ((min - 0.2f) * 10.0f))/10.0f;
		}
		//
		// Plot time Data
		for (ch=0; ch < NUM_CHANS; ch++) {
			if (time_plot_handle[ch] != 0) {
				DeleteGraphPlot(PANEL, time_plots[ch], time_plot_handle[ch], VAL_DELAYED_DRAW);
			}
			time_plot_handle[ch] = PlotWaveform(PANEL, time_plots[ch], time_data[ch], block_size, VAL_DOUBLE, gYAXIS_Scale, 0.0, 0.0, 
				(1.0/(double)SAMPLE_RATE)*1000.0, VAL_THIN_LINE, VAL_EMPTY_SQUARE,
				VAL_SOLID, 1, plot_colors[ch]);
			SetPlotAttribute (PANEL, time_plots[ch], time_plot_handle[ch], ATTR_PLOT_LG_TEXT, time_data_legend[ch]); 
		}

		// Frequency functions
		for(ch=0; ch < NUM_CHANS; ch++) {
			// Window the data
			ScaledWindowEx (time_data[ch], block_size,FLATTOP, 0.0, &windowConstants);

			AutoPowerSpectrum(time_data[ch], block_size, 1.0 / (double)SAMPLE_RATE, freq_data[ch], &df );	// Returns two sided spectrum
			strcpy(unitString,"g ");
			SpectrumUnitConversion (freq_data[ch], block_size/2, 0, 1, 1, df, windowConstants, freq_data_scaled[ch], (char *)&unitString);		
		
			// Update plot
			if (freq_plot_handle[ch] != 0) {
				DeleteGraphPlot(PANEL, PANEL_GRAPH_SPEC, freq_plot_handle[ch], VAL_DELAYED_DRAW);
			}
			freq_plot_handle[ch] = PlotWaveform(PANEL, PANEL_GRAPH_SPEC, freq_data_scaled[ch], block_size/2-1, VAL_DOUBLE, gYAXIS_Scale, 0.0, 0.0, 
				((double)SAMPLE_RATE/(double)NSAMPLES), VAL_THIN_LINE, VAL_EMPTY_SQUARE,
				VAL_SOLID, 1, plot_colors[ch]);
			SetPlotAttribute (PANEL, PANEL_GRAPH_SPEC, freq_plot_handle[ch], ATTR_PLOT_LG_TEXT, time_data_legend[ch]); 
		}
	
		// Calculate average
		gAvgIteration++;
	
		SetCtrlVal(PANEL, PANEL_NUM_COMPLETION, gAvgIteration);	// Update Iteration 
		yGain = 1.0 / (double)gAvgIteration;
		for(ch=0; ch < NUM_CHANS; ch++) {
			for(pnt=0; pnt < block_size/2; pnt++) {
				gdAvgFreqData[ch][pnt] += freq_data_scaled[ch][pnt];
			}
			// Update plot
			if (freq_avg_plot_handle[ch] != 0) {
				DeleteGraphPlot(PANEL, PANEL_GRAPH_SPEC_AVG, freq_avg_plot_handle[ch], VAL_DELAYED_DRAW);
			}
			freq_avg_plot_handle[ch] = PlotWaveform(PANEL, PANEL_GRAPH_SPEC_AVG, gdAvgFreqData[ch], block_size/2-1, VAL_DOUBLE, yGain*gYAXIS_Scale, 0.0, 0.0, 
				((double)SAMPLE_RATE/(double)NSAMPLES), VAL_THIN_LINE, VAL_EMPTY_SQUARE,
				VAL_SOLID, 1, plot_colors[ch]);
			SetPlotAttribute (PANEL, PANEL_GRAPH_SPEC_AVG, freq_avg_plot_handle[ch], ATTR_PLOT_LG_TEXT, time_data_legend[ch]); 
		}
	
	// Scale plots
		for(ch=0; ch < NUM_CHANS; ch++) {
			SetAxisScalingMode (PANEL, time_plots[ch], VAL_LEFT_YAXIS , VAL_MANUAL, time_plot_y_min, time_plot_y_max);
		}
	// Refresh plots
		for(ch=0; ch < NUM_CHANS; ch++) {
			RefreshGraph (PANEL, time_plots[ch]);
		}
		RefreshGraph (PANEL, PANEL_GRAPH_SPEC);
		RefreshGraph (PANEL, PANEL_GRAPH_SPEC_AVG);
	
	// Get results
		if (gAvgIteration == WAVE_BUFFER_COUNT) {
			GetCtrlVal(PANEL,  PANEL_NUM_FREQUENCY, &test_frequency);
			pnt = ((double)test_frequency / df);
			GetCtrlVal(PANEL,  PANEL_NUM_AMPLITUDE, &inputAccel);
			GetCtrlVal(PANEL, PANEL_NUM_TOLERANCE, &itolerance);
			tolerance = (double)itolerance / 100.0;
			
			fail = FALSE;
			for(ch=0; ch < NUM_CHANS; ch++) {
				sensitivity = (gdAvgFreqData[ch][pnt] + gdAvgFreqData[ch][pnt+1]) / 2.0; // Average across bins because max flattop
				sensitivity /= (double) gAvgIteration;	// It was an average amplitude
				sensitivity /= 20.0; // Converting from dB
				sensitivity = pow(10.0, sensitivity);	// 10^sensitivity
				sprintf(string,"%s at %d Hz\n",time_data_legend[ch], test_frequency);
				SetCtrlVal ( PANEL, PANEL_TXT_RESULTS, string);  
				sprintf(string,"%8.3f g peak\n", sensitivity);
				SetCtrlVal ( PANEL, PANEL_TXT_RESULTS, string);  

				sensitivity /= inputAccel; // Normalize to 1g
				if (gG_units == G_UNIT_RMS) {
					sensitivity *= RMS_TO_PEAK;
				}
				sensitivity /= gInt32SampleScaleFactor[ch];
				sensitivity_ms = sensitivity / G_IN_SI;	// Convert back to sample counts in m/s^2
				SetCtrlVal ( PANEL, results_display[ch], (int)(sensitivity_ms));  // Convert to sample counts in m/s^2
				// Check results
				if ((sensitivity_ms < (gIntTypical[ch] * (1.0-tolerance))) || (sensitivity_ms > (gIntTypical[ch] * (1.0+tolerance)))) {
					fail = TRUE;
				}
				// Estimate mV/g - may vary per A/D with temperature, etc.
				sensitivity *= gInt32SampleMvScaleFactor[ch];
				sprintf(string,"%8.3f mV/g estimate\n", sensitivity);
				SetCtrlVal ( PANEL, PANEL_TXT_RESULTS, string);  
			}
			gState = IDLE;
			if (fail == FALSE) {
				SetCtrlVal ( PANEL, PANEL_PASSFAIL, 0);  
			} else {
				SetCtrlVal ( PANEL, PANEL_PASSFAIL, 1);  
			}
			SetCtrlAttribute(PANEL, PANEL_CB_TEST_SENSOR, ATTR_LABEL_TEXT , "__Test Sensor");
		}
	}
}


//==============================================================================
//
// Callback to handle panel events such as close or resize
//
//==============================================================================
int CVICALLBACK panelCB (int panel, int event, void *callbackData,
		int eventData1, int eventData2)
{
	int width;
	int height;
	double ratio;

	if (event == EVENT_CLOSE) {
		if (waveInHandle != NULL) {
			mmstatus = waveInReset(waveInHandle);
			mmstatus = waveInClose(waveInHandle);
			waveInHandle = NULL;
		}
		QuitUserInterface (0);
	} else if (event == EVENT_PANEL_SIZE) {
		GetPanelAttribute ( panel, ATTR_WIDTH, &width);
		GetPanelAttribute ( panel, ATTR_HEIGHT, &height);
		ratio = (double)width / (double)height;
		if (ratio < (0.98 * WIN_RATIO_W2H) || ratio > (1.02 * WIN_RATIO_W2H)) {
			if (ratio >= WIN_RATIO_W2H) {
				width = height * WIN_RATIO_W2H;
			} else {
				height = width / WIN_RATIO_W2H;
			}
		
			SetPanelAttribute ( panel, ATTR_WIDTH, width);
			SetPanelAttribute ( panel, ATTR_HEIGHT, height);
		}
	}
	return 0;
}

//==============================================================================
// 
// Acquire data to test sensor
//
//==============================================================================
int CVICALLBACK cbTestSensor (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	UINT waveInNumDevs;
	int device;
	WAVEINCAPS wave_capability;
	char *chk;
	int i, j;
	int error;
	double sum;
	int sens_A, sens_B;
	int count_333d01 = 0;
	int first_333d01 = -1;
	char buffer[128];

	switch (event)
	{
		case EVENT_COMMIT:
			if (waveInHandle != NULL) {
				// Clean up from previous test
				if (waveInHandle != NULL) {
					SetCtrlVal ( PANEL, PANEL_TXT_RESULTS, "Closing sound port\n");  
					mmstatus = waveInReset(waveInHandle);
					if (mmstatus != MMSYSERR_NOERROR) {
						sprintf(buffer,"waveInReset error %d\n", mmstatus);
						SetCtrlVal ( PANEL, PANEL_TXT_RESULTS, buffer);
					}
					mmstatus = waveInClose(waveInHandle);
					if (mmstatus != MMSYSERR_NOERROR) {
						sprintf(buffer,"waveInClose error %d\n", mmstatus);
						SetCtrlVal ( PANEL, PANEL_TXT_RESULTS, buffer);
					}
					waveInHandle = NULL;
				}
				
			}

			if (gState == RUNNING) { 
				gState = IDLE;
				SetCtrlAttribute(PANEL, PANEL_CB_TEST_SENSOR, ATTR_LABEL_TEXT , "__Test Sensor");
				return 0;
			}
			
			// Start acquisition
			ResetTextBox (PANEL, PANEL_TXT_RESULTS, "");  // Clear box 
			
			GetCtrlVal(panelHandle, PANEL_SENS_A, &sens_A);
			GetCtrlVal(panelHandle, PANEL_SENS_B, &sens_B);

			// Calculate scale factor
			gInt32SampleScaleFactor[0] = (1.0 / ((double)(sens_A) * G_IN_SI));	// - base integer conversion to percent of FSV and then convert to G
			gInt32SampleScaleFactor[1] = (1.0 / ((double)(sens_B) * G_IN_SI));	// 1 / 2^23 - base integer conversion to percent of FSV and then convert to G
			
			// Scale to mV peak
			gInt32SampleMvScaleFactor[0] =  ((5.1444+5.049 +5.049 ) / 3.0) / 8388608.0 / 2.0 * 1000.0; // Measured amplitude from 3 sensors scalled to sample
			gInt32SampleMvScaleFactor[1] =  ((2.5636+2.5923+2.5627) / 3.0) / 8388608.0 / 2.0 * 1000.0; // Measured amplitude from 3 sensors scalled to sample divided by 2^23
			
			// Get units of shaker acceleration
			GetCtrlVal(panelHandle, PANEL_RING_UNITS, &gG_units);
			if (gG_units == G_UNIT_RMS) {
				SetCtrlAttribute  (PANEL, PANEL_GRAPH_TIME_CH1, ATTR_YNAME, "Amplitude (g RMS)");
				SetCtrlAttribute  (PANEL, PANEL_GRAPH_TIME_CH2, ATTR_YNAME, "Amplitude (g RMS)");
				SetCtrlAttribute  (PANEL, PANEL_GRAPH_SPEC, 	ATTR_YNAME, "Amplitude (g RMS dB)");
				SetCtrlAttribute  (PANEL, PANEL_GRAPH_SPEC_AVG, ATTR_YNAME, "Amplitude (g RMS dB)"); 
				gYAXIS_Scale = RMS_TO_PEAK;
			} else if (gG_units == G_UNIT_PEAK) {
				SetCtrlAttribute  (PANEL, PANEL_GRAPH_TIME_CH1, ATTR_YNAME, "Amplitude (g peak)");
				SetCtrlAttribute  (PANEL, PANEL_GRAPH_TIME_CH2, ATTR_YNAME, "Amplitude (g peak)");
				SetCtrlAttribute  (PANEL, PANEL_GRAPH_SPEC, 	ATTR_YNAME, "Amplitude (g peak dB)");
				SetCtrlAttribute  (PANEL, PANEL_GRAPH_SPEC_AVG, ATTR_YNAME, "Amplitude (g peak dB)"); 
				gYAXIS_Scale = 1.0;
			}

			// Clear average buffer
			gAvgIteration = 0;
			SetCtrlVal(PANEL, PANEL_NUM_COMPLETION, gAvgIteration);	// Update Iteration 
			for(i=0; i < NUM_CHANS; i++) {
				for(j=0; j < NSAMPLES/2; j++) {
					gdAvgFreqData[i][j] = 0.0;
				}
			}
			
			// Get number of inputs devices
			waveInNumDevs = waveInGetNumDevs();
			sprintf(buffer,"Input devices found: %d\n", waveInNumDevs);
			SetCtrlVal ( PANEL, PANEL_TXT_RESULTS, buffer);  // Clear box and put starting text up
			
			//
			// Find out if our sensor is attached
			//
			for(device=0; device < waveInNumDevs; device++) {
				error = waveInGetDevCaps(device, &wave_capability, sizeof(wave_capability));
				if (error == 0) {
					chk = strstr(wave_capability.szPname, "333D");
					if (chk != NULL) {
						count_333d01++;
						if (first_333d01 == -1) {
							first_333d01 = device;
						}
					}
				}
			}
			//
			// Error handling
			//
			if (count_333d01 == 0) {
				// Need to attach a device
				MessagePopup("Sensor Not Found Error", "333D01 sensor not found.\nPlease attach a sensor.\nOr sensor has failed USB communication.");
				return 0;
			} else if (count_333d01 > 1) {
				// Only support one device at a time
				MessagePopup("Multiple Sensors Warning", "This application will only test the first 333D01 sensor found in the audio device list.");
			}
			
			// Get string
			waveInGetDevCaps(first_333d01, &wave_capability, sizeof(wave_capability));
			SetCtrlVal ( PANEL, PANEL_TXT_RESULTS, "Sensor Under Test\n");  
			SetCtrlVal ( PANEL, PANEL_TXT_RESULTS, &wave_capability.szPname[0]);  // Skip Microphone
			SetCtrlVal ( PANEL, PANEL_TXT_RESULTS, "\n");

			//
			// set the WAVEFORMATEX structure
			//
			memset(&wfmt,0,sizeof(WAVEFORMATEX));
			wfmt.nChannels = NUM_CHANS;
			wfmt.wBitsPerSample = RESOLUTION_BITS;
			wfmt.wFormatTag = WAVE_FORMAT_PCM;
		    wfmt.nSamplesPerSec = SAMPLE_RATE;
		    wfmt.nBlockAlign = (wfmt.wBitsPerSample/8)*2;
		    wfmt.nAvgBytesPerSec = wfmt.nSamplesPerSec*wfmt.nBlockAlign;
			// Open the device
			mmstatus = waveInOpen(&waveInHandle, first_333d01, &wfmt,(unsigned long)waveInProc, 0, CALLBACK_FUNCTION);		//  | WAVE_FORMAT_DIRECT
			if (mmstatus != MMSYSERR_NOERROR) {
				sprintf(buffer,"waveInOpen error %d\n", mmstatus);
				SetCtrlVal ( PANEL, PANEL_TXT_RESULTS, buffer);
				// If we fail to open, give up
				return 0;
			}
			
			// Associate buffers with wave capture
			for(i = 0;i < WAVE_BUFFER_COUNT;i++) {														 
				if (waveheader[i].lpData == NULL) {
					waveheader[i].lpData = (char *)	malloc( sizeof(short) * NSAMPLES * (wfmt.nBlockAlign));
					if (waveheader[i].lpData == NULL) {
						SetCtrlVal ( PANEL, PANEL_TXT_RESULTS, "Memory allocation failed.\n");
						return 0;
					}
				}
				waveheader[i].dwBufferLength = NSAMPLES*( wfmt.nBlockAlign);
				mmstatus = waveInPrepareHeader(waveInHandle, &waveheader[i],sizeof(WAVEHDR));
				if (mmstatus != MMSYSERR_NOERROR) {
					sprintf(buffer,"waveInPrepareHeader error %d\n", mmstatus);
					SetCtrlVal ( PANEL, PANEL_TXT_RESULTS, buffer);
					// If we fail to open, give up
					return 0;
				}
				mmstatus = waveInAddBuffer(waveInHandle,     &waveheader[i],sizeof(WAVEHDR));
				if (mmstatus != MMSYSERR_NOERROR) {
					sprintf(buffer,"waveInAddBuffer error %d\n", mmstatus);
					SetCtrlVal ( PANEL, PANEL_TXT_RESULTS, buffer);
					// If we fail to open, give up
					return 0;
				}
			}
			// Start the acquisition - callbacks will handle it from here
			mmstatus = waveInStart(waveInHandle);
			if (mmstatus != MMSYSERR_NOERROR) {
				sprintf(buffer,"waveInStart error %d\n", mmstatus);
				SetCtrlVal ( PANEL, PANEL_TXT_RESULTS, buffer);
				// If we fail to open, give up
				return 0;
			}

			// State machine
			gState = RUNNING;
			SetCtrlAttribute(PANEL, PANEL_CB_TEST_SENSOR, ATTR_LABEL_TEXT , "__Cancel");

			break;
	}
	return 0;
}

