/* 
 * Copyright (C) 2011-2016 MicroSIP (http://www.microsip.org)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */

#pragma once

#include "resource.h"
#include "global.h"
#include "BaseDialog.h"

class Calls :
	public CBaseDialog
{
public:
	Calls(CWnd* pParent = NULL);	// standard constructor
	~Calls();
	enum { IDD = IDD_CALLS };

	void Add(pj_str_t id, CString number, CString name, int type);
	void SetDuration(pj_str_t id, int sec);
	void SetInfo(pj_str_t id, CString str);
	void Delete(int i);
	void UpdateCallButton();
	CString GetNameByNumber(CString number);

	void CallsLoad();
	void CallsClear();

private:
	CImageList* imageList;
	int nextKey;
	void CallSave(Call *pCall);
	void CallDecode(CString str, Call *pCall);
	CString CallEncode(Call *pCall);
	void Insert(Call *pCall);
	int Get(CString id);
	void MessageDlgOpen(BOOL isCall = FALSE, BOOL hasVideo = FALSE);

protected:
	virtual BOOL OnInitDialog();
	virtual void PostNcDestroy();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnMenuCall(); 
	afx_msg void OnMenuChat();
	afx_msg void OnMenuCopy();
	afx_msg void OnMenuDelete(); 
	afx_msg LRESULT OnContextMenu(WPARAM wParam,LPARAM lParam);
	afx_msg void OnNMDblclkCalls(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnEndTrack(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnEndtrack(NMHDR* pNMHDR, LRESULT* pResult);
#ifdef _GLOBAL_VIDEO
	afx_msg void OnMenuCallVideo(); 
#endif
};

