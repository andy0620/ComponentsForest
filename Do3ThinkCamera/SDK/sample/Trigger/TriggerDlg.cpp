// TriggerDlg.cpp :  Implementation file

#include "stdafx.h"
#include "Trigger.h"
#include "TriggerDlg.h"
#include "afxdialogex.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// DVP API rely on. 
#ifdef _M_X64
#pragma comment(lib, "../../../library/Visual C++/lib/x64/DVPCamera64.lib")
#else
#pragma comment(lib, "../../../library/Visual C++/lib/x86/DVPCamera.lib")
#endif

// CTriggerDlg dialog

// Callback function for acquiring the video stream.
int CTriggerDlg::OnDrawPicture(dvpHandle handle,
	dvpStreamEvent event,
	void* pContext,
	dvpFrame* pFrame,
	void* pBuffer)
{
	CTriggerDlg* pDlg = ((CTriggerDlg*)pContext);


	// Get current clock
	pDlg->m_CurGrabClock = GetTickCount();

	bool bDisplay = false;
	if (pDlg->m_dfDisplayCount != 0)
	{
		// count the time of image acquisition
		unsigned int GrabMs = (pDlg->m_CurGrabClock - pDlg->m_StartGrabClock);

		// Calculate whether the current frame is displayed
		if (GrabMs - pDlg->m_dfDisplayCount * 33.3f >= 33)
		{
			bDisplay = true;
		}
	}
	else
	{
		bDisplay = true;
		pDlg->m_StartGrabClock = pDlg->m_CurGrabClock;
	}

	if (bDisplay || pDlg->m_bTriggerMode)
	{
		// It demonstrates the usual video drawing,and it is not recommended to take a longer time operation in the callback function,
		// in order to avoid affecting the frame rate and the real-time of acquiring images.
		// The acquired image data buffer is valid only before the function returns,so the buffer pointer should not be passed out, 
		// however, the user can malloc memory and copy image data.
		dvpStatus status = dvpDrawPicture(pFrame, pBuffer, pDlg->GetDlgItem(IDC_STATIC_PREVIEW)->GetSafeHwnd(), nullptr, nullptr);
		ASSERT(status == DVP_STATUS_OK);
	}

	// "return 0" represents the image data has been discarded and no longer been used.
	return 0;
}


CTriggerDlg::CTriggerDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CTriggerDlg::IDD, pParent)
	, m_TriggerDelay(0)
{
	m_handle = 0;

	m_DelayDescr = { 0, };

	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

}


void CTriggerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_TRIGGER_DELAY, m_TriggerDelay);
	DDV_MinMaxInt(pDX, m_TriggerDelay, m_DelayDescr.iMin, m_DelayDescr.iMax);
	DDX_Control(pDX, IDC_COMBO_DEVICES, m_listDevices);
	DDX_Control(pDX, IDC_COMBO_LINE_SELECTOR, m_LineSelector);
	DDX_Control(pDX, IDC_COMBO_LINE_MODE, m_LineMode);
	DDX_Control(pDX, IDC_COMBO_LINE_SOURCE, m_LineSource);
}


void CTriggerDlg::UpdateControls()
{
	dvpStatus status;

	if (IsValidHandle(m_handle))
	{
		// The device has been opened at this time.
		// Update and enable the basic controls.
		dvpStreamState state;
		status = dvpGetStreamState(m_handle, &state);
		ASSERT(status == DVP_STATUS_OK);

		GetDlgItem(IDC_BUTTON_OPEN)->SetWindowTextW(_T("Close"));
		GetDlgItem(IDC_BUTTON_PLAY)->SetWindowText(state == STATE_STARTED ? _T("Stop") : _T("Start"));
		GetDlgItem(IDC_BUTTON_PLAY)->EnableWindow(TRUE);
		GetDlgItem(IDC_BUTTON_PROPERTY)->EnableWindow(TRUE);

		// Enable the related controls.
		if (state == STATE_STOPED) {
			GetDlgItem(IDC_BUTTON_TRIGGER_FIRE)->EnableWindow(FALSE);
		}
		else {
			GetDlgItem(IDC_BUTTON_TRIGGER_FIRE)->EnableWindow(TRUE);
		}


		// Update the window that is related to trigger function.
		bool bTrig = false;

		// Update the enable status of the trigger mode. 
		status = dvpGetBoolValue(m_handle, V_TRIGGER_MODE_B, &bTrig);
		if (status != DVP_STATUS_OK)
		{
			m_bTriggerMode = false;
			GetDlgItem(IDC_CHECK_TRIGGER)->EnableWindow(FALSE);
		}
		else
		{
			GetDlgItem(IDC_CHECK_TRIGGER)->EnableWindow(state != STATE_STARTED);
			((CButton*)GetDlgItem(IDC_CHECK_TRIGGER))->SetCheck((int)bTrig);
			m_bTriggerMode = bTrig;
		}


		GetDlgItem(IDC_BUTTON_APPLY_DELAY)->EnableWindow(bTrig);
		GetDlgItem(IDC_EDIT_TRIGGER_DELAY)->EnableWindow(bTrig);
		GetDlgItem(IDC_STATIC_DELAY)->EnableWindow(bTrig);

		if (bTrig)
		{
			GetDlgItem(IDC_BUTTON_TRIGGER_FIRE)->EnableWindow(TRUE);
		}
		else
		{
			GetDlgItem(IDC_BUTTON_TRIGGER_FIRE)->EnableWindow(FALSE);
		}

		// The following descriptions of the information will be used to update the range of values in the edit box.
		status = dvpGetInt32Value(m_handle, V_TRIGGER_DELAY_I, &m_TriggerDelay, &m_DelayDescr);
		// ASSERT(status == DVP_STATUS_OK);

		bool bLineInverter = false;

		//line inverter
		status = dvpGetBoolValue(m_handle, V_LINE_INVERTER_B, &bLineInverter);
		((CButton*)GetDlgItem(IDC_CHECK_LINE_INVERTER))->SetCheck((int)bLineInverter);
		GetDlgItem(IDC_CHECK_LINE_INVERTER)->EnableWindow(TRUE);

		m_LineSelector.ResetContent();

		int nCurEnumValue = 0;
		unsigned int nSupportNum = 0;
		int nSupportValue[64] = { 0 };

		//line selector
		status = dvpGetEnumValue(m_handle, V_LINE_SELECTOR_E, &nCurEnumValue, nSupportValue, &nSupportNum);
		if (status != DVP_STATUS_OK)
		{
			GetDlgItem(IDC_COMBO_LINE_SELECTOR)->EnableWindow(FALSE);
		}
		else
		{
			GetDlgItem(IDC_COMBO_LINE_SELECTOR)->EnableWindow(TRUE);

			int nCurIndex = 0;
			for (int i = 0; i < (int)nSupportNum; i++)
			{
				if (nSupportValue[i] == nCurEnumValue)
				{
					nCurIndex = i;
				}

				dvpEnumDescr stEnumDescr;
				//line selector descrtion
				dvpGetEnumDescr(m_handle, V_LINE_SELECTOR_E, i, &stEnumDescr);

				m_LineSelector.AddString(CStringW(stEnumDescr.szEnumName));
			}
			m_LineSelector.SetCurSel(nCurIndex);
		}
		
		m_LineMode.ResetContent();

		nCurEnumValue = 0;
		nSupportNum = 0;
		memset(nSupportValue, 0, sizeof(nSupportValue));

		//line mode
		status = dvpGetEnumValue(m_handle, V_LINE_MODE_E, &nCurEnumValue, nSupportValue, &nSupportNum);
		if (status != DVP_STATUS_OK)
		{
			GetDlgItem(IDC_COMBO_LINE_MODE)->EnableWindow(FALSE);
		}
		else
		{
			GetDlgItem(IDC_COMBO_LINE_MODE)->EnableWindow(TRUE);

			int nCurIndex = 0;
			for (int i = 0; i < (int)nSupportNum; i++)
			{
				if (nSupportValue[i] == nCurEnumValue)
				{
					nCurIndex = i;
				}

				dvpEnumDescr stEnumDescr;
				//line mode descrtion
				dvpGetEnumDescr(m_handle, V_LINE_MODE_E, i, &stEnumDescr);

				m_LineMode.AddString(CStringW(stEnumDescr.szEnumName));
			}
			m_LineMode.SetCurSel(nCurIndex);
		}
		

		m_LineSource.ResetContent();

		nCurEnumValue = 0;
		nSupportNum = 0;
		memset(nSupportValue, 0, sizeof(nSupportValue));

		//line source
		status = dvpGetEnumValue(m_handle, V_LINE_SOURCE_E, &nCurEnumValue, nSupportValue, &nSupportNum);
		if (status != DVP_STATUS_OK)
		{
			GetDlgItem(IDC_COMBO_LINE_SOURCE)->EnableWindow(FALSE);
		}
		else
		{
			GetDlgItem(IDC_COMBO_LINE_SOURCE)->EnableWindow(TRUE);

			int nCurIndex = 0;
			for (int i = 0; i < (int)nSupportNum; i++)
			{
				if (nSupportValue[i] == nCurEnumValue)
				{
					nCurIndex = i;
				}

				dvpEnumDescr stEnumDescr;
				//line source descrtion
				dvpGetEnumDescr(m_handle, V_LINE_SOURCE_E, i, &stEnumDescr);

				m_LineSource.AddString(CStringW(stEnumDescr.szEnumName));
			}
			m_LineSource.SetCurSel(nCurIndex);
		}
		


		UpdateData(FALSE);
	}
	else
	{
		// No device is opened at this time.
		// Update the basic controls.
		GetDlgItem(IDC_BUTTON_OPEN)->SetWindowTextW(_T("Open"));
		GetDlgItem(IDC_BUTTON_PLAY)->EnableWindow(FALSE);
		GetDlgItem(IDC_BUTTON_PROPERTY)->EnableWindow(FALSE);

		if (m_listDevices.GetCount() == 0)
		{
			// No device exists.
			GetDlgItem(IDC_BUTTON_OPEN)->EnableWindow(FALSE);
		}
		else
		{
			GetDlgItem(IDC_BUTTON_OPEN)->EnableWindow(TRUE);
		}

		// Update the related controls.

		GetDlgItem(IDC_CHECK_TRIGGER)->EnableWindow(FALSE);
		GetDlgItem(IDC_BUTTON_TRIGGER_FIRE)->EnableWindow(FALSE);
		GetDlgItem(IDC_STATIC_PREVIEW)->InvalidateRect(NULL);

		GetDlgItem(IDC_BUTTON_APPLY_DELAY)->EnableWindow(FALSE);
		GetDlgItem(IDC_EDIT_TRIGGER_DELAY)->EnableWindow(FALSE);
		GetDlgItem(IDC_STATIC_DELAY)->EnableWindow(FALSE);

		GetDlgItem(IDC_CHECK_LINE_INVERTER)->EnableWindow(FALSE);
		GetDlgItem(IDC_COMBO_LINE_SELECTOR)->EnableWindow(FALSE);
		GetDlgItem(IDC_COMBO_LINE_MODE)->EnableWindow(FALSE);
		GetDlgItem(IDC_COMBO_LINE_SOURCE)->EnableWindow(FALSE);

	}

	SendMessage(WM_SIZE);
}


BEGIN_MESSAGE_MAP(CTriggerDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_OPEN, &CTriggerDlg::OnBnClickedButtonOpen)
	ON_BN_CLICKED(IDC_BUTTON_PROPERTY, &CTriggerDlg::OnBnClickedButtonProperty)
	ON_BN_CLICKED(IDC_BUTTON_TRIGGER_FIRE, &CTriggerDlg::OnBnClickedButtonTriggerFire)
	ON_BN_CLICKED(IDC_BUTTON_PLAY, &CTriggerDlg::OnBnClickedButtonPlay)
	ON_WM_DESTROY()
	ON_WM_TIMER()
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_CHECK_TRIGGER, &CTriggerDlg::OnBnClickedCheckTrigger)
	ON_BN_CLICKED(IDC_BUTTON_SCAN, &CTriggerDlg::OnBnClickedButtonScan)
	ON_BN_CLICKED(IDC_BUTTON_APPLY_DELAY, &CTriggerDlg::OnBnClickedButtonApplyDelay)
	//ON_BN_CLICKED(IDC_BUTTON_APPLY_FILTER, &CTriggerDlg::OnBnClickedButtonApplyFilter)
	ON_BN_CLICKED(IDC_CHECK_USERDEFINEDNAME, &CTriggerDlg::OnBnClickedCheckUserdefinedname)
	ON_BN_CLICKED(IDC_CHECK_LINE_INVERTER, &CTriggerDlg::OnBnClickedCheckLineInverter)
	ON_CBN_SELCHANGE(IDC_COMBO_LINE_SELECTOR, &CTriggerDlg::OnSelchangeComboLineSelector)
	ON_CBN_SELCHANGE(IDC_COMBO_LINE_MODE, &CTriggerDlg::OnSelchangeComboLineMode)
	ON_CBN_SELCHANGE(IDC_COMBO_LINE_SOURCE, &CTriggerDlg::OnSelchangeComboLineSource)
END_MESSAGE_MAP()


BOOL CTriggerDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	SetIcon(m_hIcon, TRUE);			// Set big icon.
	SetIcon(m_hIcon, FALSE);		// Set small icon.

	// Initialize open mode
	// false: user dvpOpenByName open the camear
	// true : user dvpOpenByUserId open the camear
	m_bUserDefinedName = false;
	m_dfDisplayCount = 0;
	CheckDlgButton(IDC_CHECK_USERDEFINEDNAME, BST_UNCHECKED);

	// Scan devices that have been connected to a computer in advance. 
	OnBnClickedButtonScan();

	// Timer for updating the current information of the frame rate.
	SetTimer(0, 500, NULL);
	return TRUE;
}


void CTriggerDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this);
		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle.
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon.
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}


HCURSOR CTriggerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CTriggerDlg::OnBnClickedButtonTriggerFire()
{
	if (IsValidHandle(m_handle))
	{
		// Once execution of this function is equivalent to the generation of an external trigger.
		// Note:If the exposure time is very long, clicking on the "Soft Trigger the Fire" too quick may cause the failure of the trigger,
		// because the previous frame probably in a state of exposing continuously or output incompletely.
		dvpStatus status = dvpSetCommandValue(m_handle, "TriggerSoftware");
	}
}


void CTriggerDlg::OnOK()
{
	if (IsValidHandle(m_handle))
	{
		// After clicking "OK" ,configured parameters will be saved,default to the path "C:\ProgramData\DO3THINK\DVP2" if not specified.
		dvpStatus status = dvpSaveConfig(m_handle, NULL);
		ASSERT(status == DVP_STATUS_OK);
	}

	CDialogEx::OnOK();
}


void CTriggerDlg::OnBnClickedCheckTrigger()
{
	if (IsValidHandle(m_handle))
	{
		dvpStatus status;
		dvpStreamState state;
		status = dvpGetStreamState(m_handle, &state);
		ASSERT(status == DVP_STATUS_OK);

		if (state == STATE_STARTED)
		{
			// Stop the video stream.
			status = dvpStop(m_handle);
			ASSERT(status == DVP_STATUS_OK);
		}

		// Enable/disable the trigger mode.
		status = dvpSetBoolValue(m_handle, V_TRIGGER_MODE_B, ((CButton*)GetDlgItem(IDC_CHECK_TRIGGER))->GetCheck() ? true : false);

		ASSERT(status == DVP_STATUS_OK);

		if (state == STATE_STARTED)
		{
			// Start the video stream.
			status = dvpStart(m_handle);
			ASSERT(status == DVP_STATUS_OK);
		}

		UpdateControls();
	}
}


void CTriggerDlg::OnDestroy()
{
	CDialogEx::OnDestroy();

	// Close the device at the moment of  destroying the window.
	if (IsValidHandle(m_handle))
	{
		dvpStatus status = dvpStop(m_handle);
		ASSERT(status == DVP_STATUS_OK);

		status = dvpClose(m_handle);
		ASSERT(status == DVP_STATUS_OK);
		m_handle = 0;
	}

}


void CTriggerDlg::OnTimer(UINT_PTR nIDEvent)
{

	if (nIDEvent == 0)
	{
		if (IsValidHandle(m_handle))
		{
			// Update the information of the frame rate.
			dvpFrameCount count;
			dvpStatus status = dvpGetFrameCount(m_handle, &count);
			ASSERT(status == DVP_STATUS_OK);

			CString str;
			if (m_dfDisplayCount > 0 && IsDlgButtonChecked(IDC_CHECK_TRIGGER) == BST_UNCHECKED)
			{
				str.Format(_T("%s [%d frames, %.3f fps, Display %.3f fps]"),
					m_strFriendlyName,
					count.uFrameCount,
					count.fFrameRate,
					m_dfDisplayCount * 1000.0 / (m_CurGrabClock - m_StartGrabClock));
			}
			else
			{
				str.Format(_T("%s [%d frames, %.3f fps]"),
					m_strFriendlyName,
					count.uFrameCount,
					count.fFrameRate);
			}
			SetWindowText(str);
		}
	}

	CDialogEx::OnTimer(nIDEvent);
}


void CTriggerDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);

	// Adjust the video graphic window automatically according to the size of the window.
	UpdataPreWindowSize(cx, cy);
}


void CTriggerDlg::UpdataPreWindowSize(int x, int y)
{
	if (IsValidHandle(m_handle))
	{
		dvpRegion roi;
		dvpStatus status = dvpGetRoi(m_handle, &roi);

		CRect rc, rcPreview, rcOld;
		GetDlgItem(IDC_STATIC_PREVIEW)->GetWindowRect(rcOld);
		ScreenToClient(rcOld);

		GetClientRect(rc);
		rc.left += rcOld.left;
		rc.right -= 6;
		rc.top += rcOld.top;
		rc.bottom -= 6;

		rcPreview.SetRect(0, 0, roi.W, roi.H);

		if (rc.Width() * rcPreview.Height() >
			rc.Height() * rcPreview.Width())
		{
			rcPreview.SetRect(
				0,
				0,
				rc.Height() * roi.W / roi.H,
				rc.Height());
		}
		else
		{
			rcPreview.SetRect(
				0,
				0,
				rc.Width(),
				rc.Width() * roi.H / roi.W);
		}

		rcPreview.MoveToXY(rc.TopLeft());

		GetDlgItem(IDC_STATIC_PREVIEW)->MoveWindow(rcPreview);
	}
}


void CTriggerDlg::OnBnClickedButtonScan()
{
	dvpStatus status;
	dvpUint32 i, n = 0;

	// "n" represents the number of cameras that is enumerated successfully, drop-down list contains each camera's FriendlyName.
	m_listDevices.ResetContent();

	// Get the number of cameras that has been connected to a computer.
	status = dvpRefresh(&n);
	ASSERT(status == DVP_STATUS_OK);
	m_CamCount = 0;
	if (status == DVP_STATUS_OK)
	{
		// Enumeration of up to 32 cameras.
		if (n > 32)
			n = 32;


		memset(m_info, 0, sizeof(m_info));
		for (i = 0; i < n; i++)
		{
			// Acquire each camera's information one by one.
			status = dvpEnum(i, &m_info[i]);
			ASSERT(status == DVP_STATUS_OK);

			if (status == DVP_STATUS_OK)
			{
				// GUI need UNICODE,but the information acquired from cameras is ANSI,so convert the character set from ANSI to UNICODE.
				int item = CB_ERR;
				if (!m_bUserDefinedName)
				{
					item = m_listDevices.AddString(CString(m_info[m_CamCount].FriendlyName));
				}
				else
				{
					// check User Defined Name is null
					if (strlen(m_info[m_CamCount].UserID) == 0)
						continue;
					item = m_listDevices.AddString(CString(m_info[m_CamCount].UserID));
				}
				if (item != CB_ERR)
				{
					m_listDevices.SetItemData(item, m_CamCount);
				}
				m_CamCount++;
			}
		}

		if (i != 0)
		{
			m_listDevices.SetCurSel(0);
		}

		UpdateControls();
	}
}


void CTriggerDlg::OnBnClickedButtonOpen()
{
	dvpStatus status = DVP_STATUS_OK;
	CString strName;

	if (!IsValidHandle(m_handle))
	{
		m_listDevices.GetWindowText(strName);
		if (strName != "")
		{
			// Open the pointed device by the selected camera.

			if (m_bUserDefinedName)
			{
				status = dvpOpenByUserId(CStringA(strName), OPEN_NORMAL, &m_handle);
			}
			else
			{
				status = dvpOpenByName(CStringA(strName), OPEN_NORMAL, &m_handle);
			}
			ASSERT(status == DVP_STATUS_OK);

			if (status != DVP_STATUS_OK)
			{
				MessageBox(_T("Open the device failed"));
			}
			else
			{
				m_strFriendlyName = strName;

				// If it needs to display images ,the user should register a callback function and finish the operation of drawing pictures in the registered callback function.
				// Note: Drawing pictures in the callback function maybe generate some delays for acquiring image data by the use of "dvpGetFrame".
				status = dvpRegisterStreamCallback(m_handle, OnDrawPicture, STREAM_EVENT_FRAME_THREAD, this);
				ASSERT(status == DVP_STATUS_OK);
			}
		}
	}
	else
	{
		dvpStreamState state;
		dvpGetStreamState(m_handle, &state);
		if (state == STATE_STARTED)
		{
			m_dfDisplayCount = 0;
			status = dvpStop(m_handle);
			ASSERT(status == DVP_STATUS_OK);

		}

		status = dvpClose(m_handle);
		ASSERT(status == DVP_STATUS_OK);
		m_handle = 0;
		GetDlgItem(IDC_STATIC_PREVIEW)->InvalidateRect(NULL, 1);
	}

	UpdateControls();
}


void CTriggerDlg::OnBnClickedButtonPlay()
{
	if (IsValidHandle(m_handle))
	{
		dvpStreamState state;
		dvpStatus status;

		// Implement a button to start and stop according to the current video's status.
		status = dvpGetStreamState(m_handle, &state);
		ASSERT(status == DVP_STATUS_OK);

		if (state == STATE_STARTED)
		{

			status = dvpStop(m_handle);

		}
		else
		{
			// init display count
			m_dfDisplayCount = 0;

			status = dvpStart(m_handle);

		}

		ASSERT(status == DVP_STATUS_OK);
	}

	UpdateControls();
}


void CTriggerDlg::OnBnClickedButtonProperty()
{
	if (IsValidHandle(m_handle))
	{
		dvpStatus status = dvpShowPropertyModalDialog(m_handle, GetSafeHwnd());

		// At this time some configurations may change, synchronize it to the window GUI.
		UpdateControls();
		RECT rt = { 0 };
		GetClientRect(&rt);
		UpdataPreWindowSize(rt.right, rt.bottom);
	}
}


void CTriggerDlg::OnBnClickedButtonApplyDelay()
{
	if (IsValidHandle(m_handle))
	{
		UpdateData();

		dvpStatus status;

		if (m_TriggerDelay < m_DelayDescr.iMin)
			m_TriggerDelay = m_DelayDescr.iMin;
		if (m_TriggerDelay > m_DelayDescr.iMax)
			m_TriggerDelay = m_DelayDescr.iMax;

		status = dvpSetInt32Value(m_handle, V_TRIGGER_DELAY_I, m_TriggerDelay);
		ASSERT(status == DVP_STATUS_OK);

		status = dvpGetInt32Value(m_handle, V_TRIGGER_DELAY_I, &m_TriggerDelay,NULL);
		if (status == DVP_STATUS_OK)
		{
			TCHAR szBuf[128] = { 0 };
			_stprintf_s(szBuf, TEXT("%d"), m_TriggerDelay);
			SetDlgItemText(IDC_EDIT_TRIGGER_DELAY, szBuf);
		}
	}
}

void CTriggerDlg::OnBnClickedCheckUserdefinedname()
{
	if (m_bUserDefinedName == (IsDlgButtonChecked(IDC_CHECK_USERDEFINEDNAME) == BST_CHECKED ? true : false))
	{
		return;
	}

	m_bUserDefinedName = !m_bUserDefinedName;

	// save cur sel item
	CString strName;
	m_listDevices.GetWindowText(strName);

	// reset m_listDevices values
	m_listDevices.ResetContent();
	for (int i = 0; i < m_CamCount; i++)
	{
		int item = CB_ERR;
		if (!m_bUserDefinedName)
		{
			item = m_listDevices.AddString(CString(m_info[i].FriendlyName));
			if (strName == CString(m_info[i].UserID))
			{
				m_listDevices.SetCurSel(item);
			}
		}
		else
		{
			// check User Defined Name is null
			if (strlen(m_info[i].UserID) == 0)
				continue;

			item = m_listDevices.AddString(CString(m_info[i].UserID));
			if (strName == CString(m_info[i].FriendlyName))
			{
				m_listDevices.SetCurSel(item);
			}
		}
		if (item != CB_ERR)
		{
			m_listDevices.SetItemData(item, i);
		}
	}
}


void CTriggerDlg::OnBnClickedCheckLineInverter()
{
	// Enable/disable the line inverter.
	dvpStatus status = dvpSetBoolValue(m_handle, V_LINE_INVERTER_B,((CButton*)GetDlgItem(IDC_CHECK_LINE_INVERTER))->GetCheck() ? true : false);
}


void CTriggerDlg::OnSelchangeComboLineSelector()
{
	int nCurSel = m_LineSelector.GetCurSel();

	dvpEnumDescr stEnumDescr;
	//get the line selector description information by index
	dvpStatus status = dvpGetEnumDescr(m_handle, V_LINE_SELECTOR_E, nCurSel, &stEnumDescr);
	if (status == DVP_STATUS_OK)
	{
		//set the line selector 's value
		status = dvpSetEnumValue(m_handle, V_LINE_SELECTOR_E, stEnumDescr.iEnumValue);
	}
}


void CTriggerDlg::OnSelchangeComboLineMode()
{
	int nCurSel = m_LineMode.GetCurSel();

	dvpEnumDescr stEnumDescr;
	//get the line mode description information by index
	dvpStatus status = dvpGetEnumDescr(m_handle, V_LINE_MODE_E, nCurSel, &stEnumDescr);
	if (status == DVP_STATUS_OK)
	{
		//set the line mode 's value
		status = dvpSetEnumValue(m_handle, V_LINE_MODE_E, stEnumDescr.iEnumValue);
	}
}


void CTriggerDlg::OnSelchangeComboLineSource()
{
	int nCurSel = m_LineSource.GetCurSel();

	dvpEnumDescr stEnumDescr;
	//get the line source description information by index
	dvpStatus status = dvpGetEnumDescr(m_handle, V_LINE_SOURCE_E, nCurSel, &stEnumDescr);
	if (status == DVP_STATUS_OK)
	{
		//set the line source 's value
		status = dvpSetEnumValue(m_handle, V_LINE_SOURCE_E, stEnumDescr.iEnumValue);
	}
}
