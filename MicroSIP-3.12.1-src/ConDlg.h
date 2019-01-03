#include "global.h"

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
	CString m_RFName, m_RFPath; //发送接收文件名
	


	CString m_SFName, m_SFPath;           //发送文件名
	HANDLE m_FileTrd[2];    // 发送接收文件两个线程的句柄，用于等待线程结束

public:
//	BOOL modal_ornot;
	afx_msg void OnBnClickedButtonSendfile();
	void start_thread();

	afx_msg void OnBnClickedButtonSendmsg();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	void Enable(BOOL bEnable /* = TRUE */);

	void on_pjmedia_file_receive_progress(const pj_str_t *fName, unsigned int len, float p);
	void on_pjmedia_file_receive_result(const pj_str_t *fName, int status);
	void on_pjmedia_file_send_progress(const pj_str_t *fName, unsigned int len, float p);
	void on_pjmedia_file_send_result(const pj_str_t *fName, int status);
	void on_pjmedia_msg_receive_result(const pj_str_t *msg);
	void on_pjmedia_msg_send_result(int status);
};
