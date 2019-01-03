#include "global.h"

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
	CString m_RFName, m_RFPath; //���ͽ����ļ���
	


	CString m_SFName, m_SFPath;           //�����ļ���
	HANDLE m_FileTrd[2];    // ���ͽ����ļ������̵߳ľ�������ڵȴ��߳̽���

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
