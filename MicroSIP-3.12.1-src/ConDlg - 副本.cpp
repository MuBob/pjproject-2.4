// ConDlg.cpp : implementation file
//

#include "stdafx.h"
#include "microsip.h"
#include "ConDlg.h"
#include "global.h"
#include "afxdialogex.h"
#include "../../third_party/pjsua_libsteg/StegSuit.h"
#include "mainDlg.h"

extern CStegSuit m_pSteg; // //lpc 2016.12.15

// ConDlg dialog

IMPLEMENT_DYNAMIC(ConDlg, CDialog)

UINT ThreadFunc(LPVOID pParm)

{
	while (1) {
		ConDlg* pInfo = (ConDlg*)pParm;
		//CmainDlg *pMPD = (CmainDlg*)AfxGetMainWnd();
		Sleep(10);
		//if (m_pSteg.steg_status == NONE)
		//	continue;
		if (m_pSteg.bMessageArrived) {
			pInfo->PostMessage(WM_SIARRIVE, 1, 0);			
		}

		if (m_pSteg.bFileSent)
		{
			pInfo->PostMessage(WM_SIACLEAR, 2, 0);
		}
		
		if (m_pSteg.bFileArrived)
		{
			pInfo->PostMessage(WM_SIARRIVE, 2, 0);
		}
	}
	return 0;
}
ConDlg::ConDlg(CWnd* pParent /*=NULL*/)
	: CDialog(IDD_DIALOG_CON, pParent)
{
//	modal_ornot = FALSE;
	m_RFStep = 0;
	m_SFStep = 0;
	TCHAR Path[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, Path);
	m_exePath.Format(_T("%s"), Path);
	pThread = AfxBeginThread(ThreadFunc, this);

}
BOOL ConDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	return TRUE;
}

ConDlg::~ConDlg()
{
}

void ConDlg::DoDataExchange(CDataExchange* pDX)
{
	DDX_Control(pDX, IDC_BUTTON_SENDFILE, m_sendfile);
	DDX_Control(pDX, IDC_RING, m_Show);
	DDX_Text(pDX, IDC_EDIT_MSG, m_Msg);
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(ConDlg, CDialog)
	ON_MESSAGE(WM_SIARRIVE, OnSIArrive)
	ON_MESSAGE(WM_SIACLEAR, OnSIAClear)
    ON_BN_CLICKED(IDC_BUTTON_SENDFILE, &ConDlg::OnBnClickedButtonSendfile)
	ON_BN_CLICKED(IDC_BUTTON_SENDMSG, &ConDlg::OnBnClickedButtonSendmsg)
END_MESSAGE_MAP()


// ConDlg message handlers

void ConDlg::start_thread()
{

}
void ConDlg::OnBnClickedButtonSendfile()
{
	// TODO: Add your control notification handler code here
	//CFileDialog getFile(TRUE,NULL,NULL,OFN_FILEMUSTEXIST|OFN_HIDEREADONLY);
	CFileDialog dlg(TRUE, NULL, NULL, OFN_FILEMUSTEXIST | OFN_HIDEREADONLY);
	if (m_SFStep != 0)
	{
		AfxMessageBox(_T("Unfinished!"));
		return;
	}
	if (dlg.DoModal() == IDOK)
	{
		CString filename =dlg.GetFileName();
		if ((AfxMessageBox(_T("Send \"") + filename+ _T("\"?"), MB_YESNO)) == IDYES)
		{
			//20161209 �˴�Ӧ�����ݴ������


			//CTime t = CTime::GetCurrentTime();
			//CString tstr = t.Format(_T("Hour: %H  Min: %M  Sec: %S\n"));
			//TRACE(_T("------------------Send File Time----------------\n%s"), tstr.GetBuffer());
			m_SFPath = dlg.GetPathName();
			m_SFStep = 1;		
			//air
			filename = _T("<") + dlg.GetFileName() + _T(">");
			CStringA Load;
			Load = filename;
			if (Load.GetLength()+1>=m_pSteg.SIADU)
				printf("too long file name!!\r\n");
			m_pSteg.lock();
			m_pSteg.Send((void *)Load.GetBuffer(), Load.GetLength()+1, 2);
			m_pSteg.unlock();
			m_sendfile.EnableWindow(false);
			//m_strFilePath = dlg.GetPathName();
		}

	}
	UpdateData(FALSE);
}


UINT  ReceiveFileDlg(LPVOID pParam)
{
	ConDlg * Main = (ConDlg *)pParam;
	TCHAR tempPath[MAX_PATH];
	wcscpy_s(tempPath, MAX_PATH, Main->m_RFName.GetBuffer());
	CFileDialog  saveFile(FALSE, NULL, tempPath, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT);
	CTime t = CTime::GetCurrentTime();
	CString tstr = t.Format(_T("Hour: %H  Min: %M  Sec: %S\n"));
	TRACE(_T("------------------Receive File Time----------------\n%s"), tstr.GetBuffer());
	if (saveFile.DoModal() == IDOK)
	{
		CString SavePath = saveFile.GetPathName();
		CString fullpath = Main->m_exePath + _T("\\") + Main->m_RFName;
		MoveFile(_T("\\\\?\\") + fullpath, _T("\\\\?\\") + SavePath);
	}
	Main->m_RFStep = 0;
	return 0;
}
void ConDlg::Enable(BOOL bEnable /* = TRUE */)
{
	//GetDlgItem(IDC_RING)->EnableWindow(bEnable);
	GetDlgItem(IDC_BUTTON_SENDFILE)->EnableWindow(bEnable);
	GetDlgItem(IDC_BUTTON_SENDMSG)->EnableWindow(bEnable);
	GetDlgItem(IDC_EDIT_MSG)->EnableWindow(bEnable);

}

//SIA�����ݷ������
LRESULT ConDlg::OnSIAClear(WPARAM w, LPARAM l)
{
	UINT type = (UINT)w;
	//test
	//printf("when OnSIAClear called ,type = %d\n",type);
	//test
	if (type == 1)
	{
		//m_pSteg.steg_status = NONE;
		return 1;
	}
	//�����2���Ƕ������ļ�
	if (type == 2)
	{
		//�ļ�����
		if (m_SFStep == 1)
		{
			if (m_fileS.m_hFile != CFile::hFileNull)
			{
				m_fileS.Close();
			}
			m_fileS.Open(m_SFPath, CFile::modeRead);
			m_SLength = (UINT)m_fileS.GetLength();
			m_pSteg.lock();
			m_pSteg.bFileSent =false;
			m_pSteg.Send((void *)&m_SLength, sizeof(UINT), 2);
			m_pSteg.unlock();
			TRACE(_T("Send File length\n"));
			m_SndLength = 0;
			m_SFStep = 2;
		}
		/*�ڶ������ļ�*/
		else if (m_SFStep == 2)
		{
			BYTE * tempread = new BYTE[m_pSteg.SIADU];
			UINT length = m_fileS.Read(tempread, m_pSteg.SIADU);
			m_SFP.Format(_T("���ڷ����ļ���%s\r\n�����%d/%d�ֽ�(%2.0f%%)"),
				m_fileS.GetFileName().GetBuffer(), m_SndLength, m_SLength, float(m_SndLength) / float(m_SLength) * 100);
			REShow(0);
			m_pSteg.lock();
			m_pSteg.bFileSent = false;
			m_pSteg.Send((void *)tempread, length, 2);
			m_pSteg.unlock();
			m_SndLength += length;

			if (m_SndLength == m_SLength)
				m_SFStep = 3;
			delete [] tempread;
		}
		else if (m_SFStep == 3)
		{
			m_pSteg.lock();
			m_pSteg.bFileSent = false;
			m_pSteg.unlock();
			m_SFP.Format(_T("�����ļ���%s  �����"), m_fileS.GetFileName().GetBuffer());
			m_fileS.Close();
			m_SFStep = 0;
			REShow(1);
			m_sendfile.EnableWindow(true);
		}
		return 2;
	}
	return 0;
}
void ConDlg::REShow(int caller)
{
	CString temp;
	EDITSTREAM es;
	es.pfnCallback = CBStreamIn;
	es.dwError = 0;

	if (m_Content.GetLength() && m_Line.GetLength())
		m_Content = m_Content + _T("\r\n") + m_Line;
	if (m_Content.GetLength() == 0 && m_Line.GetLength()) m_Content = m_Line;
	temp = m_Content;
	if (temp.GetLength()) temp += _T("\r\n");
	if (m_SFP.GetLength()) temp += m_SFP + _T("\r\n");
	if (m_RFP.GetLength()) temp += m_RFP;

	es.dwCookie = (DWORD)& temp;
	m_Show.StreamIn(SF_TEXT | SF_UNICODE, es);

	switch (caller)
	{
	case 1:
		if (m_Content.GetLength()) m_Content = m_Content + _T("\r\n") + m_SFP;
		else m_Content = m_SFP;
		m_SFP = _T("");
		break;
	case 2:
		if (m_Content.GetLength()) m_Content = m_Content + _T("\r\n") + m_RFP;
		else m_Content = m_RFP;
		m_RFP = _T("");
		break;
	default:
		break;
	}

	this->m_Show.PostMessage(WM_VSCROLL, SB_BOTTOM, 0);
}
DWORD CALLBACK ConDlg::CBStreamIn(DWORD dwCookie, LPBYTE pbBuff, LONG cb, LONG *pcb)
{
	CString * pMsg = (CString *)dwCookie;

	if (pMsg->GetLength() * 2 < cb)
	{
		*pcb = pMsg->GetLength() * 2;
		memcpy(pbBuff, (void *)pMsg->GetBuffer(), *pcb);
		pMsg->Empty();
	}
	else
	{
		if (cb % 2) *pcb = cb - 1; else *pcb = cb;
		memcpy(pbBuff, (void *)pMsg->GetBuffer(), *pcb);
		*pMsg = pMsg->Right(pMsg->GetLength() - cb / 2);
	}

	return 0;
}

/**������Ϣ
*/

void ConDlg::OnBnClickedButtonSendmsg()
{
	UpdateData(true);
	CEdit* pBoxOne;
	pBoxOne = (CEdit*)GetDlgItem(IDC_EDIT_MSG);
	CString str;
	pBoxOne->GetWindowText(str);
	if (str.IsEmpty())
		return;// air

	
	CStringA tmp;
	tmp = str;
	m_pSteg.lock();
	//m_pSteg.Send((void *)tmp.ptr, tmp.slen + 1, 1);
	m_pSteg.Send(tmp.GetBuffer(), tmp.GetLength()+1, 1);
	m_pSteg.unlock();

	m_Line = _T("���أ�") + str;

	REShow(0);
	m_Line = _T("");
	m_Msg = _T("");
	UpdateData(false);
	// TODO: Add your control notification handler code here
}

LRESULT  ConDlg::OnSIArrive(WPARAM w, LPARAM l)
{
	unsigned char * Msg = new unsigned char[m_pSteg.SIADU];
	memset(Msg, 0, sizeof(char) * m_pSteg.SIADU);
	UINT type = (UINT)w;
	EDITSTREAM es;
	es.pfnCallback = CBStreamIn;
	es.dwError = 0;
	//test
	//printf("when OnSIArrive called ,type = %d\n",type);
	//test
	if (type == 1)
	{
		m_pSteg.lock();	
		m_pSteg.bMessageArrived = false;
		UINT length  = m_pSteg.Receive((void *)Msg, m_pSteg.SIADU, 1);
		if (length != 0)
		{
			CStringA strAnsi = (LPCSTR)Msg;
			CString temp;
			temp = strAnsi;
			m_Line.Format(_T("�Է���%s"), temp);
			REShow(0);
			m_Line = _T("");
			m_pSteg.length = 0;			
			
		}

		m_pSteg.unlock();
		delete[] Msg;
		return 1;
	}


	if (type == 2)
	{
		if (m_RFStep == 0)
		{
			m_pSteg.lock();
			m_pSteg.bFileArrived = false;
			UINT ret = m_pSteg.Receive((void *)Msg, -1, 2);
			m_pSteg.unlock();
			if (ret == 1)
			{
				printf("1\r\n");
				//CString temp = CString(Msg);
				CStringA strAnsi = (LPCSTR)Msg;
				//m_RFName.Format(_T("%s"), Msg);
				m_RFName = strAnsi;
				CString Fullpath = m_exePath + _T("\\") + m_RFName;
				if (m_fileR.m_hFile != CFile::hFileNull)
				{
					m_fileR.Close();
				}
				m_fileR.Open(Fullpath, CFile::modeCreate | CFile::modeReadWrite);
				TRACE(_T("File Name anounced: %s\n"), Msg);
				m_RFStep = 1;
			}
			else
				printf("1error!\r\n");

		}
		else if (m_RFStep == 1)
		{
			UINT TLength;
			m_pSteg.lock();
			m_pSteg.bFileArrived = false;
			UINT ret = m_pSteg.Receive((void *)& TLength, -2, 2);
			m_pSteg.unlock();
			if (ret == 2)
			{
				printf("2\r\n");
				m_RLength = TLength;
				m_RcvLength = 0;
				m_RFStep = 2;
				m_RFP.Format(_T("���ڽ������ԶԷ����ļ���%s.\r\n�����0/%d�ֽ�"),
					m_RFName.GetBuffer(), m_RLength);
				//test
				//printf("when m_RFStep == 1 , m_RFStep = %d\n",m_RFStep);
				//test
				REShow(0);
			}
			else
			{
				printf("2error!\r\n");
			}
		}
		else if (m_RFStep == 2)
		{
			m_pSteg.lock();
			m_pSteg.bFileArrived = false;
			UINT ret = m_pSteg.Receive((void *)Msg, m_RLength - m_RcvLength, 2);
			m_pSteg.unlock();
			if (ret)
			{
				m_fileR.Write(Msg, ret);
				m_fileR.Flush();
				m_RcvLength += ret;
				m_RFP.Format(_T("���ڽ������ԶԷ����ļ���%s\r\n�����%d/%d�ֽ�(%2.0f%%)"),
					m_RFName.GetBuffer(), m_RcvLength, m_RLength, float(m_RcvLength) / float(m_RLength) * 100);
				REShow(0);
			}

			if (m_RcvLength == m_RLength)
			{
				m_fileR.Close();
				m_RFStep = 0;
				REShow(2);
				CWinThread * wt;
				if ((wt = AfxBeginThread(ReceiveFileDlg, this)) != NULL)
				{
					m_FileTrd[1] = wt->m_hThread;
				}
			}
		}
		delete[] Msg;
		return 2;
	}
	delete[] Msg;
	return 0;
}
// air
BOOL ConDlg::PreTranslateMessage(MSG* pMsg)
{
	// TODO: Add your specialized code here and/or call the base class
	if (VK_RETURN   == pMsg->wParam)
	{
		OnBnClickedButtonSendmsg();//��������Ի�����ĳ��button����Ӧ������
		return 1;
	}
	return CDialog::PreTranslateMessage(pMsg);
}
