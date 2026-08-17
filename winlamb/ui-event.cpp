#include <system_error>
#include "ui-event.hpp"
using namespace _wl_internal;
using namespace wl;
using namespace wl::events;

void InternalEvents::wm_create_or_init_dialog(Func0 cb) {
	_inis.emplace_back(cb);
}

void InternalEvents::wm(UINT msg, Func1<wl::wm::Msg&> cb) {
	#ifdef _DEBUG
	if (msg == WM_CREATE || msg == WM_INITDIALOG || msg == WM_NOTIFY)
		throw std::logic_error{"For WM_CREATE, WM_INITDIALOG, WM_NOTIFY, use the specific event methods."};
	#endif
	_msgs.emplace_back(msg, cb);
}

void InternalEvents::wm_notify(WORD idFrom, int code, Func1<wl::wm::Notify&> cb) {
	_nfys.emplace_back(idFrom, code, cb);
}

void InternalEvents::clear_inis() {
	std::vector<Func0>{}.swap(_inis); // https://stackoverflow.com/a/13944912/6923555
}

void InternalEvents::clear() {
	clear_inis();
	std::vector<Msg>{}.swap(_msgs);
	std::vector<Nfy>{}.swap(_nfys);
}

bool InternalEvents::process_all(wm::Msg procMsg) const {
	bool atLeastOne = false;

	switch (procMsg.wm) {
		case WM_CREATE:
		case WM_INITDIALOG:
			for (auto &&ini : _inis) {
				ini.Call();
				atLeastOne = true;
			}
			break;

		case WM_NOTIFY: {
			NMHDR *pHdr = reinterpret_cast<NMHDR*>(procMsg.lp);
			for (auto &&nfy : _nfys) {
				if (nfy.idFrom == pHdr->idFrom && nfy.code == pHdr->code) {
					wm::Notify n{procMsg};
					nfy.cb.Call(n);
					atLeastOne = true;
				}
			}
			break;
		}

		default: { // finally, ordinary messages
			for (auto &&msg : _msgs) {
				if (msg.wm == procMsg.wm) {
					msg.cb.Call(procMsg);
					atLeastOne = true;
				}
			}
		}
	}

	return atLeastOne;
}

////////////////////////////////////////////////////////////////////////////////

void WindowEvents::wm(UINT msg, Func1<wm::Msg&> cb) {
	#ifdef _DEBUG
	if (msg == WM_CREATE || msg == WM_INITDIALOG || msg == WM_COMMAND || msg == WM_NOTIFY)
		throw std::logic_error{"For WM_CREATE, WM_INITDIALOG, WM_COMMAND or WM_NOTIFY, use the specific event methods."};
	#endif
	_msgs.emplace_back(msg, cb);
}

void WindowEvents::wm_create(Func1<wm::Create&> cb) {
	_inis.emplace_back(WM_CREATE, reinterpret_cast<Func1<wm::Msg&>&>(cb));
}

void WindowEvents::wm_init_dialog(Func1<wm::InitDialog&> cb) {
	_inis.emplace_back(WM_INITDIALOG, reinterpret_cast<Func1<wm::Msg&>&>(cb));
}

void WindowEvents::wm_command(WORD cmdId, WORD notifCode, Func0 cb) {
	_cmds.emplace_back(cmdId, notifCode, cb);
}

void WindowEvents::wm_command(WORD cmdId, Func0 cb) {
	_cmds.emplace_back(cmdId, 0, cb); // menu
	_cmds.emplace_back(cmdId, 1, cb); // accelerator
}

void WindowEvents::wm_notify(WORD idFrom, int code, Func1<wm::Notify&> cb) {
	_nfys.emplace_back(idFrom, code, cb);
}

void WindowEvents::wm_activate(Func1<wm::Activate&> cb) {
	wm(WM_ACTIVATE, reinterpret_cast<Func1<wm::Msg&>&>(cb));
}
void WindowEvents::wm_activate_app(Func1<wm::ActivateApp&> cb) {
	wm(WM_ACTIVATEAPP, reinterpret_cast<Func1<wm::Msg&>&>(cb));
}
void WindowEvents::wm_child_activate(Func0 cb) {
	wm(WM_CHILDACTIVATE, WrapMsg(cb));
}
void WindowEvents::wm_close(Func0 cb) {
	wm(WM_CLOSE, WrapMsg(cb));
}
void WindowEvents::wm_display_change(Func1<wm::DisplayChange&> cb) {
	wm(WM_DISPLAYCHANGE, reinterpret_cast<Func1<wm::Msg&>&>(cb));
}
void WindowEvents::wm_destroy(Func0 cb) {
	wm(WM_DESTROY, WrapMsg(cb));
}
void WindowEvents::wm_enable(Func1<wm::Enable&> cb) {
	wm(WM_ENABLE, reinterpret_cast<Func1<wm::Msg&>&>(cb));
}
void WindowEvents::wm_end_session(Func1<wm::EndSession&> cb) {
	wm(WM_ENDSESSION, reinterpret_cast<Func1<wm::Msg&>&>(cb));
}
void WindowEvents::wm_enter_size_move(Func0 cb) {
	wm(WM_ENTERSIZEMOVE, WrapMsg(cb));
}
void WindowEvents::wm_erase_bkgnd(Func1<wm::EraseBkgnd&> cb) {
	wm(WM_ERASEBKGND, reinterpret_cast<Func1<wm::Msg&>&>(cb));
}
void WindowEvents::wm_exit_size_move(Func0 cb) {
	wm(WM_EXITSIZEMOVE, WrapMsg(cb));
}
void WindowEvents::wm_get_dlg_code(Func1<wm::GetDlgCode&> cb) {
	wm(WM_GETDLGCODE, reinterpret_cast<Func1<wm::Msg&>&>(cb));
}
void WindowEvents::wm_h_scroll(Func1<wm::HScroll&> cb) {
	wm(WM_HSCROLL, reinterpret_cast<Func1<wm::Msg&>&>(cb));
}
void WindowEvents::wm_init_menu_popup(Func1<wm::InitMenuPopup&> cb) {
	wm(WM_INITMENUPOPUP, reinterpret_cast<Func1<wm::Msg&>&>(cb));
}
void WindowEvents::wm_kill_focus(Func1<wm::KillFocus&> cb) {
	wm(WM_KILLFOCUS, reinterpret_cast<Func1<wm::Msg&>&>(cb));
}
void WindowEvents::wm_l_button_dbl_clk(Func1<wm::LButtonDblClk&> cb) {
	wm(WM_LBUTTONDBLCLK, reinterpret_cast<Func1<wm::Msg&>&>(cb));
}
void WindowEvents::wm_l_button_down(Func1<wm::LButtonDown&> cb) {
	wm(WM_LBUTTONDOWN, reinterpret_cast<Func1<wm::Msg&>&>(cb));
}
void WindowEvents::wm_l_button_up(Func1<wm::LButtonUp&> cb) {
	wm(WM_LBUTTONUP, reinterpret_cast<Func1<wm::Msg&>&>(cb));
}
void WindowEvents::wm_m_button_dbl_clk(Func1<wm::MButtonDblClk&> cb) {
	wm(WM_MBUTTONDBLCLK, reinterpret_cast<Func1<wm::Msg&>&>(cb));
}
void WindowEvents::wm_m_button_down(Func1<wm::MButtonDown&> cb) {
	wm(WM_MBUTTONDOWN, reinterpret_cast<Func1<wm::Msg&>&>(cb));
}
void WindowEvents::wm_m_button_up(Func1<wm::MButtonUp&> cb) {
	wm(WM_MBUTTONUP, reinterpret_cast<Func1<wm::Msg&>&>(cb));
}
void WindowEvents::wm_mouse_hover(Func1<wm::MouseHover&> cb) {
	wm(WM_MOUSEHOVER, reinterpret_cast<Func1<wm::Msg&>&>(cb));
}
void WindowEvents::wm_mouse_move(Func1<wm::MouseMove&> cb) {
	wm(WM_MOUSEMOVE, reinterpret_cast<Func1<wm::Msg&>&>(cb));
}
void WindowEvents::wm_move(Func1<wm::Move&> cb) {
	wm(WM_MOVE, reinterpret_cast<Func1<wm::Msg&>&>(cb));
}
void WindowEvents::wm_moving(Func1<wm::Moving&> cb) {
	wm(WM_MOVING, reinterpret_cast<Func1<wm::Msg&>&>(cb));
}
void WindowEvents::wm_nc_calc_size(Func1<wm::NcCalcSize&> cb) {
	wm(WM_NCCALCSIZE, reinterpret_cast<Func1<wm::Msg&>&>(cb));
}
void WindowEvents::wm_nc_destroy(Func0 cb) {
	wm(WM_NCDESTROY, WrapMsg(cb));
}
void WindowEvents::wm_nc_paint(Func1<wm::NcPaint&> cb) {
	wm(WM_NCPAINT, reinterpret_cast<Func1<wm::Msg&>&>(cb));
}
void WindowEvents::wm_paint(Func0 cb) {
	wm(WM_PAINT, WrapMsg(cb));
}
void WindowEvents::wm_power_broadcast(Func1<wm::PowerBroadcast&> cb) {
	wm(WM_POWERBROADCAST, reinterpret_cast<Func1<wm::Msg&>&>(cb));
}
void WindowEvents::wm_query_open(Func0 cb) {
	wm(WM_QUERYOPEN, WrapMsg(cb));
}
void WindowEvents::wm_r_button_dbl_clk(Func1<wm::RButtonDblClk&> cb) {
	wm(WM_RBUTTONDBLCLK, reinterpret_cast<Func1<wm::Msg&>&>(cb));
}
void WindowEvents::wm_r_button_down(Func1<wm::RButtonDown&> cb) {
	wm(WM_RBUTTONDOWN, reinterpret_cast<Func1<wm::Msg&>&>(cb));
}
void WindowEvents::wm_r_button_up(Func1<wm::RButtonUp&> cb) {
	wm(WM_RBUTTONUP, reinterpret_cast<Func1<wm::Msg&>&>(cb));
}
void WindowEvents::wm_set_cursor(Func1<wm::SetCursor&> cb) {
	wm(WM_SETCURSOR, reinterpret_cast<Func1<wm::Msg&>&>(cb));
}
void WindowEvents::wm_set_focus(Func1<wm::SetFocus&> cb) {
	wm(WM_SETFOCUS, reinterpret_cast<Func1<wm::Msg&>&>(cb));
}
void WindowEvents::wm_show_window(Func1<wm::ShowWindow&> cb) {
	wm(WM_SHOWWINDOW, reinterpret_cast<Func1<wm::Msg&>&>(cb));
}
void WindowEvents::wm_size(Func1<wm::Size&> cb) {
	wm(WM_SIZE, reinterpret_cast<Func1<wm::Msg&>&>(cb));
}
void WindowEvents::wm_sizing(Func1<wm::Sizing&> cb) {
	wm(WM_SIZING, reinterpret_cast<Func1<wm::Msg&>&>(cb));
}
void WindowEvents::wm_theme_changed(Func0 cb) {
	wm(WM_THEMECHANGED, WrapMsg(cb));
}
void WindowEvents::wm_time_change(Func0 cb) {
	wm(WM_TIMECHANGE, WrapMsg(cb));
}
void WindowEvents::wm_v_scroll(Func1<wm::VScroll&> cb) {
	wm(WM_VSCROLL, reinterpret_cast<Func1<wm::Msg&>&>(cb));
}
void WindowEvents::wm_window_pos_changed(Func1<wm::WindowPosChanged&> cb) {
	wm(WM_WINDOWPOSCHANGED, reinterpret_cast<Func1<wm::Msg&>&>(cb));
}
void WindowEvents::wm_window_pos_changing(Func1<wm::WindowPosChanging&> cb) {
	wm(WM_WINDOWPOSCHANGING, reinterpret_cast<Func1<wm::Msg&>&>(cb));
}

bool WindowEvents::has_message() const {
	return !_inis.empty() || !_msgs.empty() || !_cmds.empty() || !_nfys.empty();
}

void WindowEvents::clear_inis() {
	std::vector<Msg>{}.swap(_inis); // https://stackoverflow.com/a/13944912/6923555
}

void WindowEvents::clear() {
	clear_inis();
	std::vector<Msg>{}.swap(_msgs);
	std::vector<Cmd>{}.swap(_cmds);
	std::vector<Nfy>{}.swap(_nfys);
}

void WindowEvents::process_last(wm::Msg &procMsg) const {
	// We process the last added message because the library adds some events
	// which can be overwritten by the user.

	switch (procMsg.wm) {
		case WM_CREATE:
		case WM_INITDIALOG:
			for (auto it = _inis.rbegin(); it != _inis.rend(); ++it) {
				if (it->wm == procMsg.wm) {
					it->cb.Call(procMsg);
					procMsg.didHandle = true;
					return;
				}
			}
			break;

		case WM_COMMAND: {
			wm::Command msgCmd{procMsg};
			for (auto it = _cmds.rbegin(); it != _cmds.rend(); ++it) {
				if (it->cmdId == msgCmd.control_id() && it->notifCode == msgCmd.control_notif_code()) {
					it->cb.Call();
					procMsg.didHandle = true;
					procMsg.ret = _isDlg ? TRUE : 0;
					return;
				}
			}
			break;
		}

		case WM_NOTIFY: {
			NMHDR *pHdr = reinterpret_cast<NMHDR*>(procMsg.lp);
			for (auto it = _nfys.rbegin(); it != _nfys.rend(); ++it) {
				if (it->idFrom == pHdr->idFrom && it->code == pHdr->code) {
					wm::Notify n{procMsg};
					n.ret = _isDlg ? TRUE : 0;
					it->cb.Call(n);
					procMsg.ret = n.ret;
					procMsg.didHandle = true;
					return;
				}
			}
			break;
		}

		default: { // finally, ordinary messages
			for (auto it = _msgs.rbegin(); it != _msgs.rend(); ++it) {
				if (it->wm == procMsg.wm) {
					procMsg.ret = _isDlg ? TRUE : 0;
					it->cb.Call(procMsg);
					procMsg.didHandle = true;
					return;
				}
			}
		}
	}
}
