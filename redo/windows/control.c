// 6 april 2015
#include "uipriv_windows.h"

struct singleHWND {
	uiControl *c;
	HWND hwnd;
	void (*onDestroy)(void *);
	void *onDestroyData;
};

void osSingleDestroy(void *internal)
{
	struct singleHWND *s = (struct singleHWND *) internal;

	(*(s->onDestroy))(s->onDestroyData);
	uiWindowsUnregisterWM_COMMANDHandler(s->hwnd);
	uiWindowsUnregisterWM_NOTIFYHandler(s->hwnd);
	uiWindowsUnregisterWM_HSCROLLHandler(s->hwnd);
	if (DestroyWindow(s->hwnd) == 0)
		logLastError("error destroying control in singleDestroy()");
	uiFree(s);
}

uintptr_t osSingleHandle(void *internal)
{
	struct singleHWND *s = (struct singleHWND *) internal;

	return (uintptr_t) (s->hwnd);
}

void osSingleSetParent(void *internal, uiControl *oldParent, uiControl *newParent)
{
	struct singleHWND *s = (struct singleHWND *) internal;
	HWND newParentHWND;

	newParentHWND = utilWindow;
	if (newParent != NULL)
		newParentHWND = (HWND) uiControlHandle(newParent);
	if (SetParent(s->hwnd, newParentHWND) == NULL)
		logLastError("error setting control parent in osSingleSetParent()");
}

void osSingleResize(void *internal, intmax_t x, intmax_t y, intmax_t width, intmax_t height, uiSizing *d)
{
	struct singleHWND *s = (struct singleHWND *) internal;

	moveWindow(s->hwnd, x, y, width, height, d);
}

uiSizing *osSingleSizing(void *internal, uiControl *c)
{
	return uiWindowsSizing(c);
}

void osSingleShow(void *internal)
{
	struct singleHWND *s = (struct singleHWND *) internal;

	ShowWindow(s->hwnd, SW_SHOW);
}

void osSingleHide(void *internal)
{
	struct singleHWND *s = (struct singleHWND *) internal;

	ShowWindow(s->hwnd, SW_HIDE);
}

void osSingleEnable(void *internal)
{
	struct singleHWND *s = (struct singleHWND *) internal;

	EnableWindow(s->hwnd, TRUE);
}

void osSingleDisable(void *internal)
{
	struct singleHWND *s = (struct singleHWND *) internal;

	EnableWindow(s->hwnd, FALSE);
}

// TODO integrate these two with the main control.c
static void singleSysFunc(uiControl *c, uiControlSysFuncParams *p)
{
	struct singleHWND *s = (struct singleHWND *) (c->Internal);

	switch (p->Func) {
	case uiWindowsSysFuncHasTabStops:
		if (IsWindowEnabled(s->hwnd) != 0)
			if ((getStyle(s->hwnd) & WS_TABSTOP) != 0)
				p->HasTabStops = TRUE;
		return;
	case uiWindowsSysFuncSetZOrder:
		// TODO
		return;
	}
	complain("unknown p->Func %d in singleSysFunc()", p->Func);
}

static int singleStartZOrder(uiControl *c, uiControlSysFuncParams *p)
{
	// TODO
	return 0;
}

void uiWindowsMakeControl(uiControl *c, uiWindowsMakeControlParams *p)
{
	struct singleHWND *s;

	s = uiNew(struct singleHWND);
	s->c = c;
	s->hwnd = CreateWindowExW(p->dwExStyle,
		p->lpClassName, p->lpWindowName,
		p->dwStyle | WS_CHILD | WS_VISIBLE,
		0, 0,
		// use a nonzero initial size just in case some control breaks with a zero initial size
		100, 100,
		utilWindow, NULL, p->hInstance, p->lpParam);
	if (s->hwnd == NULL)
		logLastError("error creating control in uiWindowsMakeControl()");

	uiWindowsRegisterWM_COMMANDHandler(s->hwnd, p->onWM_COMMAND, uiControl(c));
	uiWindowsRegisterWM_NOTIFYHandler(s->hwnd, p->onWM_NOTIFY, uiControl(c));
	uiWindowsRegisterWM_HSCROLLHandler(s->hwnd, p->onWM_HSCROLL, uiControl(c));

	s->onDestroy = p->onDestroy;
	s->onDestroyData = p->onDestroyData;

	if (p->useStandardControlFont)
		SendMessageW(s->hwnd, WM_SETFONT, (WPARAM) hMessageFont, (LPARAM) TRUE);

	makeControl(uiControl(c), s);

	// PreferredSize() implemented by the individual controls
	uiControl(c)->SysFunc = singleSysFunc;
	uiControl(c)->StartZOrder = singleStartZOrder;
}

char *uiWindowsControlText(uiControl *c)
{
	HWND hwnd;
	WCHAR *wtext;
	char *text;

	hwnd = (HWND) uiControlHandle(c);
	wtext = windowText(hwnd);
	text = toUTF8(wtext);
	uiFree(wtext);
	return text;
}

void uiWindowsControlSetText(uiControl *c, const char *text)
{
	HWND hwnd;
	WCHAR *wtext;

	hwnd = (HWND) uiControlHandle(c);
	wtext = toUTF16(text);
	if (SetWindowTextW(hwnd, wtext) == 0)
		logLastError("error setting control text in uiWindowsControlSetText()");
	uiFree(wtext);
}
