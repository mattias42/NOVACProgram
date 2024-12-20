#pragma once
#include "afxwin.h"
#include "afxdlgs.h"
#include "../Graphs/GraphCtrl.h"


class CEvaluatedDataStorage;

// ColumnHistoryDlg dialog
class ColumnHistoryDlg : public CPropertyPage
{
    DECLARE_DYNAMIC(ColumnHistoryDlg)


public:
    ColumnHistoryDlg();   // standard constructor
    virtual ~ColumnHistoryDlg();

    virtual BOOL OnInitDialog();

    // Dialog Data
    enum { IDD = IDD_COLUMN_HISTORY_DLG };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

    DECLARE_MESSAGE_MAP()

public:
    afx_msg void OnSize(UINT nType, int cx, int cy);
    virtual BOOL OnSetActive();
    LRESULT OnEvalSuccess(WPARAM wParam, LPARAM lParam);

    /** A pointer to a shared instance of 'CEvaluatedDataStorage' */
    CEvaluatedDataStorage* m_evalDataStorage;

    // scanner info
    int m_scannerIndex;
    CString m_serialNumber;
    CString m_siteName;

    // last day history plot was drawn
    int m_lastDay;

    void DrawPlot();
    void DrawHistoryPlots();
    void RedrawAll();
private:

    // min and max column (Y-axis range)
    int m_minColumn;
    int m_maxColumn;

    // last 24 hour plot
    CStatic m_frame;
    Graph::CGraphCtrl m_plot;

    // 10 day plot
    CStatic m_frame10;
    Graph::CGraphCtrl m_plot10;

    // 30 day plot
    CStatic m_frame30;
    Graph::CGraphCtrl m_plot30;

    // last 24 hour plot
    void InitPlot();
    void SetRange();

    // 10 and 30 day plots
    void Init10DayPlot();
    void Init30DayPlot();
    void SetHistoryRange();

    // data for 10 and 30 day plots
    double m_time[30][10000];
    double m_column[30][10000];
    int m_index[30];
    void ReadEvalLogs();

    int SECONDS_IN_DAY = 86400;
};
