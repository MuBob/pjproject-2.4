#pragma once
UINT ReceiveFileDlg(LPVOID pParam);  // �����ļ��̣߳���ʾͨ�öԻ���


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
	UINT friend ReceiveFileDlg(LPVOID pParam);  // �����������߳�ʹ�öԻ�����ı���
	LRESULT OnSIAClear(WPARAM w, LPARAM l);
	LRESULT  OnSIArrive(WPARAM w, LPARAM l);    // ���µ�SIA�����ݵ���
	virtual BOOL OnInitDialog();
	CWinThread* pThread;
	CString m_Content, m_Line, m_RFP, m_SFP;   // ������ʾ�ַ���
	void REShow(int caller);
//	static DWORD CALLBACK CBStreamIn(DWORD dwCookie, LPBYTE pbBuff, LONG cb, LONG *pcb);  // air
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	CRichEditCtrl m_Show;
	CButton  m_sendfile;
	CString m_Msg;
	DECLARE_MESSAGE_MAP()
	CString m_SFPath, m_RFName; //���ͽ����ļ���
	CFile m_fileS, m_fileR;
	UINT m_SLength, m_SndLength, m_RLength, m_RcvLength;
	UINT m_SFStep, m_RFStep;//
	CString m_exePath;      // ���ڱ���Ӧ�ó�������·��
	HANDLE m_FileTrd[2];    // ���ͽ����ļ������̵߳ľ�������ڵȴ��߳̽���

public:
//	BOOL modal_ornot;
	afx_msg void OnBnClickedButtonSendfile();
	void start_thread();

	afx_msg void OnBnClickedButtonSendmsg();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	void Enable(BOOL bEnable /* = TRUE */);
};
