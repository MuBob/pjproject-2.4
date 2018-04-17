#pragma once
UINT ReceiveFileDlg(LPVOID pParam);  // 接收文件线程，显示通用对话框


// ConDlg dialog

class ConDlg : public CDialog
{
	DECLARE_DYNAMIC(ConDlg)

public:
	ConDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~ConDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_CON };
#endif

protected:
	UINT friend ReceiveFileDlg(LPVOID pParam);  // 方便这两个线程使用对话框类的变量
	LRESULT OnSIAClear(WPARAM w, LPARAM l);
	LRESULT  OnSIArrive(WPARAM w, LPARAM l);    // 有新的SIA层数据到达
	virtual BOOL OnInitDialog();
	CWinThread* pThread;
	CString m_Content, m_Line, m_RFP, m_SFP;   // 界面显示字符串
	void REShow(int caller);
//	static DWORD CALLBACK CBStreamIn(DWORD dwCookie, LPBYTE pbBuff, LONG cb, LONG *pcb);  // air
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	CRichEditCtrl m_Show;
	CButton  m_sendfile;
	CString m_Msg;
	DECLARE_MESSAGE_MAP()
	CString m_SFPath, m_RFName; //发送接收文件名
	CFile m_fileS, m_fileR;
	UINT m_SLength, m_SndLength, m_RLength, m_RcvLength;
	UINT m_SFStep, m_RFStep;//
	CString m_exePath;      // 用于保存应用程序所在路径
	HANDLE m_FileTrd[2];    // 发送接收文件两个线程的句柄，用于等待线程结束

public:
//	BOOL modal_ornot;
	afx_msg void OnBnClickedButtonSendfile();
	void start_thread();

	afx_msg void OnBnClickedButtonSendmsg();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	void Enable(BOOL bEnable /* = TRUE */);
};
