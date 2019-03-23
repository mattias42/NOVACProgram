// ColumnHistoryDlg.cpp : implementation file
//

#include "stdafx.h"
#include "../resource.h"
#include "ColumnHistoryDlg.h"
#include "afxdialogex.h"
#include <afxwin.h>
#include <ctime>
#include <fstream>
#include "../Configuration/Configuration.h"
#include "../Configuration/ConfigurationFileHandler.h"
#include "../UserSettings.h"
#include "../Common/EvaluationLogFileHandler.h"
#include <SpectralEvaluation/Utils.h>

extern CConfigurationSetting	g_settings;
extern CUserSettings			g_userSettings;

// ColumnHistoryDlg dialog

using namespace Graph;
using namespace FileHandler;

IMPLEMENT_DYNAMIC(ColumnHistoryDlg, CPropertyPage)

// Implemented in View_Scanner
void ResizeGraphControl(CGraphCtrl& controlToResize, CStatic& boundingFrame, CWnd* owningWindow);

ColumnHistoryDlg::ColumnHistoryDlg()
	: CPropertyPage(ColumnHistoryDlg::IDD)
{
	m_evalDataStorage = NULL;
	m_scannerIndex = 0;
	m_serialNumber.Format("");
	m_siteName.Format("");
}

ColumnHistoryDlg::~ColumnHistoryDlg()
{
}

void ColumnHistoryDlg::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	
	DDX_Control(pDX, IDC_COLUMN_FRAME, m_frame);
	DDX_Control(pDX, IDC_COLUMN_10DAY_FRAME, m_frame10);
	DDX_Control(pDX, IDC_COLUMN_30DAY_FRAME, m_frame30);
}


BEGIN_MESSAGE_MAP(ColumnHistoryDlg, CPropertyPage)
	ON_WM_SIZE()
	//ON_WM_CLOSE()
END_MESSAGE_MAP()


BOOL ColumnHistoryDlg::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	m_minColumn = g_settings.scanner[m_scannerIndex].minColumn;
	m_maxColumn = g_settings.scanner[m_scannerIndex].maxColumn;
	
	// Initialize plots
	InitPlot();
	Init10DayPlot();
	Init30DayPlot();

	// Read evaluation logs and plot columns
	DrawHistoryPlots();

	return TRUE;
}

void ColumnHistoryDlg::InitPlot() {

	CRect rect;
	m_frame.GetWindowRect(rect);
	int height = rect.bottom - rect.top;
	int width = rect.right - rect.left;
	rect.top = 20;
	rect.bottom = height - 10;
	rect.left = 10;
	rect.right = width - 10;
	m_plot.Create(WS_VISIBLE | WS_CHILD, rect, &m_frame);

	Common common;
	m_plot.SetXUnits(common.GetString(AXIS_UTCTIME));
	m_plot.SetXAxisNumberFormat(FORMAT_DATETIME);
	if (g_userSettings.m_columnUnit == UNIT_PPMM)
		m_plot.SetYUnits("Column [ppmm]");
	else if (g_userSettings.m_columnUnit == UNIT_MOLEC_CM2)
		m_plot.SetYUnits("Column [molec/cm�]");
	m_plot.EnableGridLinesX(true);
	m_plot.SetPlotColor(RGB(255, 255, 255));
	m_plot.SetGridColor(RGB(255, 255, 255));
	m_plot.SetBackgroundColor(RGB(0, 0, 0));
	m_plot.SetCircleColor(RGB(255,0,0));
	m_plot.SetCircleRadius(1);
}

void ColumnHistoryDlg::Init10DayPlot() {

	CRect rect;
	m_frame10.GetWindowRect(rect);
	int height = rect.bottom - rect.top;
	int width = rect.right - rect.left;
	rect.top = 20;
	rect.bottom = height - 10;
	rect.left = 10;
	rect.right = width - 10;
	m_plot10.Create(WS_VISIBLE | WS_CHILD, rect, &m_frame10);

	Common common;
	m_plot10.SetXUnits(common.GetString(AXIS_UTCTIME));
	m_plot10.SetXAxisNumberFormat(FORMAT_DATE);
	if (g_userSettings.m_columnUnit == UNIT_PPMM)
		m_plot10.SetYUnits("Column [ppmm]");
	else if (g_userSettings.m_columnUnit == UNIT_MOLEC_CM2)
		m_plot10.SetYUnits("Column [molec/cm�]");
	m_plot10.EnableGridLinesX(true);
	m_plot10.SetPlotColor(RGB(255, 255, 255));
	m_plot10.SetGridColor(RGB(255, 255, 255));
	m_plot10.SetBackgroundColor(RGB(0, 0, 0));
	m_plot10.SetCircleColor(RGB(255, 0, 0));
	m_plot10.SetCircleRadius(1);
}

void ColumnHistoryDlg::Init30DayPlot() {

	CRect rect;
	m_frame30.GetWindowRect(rect);
	int height = rect.bottom - rect.top;
	int width = rect.right - rect.left;
	rect.top = 20; 
	rect.bottom = height - 10;
	rect.left = 10; 
	rect.right = width - 10;
	m_plot30.Create(WS_VISIBLE | WS_CHILD, rect, &m_frame30);

	Common common;
	m_plot30.SetXUnits(common.GetString(AXIS_UTCTIME));
	m_plot30.SetXAxisNumberFormat(FORMAT_DATE);
	if (g_userSettings.m_columnUnit == UNIT_PPMM)
		m_plot30.SetYUnits("Column [ppmm]");
	else if (g_userSettings.m_columnUnit == UNIT_MOLEC_CM2)
		m_plot30.SetYUnits("Column [molec/cm�]");
	m_plot30.EnableGridLinesX(true);
	m_plot30.SetPlotColor(RGB(255, 255, 255));
	m_plot30.SetGridColor(RGB(255, 255, 255));
	m_plot30.SetBackgroundColor(RGB(0, 0, 0));
	m_plot30.SetCircleColor(RGB(255, 0, 0));
	m_plot30.SetCircleRadius(1);

}
void ColumnHistoryDlg::DrawPlot() {
	// variables
	const int BUFFER_SIZE = 10000;
	double time[BUFFER_SIZE], column[BUFFER_SIZE], columnError[BUFFER_SIZE];

	// Get the data
	int dataLength = m_evalDataStorage->GetColumnData(m_serialNumber, column, columnError, BUFFER_SIZE, true);

	// If there's no data then don't draw anything
	if (dataLength <= 0)
		return;
	
	// remove the old plot
	m_plot.CleanPlot();

	// Set the unit of the plot
	if (g_userSettings.m_columnUnit == UNIT_PPMM)
		m_plot.SetYUnits("Column [ppmm]");
	else if (g_userSettings.m_columnUnit == UNIT_MOLEC_CM2)
		m_plot.SetYUnits("Column [molec/cm�]");

	SetRange();
	
	m_evalDataStorage->GetTimeData(m_serialNumber, time, BUFFER_SIZE, true);

	// draw the columns;
	m_plot.XYPlot(time, column, dataLength, CGraphCtrl::PLOT_FIXED_AXIS | CGraphCtrl::PLOT_CIRCLES);
}

void ColumnHistoryDlg::DrawHistoryPlots() {
	// clean plots
	if (!IsWindow(m_frame10.m_hWnd) || !IsWindow(m_frame30.m_hWnd)) {
		return;
	}
	m_plot10.CleanPlot();
	m_plot30.CleanPlot();

	// get column units
	double unitConversionFactor = 0;
	if (g_userSettings.m_columnUnit == UNIT_PPMM)
		unitConversionFactor = 1.0;
	else if (g_userSettings.m_columnUnit == UNIT_MOLEC_CM2)
		unitConversionFactor = 2.5e15;

	// set time range
	SetHistoryRange();

	// get current time UTC
	time_t rawtime;
	struct tm * utc;
	time(&rawtime);
	utc = gmtime(&rawtime);
	// get time offset
	time_t offset = mktime(utc) - rawtime;
	if (utc->tm_isdst) {
		offset -= 3600;
	}
	// make it midnight (00:00:00) of current day
	utc->tm_hour = 0;
	utc->tm_min = 0;
	utc->tm_sec = 0;

	// Read last 30 days worth of eval logs
	int index = 0;
	const int BUFFER_SIZE = 3000;
	double time[BUFFER_SIZE], column[BUFFER_SIZE];
	CString path, dateStr;
	CWaitCursor wait;
	for (int day = 0; day < 30; day++) {
		FileHandler::CEvaluationLogFileHandler evalLogReader;

		// get date
		utc->tm_sec -= (SECONDS_IN_DAY);
		time_t epochDay = mktime(utc);
		int year = utc->tm_year + 1900;
		int month = utc->tm_mon + 1;
		int mday = utc->tm_mday;
		dateStr.Format("%04d.%02d.%02d", year, month, mday);

		// Read file
		path.Format("%sOutput\\%s\\%s\\EvaluationLog_%s_%s.txt",
			(LPCTSTR)g_settings.outputDirectory,
			(LPCTSTR)dateStr,
			(LPCTSTR)m_serialNumber,
			(LPCTSTR)m_serialNumber,
			(LPCTSTR)dateStr);

		// check if file exists
		std::ifstream ifile(path);
		if (!ifile) {
			continue;
		}

		// Try to read the eval-log
		evalLogReader.m_evaluationLog.Format(path);
		if (FAIL == evalLogReader.ReadEvaluationLog())
			continue;

		// check if there are scan in log
		if (evalLogReader.m_scanNum <= 0) {
			continue;
		}

		// Plot the read in data
		int scanNum = evalLogReader.m_scanNum;
		for (int j = 0; j < scanNum; ++j) {
			Evaluation::CScanResult &sr = evalLogReader.m_scan[j];
			for (unsigned long k = 0; k < sr.GetEvaluatedNum(); ++k) {
				// Check if this is a dark measurement, if so then don't include it...
				// 1. Clean the spectrum name from special characters...
				std::string spectrumName = CleanString(sr.GetSpectrumInfo(k).m_name);
				Trim(spectrumName, " \t");
				if (fabs(sr.GetScanAngle(k)) - 180.0 < 1e-3 && (EqualsIgnoringCase(spectrumName, "offset") || EqualsIgnoringCase(spectrumName, "dark_cur") || EqualsIgnoringCase(spectrumName, "dark"))) {
					continue;
				}

				CDateTime st;
				sr.GetStartTime(k, st);
				int startsec = st.hour * 3600 + st.minute * 60 + st.second;
				double epoch = (double)(epochDay + startsec - offset); // unsure why offset substraction is needed but it is
				double col = sr.GetColumn(k, 0); //Assumes SO2 ref is at index 0
				bool isBadFit = sr.IsBad(k);
				if (!isBadFit) {
					time[index] = epoch;
					column[index] = col*unitConversionFactor;
					index++;
				}
			}

			index = min(BUFFER_SIZE, index); // make sure no overflow
			if (day < 10) {
				m_plot10.XYPlot(time, column, index, CGraphCtrl::PLOT_FIXED_AXIS | CGraphCtrl::PLOT_CIRCLES);
			}
			m_plot30.XYPlot(time, column, index, CGraphCtrl::PLOT_FIXED_AXIS | CGraphCtrl::PLOT_CIRCLES);

			index = 0;
		}
	}
	wait.Restore();
}

// ColumnHistoryDlg message handlers


void ColumnHistoryDlg::OnSize(UINT nType, int cx, int cy)
{
	CPropertyPage::OnSize(nType, cx, cy);

    const int rightMargin = 50;

    CRect thisClientRegion;
    GetWindowRect(thisClientRegion);
    this->ScreenToClient(thisClientRegion);

    // Adjust the width of each of the column plots
    if (this->m_frame.m_hWnd != NULL) {
        CRect frameClientRegion;
        m_frame.GetWindowRect(&frameClientRegion);
        this->ScreenToClient(frameClientRegion);

        frameClientRegion.right = thisClientRegion.right - rightMargin;
        m_frame.MoveWindow(frameClientRegion);

        ResizeGraphControl(m_plot, m_frame, this);
    }

    if (this->m_frame10.m_hWnd != NULL) {
        CRect frameClientRegion;
        m_frame10.GetWindowRect(&frameClientRegion);
        this->ScreenToClient(frameClientRegion);

        frameClientRegion.right = thisClientRegion.right - rightMargin;
        m_frame10.MoveWindow(frameClientRegion);

        ResizeGraphControl(m_plot10, m_frame10, this);
    }

    if (this->m_frame30.m_hWnd != NULL) {
        CRect frameClientRegion;
        m_frame30.GetWindowRect(&frameClientRegion);
        this->ScreenToClient(frameClientRegion);

        frameClientRegion.right = thisClientRegion.right - rightMargin;
        m_frame30.MoveWindow(frameClientRegion);

        ResizeGraphControl(m_plot30, m_frame30, this);
    }
}

BOOL ColumnHistoryDlg::OnSetActive()
{	

	DrawPlot();
	return CPropertyPage::OnSetActive();
}

void ColumnHistoryDlg::SetRange() {
	struct tm *tm;
	time_t t;
	time(&t);
	tm = gmtime(&t);
	time_t endtime = t;
	time_t starttime = endtime - SECONDS_IN_DAY;
	m_plot.SetRange(starttime, endtime, 0, m_minColumn, m_maxColumn, 0);
}

void ColumnHistoryDlg::SetHistoryRange() {
	// get end time
	struct tm *tm;
	time_t t;
	time(&t);
	tm = gmtime(&t);
	time_t endtime = t - (3600 * tm->tm_hour + 60 * tm->tm_min + tm->tm_sec);
	// get start time and set 10 day plot
	time_t starttime = endtime - SECONDS_IN_DAY * 10;
	m_plot10.SetRange(starttime, endtime, 0, m_minColumn, m_maxColumn, 0);
	// get start time and set 30 day plot
	starttime = endtime - SECONDS_IN_DAY * 30;
	m_plot30.SetRange(starttime, endtime, 0, m_minColumn, m_maxColumn, 0);
}
void ColumnHistoryDlg::RedrawAll() {
	DrawPlot();
	DrawHistoryPlots();
}
