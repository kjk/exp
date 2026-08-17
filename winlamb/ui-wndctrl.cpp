#include <memory>
#include <system_error>
#include "ui-wndctrl.hpp"
using namespace _wl_internal;
using namespace wl;
using namespace wl::events;

constexpr bool operator==(const SIZE a, const SIZE b) { return a.cx == b.cx && a.cy == b.cy; }

void NativeCtrlBase::create_wnd(WORD ctrlId, DWORD exStyle, const wchar_t *className,
	std::wstring &&title, DWORD style, POINT pos, SIZE size)
{
	#ifdef _DEBUG
	if (_hWnd)
		throw std::logic_error{"Cannot create control twice."};
	if (!_parent._hWnd)
		throw std::logic_error{"Cannot create control before parent."};
	#endif

	_hWnd = CreateWindowExW(exStyle, className, title.c_str(), style,
		pos.x, pos.y, size.cx, size.cy, _parent._hWnd,
		reinterpret_cast<HMENU>(ctrlId), wnd_hinst(_parent._hWnd), nullptr);
	#ifdef _DEBUG
	if (!_hWnd)
		throw std::system_error(GetLastError(), std::system_category(), "NativeCtrlBase: CreateWindowEx failed");
	#endif

	install_subclass();
}

void NativeCtrlBase::assign_dlg(WORD ctrlId) {
	#ifdef _DEBUG
	if (_hWnd)
		throw std::logic_error{"Cannot assign control twice."};
	if (!_parent._hWnd)
		throw std::logic_error{"Cannot assign control before parent."};
	#endif

	_hWnd = GetDlgItem(_parent._hWnd, ctrlId);
	#ifdef _DEBUG
	if (!_hWnd)
		throw std::system_error(GetLastError(), std::system_category(), "NativeCtrlBase: GetDlgItem failed");
	#endif

	install_subclass();
}

void NativeCtrlBase::install_subclass() {
	static UINT_PTR subclassId = 0;
	if (_subclassEvents.has_message()) {
		BOOL ok = SetWindowSubclass(_hWnd, subclass_proc, ++subclassId, reinterpret_cast<DWORD_PTR>(this));
		#ifdef _DEBUG
		if (!ok)
			throw std::runtime_error{"SetWindowSubclass failed."};
		#endif
	}
}

LRESULT CALLBACK NativeCtrlBase::subclass_proc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp,
	UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	NativeCtrlBase *pSelf = reinterpret_cast<NativeCtrlBase*>(dwRefData);

	wm::Msg procMsg{msg, wp, lp};
	if (pSelf)
		pSelf->_subclassEvents.process_last(procMsg);

	if (msg == WM_NCDESTROY) { // always check
		// https://devblogs.microsoft.com/oldnewthing/20031111-00/?p=41883
		BOOL ok = RemoveWindowSubclass(hWnd, subclass_proc, uIdSubclass);
		#ifdef _DEBUG
		if (!ok)
			throw std::runtime_error{"RemoveWindowSubclass failed."};
		#endif
		if (pSelf)
			pSelf->_subclassEvents.clear();
	}

	return procMsg.didHandle ? procMsg.ret : DefSubclassProc(hWnd, msg, wp, lp);
}

////////////////////////////////////////////////////////////////////////////////

void ButtonEvents::bcn_drop_down(Func1<NMBCDROPDOWN&> cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, BCN_DROPDOWN, WrapNfy(cb));
}
void ButtonEvents::bcn_hot_item_change(Func1<NMBCHOTITEM&> cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, BCN_HOTITEMCHANGE, WrapNfy(cb));
}
void ButtonEvents::bn_clicked(Func0 cb) {
	_ctrlEvents._parent._userEvents.wm_command(_ctrlEvents._ctrlId, BN_CLICKED, cb);
}
void ButtonEvents::bn_dbl_clk(Func0 cb) {
	_ctrlEvents._parent._userEvents.wm_command(_ctrlEvents._ctrlId, BN_DBLCLK, cb);
}
void ButtonEvents::bn_kill_focus(Func0 cb) {
	_ctrlEvents._parent._userEvents.wm_command(_ctrlEvents._ctrlId, BN_KILLFOCUS, cb);
}
void ButtonEvents::bn_set_focus(Func0 cb) {
	_ctrlEvents._parent._userEvents.wm_command(_ctrlEvents._ctrlId, BN_SETFOCUS, cb);
}
void ButtonEvents::nm_custom_draw(std::function<DWORD(NMCUSTOMDRAW&)> &&cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, NM_CUSTOMDRAW, WrapNfyRet<DWORD, NMCUSTOMDRAW>(std::move(cb)));
}

void ComboBoxEvents::cbn_close_up(Func0 cb) {
	_ctrlEvents._parent._userEvents.wm_command(_ctrlEvents._ctrlId, CBN_CLOSEUP, cb);
}
void ComboBoxEvents::cbn_dbl_clk(Func0 cb) {
	_ctrlEvents._parent._userEvents.wm_command(_ctrlEvents._ctrlId, CBN_DBLCLK, cb);
}
void ComboBoxEvents::cbn_drop_down(Func0 cb) {
	_ctrlEvents._parent._userEvents.wm_command(_ctrlEvents._ctrlId, CBN_DROPDOWN, cb);
}
void ComboBoxEvents::cbn_edit_change(Func0 cb) {
	_ctrlEvents._parent._userEvents.wm_command(_ctrlEvents._ctrlId, CBN_EDITCHANGE, cb);
}
void ComboBoxEvents::cbn_edit_update(Func0 cb) {
	_ctrlEvents._parent._userEvents.wm_command(_ctrlEvents._ctrlId, CBN_EDITUPDATE, cb);
}
void ComboBoxEvents::cbn_kill_focus(Func0 cb) {
	_ctrlEvents._parent._userEvents.wm_command(_ctrlEvents._ctrlId, CBN_KILLFOCUS, cb);
}
void ComboBoxEvents::cbn_sel_change(Func0 cb) {
	_ctrlEvents._parent._userEvents.wm_command(_ctrlEvents._ctrlId, CBN_SELCHANGE, cb);
}
void ComboBoxEvents::cbn_sel_end_cancel(Func0 cb) {
	_ctrlEvents._parent._userEvents.wm_command(_ctrlEvents._ctrlId, CBN_SELENDCANCEL, cb);
}
void ComboBoxEvents::cbn_sel_end_ok(Func0 cb) {
	_ctrlEvents._parent._userEvents.wm_command(_ctrlEvents._ctrlId, CBN_SELENDOK, cb);
}
void ComboBoxEvents::cbn_set_focus(Func0 cb) {
	_ctrlEvents._parent._userEvents.wm_command(_ctrlEvents._ctrlId, CBN_SETFOCUS, cb);
}

void DateTimePickerEvents::dtn_close_up(Func0 cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, DTN_CLOSEUP, WrapNfy(cb));
}
void DateTimePickerEvents::dtn_date_time_change(Func1<NMDATETIMECHANGE&> cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, DTN_DATETIMECHANGE, WrapNfy(cb));
}
void DateTimePickerEvents::dtn_drop_down(Func0 cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, DTN_DROPDOWN, WrapNfy(cb));
}
void DateTimePickerEvents::dtn_format(Func1<NMDATETIMEFORMATW&> cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, DTN_FORMATW, WrapNfy(cb));
}
void DateTimePickerEvents::dtn_format_query(Func1<NMDATETIMEFORMATQUERYW&> cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, DTN_FORMATQUERYW, WrapNfy(cb));
}
void DateTimePickerEvents::dtn_user_string(Func1<NMDATETIMESTRINGW&> cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, DTN_USERSTRINGW, WrapNfy(cb));
}
void DateTimePickerEvents::dtn_wm_key_down(Func1<NMDATETIMEWMKEYDOWNW&> cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, DTN_WMKEYDOWNW, WrapNfy(cb));
}
void DateTimePickerEvents::nm_kill_focus(Func0 cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, NM_KILLFOCUS, WrapNfy(cb));
}
void DateTimePickerEvents::nm_set_focus(Func0 cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, NM_SETFOCUS, WrapNfy(cb));
}

void EditEvents::en_change(Func0 cb) {
	_ctrlEvents._parent._userEvents.wm_command(_ctrlEvents._ctrlId, EN_CHANGE, cb);
}
void EditEvents::en_h_scroll(Func0 cb) {
	_ctrlEvents._parent._userEvents.wm_command(_ctrlEvents._ctrlId, EN_HSCROLL, cb);
}
void EditEvents::en_kill_focus(Func0 cb) {
	_ctrlEvents._parent._userEvents.wm_command(_ctrlEvents._ctrlId, EN_KILLFOCUS, cb);
}
void EditEvents::en_max_text(Func0 cb) {
	_ctrlEvents._parent._userEvents.wm_command(_ctrlEvents._ctrlId, EN_MAXTEXT, cb);
}
void EditEvents::en_set_focus(Func0 cb) {
	_ctrlEvents._parent._userEvents.wm_command(_ctrlEvents._ctrlId, EN_SETFOCUS, cb);
}
void EditEvents::en_update(Func0 cb) {
	_ctrlEvents._parent._userEvents.wm_command(_ctrlEvents._ctrlId, EN_UPDATE, cb);
}
void EditEvents::en_v_scroll(Func0 cb) {
	_ctrlEvents._parent._userEvents.wm_command(_ctrlEvents._ctrlId, EN_VSCROLL, cb);
}

void ListViewEvents::lvn_begin_drag(Func1<NMLISTVIEW&> cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, LVN_BEGINDRAG, WrapNfy(cb));
}
void ListViewEvents::lvn_begin_label_edit(std::function<bool(NMLVDISPINFOW&)> &&cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, LVN_BEGINLABELEDITW, WrapNfyRet<bool, NMLVDISPINFOW>(std::move(cb)));
}
void ListViewEvents::lvn_begin_r_drag(Func1<NMLISTVIEW&> cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, LVN_BEGINRDRAG, WrapNfy(cb));
}
void ListViewEvents::lvn_begin_scroll(Func1<NMLVSCROLL&> cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, LVN_BEGINSCROLL, WrapNfy(cb));
}
void ListViewEvents::lvn_column_click(Func1<NMLISTVIEW&> cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, LVN_COLUMNCLICK, WrapNfy(cb));
}
void ListViewEvents::lvn_column_drop_down(Func1<NMLISTVIEW&> cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, LVN_COLUMNDROPDOWN, WrapNfy(cb));
}
void ListViewEvents::lvn_column_overflow_click(Func1<NMLISTVIEW&> cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, LVN_COLUMNOVERFLOWCLICK, WrapNfy(cb));
}
void ListViewEvents::lvn_delete_all_items(std::function<bool(NMLISTVIEW&)> &&cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, LVN_DELETEALLITEMS, WrapNfyRet<bool, NMLISTVIEW>(std::move(cb)));
}
void ListViewEvents::lvn_delete_item(Func1<NMLISTVIEW&> cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, LVN_DELETEITEM, WrapNfy(cb));
}
void ListViewEvents::lvn_end_label_edit(std::function<bool(NMLVDISPINFOW&)> &&cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, LVN_ENDLABELEDITW, WrapNfyRet<bool, NMLVDISPINFOW>(std::move(cb)));
}
void ListViewEvents::lvn_end_scroll(Func1<NMLVSCROLL&> cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, LVN_ENDSCROLL, WrapNfy(cb));
}
void ListViewEvents::lvn_insert_item(Func1<NMLISTVIEW&> cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, LVN_INSERTITEM, WrapNfy(cb));
}
void ListViewEvents::lvn_item_activate(Func1<NMITEMACTIVATE&> cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, LVN_ITEMACTIVATE, WrapNfy(cb));
}
void ListViewEvents::lvn_item_changed(Func1<NMLISTVIEW&> cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, LVN_ITEMCHANGED, WrapNfy(cb));
}
void ListViewEvents::lvn_item_changing(std::function<bool(NMLISTVIEW&)> &&cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, LVN_ITEMCHANGING, WrapNfyRet<bool, NMLISTVIEW>(std::move(cb)));
}
void ListViewEvents::lvn_key_down(Func1<NMLVKEYDOWN&> cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, LVN_KEYDOWN, WrapNfy(cb));
}
void ListViewEvents::nm_click(Func1<NMITEMACTIVATE&> cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, NM_CLICK, WrapNfy(cb));
}
void ListViewEvents::nm_custom_draw(std::function<DWORD(NMLVCUSTOMDRAW&)> &&cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, NM_CUSTOMDRAW, WrapNfyRet<DWORD, NMLVCUSTOMDRAW>(std::move(cb)));
}
void ListViewEvents::nm_dbl_clk(Func1<NMITEMACTIVATE&> cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, NM_DBLCLK, WrapNfy(cb));
}
void ListViewEvents::nm_kill_focus(Func1<NMHDR&> cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, NM_KILLFOCUS, WrapNfy(cb));
}
void ListViewEvents::nm_r_click(Func1<NMITEMACTIVATE&> cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, NM_RCLICK, WrapNfy(cb));
}
void ListViewEvents::nm_r_dbl_clk(Func1<NMITEMACTIVATE&> cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, NM_RDBLCLK, WrapNfy(cb));
}
void ListViewEvents::nm_released_capture(Func1<NMHDR&> cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, NM_RELEASEDCAPTURE, WrapNfy(cb));
}
void ListViewEvents::nm_set_focus(Func1<NMHDR&> cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, NM_SETFOCUS, WrapNfy(cb));
}

void MonthCalendarEvents::mcn_get_day_state(Func1<NMDAYSTATE&> cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, MCN_GETDAYSTATE, WrapNfy(cb));
}
void MonthCalendarEvents::mcn_sel_change(Func1<NMSELCHANGE&> cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, MCN_SELCHANGE, WrapNfy(cb));
}
void MonthCalendarEvents::mcn_select(Func1<NMSELCHANGE&> cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, MCN_SELECT, WrapNfy(cb));
}
void MonthCalendarEvents::mcn_view_change(Func1<NMVIEWCHANGE&> cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, MCN_VIEWCHANGE, WrapNfy(cb));
}
void MonthCalendarEvents::nm_released_capture(Func0 cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, NM_RELEASEDCAPTURE, WrapNfy(cb));
}

void RadioGroupEvents::bn_clicked(Func1<int> cb) {
	struct Ctx { Func1<int> cb; int i; };
	for (int i = 0; i < _ownerGroup.radios.count(); ++i) {
		auto* ctx = new Ctx{cb, i};
		_ownerGroup.radios[i].on().bn_clicked(MkFunc0(+[](Ctx* c) { c->cb.Call(c->i); }, ctx));
	}
}
void RadioGroupEvents::bn_dbl_clk(Func1<int> cb) {
	struct Ctx { Func1<int> cb; int i; };
	for (int i = 0; i < _ownerGroup.radios.count(); ++i) {
		auto* ctx = new Ctx{cb, i};
		_ownerGroup.radios[i].on().bn_dbl_clk(MkFunc0(+[](Ctx* c) { c->cb.Call(c->i); }, ctx));
	}
}
void RadioGroupEvents::bn_kill_focus(Func1<int> cb) {
	struct Ctx { Func1<int> cb; int i; };
	for (int i = 0; i < _ownerGroup.radios.count(); ++i) {
		auto* ctx = new Ctx{cb, i};
		_ownerGroup.radios[i].on().bn_kill_focus(MkFunc0(+[](Ctx* c) { c->cb.Call(c->i); }, ctx));
	}
}
void RadioGroupEvents::bn_set_focus(Func1<int> cb) {
	struct Ctx { Func1<int> cb; int i; };
	for (int i = 0; i < _ownerGroup.radios.count(); ++i) {
		auto* ctx = new Ctx{cb, i};
		_ownerGroup.radios[i].on().bn_set_focus(MkFunc0(+[](Ctx* c) { c->cb.Call(c->i); }, ctx));
	}
}

void StaticEvents::stn_clicked(Func0 cb) {
	_ctrlEvents._parent._userEvents.wm_command(_ctrlEvents._ctrlId, STN_CLICKED, cb);
}
void StaticEvents::stn_dbl_clk(Func0 cb) {
	_ctrlEvents._parent._userEvents.wm_command(_ctrlEvents._ctrlId, STN_DBLCLK, cb);
}
void StaticEvents::stn_disable(Func0 cb) {
	_ctrlEvents._parent._userEvents.wm_command(_ctrlEvents._ctrlId, STN_DISABLE, cb);
}
void StaticEvents::stn_enable(Func0 cb) {
	_ctrlEvents._parent._userEvents.wm_command(_ctrlEvents._ctrlId, STN_ENABLE, cb);
}

void StatusBarEvents::nm_click(std::function<bool(NMMOUSE&)> &&cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, NM_CLICK, WrapNfyRet<bool, NMMOUSE>(std::move(cb)));
}
void StatusBarEvents::nm_dbl_clk(std::function<bool(NMMOUSE&)> &&cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, NM_DBLCLK, WrapNfyRet<bool, NMMOUSE>(std::move(cb)));
}
void StatusBarEvents::nm_r_click(std::function<bool(NMMOUSE&)> &&cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, NM_RCLICK, WrapNfyRet<bool, NMMOUSE>(std::move(cb)));
}
void StatusBarEvents::nm_r_dbl_clk(std::function<bool(NMMOUSE&)> &&cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, NM_RDBLCLK, WrapNfyRet<bool, NMMOUSE>(std::move(cb)));
}
void StatusBarEvents::sbn_simple_mode_change(Func1<NMMOUSE&> cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, SBN_SIMPLEMODECHANGE, WrapNfy(cb));
}

void SysLinkEvents::nm_click(Func1<NMLINK&> cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, NM_CLICK, WrapNfy(cb));
}

void TrackbarEvents::trbn_thumb_pos_changing(std::function<bool(NMTRBTHUMBPOSCHANGING&)> &&cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, TRBN_THUMBPOSCHANGING, WrapNfyRet<bool, NMTRBTHUMBPOSCHANGING>(std::move(cb)));
}
void TrackbarEvents::wm_h_scroll(Func1<wm::HScroll&> cb) {
	struct Ctx { Func1<wm::HScroll&> cb; WORD ctrlId; };
	auto* c = new Ctx{cb, _ctrlEvents._ctrlId};
	_ctrlEvents._parent._userEvents.wm(WM_HSCROLL, MkFunc1(+[](Ctx* c, wl::wm::Msg& p) {
		wm::HScroll hs{p};
		if (c->ctrlId == GetDlgCtrlID(hs.hwnd_scrollbar()))
			c->cb.Call(hs);
	}, c));
}
void TrackbarEvents::wm_v_scroll(Func1<wm::VScroll&> cb) {
	struct Ctx { Func1<wm::VScroll&> cb; WORD ctrlId; };
	auto* c = new Ctx{cb, _ctrlEvents._ctrlId};
	_ctrlEvents._parent._userEvents.wm(WM_VSCROLL, MkFunc1(+[](Ctx* c, wl::wm::Msg& p) {
		wm::VScroll vs{p};
		if (c->ctrlId == GetDlgCtrlID(vs.hwnd_scrollbar()))
			c->cb.Call(vs);
	}, c));
}
void TrackbarEvents::nm_custom_draw(std::function<DWORD(NMCUSTOMDRAW&)> &&cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, NM_CUSTOMDRAW, WrapNfyRet<DWORD, NMCUSTOMDRAW>(std::move(cb)));
}
void TrackbarEvents::nm_released_capture(Func0 cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, NM_RELEASEDCAPTURE, WrapNfy(cb));
}

void TreeViewEvents::tvn_async_draw(Func1<NMTVASYNCDRAW&> cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, TVN_ASYNCDRAW, WrapNfy(cb));
}
void TreeViewEvents::tvn_begin_drag(Func1<NMTREEVIEWW&> cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, TVN_BEGINDRAGW, WrapNfy(cb));
}
void TreeViewEvents::tvn_begin_label_edit(std::function<bool(NMTVDISPINFOW&)> &&cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, TVN_BEGINLABELEDITW, WrapNfyRet<bool, NMTVDISPINFOW>(std::move(cb)));
}
void TreeViewEvents::tvn_begin_r_drag(Func1<NMTREEVIEWW&> cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, TVN_BEGINRDRAGW, WrapNfy(cb));
}
void TreeViewEvents::tvn_delete_item(Func1<NMTREEVIEWW&> cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, TVN_DELETEITEMW, WrapNfy(cb));
}
void TreeViewEvents::tvn_end_label_edit(std::function<bool(NMTVDISPINFOW&)> &&cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, TVN_ENDLABELEDITW, WrapNfyRet<bool, NMTVDISPINFOW>(std::move(cb)));
}
void TreeViewEvents::tvn_get_disp_info(Func1<NMTVDISPINFOW&> cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, TVN_GETDISPINFOW, WrapNfy(cb));
}
void TreeViewEvents::tvn_get_info_tip(Func1<NMTVGETINFOTIPW&> cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, TVN_GETINFOTIPW, WrapNfy(cb));
}
void TreeViewEvents::tvn_item_changed(Func1<NMTVITEMCHANGE&> cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, TVN_ITEMCHANGEDW, WrapNfy(cb));
}
void TreeViewEvents::tvn_item_changing(std::function<bool(NMTVITEMCHANGE&)> &&cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, TVN_ITEMCHANGINGW, WrapNfyRet<bool, NMTVITEMCHANGE>(std::move(cb)));
}
void TreeViewEvents::tvn_item_expanded(Func1<NMTREEVIEWW&> cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, TVN_ITEMEXPANDEDW, WrapNfy(cb));
}
void TreeViewEvents::tvn_item_expanding(std::function<bool(NMTREEVIEWW&)> &&cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, TVN_ITEMEXPANDINGW, WrapNfyRet<bool, NMTREEVIEWW>(std::move(cb)));
}
void TreeViewEvents::tvn_key_down(std::function<int(NMTVKEYDOWN&)> &&cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, TVN_KEYDOWN, WrapNfyRet<int, NMTVKEYDOWN>(std::move(cb)));
}
void TreeViewEvents::tvn_sel_changed(Func1<NMTREEVIEWW&> cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, TVN_SELCHANGEDW, WrapNfy(cb));
}
void TreeViewEvents::tvn_sel_changing(std::function<bool(NMTREEVIEWW&)> &&cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, TVN_SELCHANGINGW, WrapNfyRet<bool, NMTREEVIEWW>(std::move(cb)));
}
void TreeViewEvents::tvn_set_disp_info(Func1<NMTVDISPINFOW&> cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, TVN_SETDISPINFOW, WrapNfy(cb));
}
void TreeViewEvents::tvn_single_expand(std::function<WORD(NMTREEVIEWW&)> &&cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, TVN_SINGLEEXPAND, WrapNfyRet<WORD, NMTREEVIEWW>(std::move(cb)));
}
void TreeViewEvents::nm_click(std::function<int()> &&cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, NM_CLICK, WrapNfyRet<int>(std::move(cb)));
}
void TreeViewEvents::nm_custom_draw(std::function<DWORD(NMTVCUSTOMDRAW&)> &&cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, NM_CUSTOMDRAW, WrapNfyRet<DWORD, NMTVCUSTOMDRAW>(std::move(cb)));
}
void TreeViewEvents::nm_dbl_clk(std::function<int()> &&cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, NM_DBLCLK, WrapNfyRet<int>(std::move(cb)));
}
void TreeViewEvents::nm_kill_focus(Func0 cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, NM_KILLFOCUS, WrapNfy(cb));
}
void TreeViewEvents::nm_r_click(std::function<int()> &&cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, NM_RCLICK, WrapNfyRet<int>(std::move(cb)));
}
void TreeViewEvents::nm_r_dbl_clk(std::function<int()> &&cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, NM_RDBLCLK, WrapNfyRet<int>(std::move(cb)));
}
void TreeViewEvents::nm_return(std::function<int()> &&cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, NM_RETURN, WrapNfyRet<int>(std::move(cb)));
}
void TreeViewEvents::nm_set_cursor(std::function<int(NMMOUSE&)> &&cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, NM_SETCURSOR, WrapNfyRet<int, NMMOUSE>(std::move(cb)));
}
void TreeViewEvents::nm_set_focus(Func0 cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, NM_SETFOCUS, WrapNfy(cb));
}

void TabEvents::tcn_focus_change(Func0 cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, TCN_FOCUSCHANGE, WrapNfy(cb));
}
void TabEvents::tcn_get_object(Func1<NMOBJECTNOTIFY&> cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, TCN_GETOBJECT, WrapNfy(cb));
}
void TabEvents::tcn_key_down(Func1<NMTCKEYDOWN&> cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, TCN_KEYDOWN, WrapNfy(cb));
}
void TabEvents::tcn_sel_change(Func0 cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, TCN_SELCHANGE, WrapNfy(cb));
}
void TabEvents::tcn_sel_changing(std::function<bool()> &&cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, TCN_SELCHANGING, WrapNfyRet<bool>(std::move(cb)));
}
void TabEvents::nm_click(Func0 cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, NM_CLICK, WrapNfy(cb));
}
void TabEvents::nm_dbl_clk(Func0 cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, NM_DBLCLK, WrapNfy(cb));
}
void TabEvents::nm_r_click(Func0 cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, NM_RCLICK, WrapNfy(cb));
}
void TabEvents::nm_r_dbl_clk(Func0 cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, NM_RDBLCLK, WrapNfy(cb));
}
void TabEvents::nm_released_capture(Func0 cb) {
	_ctrlEvents._parent._userEvents.wm_notify(_ctrlEvents._ctrlId, NM_RELEASEDCAPTURE, WrapNfy(cb));
}

////////////////////////////////////////////////////////////////////////////////

Button::Button(IWindowParent &owner, ButtonOpts creationOpts)
	: _ctrl{owner.base()}, _events{owner.base(), valid_ctrl_id(creationOpts.ctrlId)}
{
	struct Ctx { Button* self; IWindowParent* owner; ButtonOpts opts; };
	auto* c = new Ctx{this, &owner, std::move(creationOpts)};
	_ctrl._parent._preEvents.wm_create_or_init_dialog(MkFunc0(+[](Ctx* c) {
		if (c->opts.size == ButtonOpts{}.size)
			c->opts.size = dpi::sz(c->opts.size); // special case: default size

		c->self->_ctrl.create_wnd(c->self->ctrl_id(), c->opts.styleEx, L"BUTTON", std::move(c->opts.text),
			c->opts.style, c->opts.pos, c->opts.size);
		apply_ui_font(c->self->hwnd());
		c->self->_ctrl._parent._layout.add(c->self->hwnd(), c->opts.layout);
		delete c;
	}, c));
}

Button::Button(IWindowParent &owner, WORD ctrlId, Lay layout)
	: _ctrl{owner.base()}, _events{owner.base(), ctrlId}
{
	struct Ctx { Button* self; Lay layout; };
	auto* c = new Ctx{this, layout};
	_ctrl._parent._preEvents.wm_create_or_init_dialog(MkFunc0(+[](Ctx* c) {
		c->self->_ctrl.assign_dlg(c->self->ctrl_id());
		c->self->_ctrl._parent._layout.add(c->self->hwnd(), c->layout);
		delete c;
	}, c));
}

const Button& Button::set_text(WStrView newText) const {
	set_wnd_text(hwnd(), newText);
	return *this;
}

const Button& Button::trigger_click() const {
	SendMessageW(hwnd(), BM_CLICK, 0, 0);
	return *this;
}

////////////////////////////////////////////////////////////////////////////////

CheckBox::CheckBox(IWindowParent &owner, CheckBoxOpts creationOpts)
	: _ctrl{owner.base()}, _events{owner.base(), valid_ctrl_id(creationOpts.ctrlId)}
{
	struct Ctx { CheckBox* self; IWindowParent* owner; CheckBoxOpts opts; };
	auto* c = new Ctx{this, &owner, std::move(creationOpts)};
	_ctrl._parent._preEvents.wm_create_or_init_dialog(MkFunc0(+[](Ctx* c) {
		if (!c->opts.size.cx && !c->opts.size.cy)
			c->opts.size = calc_text_bound_box_with_check(str::remove_accel_ampersands(c->opts.text));

		c->self->_ctrl.create_wnd(c->self->ctrl_id(), c->opts.styleEx, L"BUTTON", std::move(c->opts.text),
			c->opts.style, c->opts.pos, c->opts.size);
		apply_ui_font(c->self->hwnd());
		c->self->_ctrl._parent._layout.add(c->self->hwnd(), c->opts.layout);
		c->self->set_state(c->opts.state);
		delete c;
	}, c));
}

CheckBox::CheckBox(IWindowParent &owner, WORD ctrlId, Lay layout)
	: _ctrl{owner.base()}, _events{owner.base(), ctrlId}
{
	struct Ctx { CheckBox* self; Lay layout; };
	auto* c = new Ctx{this, layout};
	_ctrl._parent._preEvents.wm_create_or_init_dialog(MkFunc0(+[](Ctx* c) {
		c->self->_ctrl.assign_dlg(c->self->ctrl_id());
		c->self->_ctrl._parent._layout.add(c->self->hwnd(), c->layout);
		delete c;
	}, c));
}

WORD CheckBox::state() const {
	return static_cast<WORD>(SendMessageW(hwnd(), BM_GETCHECK, 0, 0));
}

const CheckBox& CheckBox::set_state(WORD bstFlag) const {
	bstFlag &= (BST_CHECKED | BST_INDETERMINATE | BST_UNCHECKED); // sanitize
	SendMessageW(hwnd(), BM_SETCHECK, bstFlag, 0);
	return *this;
}

const CheckBox& CheckBox::set_text(WStrView newText) const {
	set_wnd_text(hwnd(), newText);
	return *this;
}

const CheckBox& CheckBox::set_text_resize(WStrView newText) const {
	set_text(newText);
	SIZE bounds = calc_text_bound_box_with_check(str::remove_accel_ampersands(newText));
	SetWindowPos(hwnd(), nullptr, 0, 0, bounds.cx, bounds.cy, SWP_NOZORDER | SWP_NOMOVE);
	return *this;
}

////////////////////////////////////////////////////////////////////////////////

std::wstring ComboBox::ItemCollection::operator[](int index) const {
	size_t nChars = SendMessageW(_owner.hwnd(), CB_GETLBTEXTLEN, index, 0);
	std::wstring s(nChars + 1, L'\0');

	LRESULT ret = SendMessageW(_owner.hwnd(), CB_GETLBTEXT, index, reinterpret_cast<LPARAM>(s.data()));
	#ifdef _DEBUG
	if (ret == CB_ERR)
		throw std::runtime_error{"CB_GETLBTEXT failed to get text."};
	#endif

	s.resize(nChars);
	return s;
}

void ComboBox::ItemCollection::add(WStrView text) const {
	LRESULT ret = SendMessageW(_owner.hwnd(), CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(text.c_str()));
	#ifdef _DEBUG
	if (ret == CB_ERR || ret == CB_ERRSPACE)
		throw std::runtime_error{"CB_ADDSTRING failed to add text."};
	#endif
}

void ComboBox::ItemCollection::add(std::initializer_list<WStrView> texts) const {
	for (auto &&s : texts)
		add(s);
}

size_t ComboBox::ItemCollection::count() const {
	return SendMessageW(_owner.hwnd(), CB_GETCOUNT, 0, 0);
}

void ComboBox::ItemCollection::delete_all() const {
	SendMessageW(_owner.hwnd(), CB_RESETCONTENT, 0, 0);
}

void ComboBox::ItemCollection::select(int index) const {
	SendMessageW(_owner.hwnd(), CB_SETCURSEL, index, 0);
}

int ComboBox::ItemCollection::selected_index() const {
	return static_cast<int>(SendMessageW(_owner.hwnd(), CB_GETCURSEL, 0, 0));
}

std::optional<std::wstring> ComboBox::ItemCollection::selected_text() const {
	int selIdx = selected_index();
	return selIdx == -1 ? std::nullopt : std::make_optional(operator[](selIdx));
}

//------------------------------------------------------------------------------

ComboBox::ComboBox(IWindowParent &owner, ComboBoxOpts creationOpts)
	: _ctrl{owner.base()}, _events{owner.base(), valid_ctrl_id(creationOpts.ctrlId)}
{
	struct Ctx { ComboBox* self; IWindowParent* owner; ComboBoxOpts opts; };
	auto* c = new Ctx{this, &owner, std::move(creationOpts)};
	_ctrl._parent._preEvents.wm_create_or_init_dialog(MkFunc0(+[](Ctx* c) {
		if (c->opts.width == ComboBoxOpts{}.width)
			c->opts.width = dpi::x(c->opts.width); // special case: default width

		c->self->_ctrl.create_wnd(c->self->ctrl_id(), c->opts.styleEx, L"COMBOBOX", {},
			c->opts.style, c->opts.pos, {.cx = c->opts.width});
		apply_ui_font(c->self->hwnd());
		c->self->_ctrl._parent._layout.add(c->self->hwnd(), c->opts.layout);
		for (auto &&s : c->opts.texts)
			c->self->items.add(s);
		delete c;
	}, c));
}

ComboBox::ComboBox(IWindowParent &owner, WORD ctrlId, Lay layout)
	: _ctrl{owner.base()}, _events{owner.base(), ctrlId}
{
	struct Ctx { ComboBox* self; Lay layout; };
	auto* c = new Ctx{this, layout};
	_ctrl._parent._preEvents.wm_create_or_init_dialog(MkFunc0(+[](Ctx* c) {
		c->self->_ctrl.assign_dlg(c->self->ctrl_id());
		c->self->_ctrl._parent._layout.add(c->self->hwnd(), c->layout);
		delete c;
	}, c));
}

////////////////////////////////////////////////////////////////////////////////

DateTimePicker::DateTimePicker(IWindowParent &owner, DateTimePickerOpts creationOpts)
	: _ctrl{owner.base()}, _events{owner.base(), valid_ctrl_id(creationOpts.ctrlId)}
{
	struct Ctx { DateTimePicker* self; IWindowParent* owner; DateTimePickerOpts opts; };
	auto* c = new Ctx{this, &owner, std::move(creationOpts)};
	_ctrl._parent._preEvents.wm_create_or_init_dialog(MkFunc0(+[](Ctx* c) {
		if (c->opts.size == DateTimePickerOpts{}.size)
			c->opts.size = dpi::sz(c->opts.size); // special case: default size

		c->self->_ctrl.create_wnd(c->self->ctrl_id(), c->opts.styleEx, DATETIMEPICK_CLASSW, {},
			c->opts.style, c->opts.pos, c->opts.size);
		apply_ui_font(c->self->hwnd());
		c->self->_ctrl._parent._layout.add(c->self->hwnd(), c->opts.layout);
		if (c->opts.value.wYear)
			c->self->set_value(c->opts.value);
		delete c;
	}, c));
}

DateTimePicker::DateTimePicker(IWindowParent &owner, WORD ctrlId, Lay layout)
	: _ctrl{owner.base()}, _events{owner.base(), ctrlId}
{
	struct Ctx { DateTimePicker* self; Lay layout; };
	auto* c = new Ctx{this, layout};
	_ctrl._parent._preEvents.wm_create_or_init_dialog(MkFunc0(+[](Ctx* c) {
		c->self->_ctrl.assign_dlg(c->self->ctrl_id());
		c->self->_ctrl._parent._layout.add(c->self->hwnd(), c->layout);
		delete c;
	}, c));
}

SYSTEMTIME DateTimePicker::value() const {
	SYSTEMTIME st{};
	DWORD ret = DateTime_GetSystemtime(hwnd(), &st);
	#ifdef _DEBUG
	if (ret == GDT_VALID)
		throw std::runtime_error{"DateTime_SetSystemtime failed to set value."};
	#endif
	return st;
}

const DateTimePicker& DateTimePicker::set_value(const SYSTEMTIME &st) const {
	BOOL ok = DateTime_SetSystemtime(hwnd(), GDT_VALID, &st);
	#ifdef _DEBUG
	if (!ok)
		throw std::runtime_error{"DateTime_SetSystemtime failed to set value."};
	#endif
	return *this;
}

////////////////////////////////////////////////////////////////////////////////

Edit::Edit(IWindowParent &owner, EditOpts creationOpts)
	: _ctrl{owner.base()}, _events{owner.base(), valid_ctrl_id(creationOpts.ctrlId)}
{
	struct Ctx { Edit* self; IWindowParent* owner; EditOpts opts; };
	auto* c = new Ctx{this, &owner, std::move(creationOpts)};
	_ctrl._parent._preEvents.wm_create_or_init_dialog(MkFunc0(+[](Ctx* c) {
		if (c->opts.size == EditOpts{}.size)
			c->opts.size = dpi::sz(c->opts.size); // special case: default size

		c->self->_ctrl.create_wnd(c->self->ctrl_id(), c->opts.styleEx, L"EDIT", std::move(c->opts.text),
			c->opts.style, c->opts.pos, c->opts.size);
		apply_ui_font(c->self->hwnd());
		c->self->_ctrl._parent._layout.add(c->self->hwnd(), c->opts.layout);
		delete c;
	}, c));
}

Edit::Edit(IWindowParent &owner, WORD ctrlId, Lay layout)
	: _ctrl{owner.base()}, _events{owner.base(), ctrlId}
{
	struct Ctx { Edit* self; Lay layout; };
	auto* c = new Ctx{this, layout};
	_ctrl._parent._preEvents.wm_create_or_init_dialog(MkFunc0(+[](Ctx* c) {
		c->self->_ctrl.assign_dlg(c->self->ctrl_id());
		c->self->_ctrl._parent._layout.add(c->self->hwnd(), c->layout);
		delete c;
	}, c));
}

const Edit& Edit::set_text(WStrView newText) const {
	set_wnd_text(hwnd(), newText);
	return *this;
}

////////////////////////////////////////////////////////////////////////////////

std::vector<std::wstring> ListView::Column::item_texts() const {
	size_t count = _owner.items.count();
	std::vector<std::wstring> texts;
	texts.reserve(count);
	for (UINT i = 0; i < count; ++i)
		texts.emplace_back(_owner.items[i].text_of(_index));
	return texts;
}

std::vector<std::wstring> ListView::Column::selected_item_texts() const {
	std::vector<std::wstring> texts;
	texts.reserve(_owner.items.selected_count());

	int idx = -1;
	for (;;) {
		idx = ListView_GetNextItem(_owner.hwnd(), idx, LVNI_SELECTED);
		if (idx == -1) break;
		texts.emplace_back(_owner.items[idx].text_of(_index));
	}
	return texts;
}

int ListView::Column::justif() const {
	HWND hHeader = ListView_GetHeader(_owner.hwnd());
	#ifdef _DEBUG
	if (!hHeader)
		throw std::runtime_error{"ListView_GetHeader failed to get justification."};
	#endif

	HDITEMW hdi{.mask = HDI_FORMAT};
	BOOL ok = Header_GetItem(hHeader, _index, &hdi);
	#ifdef _DEBUG
	if (!ok)
		throw std::runtime_error{"Header_GetItem failed to get justification."};
	#endif

	return hdi.fmt & (HDF_LEFT | HDF_CENTER | HDF_RIGHT); // filter out
}

const ListView::Column& ListView::Column::set_justif(WORD hdf) const {
	HWND hHeader = ListView_GetHeader(_owner.hwnd());
	#ifdef _DEBUG
	if (!hHeader)
		throw std::runtime_error{"ListView_GetHeader failed to set justification."};
	#endif

	HDITEMW hdi{.mask = HDI_FORMAT};
	BOOL ok = Header_GetItem(hHeader, _index, &hdi); // first, retrieve current
	#ifdef _DEBUG
	if (!ok)
		throw std::runtime_error{"Header_GetItem failed to get justification."};
	#endif

	hdi.fmt &= ~(HDF_CENTER | HDF_LEFT | HDF_RIGHT); // clear all three
	hdi.fmt |= (hdf & (HDF_LEFT | HDF_CENTER | HDF_RIGHT)); // sanitize
	ok = Header_SetItem(hHeader, _index, &hdi);
	#ifdef _DEBUG
	if (!ok)
		throw std::runtime_error{"Header_SetItem failed to set justification."};
	#endif

	return *this;
}

WORD ListView::Column::sort_arrow() const {
	HWND hHeader = ListView_GetHeader(_owner.hwnd());
	#ifdef _DEBUG
	if (!hHeader)
		throw std::runtime_error{"ListView_GetHeader failed to get sort arrow."};
	#endif

	HDITEMW hdi{.mask = HDI_FORMAT};
	BOOL ok = Header_GetItem(hHeader, _index, &hdi);
	#ifdef _DEBUG
	if (!ok)
		throw std::runtime_error{"Header_GetItem failed to get sort arrow."};
	#endif

	return hdi.fmt & (HDF_SORTDOWN | HDF_SORTUP); // filter out
}

const ListView::Column& ListView::Column::set_sort_arrow(WORD hdf) const {
	HWND hHeader = ListView_GetHeader(_owner.hwnd());
	#ifdef _DEBUG
	if (!hHeader)
		throw std::runtime_error{"ListView_GetHeader failed to set sort arrow."};
	#endif

	UINT numCols = Header_GetItemCount(hHeader);

	for (UINT i = 0; i < numCols; ++i) {
		HDITEMW hdi{.mask = HDI_FORMAT};
		BOOL ok = Header_GetItem(hHeader, i, &hdi); // first, retrieve current
		#ifdef _DEBUG
		if (!ok)
			throw std::runtime_error{"Header_GetItem failed to get sort arrow."};
		#endif

		hdi.fmt &= ~(HDF_SORTDOWN | HDF_SORTUP); // clear all two
		if (i == _index) // only the targeted column will have the flag set
			hdi.fmt |= (hdf & (HDF_SORTDOWN | HDF_SORTUP)); // sanitize

		ok = Header_SetItem(hHeader, i, &hdi);
		#ifdef _DEBUG
		if (!ok)
			throw std::runtime_error{"Header_SetItem failed to set sort arrow."};
		#endif
	}

	return *this;
}

std::wstring ListView::Column::text() const {
	std::wstring buf(64, L'\0'); // arbitrary

	LVCOLUMNW lvc{
		.mask = LVCF_TEXT,
		.pszText = buf.data(),
		.cchTextMax = static_cast<int>(buf.size()),
	};

	ListView_GetColumn(_owner.hwnd(), _index, &lvc);
	str::trim_nulls(buf);
	return buf;
}

const ListView::Column& ListView::Column::set_text(WStrView newText) const {
	LVCOLUMNW lvc{
		.mask = LVCF_TEXT,
		.pszText = const_cast<LPWSTR>(newText.c_str()),
	};
	ListView_SetColumn(_owner.hwnd(), _index, &lvc);
	return *this;
}

UINT ListView::Column::width() const {
	return ListView_GetColumnWidth(_owner.hwnd(), _index);
}

const ListView::Column& ListView::Column::set_width(UINT width) const {
	ListView_SetColumnWidth(_owner.hwnd(), _index, width);
	return *this;
}

const ListView::Column& ListView::Column::set_width_to_fill() const {
	size_t numCols = _owner.cols.count();
	if (numCols == 0) return *this;

	UINT cxUsed = 0;
	for (UINT i = 0; i < numCols; ++i) {
		if (i != _index)
			cxUsed += _owner.cols[i].width(); // retrieve cx of each column, but us
	}

	RECT rc{};
	GetClientRect(_owner.hwnd(), &rc); // list view client area
	return set_width(rc.right - cxUsed);
}

//------------------------------------------------------------------------------

ListView::Column ListView::ColumnCollection::add(WStrView text, UINT width) const {
	LVCOLUMNW lvc{
		.mask = LVCF_TEXT | LVCF_WIDTH,
		.cx = static_cast<int>(width),
		.pszText = const_cast<LPWSTR>(text.c_str()),
	};
	int index = ListView_InsertColumn(_owner.hwnd(), 0xffff, &lvc); // insert as the last column
	#ifdef _DEBUG
	if (index == -1)
		throw std::runtime_error{"ListView_InsertColumn failed."};
	#endif
	return {_owner, index}; // return newly added column
}

size_t ListView::ColumnCollection::count() const {
	HWND hHeader = ListView_GetHeader(_owner.hwnd());
	return Header_GetItemCount(hHeader);
}

//------------------------------------------------------------------------------

bool ListView::Item::is_focused() const {
	return ListView_GetItemState(_owner.hwnd(), _index, LVIS_FOCUSED) & LVIS_FOCUSED;
}

const ListView::Item& ListView::Item::focus() const {
	ListView_SetItemState(_owner.hwnd(), _index, LVIS_FOCUSED, LVIS_FOCUSED);
	return *this;
}

int ListView::Item::icon_index() const {
	#ifdef _DEBUG
	if (!_owner._imgList16.himagelist()
			|| !_owner._imgList16.count()
			|| !_owner._imgList32.himagelist()
			|| !_owner._imgList32.count())
		throw std::logic_error{"No icons have been added to any image list."};
	#endif

	LVITEMW lvi{
		.mask = LVIF_IMAGE,
		.iItem = _index,
	};

	BOOL ok = ListView_GetItem(_owner.hwnd(), &lvi);
	#ifdef _DEBUG
	if (!ok)
		throw std::runtime_error{"ListView_GetItem failed to get LPARAM."};
	#endif

	return lvi.iImage;
}

const ListView::Item& ListView::Item::set_icon_index(int iconIndex) const {
	#ifdef _DEBUG
	if (!_owner._imgList16.himagelist()
			|| !_owner._imgList16.count()
			|| !_owner._imgList32.himagelist()
			|| !_owner._imgList32.count())
		throw std::logic_error{"No icons have been added to any image list."};
	#endif

	LVITEMW lvi{
		.mask = LVIF_IMAGE,
		.iItem = _index,
		.iImage = iconIndex,
	};

	BOOL ok = ListView_SetItem(_owner.hwnd(), &lvi);
	#ifdef _DEBUG
	if (!ok)
		throw std::runtime_error{"ListView_SetItem failed to set icon."};
	#endif

	return *this;
}

const ListView::Item& ListView::Item::remove() const {
	ListView_DeleteItem(_owner.hwnd(), _index);
	return *this;
}

bool ListView::Item::is_selected() const {
	return ListView_GetItemState(_owner.hwnd(), _index, LVIS_SELECTED) & LVIS_SELECTED;
}

const ListView::Item& ListView::Item::select(bool doSelect) const {
	ListView_SetItemState(_owner.hwnd(), _index, doSelect ? LVIS_SELECTED : 0, LVIS_SELECTED);
	return *this;
}

std::wstring ListView::Item::text_of(UINT columnIndex) const {
	UINT curBufSz = str::SSO_LEN;
	std::wstring buf(curBufSz, L'\0');

	for (;;) {
		LVITEMW lvi{
			.mask = LVIF_TEXT,
			.iSubItem = static_cast<int>(columnIndex),
			.pszText = buf.data(),
			.cchTextMax = static_cast<int>(curBufSz),
		};

		UINT recvChars = static_cast<UINT>(
			SendMessageW(_owner.hwnd(), LVM_GETITEMTEXTW, _index, reinterpret_cast<LPARAM>(&lvi)));
		recvChars += 1; // plus terminating null count

		if (recvChars < curBufSz) { // to break, must have at least 1 char gap
			str::trim_nulls(buf);
			return buf;
		}

		curBufSz *= 2; // double the buffer size to try again
		buf.resize(curBufSz, L'\0');
	}
}

const ListView::Item& ListView::Item::set_text_of(UINT columnIndex, WStrView newText) const {
	LVITEMW lvi{};
	lvi.iSubItem = static_cast<int>(columnIndex);
	lvi.pszText = const_cast<LPWSTR>(newText.c_str());
	SendMessageW(_owner.hwnd(), LVM_SETITEMTEXTW, static_cast<WPARAM>(_index), reinterpret_cast<LPARAM>(&lvi));
	return *this;
}

const ListView::Item& ListView::Item::set_texts(std::initializer_list<WStrView> texts) const {
	size_t nCols = _owner.cols.count();
	size_t nToUpdate = std::min(nCols, texts.size());
	size_t i = 0;

	for (auto &&text : texts) {
		if (i >= nToUpdate) break;
		set_text_of(static_cast<UINT>(i), text);
		++i;
	}

	return *this;
}

UINT ListView::Item::unique_id() const {
	return ListView_MapIndexToID(_owner.hwnd(), _index);
}

bool ListView::Item::is_visible() const {
	return ListView_IsItemVisible(_owner.hwnd(), _index);
}

LPARAM ListView::Item::raw_data() const {
	LVITEMW lvi{
		.mask = LVIF_PARAM,
		.iItem = _index,
	};

	BOOL ok = ListView_GetItem(_owner.hwnd(), &lvi);
	#ifdef _DEBUG
	if (!ok)
		throw std::runtime_error{"ListView_GetItem failed to get LPARAM."};
	#endif

	return lvi.lParam;
}

const ListView::Item& ListView::Item::set_raw_data(LPARAM data) const {
	LVITEMW lvi{
		.mask = LVIF_PARAM,
		.iItem = _index,
		.lParam = data,
	};

	BOOL ok = ListView_SetItem(_owner.hwnd(), &lvi);
	#ifdef _DEBUG
	if (!ok)
		throw std::runtime_error{"ListView_SetItem failed to set LPARAM."};
	#endif

	return *this;
}

//------------------------------------------------------------------------------

ListView::Item ListView::ItemCollection::add(WStrView text,
	std::initializer_list<WStrView> otherColumnsTexts, int iconIndex) const
{
	LVITEMW lvi{
		.mask = LVIF_TEXT | static_cast<UINT>(iconIndex > -1 ? LVIF_IMAGE : 0),
		.iItem = 0x0fff'ffff, // insert as the last item
		.pszText = const_cast<LPWSTR>(text.c_str()),
		.iImage = iconIndex,
	};
	int index = ListView_InsertItem(_owner.hwnd(), &lvi);
	#ifdef _DEBUG
	if (index == -1)
		throw std::runtime_error{"ListView_InsertItem failed."};
	#endif

	Item newItem{_owner, index};

	for (auto colText = otherColumnsTexts.begin(); colText != otherColumnsTexts.end(); ++colText) {
		size_t idx = std::distance(otherColumnsTexts.begin(), colText);
		newItem.set_text_of(static_cast<UINT>(idx) + 1, *colText);
	}

	return newItem; // return newly added item
}

size_t ListView::ItemCollection::count() const {
	return ListView_GetItemCount(_owner.hwnd());
}

void ListView::ItemCollection::delete_all() const {
	ListView_DeleteAllItems(_owner.hwnd());
}

void ListView::ItemCollection::delete_selected() const {
	for (;;) {
		int idxFound = ListView_GetNextItem(_owner.hwnd(), -1, LVNI_SELECTED); // always search first one
		if (idxFound == -1) break;
		ListView_DeleteItem(_owner.hwnd(), idxFound);
	}
}

ListView::Item* ListView::ItemCollection::focused() {
	int idxFound = ListView_GetNextItem(_owner.hwnd(), -1, LVNI_FOCUSED);
	if (idxFound == -1) return nullptr;
	std::construct_at(&_lastResult, _owner, idxFound);
	return &_lastResult;
}

ListView::Item* ListView::ItemCollection::get_by_unique_id(UINT uid) {
	int idx = ListView_MapIDToIndex(_owner.hwnd(), uid);
	if (idx == -1) return nullptr;
	std::construct_at(&_lastResult, _owner, idx);
	return &_lastResult;
}

ListView::Item* ListView::ItemCollection::hit_test(POINT pt) {
	LVHITTESTINFO lvhti{.pt = pt};
	int idxFound = ListView_HitTestEx(_owner.hwnd(), &lvhti);
	if (idxFound == -1) return nullptr;
	std::construct_at(&_lastResult, _owner, idxFound);
	return &_lastResult;
}

void ListView::ItemCollection::select_all(bool doSelect) const {
	if (GetWindowLongPtrW(_owner.hwnd(), GWL_STYLE) & LVS_SINGLESEL) [[unlikely]] {
		return; // single-sel list views cannot have all items selected
	}
	ListView_SetItemState(_owner.hwnd(), -1, doSelect ? LVIS_SELECTED : 0, LVIS_SELECTED);
}

std::vector<ListView::Item> ListView::ItemCollection::selected() const {
	std::vector<Item> items;
	items.reserve(selected_count());

	int idx = -1;
	for (;;) {
		idx = ListView_GetNextItem(_owner.hwnd(), idx, LVNI_SELECTED);
		if (idx == -1) break;
		items.emplace_back(_owner, idx);
	}
	return items;
}

size_t ListView::ItemCollection::selected_count() const {
	return ListView_GetSelectedCount(_owner.hwnd());
}

void ListView::ItemCollection::sort(std::function<int(Item, Item)> cb) const {
	struct Info final {
		const ListView &owner;
		std::function<int(Item, Item)> cb;
	};
	Info nfo = {
		.owner = _owner,
		.cb = std::move(cb),
	};

	BOOL ok = ListView_SortItemsEx(_owner.hwnd(), [](LPARAM idxA, LPARAM idxB, LPARAM lp) -> int { // receives indexes
		Info* pNfo = reinterpret_cast<Info*>(lp);
		return pNfo->cb(
			pNfo->owner.items[static_cast<int>(idxA)],
			pNfo->owner.items[static_cast<int>(idxB)]);
	}, &nfo);
	#ifdef _DEBUG
	if (!ok)
		throw std::runtime_error{"ListView_SortItemsEx failed."};
	#endif
}

ListView::Item* ListView::ItemCollection::topmost_visible() {
	int idx = ListView_GetTopIndex(_owner.hwnd());
	if (idx == -1) return nullptr;
	std::construct_at(&_lastResult, _owner, idx);
	return &_lastResult;
}

//------------------------------------------------------------------------------

ListView::ListView(IWindowParent &owner, ListViewOpts creationOpts)
	: _ctrl{owner.base()}, _events{owner.base(), valid_ctrl_id(creationOpts.ctrlId)}
{
	load_context_menu(creationOpts.contextMenuId);

	struct Ctx { ListView* self; IWindowParent* owner; ListViewOpts opts; };
	auto* c = new Ctx{this, &owner, std::move(creationOpts)};
	_ctrl._parent._preEvents.wm_create_or_init_dialog(MkFunc0(+[](Ctx* c) {
		if (c->opts.size == ListViewOpts{}.size)
			c->opts.size = dpi::sz(c->opts.size); // special case: default size

		c->self->_ctrl.create_wnd(c->self->ctrl_id(), c->opts.styleEx, WC_LISTVIEWW, {},
			c->opts.style | LVS_SHAREIMAGELISTS, c->opts.pos, c->opts.size);
		if (c->opts.styleExListView)
			c->self->set_extended_style(true, c->opts.styleExListView);
		c->self->_ctrl._parent._layout.add(c->self->hwnd(), c->opts.layout);
		for (auto &&col : c->opts.cols)
			c->self->cols.add(col.text, col.width);

		for (auto &&ico : c->opts.icons16) {
			if (ico.id) c->self->icons_16().add_resource(ico.id);
			else        c->self->icons_16().add_shell_ext(ico.ext);
		}
		for (auto &&ico : c->opts.icons32) {
			if (ico.id) c->self->icons_32().add_resource(ico.id);
			else        c->self->icons_32().add_shell_ext(ico.ext);
		}
		delete c;
	}, c));

	custom_events();
}

ListView::ListView(IWindowParent &owner, WORD ctrlId, Lay layout, WORD contextMenuId)
	: _ctrl{owner.base()}, _events{owner.base(), ctrlId}
{
	load_context_menu(contextMenuId);

	struct Ctx { ListView* self; Lay layout; };
	auto* c = new Ctx{this, layout};
	_ctrl._parent._preEvents.wm_create_or_init_dialog(MkFunc0(+[](Ctx* c) {
		c->self->_ctrl.assign_dlg(c->self->ctrl_id());
		SetWindowLongPtrW(c->self->hwnd(), GWL_STYLE, GetWindowLongPtrW(c->self->hwnd(), GWL_STYLE) | LVS_SHAREIMAGELISTS);
		c->self->_ctrl._parent._layout.add(c->self->hwnd(), c->layout);
		delete c;
	}, c));

	custom_events();
}

const ListView& ListView::set_extended_style(bool doSet, DWORD exStyle) const {
	ListView_SetExtendedListViewStyleEx(hwnd(), exStyle, doSet ? exStyle : 0);
	return *this;
}

IStoreIcon& ListView::icons_16() {
	if (!_imgList16.himagelist()) { // not created yet?
		_imgList16.create();
		ListView_SetImageList(hwnd(), _imgList16.himagelist(), LVSIL_SMALL);
	}
	return _imgList16;
}

IStoreIcon& ListView::icons_32() {
	if (!_imgList32.himagelist()) { // not created yet?
		_imgList32.create();
		ListView_SetImageList(hwnd(), _imgList32.himagelist(), LVSIL_NORMAL);
	}
	return _imgList32;
}

void ListView::load_context_menu(WORD contextMenuId) {
	if (!contextMenuId) return;

	_hMenuContext = LoadMenuW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(contextMenuId));
	#ifdef _DEBUG
	if (!_hMenuContext)
		throw std::system_error(GetLastError(), std::system_category(), "LoadMenu failed to load ListView context menu");
	#endif
}

void ListView::custom_events() {
	_ctrl._subclassEvents.wm(WM_GETDLGCODE, MkFunc1(+[](ListView* self, wm::Msg& m) {
		wm::GetDlgCode p{m};
		if (!p.is_query() && p.msg()->message == WM_KEYDOWN && p.vkey_code() == VK_RETURN) { // Enter key
			NMLVKEYDOWN nmlvkd{
				.hdr{
					.hwndFrom = self->hwnd(),
					.idFrom = self->ctrl_id(),
					.code = LVN_KEYDOWN,
				},
				.wVKey = VK_RETURN,
			};
			HWND hParent = GetAncestor(self->hwnd(), GA_PARENT);
			SendMessageW(hParent, WM_NOTIFY, self->ctrl_id(), reinterpret_cast<LPARAM>(&nmlvkd)); // send Enter key to parent
		}
		m.ret = DefSubclassProc(self->hwnd(), WM_GETDLGCODE, p.wparam(), p.lparam()); // let system define DLGC
	}, this));

	_ctrl._parent._preEvents.wm_notify(ctrl_id(), LVN_KEYDOWN, MkFunc1(+[](ListView* self, wm::Notify& p) {
		NMLVKEYDOWN &nmk = p.hdr<NMLVKEYDOWN>();
		bool hasCtrl = GetAsyncKeyState(VK_CONTROL) & 0x8000;

		if (hasCtrl && nmk.wVKey == 'A') { // Ctrl+A pressed?
			self->items.select_all(true);
		} else if (nmk.wVKey == VK_APPS) { // context menu key?
			bool hasShift = GetAsyncKeyState(VK_SHIFT) & 0x8000;
			self->show_context_menu(false, hasCtrl, hasShift);
		}
	}, this));

	_ctrl._parent._preEvents.wm_notify(ctrl_id(), NM_RCLICK, MkFunc1(+[](ListView* self, wm::Notify& p) {
		NMITEMACTIVATE &nmi = p.hdr<NMITEMACTIVATE>();
		bool hasCtrl = nmi.uKeyFlags & LVKF_CONTROL;
		bool hasShift = nmi.uKeyFlags & LVKF_SHIFT;

		self->show_context_menu(true, hasCtrl, hasShift);
	}, this));

	_ctrl._parent._postEvents.wm(WM_DESTROY, MkFunc1(+[](ListView* self, wm::Msg&) {
		if (self->_hMenuContext)
			DestroyMenu(self->_hMenuContext);
	}, this));
}

void ListView::show_context_menu(bool followCursor, bool hasCtrl, bool hasShift) {
	if (!_hMenuContext) return;

	POINT menuPos{};
	if (followCursor) { // usually when fired by a right-click
		GetCursorPos(&menuPos); // relative to screen
		ScreenToClient(hwnd(), &menuPos); // now relative to listview
		Item *itemOver = items.hit_test(menuPos);
		if (!itemOver) { // no item was right-clicked
			items.select_all(false);
		} else if (!hasCtrl && !hasShift) {
			itemOver->select(true).focus(); // if note yet
		}
		focus(); // because a right-click won't set the focus by itself
	} else { // usually fired by the context meny key
		Item *itemFocused = items.focused();
		if (itemFocused && itemFocused->is_visible()) {
			RECT rc{.left = LVIR_BOUNDS};
			SendMessageW(hwnd(), LVM_GETITEMRECT, itemFocused->index(), reinterpret_cast<LPARAM>(&rc));
			menuPos = {.x = rc.left + 16, .y = rc.top + (rc.bottom - rc.top) / 2};
		} else { // no item is focused and visible
			menuPos = {.x = 6, .y = 10}; // arbitrary coordinates
		}
	}

	HWND hParent = GetParent(hwnd());
	HMENU hSubMenu = GetSubMenu(_hMenuContext, 0); // pop the first submenu
	#ifdef _DEBUG
	if (!hSubMenu)
		throw std::invalid_argument{"GetSubMenu failed to load ListView context submenu."};
	#endif
	ClientToScreen(hwnd(), &menuPos); // from listview to screen
	SetForegroundWindow(hParent);
	BOOL retTrk = TrackPopupMenu(hSubMenu, TPM_LEFTBUTTON, menuPos.x, menuPos.y, 0, hParent, nullptr);
	#ifdef _DEBUG
	if (!retTrk)
		throw std::runtime_error{"TrackPopupMenu failed to load ListView context submenu."};
	#endif
	PostMessageW(hParent, WM_NULL, 0, 0); // necessary according to TrackPopupMenu docs
}

////////////////////////////////////////////////////////////////////////////////

MonthCalendar::MonthCalendar(IWindowParent &owner, wl::MonthCalendarOpts creationOpts)
	: _ctrl{owner.base()}, _events{owner.base(), valid_ctrl_id(creationOpts.ctrlId)}
{
	struct Ctx { MonthCalendar* self; IWindowParent* owner; MonthCalendarOpts opts; };
	auto* c = new Ctx{this, &owner, std::move(creationOpts)};
	_ctrl._parent._preEvents.wm_create_or_init_dialog(MkFunc0(+[](Ctx* c) {
		c->self->_ctrl.create_wnd(c->self->ctrl_id(), c->opts.styleEx, MONTHCAL_CLASSW, {}, c->opts.style, c->opts.pos, {});

		RECT rcBounds{};
		SendMessageW(c->self->hwnd(), MCM_GETMINREQRECT, 0, reinterpret_cast<LPARAM>(&rcBounds)); // request ideal size
		SetWindowPos(c->self->hwnd(), nullptr, 0, 0, rcBounds.right, rcBounds.bottom, SWP_NOZORDER | SWP_NOMOVE);

		if (c->opts.value.wYear && c->opts.value.wMonth)
			c->self->set_value(c->opts.value);

		c->self->_ctrl._parent._layout.add(c->self->hwnd(), c->opts.layout);
		delete c;
	}, c));
}

MonthCalendar::MonthCalendar(IWindowParent &owner, WORD ctrlId, Lay layout)
	: _ctrl{owner.base()}, _events{owner.base(), ctrlId}
{
	struct Ctx { MonthCalendar* self; Lay layout; };
	auto* c = new Ctx{this, layout};
	_ctrl._parent._preEvents.wm_create_or_init_dialog(MkFunc0(+[](Ctx* c) {
		c->self->_ctrl.assign_dlg(c->self->ctrl_id());
		c->self->_ctrl._parent._layout.add(c->self->hwnd(), c->layout);
		delete c;
	}, c));
}

SYSTEMTIME MonthCalendar::value() const {
	SYSTEMTIME st{};
	LRESULT ret = SendMessageW(hwnd(), MCM_GETCURSEL, 0, reinterpret_cast<LPARAM>(&st));
	#ifdef _DEBUG
	if (!ret)
		throw std::runtime_error{"MCM_GETCURSEL failed."};
	#endif
	return st;
}

const MonthCalendar& MonthCalendar::set_value(const SYSTEMTIME &st) const {
	LRESULT ret = SendMessageW(hwnd(), MCM_SETCURSEL, 0, reinterpret_cast<LPARAM>(&st));
	#ifdef _DEBUG
	if (!ret)
		throw std::runtime_error{"MCM_SETCURSEL failed."};
	#endif
	return *this;
}

////////////////////////////////////////////////////////////////////////////////

RadioButton::RadioButton(IWindowParent &owner, RadioButtonOpts creationOpts)
	: _ctrl{owner.base()}, _events{owner.base(), valid_ctrl_id(creationOpts.ctrlId)}
{
	struct Ctx { RadioButton* self; IWindowParent* owner; RadioButtonOpts opts; };
	auto* c = new Ctx{this, &owner, std::move(creationOpts)};
	_ctrl._parent._preEvents.wm_create_or_init_dialog(MkFunc0(+[](Ctx* c) {
		if (!c->opts.size.cx && !c->opts.size.cy)
			c->opts.size = calc_text_bound_box_with_check(str::remove_accel_ampersands(c->opts.text));

		c->self->_ctrl.create_wnd(c->self->ctrl_id(), c->opts.styleEx, L"BUTTON", std::move(c->opts.text),
			c->opts.style, c->opts.pos, c->opts.size);
		apply_ui_font(c->self->hwnd());
		c->self->_ctrl._parent._layout.add(c->self->hwnd(), c->opts.layout);
		if (c->opts.selected) c->self->select();
		delete c;
	}, c));
}

RadioButton::RadioButton(IWindowParent &owner, WORD ctrlId, Lay layout)
	: _ctrl{owner.base()}, _events{owner.base(), ctrlId}
{
	struct Ctx { RadioButton* self; Lay layout; };
	auto* c = new Ctx{this, layout};
	_ctrl._parent._preEvents.wm_create_or_init_dialog(MkFunc0(+[](Ctx* c) {
		c->self->_ctrl.assign_dlg(c->self->ctrl_id());
		c->self->_ctrl._parent._layout.add(c->self->hwnd(), c->layout);
		delete c;
	}, c));
}

bool RadioButton::is_selected() const {
	return SendMessageW(hwnd(), BM_GETCHECK, 0, 0) == BST_CHECKED;
}

const RadioButton& RadioButton::select() const {
	SendMessageW(hwnd(), BM_SETCHECK, BST_CHECKED, 0);
	return *this;
}

const RadioButton& RadioButton::set_text(WStrView newText) const {
	set_wnd_text(hwnd(), newText);
	return *this;
}

const RadioButton& RadioButton::set_text_resize(WStrView newText) const {
	set_text(newText);
	SIZE bounds = calc_text_bound_box_with_check(str::remove_accel_ampersands(newText));
	SetWindowPos(hwnd(), nullptr, 0, 0, bounds.cx, bounds.cy, SWP_NOZORDER | SWP_NOMOVE);
	return *this;
}

////////////////////////////////////////////////////////////////////////////////

RadioGroup::RadioGroup(IWindowParent &owner, std::initializer_list<RadioButtonOpts> creationOpts)
	: _owner{owner}, _radios{creationOpts.size()}, _events{*this}
{
	#ifdef _DEBUG
	if (!creationOpts.size())
		throw std::invalid_argument{"Cannot create a RadioGroup with zero radio controls."};
	#endif

	size_t i = 0;
	for (auto &&opt : creationOpts) {
		RadioButtonOpts fixedOpts{opt};
		if (!i) fixedOpts.style |= WS_GROUP; // first radio of the group
		else    fixedOpts.style &= ~WS_GROUP;

		new (&_radios[i++]) RadioButton{owner, std::move(fixedOpts)}; // invoke constructor manually
	}
}

RadioGroup::RadioGroup(IWindowParent &owner, Lay layout, std::initializer_list<WORD> ctrlIds)
	: _owner{owner}, _radios{ctrlIds.size()}, _events{*this}
{
	#ifdef _DEBUG
	if (!ctrlIds.size())
		throw std::invalid_argument{"Cannot create a RadioGroup with zero radio controls."};
	#endif

	size_t i = 0;
	for (WORD ctrlId : ctrlIds)
		new (&_radios[i++]) RadioButton{owner, ctrlId, layout}; // invoke constructor manually
}

////////////////////////////////////////////////////////////////////////////////

Static::Static(IWindowParent &owner, StaticOpts creationOpts)
	: _ctrl{owner.base()}, _events{owner.base(), valid_ctrl_id(creationOpts.ctrlId)}
{
	struct Ctx { Static* self; IWindowParent* owner; StaticOpts opts; };
	auto* c = new Ctx{this, &owner, std::move(creationOpts)};
	_ctrl._parent._preEvents.wm_create_or_init_dialog(MkFunc0(+[](Ctx* c) {
		if (!c->opts.size.cx && !c->opts.size.cy)
			c->opts.size = calc_text_bound_box(str::remove_accel_ampersands(c->opts.text));

		c->self->_ctrl.create_wnd(c->self->ctrl_id(), c->opts.styleEx, L"STATIC", std::move(c->opts.text),
			c->opts.style, c->opts.pos, c->opts.size);
		apply_ui_font(c->self->hwnd());
		c->self->_ctrl._parent._layout.add(c->self->hwnd(), c->opts.layout);
		delete c;
	}, c));
}

Static::Static(IWindowParent &owner, WORD ctrlId, Lay layout)
	: _ctrl{owner.base()}, _events{owner.base(), ctrlId}
{
	struct Ctx { Static* self; Lay layout; };
	auto* c = new Ctx{this, layout};
	_ctrl._parent._preEvents.wm_create_or_init_dialog(MkFunc0(+[](Ctx* c) {
		c->self->_ctrl.assign_dlg(c->self->ctrl_id());
		c->self->_ctrl._parent._layout.add(c->self->hwnd(), c->layout);
		delete c;
	}, c));
}

const Static& Static::set_text(WStrView newText) const {
	set_wnd_text(hwnd(), newText);
	return *this;
}

const Static& Static::set_text_resize(WStrView newText) const {
	set_text(newText);
	SIZE bounds = calc_text_bound_box(str::remove_accel_ampersands(newText));
	SetWindowPos(hwnd(), nullptr, 0, 0, bounds.cx, bounds.cy, SWP_NOZORDER | SWP_NOMOVE);
	return *this;
}

////////////////////////////////////////////////////////////////////////////////

std::wstring StatusBar::Part::text() const {
	LRESULT len = SendMessageW(_owner.hwnd(), SB_GETTEXTLENGTHW, 0, 0);
	std::wstring buf(len + 1, L'\0');
	SendMessageW(_owner.hwnd(), SB_GETTEXTW, _index, reinterpret_cast<LPARAM>(buf.data()));
	buf.resize(len);
	return buf;
}

const StatusBar::Part& StatusBar::Part::set_text(WStrView newText) const {
	SendMessageW(_owner.hwnd(), SB_SETTEXTW, MAKELONG(_index, 0),
		reinterpret_cast<LPARAM>(newText.c_str()));
	return *this;
}

const StatusBar::Part& StatusBar::Part::set_icon_index(int iconIndex) const {
	#ifdef _DEBUG
	if (!_owner._iconStore16.count())
		throw std::logic_error{"No icons have been added to the icon store."};
	#endif

	SendMessageW(_owner.hwnd(), SB_SETICON, _index,
		reinterpret_cast<LPARAM>(_owner._iconStore16[iconIndex]));
	return *this;
}

//------------------------------------------------------------------------------

StatusBar::StatusBar(IWindowParent &owner, StatusBarOpts creationOpts) :
	_ctrl{owner.base()},
	_events{owner.base(), valid_ctrl_id(creationOpts.ctrlId)},
	_parts{std::move(creationOpts.parts)}
{
	_rightEdges.resize(_parts.size(), 0);
	for (auto &&ico : creationOpts.icons) {
		if (ico.id) _iconStore16.add_resource(ico.id);
		else        _iconStore16.add_shell_ext(ico.ext);
	}

	struct Ctx { StatusBar* self; IWindowParent* owner; };
	auto* c = new Ctx{this, &owner};
	_ctrl._parent._preEvents.wm_create_or_init_dialog(MkFunc0(+[](Ctx* c) {
		DWORD parentStyle = static_cast<DWORD>(GetWindowLongPtrW(c->owner->hwnd(), GWL_STYLE));
		bool isParentResizable = (parentStyle & WS_MAXIMIZEBOX) || (parentStyle & WS_SIZEBOX);
		DWORD style = WS_CHILD | WS_VISIBLE | SBARS_TOOLTIPS | (isParentResizable ? SBARS_SIZEGRIP : 0);
		c->self->_ctrl.create_wnd(c->self->ctrl_id(), WS_EX_LEFT, STATUSCLASSNAMEW, {}, style, {}, {});

		RECT rcParent{};
		GetClientRect(c->owner->hwnd(), &rcParent);
		c->self->resize_to_fit_parent(wm::Msg{WM_SIZE, SIZE_RESTORED, MAKELPARAM(rcParent.right, 0)}); // will create the parts

		for (UINT i = 0; i < c->self->_parts.size(); ++i) { // add text
			if (!c->self->_parts[i].text.empty())
				c->self->parts[i].set_text(c->self->_parts[i].text);
		}
		delete c;
	}, c));

	_ctrl._parent._postEvents.wm_create_or_init_dialog(MkFunc0(+[](StatusBar* c) {
		for (UINT i = 0; i < c->_parts.size(); ++i) { // icons are manually added by user in WM_CREATE/WM_INITDIALOG
			if (c->_parts[i].iconIndex > -1)
				c->parts[i].set_icon_index(c->_parts[i].iconIndex);
		}
	}, this));

	_ctrl._parent._preEvents.wm(WM_SIZE, MkFunc1(+[](StatusBar* self, wm::Msg& m) {
		wm::Size p{m};
		self->resize_to_fit_parent(p);
	}, this));
}

void StatusBar::resize_to_fit_parent(wm::Size p) {
	if (p.is_minimized() || !hwnd()) return;

	SendMessageW(hwnd(), WM_SIZE, 0, 0); // tell status bar to fit parent
	if (_parts.empty()) return;

	int totalWeight = 0; // total weight of all flex parts
	int cxVariable = p.sz().cx; // remaning width to be divided among flex parts
	for (int i = 0; i < _parts.size(); ++i) {
		if (parts[i].is_fixed_width())
			cxVariable -= _parts[i].width; // decrease available width room
		else
			totalWeight += _parts[i].flex;
	}

	int cxTotal = p.sz().cx;
	for (int i = static_cast<int>(_parts.size()); i-- > 0; ) { // fill right edges array with the right edge of each part
		_rightEdges[i] = cxTotal;
		if (parts[i].is_fixed_width())
			cxTotal -= _parts[i].width;
		else
			cxTotal -= (cxVariable / totalWeight) * _parts[i].flex;
	}
	SendMessageW(hwnd(), SB_SETPARTS, _rightEdges.size(), reinterpret_cast<LPARAM>(_rightEdges.data()));
}

////////////////////////////////////////////////////////////////////////////////

SysLink::SysLink(IWindowParent &owner, SysLinkOpts creationOpts)
	: _ctrl{owner.base()}, _events{owner.base(), valid_ctrl_id(creationOpts.ctrlId)}
{
	struct Ctx { SysLink* self; IWindowParent* owner; SysLinkOpts opts; };
	auto* c = new Ctx{this, &owner, std::move(creationOpts)};
	_ctrl._parent._preEvents.wm_create_or_init_dialog(MkFunc0(+[](Ctx* c) {
		if (!c->opts.size.cx && !c->opts.size.cy)
			c->opts.size = calc_text_bound_box(str::remove_accel_ampersands(remove_html_anchor(c->opts.text)));

		c->self->_ctrl.create_wnd(c->self->ctrl_id(), c->opts.styleEx, WC_LINK, std::move(c->opts.text),
			c->opts.style, c->opts.pos, c->opts.size);
		apply_ui_font(c->self->hwnd());
		c->self->_ctrl._parent._layout.add(c->self->hwnd(), c->opts.layout);
		delete c;
	}, c));
}

SysLink::SysLink(IWindowParent &owner, WORD ctrlId, Lay layout)
	: _ctrl{owner.base()}, _events{owner.base(), ctrlId}
{
	struct Ctx { SysLink* self; Lay layout; };
	auto* c = new Ctx{this, layout};
	_ctrl._parent._preEvents.wm_create_or_init_dialog(MkFunc0(+[](Ctx* c) {
		c->self->_ctrl.assign_dlg(c->self->ctrl_id());
		c->self->_ctrl._parent._layout.add(c->self->hwnd(), c->layout);
		delete c;
	}, c));
}

const SysLink& SysLink::set_text(WStrView newText) const {
	set_wnd_text(hwnd(), newText);
	return *this;
}

const SysLink& SysLink::set_text_resize(WStrView newText) const {
	set_text(newText);
	SIZE bounds = calc_text_bound_box(str::remove_accel_ampersands(remove_html_anchor(newText)));
	SetWindowPos(hwnd(), nullptr, 0, 0, bounds.cx, bounds.cy, SWP_NOZORDER | SWP_NOMOVE);
	return *this;
}

std::wstring SysLink::remove_html_anchor(WStrView text) {
	size_t textLen = text.length();
	std::wstring out;
	out.reserve(textLen);

	bool withinAnchor = false;
	for (size_t i = 0; i < textLen; ++i) {
		if (withinAnchor) {
			if (text[i] == L'>') {
				withinAnchor = false;
				continue;
			}
		}
		if (text[i] == L'<') {
			withinAnchor = true;
			continue;
		}
		if (!withinAnchor)
			out.push_back(text[i]);
	}

	return out;
}

////////////////////////////////////////////////////////////////////////////////

const Tab::Item& Tab::Item::select() const {
	TabCtrl_SetCurSel(_owner.hwnd(), _index);
	return *this;
}

std::wstring Tab::Item::text() const {
	std::wstring buf(128, L'\0'); // arbitrary

	TCITEMW tci{
		.mask = TCIF_TEXT,
		.pszText = buf.data(),
		.cchTextMax = static_cast<int>(buf.size()),
	};

	BOOL ok = TabCtrl_GetItem(_owner.hwnd(), _index, &tci);
	#ifdef _DEBUG
	if (!ok)
		throw std::runtime_error{"TabCtrl_GetItem failed to get text."};
	#endif

	str::trim_nulls(buf);
	return buf;
}

const Tab::Item& Tab::Item::set_text(WStrView newText) const {
	TCITEMW tci{
		.mask = TCIF_TEXT,
		.pszText = const_cast<LPWSTR>(newText.c_str()),
	};

	BOOL ok = TabCtrl_SetItem(_owner.hwnd(), _index, &tci);
	#ifdef _DEBUG
	if (!ok)
		throw std::runtime_error{"TabCtrl_SetItem failed to set text."};
	#endif

	return *this;
}

//------------------------------------------------------------------------------

size_t Tab::ItemCollection::count() const {
	return TabCtrl_GetItemCount(_owner.hwnd());
}

Tab::Item* Tab::ItemCollection::focused() {
	int idxFound = TabCtrl_GetCurFocus(_owner.hwnd());
	if (idxFound == -1) return nullptr;
	_lastResult.~Item();
	::new (&_lastResult) Item{_owner, idxFound};
	return &_lastResult;
}

Tab::Item* Tab::ItemCollection::selected() {
	int idxFound = TabCtrl_GetCurSel(_owner.hwnd());
	if (idxFound == -1) return nullptr;
	_lastResult.~Item();
	::new (&_lastResult) Item{_owner, idxFound};
	return &_lastResult;
}

//------------------------------------------------------------------------------

Tab::Tab(IWindowParent &owner, TabOpts creationOpts) :
	_ctrl{owner.base()},
	_events{owner.base(), valid_ctrl_id(creationOpts.ctrlId)},
	_children{creationOpts.titles.size()}
{
	#ifdef _DEBUG
	if (creationOpts.titles.empty())
		throw std::invalid_argument{"Cannot create a Tab with zero items."};
	else if (creationOpts.titles.size() > 100) // arbitrary
		throw std::invalid_argument{"Cannot create a Tab with more than 100 items."};
	#endif

	for (size_t i = 0; i < creationOpts.titles.size(); ++i)
		new (&_children[i]) WindowControl{owner, ControlOpts{}}; // invoke constructor manually

	struct Ctx { Tab* self; IWindowParent* owner; TabOpts opts; };
	auto* c = new Ctx{this, &owner, std::move(creationOpts)};
	_ctrl._parent._preEvents.wm_create_or_init_dialog(MkFunc0(+[](Ctx* c) {
		if (c->opts.size == TabOpts{}.size)
			c->opts.size = dpi::sz(c->opts.size); // special case: default size

		c->self->_ctrl.create_wnd(c->self->ctrl_id(), c->opts.styleEx, WC_TABCONTROLW, {},
			c->opts.style, c->opts.pos, c->opts.size);
		apply_ui_font(c->self->hwnd());
		if (c->opts.styleExTab)
			c->self->set_extended_style(true, c->opts.styleExTab);
		c->self->_ctrl._parent._layout.add(c->self->hwnd(), c->opts.layout);
		for (auto &&title : c->opts.titles)
			c->self->create_tab(title);
		if (c->opts.selected)
			c->self->items[static_cast<int>(c->opts.selected)].select();
		c->self->display_cur_tab();
		delete c;
	}, c));

	custom_events();
}

Tab::Tab(IWindowParent &owner, WORD ctrlId, Lay layout,
	std::initializer_list<WORD> childrenDlgIds, std::initializer_list<WStrView> titles)
	: _ctrl{owner.base()}, _events{owner.base(), ctrlId}, _children{childrenDlgIds.size()}
{
	#ifdef _DEBUG
	if (childrenDlgIds.size() != titles.size())
		throw std::invalid_argument{"You must provide the same number of childrenDlgIds and titles."};
	else if (!childrenDlgIds.size())
		throw std::invalid_argument{"Cannot create a Tab with zero items."};
	else if (childrenDlgIds.size() > 100) // arbitrary
		throw std::invalid_argument{"Cannot create a Tab with more than 100 items."};
	#endif

	size_t i = 0;
	for (WORD dlgId : childrenDlgIds)
		new (&_children[i]) WindowControl{owner, dlgId}; // invoke constructor manually

	struct Ctx { Tab* self; Lay layout; std::initializer_list<WStrView> titles; };
	auto* c = new Ctx{this, layout, std::move(titles)};
	_ctrl._parent._preEvents.wm_create_or_init_dialog(MkFunc0(+[](Ctx* c) {
		c->self->_ctrl.assign_dlg(c->self->ctrl_id());
		apply_ui_font(c->self->hwnd());
		c->self->_ctrl._parent._layout.add(c->self->hwnd(), c->layout);
		for (auto &&title : c->titles)
			c->self->create_tab(title);
		c->self->display_cur_tab(); // 1st tab will be selected by default
		delete c;
	}, c));

	custom_events();
}

const Tab& Tab::set_extended_style(bool doSet, DWORD exStyle) const {
	SendMessageW(hwnd(), TCM_SETEXTENDEDSTYLE, exStyle, doSet ? exStyle : 0);
	return *this;
}

void Tab::create_tab(WStrView title) const {
	TCITEMW tci{
		.mask = TCIF_TEXT,
		.pszText = const_cast<wchar_t*>(title.c_str()),
	};

	int newIdx = TabCtrl_InsertItem(hwnd(), 0x0fff'ffff, &tci);
	#ifdef _DEBUG
	if (newIdx == -1)
		throw std::runtime_error{"TabCtrl_InsertItem failed."};
	#endif
}

void Tab::display_cur_tab() const {
	if (_children.empty()) return;

	int selIdx = TabCtrl_GetCurSel(hwnd());
	if (selIdx == -1) return;
	size_t index = static_cast<size_t>(selIdx);

	for (size_t i = 0; i < _children.size(); ++i) {
		if (i != index) ShowWindow(_children[i].hwnd(), SW_HIDE); // hide all others
	}

	RECT rcTab{};
	GetWindowRect(hwnd(), &rcTab);
	screen_to_client_rc(GetParent(hwnd()), &rcTab);
	TabCtrl_AdjustRect(hwnd(), FALSE, &rcTab);
	SetWindowPos(_children[index].hwnd(), nullptr,
		rcTab.left, rcTab.top, rcTab.right - rcTab.left, rcTab.bottom - rcTab.top,
		SWP_NOZORDER | SWP_SHOWWINDOW);
}

void Tab::custom_events() {
	_ctrl._parent._preEvents.wm_notify(ctrl_id(), TCN_SELCHANGE, MkFunc1(+[](Tab* self, wm::Notify&) {
		self->display_cur_tab();
	}, this));

	_ctrl._parent._preEvents.wm(WM_SIZE, MkFunc1(+[](Tab* self, wm::Msg&) {
		self->display_cur_tab();
	}, this));
}

////////////////////////////////////////////////////////////////////////////////

Trackbar::Trackbar(IWindowParent &owner, TrackbarOpts creationOpts)
	: _ctrl{owner.base()}, _events{owner.base(), valid_ctrl_id(creationOpts.ctrlId)}
{
	struct Ctx { Trackbar* self; IWindowParent* owner; TrackbarOpts opts; };
	auto* c = new Ctx{this, &owner, std::move(creationOpts)};
	_ctrl._parent._preEvents.wm_create_or_init_dialog(MkFunc0(+[](Ctx* c) {
		if (c->opts.size == TrackbarOpts{}.size)
			c->opts.size = dpi::sz(c->opts.size); // special case: default size

		c->self->_ctrl.create_wnd(c->self->ctrl_id(), c->opts.styleEx, TRACKBAR_CLASSW, {},
			c->opts.style, c->opts.pos, c->opts.size);
		c->self->_ctrl._parent._layout.add(c->self->hwnd(), c->opts.layout);
		if (c->opts.range != std::pair{0, 100}) c->self->set_range(c->opts.range);
		if (c->opts.pageSize) c->self->set_page_size(c->opts.pageSize);
		if (c->opts.value) c->self->set_pos(c->opts.value);
		delete c;
	}, c));
}

Trackbar::Trackbar(IWindowParent &owner, WORD ctrlId, Lay layout)
	: _ctrl{owner.base()}, _events{owner.base(), ctrlId}
{
	struct Ctx { Trackbar* self; Lay layout; };
	auto* c = new Ctx{this, layout};
	_ctrl._parent._preEvents.wm_create_or_init_dialog(MkFunc0(+[](Ctx* c) {
		c->self->_ctrl.assign_dlg(c->self->ctrl_id());
		c->self->_ctrl._parent._layout.add(c->self->hwnd(), c->layout);
		delete c;
	}, c));
}

int Trackbar::page_size() const {
	return static_cast<int>(SendMessageW(hwnd(), TBM_GETPAGESIZE, 0, 0));
}

const Trackbar& Trackbar::set_page_size(int pageSize) const {
	SendMessageW(hwnd(), TBM_SETPAGESIZE, 0, pageSize);
	return *this;
}

int Trackbar::pos() const {
	return static_cast<int>(SendMessageW(hwnd(), TBM_GETPOS, 0, 0));
}

const Trackbar& Trackbar::set_pos(int value) const {
	SendMessageW(hwnd(), TBM_SETPOS, TRUE, value);
	return *this;
}

std::pair<int, int> Trackbar::range() const {
	int rmin = static_cast<int>(SendMessageW(hwnd(), TBM_GETRANGEMIN, 0, 0));
	int rmax = static_cast<int>(SendMessageW(hwnd(), TBM_GETRANGEMAX, 0, 0));
	return {rmin, rmax};
}

const Trackbar& Trackbar::set_range(int rangeMin, int rangeMax) const {
	SendMessageW(hwnd(), TBM_SETRANGEMIN, TRUE, rangeMin);
	SendMessageW(hwnd(), TBM_SETRANGEMAX, TRUE, rangeMax);
	return *this;
}

const Trackbar& Trackbar::set_range(std::pair<int, int> rangeMinMax) const {
	return set_range(rangeMinMax.first, rangeMinMax.second);
}

////////////////////////////////////////////////////////////////////////////////

TreeView::Item TreeView::ChildCollection::operator[](size_t index) const {
	HTREEITEM hChild = TreeView_GetChild(_pOwner->hwnd(), _hItem);
	for (size_t i = 0; i < index; ++i) {
		hChild = TreeView_GetNextSibling(_pOwner->hwnd(), hChild);
		if (!hChild) break;
	}
	return Item{*_pOwner, hChild};
}

TreeView::Item TreeView::ChildCollection::add(WStrView text, int iconIndex) const {
	TVINSERTSTRUCTW tvi{
		.hParent = _hItem,
		.hInsertAfter = TVI_LAST,
		.itemex = {
			.mask = TVIF_TEXT | static_cast<UINT>(iconIndex > -1 ? TVIF_IMAGE : 0),
			.pszText = const_cast<LPWSTR>(text.c_str()),
			.iImage = iconIndex,
		},
	};

	HTREEITEM hItemNew = TreeView_InsertItem(_pOwner->hwnd(), &tvi);
	#ifdef _DEBUG
	if (!hItemNew)
		throw std::runtime_error{"TreeView_InsertItem failed."};
	#endif

	return Item{*_pOwner, hItemNew};
}

size_t TreeView::ChildCollection::count() const {
	HTREEITEM hChild = TreeView_GetChild(_pOwner->hwnd(), _hItem);
	size_t n = 0;
	while (hChild) {
		++n;
		hChild = TreeView_GetNextSibling(_pOwner->hwnd(), hChild);
	}
	return n;
}

//------------------------------------------------------------------------------

const TreeView::Item& TreeView::Item::ensure_visible() const {
	TreeView_EnsureVisible(children._pOwner->hwnd(), children._hItem);
	return *this;
}

bool TreeView::Item::is_expanded() const {
	return TreeView_GetItemState(children._pOwner->hwnd(), children._hItem, TVIS_EXPANDED) & TVIS_EXPANDED;
}

const TreeView::Item& TreeView::Item::expand(bool doExpand) const {
	TreeView_Expand(children._pOwner->hwnd(), children._hItem, doExpand ? TVE_EXPAND : TVE_COLLAPSE);
	return *this;
}

int TreeView::Item::icon_index() const {
	#ifdef _DEBUG
	if (!children._pOwner->_imgList16.himagelist()
		|| !children._pOwner->_imgList16.count())
		throw std::logic_error{"No icons have been added to any image list."};
	#endif

	TVITEMEXW tvi{
		.mask = TVIF_IMAGE,
		.hItem = children._hItem,
	};

	BOOL ok = TreeView_GetItem(children._pOwner->hwnd(), &tvi);
	#ifdef _DEBUG
	if (!ok)
		throw std::runtime_error{"TreeView_GetItem failed to get icon."};
	#endif

	return tvi.iImage;
}

const TreeView::Item& TreeView::Item::set_icon_index(int iconIndex) const {
	#ifdef _DEBUG
	if (!children._pOwner->_imgList16.himagelist()
		|| !children._pOwner->_imgList16.count())
		throw std::logic_error{"No icons have been added to any image list."};
	#endif

	TVITEMEXW tvi{
		.mask = TVIF_IMAGE,
		.hItem = children._hItem,
		.iImage = iconIndex,
	};

	BOOL ok = TreeView_SetItem(children._pOwner->hwnd(), &tvi);
	#ifdef _DEBUG
	if (!ok)
		throw std::runtime_error{"TreeView_SetItem failed to set icon."};
	#endif

	return *this;
}

size_t TreeView::Item::index() const {
	HTREEITEM hItem = children._hItem;
	size_t i = 0;
	for (;;) {
		hItem = TreeView_GetPrevSibling(children._pOwner->hwnd(), hItem);
		if (!hItem) break;
		++i;
	}
	return i;
}

TreeView::Item TreeView::Item::next_sibling() const {
	HTREEITEM hItem = TreeView_GetNextSibling(children._pOwner->hwnd(), children._hItem);
	return Item{*children._pOwner, hItem};
}

TreeView::Item TreeView::Item::parent() const {
	HTREEITEM hItem = TreeView_GetNextItem(children._pOwner->hwnd(), children._hItem, TVGN_PARENT);
	return Item{*children._pOwner, hItem};
}

TreeView::Item TreeView::Item::prev_sibling() const {
	HTREEITEM hItem = TreeView_GetPrevSibling(children._pOwner->hwnd(), children._hItem);
	return Item{*children._pOwner, hItem};
}

const TreeView::Item& TreeView::Item::remove() const {
	TreeView_DeleteItem(children._pOwner->hwnd(), children._hItem);
	return *this;
}

std::wstring TreeView::Item::text() const {
	std::wstring buf(128, L'\0'); // arbitrary

	TVITEMEXW tvi{
		.mask = TVIF_TEXT,
		.hItem = children._hItem,
		.pszText = buf.data(),
		.cchTextMax = static_cast<int>(buf.size()),
	};

	BOOL ok = TreeView_GetItem(children._pOwner->hwnd(), &tvi);
	#ifdef _DEBUG
	if (!ok)
		throw std::runtime_error{"TreeView_GetItem failed to get text."};
	#endif

	str::trim_nulls(buf);
	return buf;
}

const TreeView::Item& TreeView::Item::set_text(WStrView newText) const {
	TVITEMEXW tvi{
		.mask = TVIF_TEXT,
		.hItem = children._hItem,
		.pszText = const_cast<LPWSTR>(newText.c_str()),
	};

	BOOL ok = TreeView_SetItem(children._pOwner->hwnd(), &tvi);
	#ifdef _DEBUG
	if (!ok)
		throw std::runtime_error{"TreeView_SetItem failed to set text."};
	#endif

	return *this;
}

LPARAM TreeView::Item::raw_data() const {
	TVITEMEXW tvi{
		.mask = TVIF_PARAM,
		.hItem = children._hItem,
	};

	BOOL ok = TreeView_GetItem(children._pOwner->hwnd(), &tvi);
	#ifdef _DEBUG
	if (!ok)
		throw std::runtime_error{"TreeView_SetItem failed to get LPARAM."};
	#endif

	return tvi.lParam;
}

const TreeView::Item& TreeView::Item::set_raw_data(LPARAM data) const {
	TVITEMEXW tvi{
		.mask = TVIF_PARAM,
		.hItem = children._hItem,
		.lParam = data,
	};

	BOOL ok = TreeView_SetItem(children._pOwner->hwnd(), &tvi);
	#ifdef _DEBUG
	if (!ok)
		throw std::runtime_error{"TreeView_SetItem failed to set LPARAM."};
	#endif

	return *this;
}

//------------------------------------------------------------------------------

size_t TreeView::ItemCollection::count() const {
	return TreeView_GetCount(_owner.hwnd());
}

TreeView::Item TreeView::ItemCollection::first_visible() const {
	HTREEITEM hItem = TreeView_GetFirstVisible(_owner.hwnd());
	return Item{_owner, hItem};
}

TreeView::Item TreeView::ItemCollection::last_visible() const {
	HTREEITEM hItem = TreeView_GetLastVisible(_owner.hwnd());
	return Item{_owner, hItem};
}

TreeView::Item TreeView::ItemCollection::selected() const {
	HTREEITEM hItem = TreeView_GetSelection(_owner.hwnd());
	return Item{_owner, hItem};
}

//------------------------------------------------------------------------------

TreeView::TreeView(IWindowParent &owner, TreeViewOpts creationOpts)
	: _ctrl{owner.base()}, _events{owner.base(), valid_ctrl_id(creationOpts.ctrlId)}
{
	struct Ctx { TreeView* self; IWindowParent* owner; TreeViewOpts opts; };
	auto* c = new Ctx{this, &owner, std::move(creationOpts)};
	_ctrl._parent._preEvents.wm_create_or_init_dialog(MkFunc0(+[](Ctx* c) {
		if (c->opts.size == TreeViewOpts{}.size)
			c->opts.size = dpi::sz(c->opts.size); // special case: default size

		c->self->_ctrl.create_wnd(c->self->ctrl_id(), c->opts.styleEx, WC_TREEVIEWW, {},
			c->opts.style, c->opts.pos, c->opts.size);
		if (c->opts.styleExTreeView)
			c->self->set_extended_style(true, c->opts.styleExTreeView);
		c->self->_ctrl._parent._layout.add(c->self->hwnd(), c->opts.layout);

		for (auto &&ico : c->opts.icons) {
			if (ico.id) c->self->icons().add_resource(ico.id);
			else        c->self->icons().add_shell_ext(ico.ext);
		}
		delete c;
	}, c));
}

TreeView::TreeView(IWindowParent &owner, WORD ctrlId, Lay layout)
	: _ctrl{owner.base()}, _events{owner.base(), ctrlId}
{
	struct Ctx { TreeView* self; Lay layout; };
	auto* c = new Ctx{this, layout};
	_ctrl._parent._preEvents.wm_create_or_init_dialog(MkFunc0(+[](Ctx* c) {
		c->self->_ctrl.assign_dlg(c->self->ctrl_id());
		c->self->_ctrl._parent._layout.add(c->self->hwnd(), c->layout);
		delete c;
	}, c));
}

const TreeView& TreeView::set_extended_style(bool doSet, DWORD exStyle) const {
	HRESULT hr = TreeView_SetExtendedStyle(hwnd(), doSet ? exStyle : 0, exStyle);
	#ifdef _DEBUG
	if (FAILED(hr))
		throw std::system_error(GetLastError(), std::system_category(), "TreeView_SetExtendedStyle failed");
	#endif
	return *this;
}

IStoreIcon& TreeView::icons() {
	if (!_imgList16.himagelist()) { // not created yet?
		_imgList16.create();
		TreeView_SetImageList(hwnd(), _imgList16.himagelist(), TVSIL_NORMAL);
	}
	return _imgList16;
}
