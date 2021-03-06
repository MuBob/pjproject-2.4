// ConDlg.cpp : implementation file
//

#include "stdafx.h"
#include "microsip.h"
#include "ConDlg.h"
#include "afxdialogex.h"
#include <string>

IMPLEMENT_DYNAMIC(ConDlg, CDialog)

ConDlg::ConDlg(CWnd* pParent /*=NULL*/)
	: CDialog(IDD_DIALOG_CON, pParent)
{
	
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
    ON_BN_CLICKED(IDC_BUTTON_SENDFILE, &ConDlg::OnBnClickedButtonSendfile)
	ON_BN_CLICKED(IDC_BUTTON_SENDMSG, &ConDlg::OnBnClickedButtonSendmsg)
END_MESSAGE_MAP()


// ConDlg message handlers

void ConDlg::start_thread()
{

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
		if ((_taccess(SavePath, 0) != -1))
		{
			DeleteFile(SavePath);
		}
		CString fullpath;
		fullpath.Format(_T("\\\\?\\%s"), Main->m_RFPath.GetBuffer());;
		MoveFile(fullpath, _T("\\\\?\\") + SavePath);
	}
	return 0;
}
void ConDlg::Enable(BOOL bEnable /* = TRUE */)
{
	//GetDlgItem(IDC_RING)->EnableWindow(bEnable);
	GetDlgItem(IDC_BUTTON_SENDFILE)->EnableWindow(bEnable);
	GetDlgItem(IDC_BUTTON_SENDMSG)->EnableWindow(bEnable);
	GetDlgItem(IDC_EDIT_MSG)->EnableWindow(bEnable);
	if (!bEnable)
	{
		CStringA afName;
		pj_str_t strName;
		if (pjsua_steg_is_file_sending()>0)
		{
			afName = m_SFName;
			strName = StrAToPjStr(afName);
			strName.ptr = afName.GetBuffer();
			strName.slen = afName.GetLength();
			pjsua_steg_cancel_file_translate(&strName);
		}
		if (pjsua_steg_is_file_receiving()>0)
		{
			afName = m_RFName;
			strName = StrAToPjStr(afName);
			strName.ptr = afName.GetBuffer();
			strName.slen = afName.GetLength();
			pjsua_steg_cancel_file_translate(&strName);
		}
	}
}

CString ConDlg::ConvertUTF8ToCString(const pj_str_t* msg)
{//string类型的utf-8字符串转为CString类型的unicode字符串
		/* 预转换，得到所需空间的大小 */
	int nLen = ::MultiByteToWideChar(CP_UTF8, NULL, msg->ptr, msg->slen, NULL, 0);
	/* 转换为Unicode */
	std::wstring wbuffer;
	wbuffer.resize(nLen);
	::MultiByteToWideChar(CP_UTF8, NULL, msg->ptr, msg->slen,
		(LPWSTR)(wbuffer.data()), wbuffer.length());

#ifdef UNICODE
	return(CString(wbuffer.data(), wbuffer.length()));
#else
	/*
	 * 转换为ANSI
	 * 得到转换后长度
	 */
	nLen = WideCharToMultiByte(CP_ACP, 0,
		wbuffer.data(), wbuffer.length(), NULL, 0, NULL, NULL);

	std::string ansistr;
	ansistr.resize(nLen);

	/* 把unicode转成ansi */
	WideCharToMultiByte(CP_ACP, 0, (LPWSTR)(wbuffer.data()), wbuffer.length(),
		(LPSTR)(ansistr.data()), ansistr.size(), NULL, NULL);
	return(CString(ansistr.data(), ansistr.length()));
#endif
}

pj_str_t *ConDlg::UnicodeToUtf8(CString Unicodestr)
{
	char *buffer = NULL;
	int  length;

#ifdef _UNICODE
	length = WideCharToMultiByte(CP_UTF8, 0, Unicodestr, -1, NULL, 0, NULL, NULL);
#else
	return NULL;
#endif
	if (length <= 0)
	{
		return NULL;
	}

	buffer = new char[length];
	memset(buffer, 0, length);
#ifdef _UNICODE
	WideCharToMultiByte(CP_UTF8, 0, Unicodestr, -1, buffer, length-1, NULL, NULL);
#else
	strcpy_s(buffer, length, strValue);
#endif
	pj_str_t *result = new pj_str_t;
	result->ptr = buffer;
	result->slen = length;
	//	free(buffer);
	return result;
}

// air 2017.7.28
namespace rtf
{
	//	To avoid issues with multiple quoting and conversion of RTF characters
	//	we use \001 as a prefix/escape character
	//	There is a very tight link between the entries here and the font & color tables 
	//	initialised in the prefix.

	// Attributes
	const CString bold(_T("\001\\b "));
	const CString nobold(_T("\001\\b0 "));
	const CString underline(_T("\001\\ul "));
	const CString nounderline(_T("\001\\ul0 "));
	const CString italic(_T("\001\\i "));
	const CString noitalic(_T("\001\\i0 "));

	// Fonts
	const CString roman(_T("\001\\plain\001\\f2\001\\fs20 "));
	const CString fixed(_T("\001\\plain\001\\f5\001\\fs20 "));

	// foreground Colours
	const CString black(_T("\001\\cf0 "));
	const CString maroon(_T("\001\\cf1 "));
	const CString green(_T("\001\\cf2 "));
	const CString olive(_T("\001\\cf3 "));
	const CString purple(_T("\001\\cf4 "));
	const CString teal(_T("\001\\cf5 "));
	const CString grey(_T("\001\\cf6 "));
	const CString navy(_T("\001\\cf7 "));
	const CString red(_T("\001\\cf8 "));
	const CString lime(_T("\001\\cf9 "));
	const CString silver(_T("\001\\cf11 "));
	const CString yellow(_T("\001\\cf10 "));
	const CString fuchsia(_T("\001\\cf12 "));
	const CString aqua(_T("\001\\cf13 "));
	const CString white(_T("\001\\cf14 "));
	const CString blue(_T("\001\\cf15 "));
	//	const CString brown(	_T( "\001\\804000 " ) );

	// Other
	const CString bullet(_T("\001{\001\\pntext\001\\f1\001\\'b7\001\\tab\001}"));

	CString fontsize(int ptSize)
	{
		CString str;
		str.Format(_T("\001\\fs%d"), ptSize * 2);
		return str;
	}

	// not generally accessible outside this package
	//deff0: default font: f0
	//deftab720: default table720
	//using MS Sans Serif \'cb\'ce\'cc\'e5 to support the Chinese/Japanese characters
	const CString Prefix =
		_T("{\\rtf1\\ansi\\ansicpg936\\deff0\\deftab720\n")
		_T("{\\fonttbl")
		_T("{\\f0\\fswiss MS Sans Serif \'cb\'ce\'cc\'e5;}")
		_T("{\\f1\\froman\\fcharset2 Symbol \'cb\'ce\'cc\'e5;}")
		_T("{\\f2\\froman Times New Roman \'cb\'ce\'cc\'e5;}")
		_T("{\\f3\\froman Times New Roman \'cb\'ce\'cc\'e5;}")
		_T("{\\f4\\fswiss\\fprq2 MS Sans Serif \'cb\'ce\'cc\'e5;}")
		_T("{\\f5\\fmodern\\fprq1 Courier New \'cb\'ce\'cc\'e5;}")
		_T("{\\f6\\froman\\fprq2 Times New Roman \'cb\'ce\'cc\'e5;}}")
		_T("\n")
		_T("{\\colortbl")
		_T("\\red0\\green0\\blue0;")
		_T("\\red128\\green0\\blue0;")
		_T("\\red0\\green128\\blue0;")
		_T("\\red128\\green128\\blue0;")
		_T("\\red128\\green0\\blue128;")
		_T("\\red0\\green128\\blue128;")
		_T("\\red128\\green128\\blue128;")
		_T("\\red0\\green0\\blue128;")
		_T("\\red255\\green0\\blue0;")
		_T("\\red0\\green255\\blue0;")
		_T("\\red255\\green255\\blue0;")
		_T("\\red192\\green192\\blue192;")
		_T("\\red255\\green0\\blue255;")
		_T("\\red0\\green255\\blue255;")
		_T("\\red255\\green255\\blue255;")
		_T("\\red0\\green0\\blue255;}\n")
		_T("\\pard\\plain\\f0\\fs20 ");

	const CString Postfix = _T("\n\\par }");
};

//	const CString Postfix = _T( "}" );
static CString RTFprepare(const CString & str)
{
	CString ret;
	int size = str.GetLength();
	for (int i = 0; i < size; i++)
	{
		switch (str[i])
		{
		case '\001':
			if (i < size - 1)
			{
				i++;
				ret += str[i];
			}
			break;
		case '\n':
			ret += "\n\\par ";
			break;
		case '\r':
			break;
		case '\t':
			ret += "\\tab ";
			break;
		case '\\':
			ret += "\\\\";
			break;
		case '{':
			ret += "\\{";
			break;
		case '}':
			ret += "\\}";
			break;
		default:
			ret += str[i];
			break;
		}
	}
	return ret;
}
// air 2017.7.28
void ConDlg::REShow(int caller)
{
	CString temp;
	//EDITSTREAM es;
	//es.pfnCallback = CBStreamIn;
	//es.dwError = 0;
	
	if (!m_Content.IsEmpty() && !m_Line.IsEmpty())
		m_Content += _T("\n");
	m_Content += m_Line;

	temp = m_Content;
	if (m_SFP.GetLength())
		temp += _T("\n")+ rtf::olive + m_SFP;
	if (m_RFP.GetLength()) 
		temp += _T("\n") + rtf::purple + m_RFP;

	//if (m_Content.GetLength() && m_Line.GetLength())
	//	m_Content = m_Content + _T("\r\n") + m_Line;
	//if (m_Content.GetLength() == 0 && m_Line.GetLength()) m_Content = m_Line;
	//temp = m_Content;
	//if (temp.GetLength()) temp += _T("\r\n");
	//if (m_SFP.GetLength()) temp += m_SFP + _T("\r\n");
	//if (m_RFP.GetLength()) temp += m_RFP;

	//es.dwCookie = (DWORD)& temp;
	//m_Show.StreamIn(SF_TEXT | SF_UNICODE, es);
	temp = RTFprepare(temp);
	temp = rtf::Prefix + temp + rtf::Postfix;
	m_Show.SetSel(0, -1);
	m_Show.ReplaceSel(temp);

	switch (caller)
	{
	case 1:
		if (m_Content.GetLength()) m_Content = m_Content + _T("\n") + rtf::olive + m_SFP;
		else m_Content = m_SFP;
		m_SFP = _T("");
		break;
	case 2:
		if (m_Content.GetLength()) m_Content = m_Content + _T("\n") + rtf::purple + m_RFP;
		else m_Content = m_RFP;
		m_RFP = _T("");
		break;
	default:
		break;
	}

	this->m_Show.PostMessage(WM_VSCROLL, SB_BOTTOM, 0);
}

/**
发送文件
*/
void ConDlg::OnBnClickedButtonSendfile()
{
	// TODO: Add your control notification handler code here
	//CFileDialog getFile(TRUE,NULL,NULL,OFN_FILEMUSTEXIST|OFN_HIDEREADONLY);
	CFileDialog dlg(TRUE, NULL, NULL, OFN_FILEMUSTEXIST | OFN_HIDEREADONLY);
	if (pjsua_steg_is_file_sending() != 0)
	{
		AfxMessageBox(_T("Unfinished!"));
		return;
	}

	if (dlg.DoModal() == IDOK)
	{
		CString filename = dlg.GetFileName();
		CFileStatus FileStatus;
		CFile::GetStatus(filename, FileStatus);
		if (FileStatus.m_size > 100 * 1024)
		{
			if ((AfxMessageBox(_T("Do you want to send a file over 100KB?"), MB_YESNO | MB_DEFBUTTON2)) != IDYES)
				return;
		}
		if ((AfxMessageBox(_T("Send \"") + filename + _T("\"?"), MB_YESNO)) == IDYES)
		{
			//20161209 此处应有数据处理代码

			CStringA fa_path, fa_name;
//			CString fs_name;
			CStringA fs_name;
			m_SFPath = dlg.GetPathName();
//			pj_str_t* sfPath = UnicodeToUtf8(m_SFPath);
			fa_path = m_SFPath;
			pj_str_t sfPath = StrAToPjStr(fa_path);
			sfPath.ptr = fa_path.GetBuffer();
			sfPath.slen = fa_path.GetLength();
			m_SFName = filename;
			fs_name = _T("<") + filename + _T(">");
//			pj_str_t* sfName = UnicodeToUtf8(fs_name);
			pj_str_t sfName = StrAToPjStr(fs_name);
			sfName.ptr = fs_name.GetBuffer();
			sfName.slen = fs_name.GetLength();
			if (sfName.slen + 1 >= pjsua_steg_file_get_max_len())
				printf("too long file name!!\r\n");
			pj_status_t status = pjsua_steg_send_file(&sfPath, &sfName);
//			pj_status_t status = pjsua_steg_send_file(&sfPath, sfName);

			//			m_pSteg.lock();
			//			m_pSteg.Send((void *)Load.GetBuffer(), Load.GetLength()+1, 2);
			//			m_pSteg.unlock();
			m_sendfile.EnableWindow(status < 0);
			//m_strFilePath = dlg.GetPathName();
		}

	}
	UpdateData(FALSE);
}


/**
发送消息
*/
void ConDlg::OnBnClickedButtonSendmsg()
{
	int status = pjsua_steg_is_file_sending();
	CStringA afName;
	pj_str_t strName;
//	pj_str_t* strName;
	if (status>0)
	{
		PJ_LOG(4, (THIS_FILE, "send cancel name str=%s", m_SFName));
//		strName = UnicodeToUtf8(m_SFName);
		afName = m_SFName;
		strName = StrAToPjStr(afName);
		strName.ptr = afName.GetBuffer();
		strName.slen = afName.GetLength();
		PJ_LOG(4, (THIS_FILE, "send cancel name=%s, len=%d", strName.ptr, strName.slen));
		pjsua_steg_cancel_file_translate(&strName);
//		pjsua_steg_cancel_file_translate(strName);
		return;
	}
	else {
		status = pjsua_steg_is_file_receiving();
		if (status>0)
		{
			PJ_LOG(4, (THIS_FILE, "receive cancel name str=%s, status=%d", m_RFName, status));
//			strName = UnicodeToUtf8(m_SFName);
			afName = m_RFName;
			strName = StrAToPjStr(afName);
			strName.ptr = afName.GetBuffer();
			strName.slen = afName.GetLength();
			PJ_LOG(4, (THIS_FILE, "receive cancel name=%s, len=%d", strName.ptr, strName.slen));
			pjsua_steg_cancel_file_translate(&strName);
//			pjsua_steg_cancel_file_translate(strName);
			return;
		}
	}
	UpdateData(true);
	CEdit* pBoxOne;
	pBoxOne = (CEdit*)GetDlgItem(IDC_EDIT_MSG);
	CString str;
	pBoxOne->GetWindowText(str);
	if (str.IsEmpty())
		return;// air

	CStringA stra;
	stra = str;
	pj_str_t tmp = StrAToPjStr(stra);
	tmp.ptr = stra.GetBuffer();
	tmp.slen = stra.GetLength();
//	pj_str_t* tmp = UnicodeToUtf8(str);
	// air 2017.7.25
	if (tmp.slen + 1 >  pjsua_steg_file_get_max_len())
	{
		AfxMessageBox(_T("消息字符数超过限制！"));
		return;
	}
	pjsua_steg_send_msg(&tmp);
//	pjsua_steg_send_msg(tmp);
	CTime t = CTime::GetCurrentTime();
	CString tstr = t.Format(_T("[%H:%M:%S]:"));
	m_Line = rtf::grey+ tstr + rtf::red +  str;

//	m_Line = rtf::red + _T("[本地]： ") + str;

	REShow(0);
	m_Line = _T("");
	m_Msg = _T("");
	UpdateData(false);
	// TODO: Add your control notification handler code here
}

// air
BOOL ConDlg::PreTranslateMessage(MSG* pMsg)
{
	// TODO: Add your specialized code here and/or call the base class
	if (VK_RETURN == pMsg->wParam)
	{
		OnBnClickedButtonSendmsg();//这里是你对话框上某个button的响应函数名
		return True;
	}
	return CDialog::PreTranslateMessage(pMsg);
}

void ConDlg::on_pjmedia_file_receive_progress(const pj_str_t *fName, unsigned int len, float p)
{
	int r_len = (int)len;
	int received_len = (int)len*p;
	float per = p * 100;
//	m_RFPath = ConvertUTF8ToCString(fName);
	m_RFName = PjToStr(fName, FALSE);
	m_RFP.Format(_T("正在接收来自对方的文件“%s”\r\n已完成%d/%d字节(%.2f%%)"),
		m_RFName, received_len, r_len, per);
	REShow(0);
}

void ConDlg::on_pjmedia_file_receive_result(const pj_str_t *fName, int status)
{
	if (status==PJ_SUCCESS)
	{
//		m_RFPath = ConvertUTF8ToCString(fName);
		m_RFPath = PjToStr(fName, FALSE);
		m_RFP.Format(_T("已经接收来自对方的文件“%s”"), m_RFName);
		REShow(2);
		CWinThread * wt;
		m_sendfile.EnableWindow(TRUE);//air 2017.7.27
		if ((wt = AfxBeginThread(ReceiveFileDlg, this)) != NULL)
		{
			m_FileTrd[1] = wt->m_hThread;
		}

	}
	else {
		if (fName->slen>0)
		{
//			m_RFPath = ConvertUTF8ToCString(fName);
			m_RFPath = PjToStr(fName, FALSE);
		}
		else {
			m_RFPath = "未知";
		}
		m_RFP.Format(_T("已中断接收来自对方的文件“%s”"), m_RFPath);
		REShow(2);
	}
}

void ConDlg::on_pjmedia_file_send_progress(const pj_str_t *fName, unsigned int len, float p)
{
	int s_len = (int)len;
	int sended_len = (int)len*p;
	float per = p * 100;
//	CString temp = ConvertUTF8ToCString(fName);
	CString temp= PjToStr(fName, FALSE);
	m_SFP.Format(_T("正在发送文件：“%s”\r\n已完成%d/%d字节(%.2f%%)"),
		temp, sended_len, s_len, per);
	REShow(0);
}

void ConDlg::on_pjmedia_file_send_result(const pj_str_t *fName, int status)
{
	//	CString temp = ConvertUTF8ToCString(fName);
	CString temp = PjToStr(fName, FALSE);
	if (status==PJ_SUCCESS)
	{
		m_SFP.Format(_T("发送文件“%s”已完成"), temp);
	}
	else {
		m_SFP.Format(_T("发送文件“%s”失败，请尝试重新发送"), temp);
	}
	REShow(1);
	m_sendfile.EnableWindow(true);
}

void ConDlg::on_pjmedia_msg_receive_result(const pj_str_t *msg)
{
	if (msg->slen>0)
	{
//		CString temp = ConvertUTF8ToCString(msg);
		CString temp = PjToStr(msg, FALSE);
		CTime t = CTime::GetCurrentTime();
		CString tstr = t.Format(_T("[%H:%M:%S]:"));
		m_Line = rtf::grey + tstr + rtf::blue + temp;
		REShow(0);
		m_Line = _T("");
	}
}

void ConDlg::on_pjmedia_msg_send_result(int status)
{
	PJ_LOG(4, (THIS_FILE, "after send message status:%d", status));
}
