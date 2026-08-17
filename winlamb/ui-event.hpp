#pragma once
#include <functional>
#include "ui-app.hpp"

/// @brief [Message] crackers, passed as arguments to the window events.
///
/// [Message]: https://learn.microsoft.com/en-us/windows/win32/learnwin32/window-messages
namespace wl::wm {

	/// @brief Raw data of an ordinary window message.
	///
	/// Convertible to the specific window message structs.
	struct Msg {
		UINT wm;
		WPARAM wp;
		LPARAM lp;
		bool didHandle = false;
		LRESULT ret = 0;
	};

	struct Activate : public Msg {
		constexpr Activate(const Msg &p) : Msg{p} { }
		[[nodiscard]] constexpr WORD active_state() const   { return LOWORD(wp); }
		[[nodiscard]] constexpr bool is_minimized() const   { return HIWORD(wp) != 0; }
		[[nodiscard]] HWND           swapped_window() const { return reinterpret_cast<HWND>(lp); }
	};

	struct ActivateApp : public Msg {
		constexpr ActivateApp(const Msg &p) : Msg{p} { }
		[[nodiscard]] constexpr bool  is_being_discarded() const { return wp != 0; }
		[[nodiscard]] constexpr DWORD thread_id() const          { return static_cast<DWORD>(lp); }
	};

	struct Command : public Msg {
		constexpr Command(const Msg &p) : Msg{p} { }
		[[nodiscard]] constexpr bool is_from_menu() const        { return HIWORD(wp) == 0; }
		[[nodiscard]] constexpr bool is_from_accelerator() const { return HIWORD(wp) == 1; }
		[[nodiscard]] constexpr bool is_from_control() const     { return !is_from_menu() && !is_from_accelerator(); }
		[[nodiscard]] constexpr WORD menu_id() const             { return control_id(); }
		[[nodiscard]] constexpr WORD accelerator_id() const      { return control_id(); }
		[[nodiscard]] constexpr WORD control_id() const          { return LOWORD(wp); }
		[[nodiscard]] constexpr WORD control_notif_code() const  { return HIWORD(wp); }
		[[nodiscard]] HWND           control_hwnd() const        { return reinterpret_cast<HWND>(lp); }
	};

	struct Create : public Msg {
		constexpr Create(const Msg &p) : Msg{p} { }
		[[nodiscard]] const CREATESTRUCTW& crate_struct() const { return *reinterpret_cast<const CREATESTRUCTW*>(lp); }
	};

	struct DisplayChange : public Msg {
		constexpr DisplayChange(const Msg &p) : Msg{p} { }
		[[nodiscard]] constexpr int bits_per_pixel() const { return static_cast<int>(wp); }
		[[nodiscard]] constexpr SIZE sz() const            { return {.cx = LOWORD(lp), .cy = HIWORD(lp)}; }
	};

	struct Enable : public Msg {
		constexpr Enable(const Msg &p) : Msg{p} { }
		[[nodiscard]] constexpr bool was_enabled() const { return wp != FALSE; }
	};

	struct EndSession : public Msg {
		constexpr EndSession(const Msg &p) : Msg{p} { }
		[[nodiscard]] constexpr DWORD endsession_flag() const        { return static_cast<DWORD>(lp); }
		[[nodiscard]] constexpr bool  is_session_being_ended() const { return wp != FALSE; }
		[[nodiscard]] constexpr bool  is_system_issue() const        { return (endsession_flag() & ENDSESSION_CLOSEAPP) != 0; }
		[[nodiscard]] constexpr bool  is_forced_critical() const     { return (endsession_flag() & ENDSESSION_CRITICAL) != 0; }
		[[nodiscard]] constexpr bool  is_logoff() const              { return (endsession_flag() & ENDSESSION_LOGOFF) != 0; }
		[[nodiscard]] constexpr bool  is_shutdown_restart() const    { return lp == 0; }
	};

	struct EraseBkgnd : public Msg {
		constexpr EraseBkgnd(const Msg &p) : Msg{p} { }
		[[nodiscard]] HDC hdc() const { return reinterpret_cast<HDC>(wp); }
	};

	struct GetDlgCode : public Msg {
		constexpr GetDlgCode(const Msg &p) : Msg{p} { }
		[[nodiscard]] constexpr WPARAM wparam() const { return wp; }
		[[nodiscard]] constexpr LPARAM lparam() const { return lp; }
		[[nodiscard]] constexpr BYTE   vkey_code() const { return static_cast<BYTE>(wp); }
		[[nodiscard]] constexpr bool   is_query() const  { return lp == 0; }
		[[nodiscard]] MSG*             msg() const       { return this->is_query() ? nullptr : reinterpret_cast<MSG*>(lp); }
		[[nodiscard]] bool             has_alt() const   { return (GetAsyncKeyState(VK_MENU) & 0x8000) != 0; }
		[[nodiscard]] bool             has_ctrl() const  { return (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0; }
		[[nodiscard]] bool             has_shift() const { return (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0; }
	};

	struct HScroll : public Msg {
		constexpr HScroll(const Msg &p) : Msg{p} { }
		[[nodiscard]] constexpr WORD scroll_request() const { return LOWORD(wp); }
		[[nodiscard]] constexpr WORD scroll_pos() const     { return HIWORD(wp); }
		[[nodiscard]] HWND           hwnd_scrollbar() const { return reinterpret_cast<HWND>(lp); }
	};
	struct VScroll : public HScroll { constexpr VScroll(const Msg &p) : HScroll{p} { } };

	struct InitDialog : public Msg {
		constexpr InitDialog(const Msg &p) : Msg{p} { }
		[[nodiscard]] HWND hwnd_focus() const { return reinterpret_cast<HWND>(wp); }
	};

	struct InitMenuPopup : public Msg {
		constexpr InitMenuPopup(const Msg &p) : Msg{p} { }
		[[nodiscard]] HMENU           hmenu() const              { return reinterpret_cast<HMENU>(wp); }
		[[nodiscard]] constexpr short relative_pos() const       { return LOWORD(lp); }
		[[nodiscard]] constexpr bool  is_window_menu() const     { return HIWORD(lp) != FALSE; }
		[[nodiscard]] UINT            first_menu_item_id() const { return GetMenuItemID(hmenu(), 0); }
	};

	struct KillFocus : public Msg {
		constexpr KillFocus(const Msg &p) : Msg{p} { }
		[[nodiscard]] HWND focused_hwnd() const { return reinterpret_cast<HWND>(wp); }
	};

	struct LButtonDblClk : public Msg {
		constexpr LButtonDblClk(const Msg &p) : Msg{p} { }
		[[nodiscard]] constexpr WORD  mk_flag() const       { return static_cast<WORD>(wp); }
		[[nodiscard]] constexpr bool  has_ctrl() const      { return (mk_flag() & MK_CONTROL) != 0; }
		[[nodiscard]] constexpr bool  has_shift() const     { return (mk_flag() & MK_SHIFT) != 0; }
		[[nodiscard]] constexpr bool  is_left_btn() const   { return (mk_flag() & MK_LBUTTON) != 0; }
		[[nodiscard]] constexpr bool  is_middle_btn() const { return (mk_flag() & MK_MBUTTON) != 0; }
		[[nodiscard]] constexpr bool  is_right_btn() const  { return (mk_flag() & MK_RBUTTON) != 0; }
		[[nodiscard]] constexpr bool  is_x1_btn() const     { return (mk_flag() & MK_XBUTTON1) != 0; }
		[[nodiscard]] constexpr bool  is_x2_btn() const     { return (mk_flag() & MK_XBUTTON2) != 0; }
		[[nodiscard]] constexpr POINT pos() const           { return {.x = LOWORD(lp), .y = HIWORD(lp)}; }
	};
	struct LButtonDown   : public LButtonDblClk { constexpr LButtonDown(const Msg &p)   : LButtonDblClk{p} { } };
	struct LButtonUp     : public LButtonDblClk { constexpr LButtonUp(const Msg &p)     : LButtonDblClk{p} { } };
	struct MButtonDblClk : public LButtonDblClk { constexpr MButtonDblClk(const Msg &p) : LButtonDblClk{p} { } };
	struct MButtonDown   : public LButtonDblClk { constexpr MButtonDown(const Msg &p)   : LButtonDblClk{p} { } };
	struct MButtonUp     : public LButtonDblClk { constexpr MButtonUp(const Msg &p)     : LButtonDblClk{p} { } };
	struct MouseHover    : public LButtonDblClk { constexpr MouseHover(const Msg &p)    : LButtonDblClk{p} { } };
	struct MouseMove     : public LButtonDblClk { constexpr MouseMove(const Msg &p)     : LButtonDblClk{p} { } };
	struct RButtonDblClk : public LButtonDblClk { constexpr RButtonDblClk(const Msg &p) : LButtonDblClk{p} { } };
	struct RButtonDown   : public LButtonDblClk { constexpr RButtonDown(const Msg &p)   : LButtonDblClk{p} { } };
	struct RButtonUp     : public LButtonDblClk { constexpr RButtonUp(const Msg &p)     : LButtonDblClk{p} { } };

	struct Move : public Msg {
		constexpr Move(const Msg &p) : Msg{p} { }
		[[nodiscard]] constexpr POINT client_area_pos() const { return {.x = LOWORD(lp), .y = HIWORD(lp)}; }
	};

	struct Moving : public Msg {
		constexpr Moving(const Msg &p) : Msg{p} { }
		[[nodiscard]] RECT& window_pos() const { return *reinterpret_cast<RECT*>(lp); }
	};

	struct NcCalcSize : public Msg {
		constexpr NcCalcSize(const Msg &p) : Msg{p} { }
		[[nodiscard]] constexpr bool     should_indicate_valid_part() const { return wp != 0; }
		[[nodiscard]] NCCALCSIZE_PARAMS& nccalcsize_params() const          { return *reinterpret_cast<NCCALCSIZE_PARAMS*>(lp); }
		[[nodiscard]] RECT&              rect() const                       { return *reinterpret_cast<RECT*>(lp); }
	};

	struct NcPaint : public Msg {
		constexpr NcPaint(const Msg &p) : Msg{p} { }
		[[nodiscard]] constexpr WPARAM wparam() const { return wp; }
		[[nodiscard]] constexpr LPARAM lparam() const { return lp; }
		[[nodiscard]] HRGN      hrgn() const          { return reinterpret_cast<HRGN>(wp); }
	};

	struct Notify : public Msg {
		constexpr Notify(const Msg &p) : Msg{p} { }
		template<typename T> [[nodiscard]] T& hdr() { return *reinterpret_cast<T*>(lp); }
	};

	struct PowerBroadcast : public Msg {
		constexpr PowerBroadcast(const Msg &p) : Msg{p} { }
		[[nodiscard]] constexpr WORD                pbt_flag() const   { return static_cast<WORD>(wp); }
		[[nodiscard]] const POWERBROADCAST_SETTING& event_data() const { return *reinterpret_cast<const POWERBROADCAST_SETTING*>(lp); }
	};

	struct SetCursor : public Msg {
		constexpr SetCursor(const Msg &p) : Msg{p} { }
		[[nodiscard]] HWND cursor_hwnd() const              { return reinterpret_cast<HWND>(wp); }
		[[nodiscard]] constexpr short hit_test_code() const { return static_cast<short>(LOWORD(lp)); }
		[[nodiscard]] constexpr WORD  mouse_msg_id() const  { return HIWORD(lp); }
	};

	struct SetFocus : public Msg {
		constexpr SetFocus(const Msg &p) : Msg{p} { }
		[[nodiscard]] HWND hwnd_losing_focus() const { return reinterpret_cast<HWND>(wp); }
	};

	struct ShowWindow : public Msg {
		constexpr ShowWindow(const Msg &p) : Msg{p} { }
		[[nodiscard]] constexpr bool is_shown() const { return wp == TRUE; }
		[[nodiscard]] constexpr WORD status() const   { return static_cast<WORD>(lp); }
	};

	struct Size : public Msg {
		constexpr Size(const Msg &p) : Msg{p} { }
		[[nodiscard]] constexpr WORD size_flag() const { return static_cast<WORD>(wp); }
		[[nodiscard]] constexpr bool is_other_maximized() const { return size_flag() == SIZE_MAXHIDE; }
		[[nodiscard]] constexpr bool is_maximized() const       { return size_flag() == SIZE_MAXIMIZED; }
		[[nodiscard]] constexpr bool is_other_restored() const  { return size_flag() == SIZE_MAXSHOW; }
		[[nodiscard]] constexpr bool is_minimized() const       { return size_flag() == SIZE_MINIMIZED; }
		[[nodiscard]] constexpr bool is_restored() const        { return size_flag() == SIZE_RESTORED; }
		[[nodiscard]] constexpr SIZE sz() const                 { return {.cx = LOWORD(lp), .cy = HIWORD(lp)}; }
	};

	struct Sizing : public Msg {
		constexpr Sizing(const Msg &p) : Msg{p} { }
		[[nodiscard]] constexpr WORD edge() const          { return static_cast<WORD>(wp); }
		[[nodiscard]] RECT&          screen_coords() const { return *reinterpret_cast<RECT*>(lp); }
	};

	struct WindowPosChanged : public Msg {
		constexpr WindowPosChanged(const Msg &p) : Msg{p} { }
		[[nodiscard]] const WINDOWPOS& info() const { return *reinterpret_cast<const WINDOWPOS*>(lp); }
	};

	struct WindowPosChanging : public Msg {
		constexpr WindowPosChanging(const Msg &p) : Msg{p} { }
		[[nodiscard]] WINDOWPOS& info() const { return *reinterpret_cast<WINDOWPOS*>(lp); }
	};

}

namespace _wl_internal {

	/** Keeps pre and post internal events. */
	class InternalEvents final : private wl::NoCopyNoMove {
	public:
		struct Msg final { // ordinary WM messages
			UINT wm;
			Func1<wl::wm::Msg&> cb;
		};
		struct Nfy final { // WM_NOTIFY messages
			WORD idFrom;
			int code;
			Func1<wl::wm::Notify&> cb;
		};

		constexpr explicit InternalEvents(bool isDlg) : _isDlg{isDlg} { }

		void wm_create_or_init_dialog(Func0 cb);
		void wm(UINT msg, Func1<wl::wm::Msg&> cb);
		void wm_notify(WORD idFrom, int code, Func1<wl::wm::Notify&> cb);
		void clear_inis(); // will delete WM_CREATE and WM_INITDIALOG
		void clear();
		[[nodiscard]] bool process_all(wl::wm::Msg procMsg) const;

		bool _isDlg;
		std::vector<Func0> _inis{}; // WM_CREATE, WM_INITDIALOG
		std::vector<Msg> _msgs{};
		std::vector<Nfy> _nfys{}; // WM_NOTIFY
	};

	class WndBase; // forward declaration
	class NativeCtrlBase;

	/// Wraps Func1<T&> into Func1<Notify&> by extracting hdr<T>().
	template<typename T>
	Func1<wl::wm::Notify&> WrapNfy(Func1<T&> cb) {
		auto* c = new Func1<T&>(cb);
		return MkFunc1(+[](Func1<T&>* c, wl::wm::Notify& p) {
			c->Call(p.hdr<T>());
		}, c);
	}

	/// Wraps Func0 into Func1<Notify&>, ignoring the Notify arg.
	inline Func1<wl::wm::Notify&> WrapNfy(Func0 cb) {
		auto* c = new Func0(cb);
		return MkFunc1(+[](Func0* c, wl::wm::Notify&) {
			c->Call();
		}, c);
	}

	/// Wraps Func0 into Func1<Msg&>, ignoring the Msg arg.
	inline Func1<wl::wm::Msg&> WrapMsg(Func0 cb) {
		auto* c = new Func0(cb);
		return MkFunc1(+[](Func0* c, wl::wm::Msg&) {
			c->Call();
		}, c);
	}

	/// Wraps std::function<R(T&)> into Func1<Notify&>, setting p.ret from return value.
	template<typename R, typename T>
	Func1<wl::wm::Notify&> WrapNfyRet(std::function<R(T&)> cb) {
		using F = std::function<R(T&)>;
		auto* c = new F(std::move(cb));
		return MkFunc1(+[](F* c, wl::wm::Notify& p) {
			p.ret = (*c)(p.hdr<T>());
		}, c);
	}

	/// Wraps std::function<R()> into Func1<Notify&>, setting p.ret from return value.
	template<typename R>
	Func1<wl::wm::Notify&> WrapNfyRet(std::function<R()> cb) {
		using F = std::function<R()>;
		auto* c = new F(std::move(cb));
		return MkFunc1(+[](F* c, wl::wm::Notify& p) {
			p.ret = (*c)();
		}, c);
	}

}

/** @brief Events for windows and controls. */
namespace wl::events {

	/** Native `IWindowParent` events. */
	class WindowEvents final : private wl::NoCopyNoMove {
	private:
		struct Msg final { // ordinary WM messages
			UINT wm;
			Func1<wl::wm::Msg&> cb;
		};
		struct Cmd final { // WM_COMMAND messages
			WORD cmdId;
			WORD notifCode;
			Func0 cb;
		};
		struct Nfy final { // WM_NOTIFY messages
			WORD idFrom;
			int code;
			Func1<wl::wm::Notify&> cb;
		};

		constexpr explicit WindowEvents(bool isDlg) : _isDlg{isDlg} { }

	public:
		/// Adds a callback to an ordinary WM message.
		///
		/// This is a general method for custom messages, always prefer using the
		/// specific message methods.
		///
		/// Example:
		///
		/// ```cpp
		/// wnd.on().wm(WM_ENTERIDLE, [](wl::wm::Msg& p) {
		///     // ...
		///     p.ret = 0;
		/// });
		/// ```
		void wm(UINT msg, Func1<wl::wm::Msg&> cb);

		/// Handles the [`WM_CREATE`] message.
		///
		/// Note that this message is sent only to windows create
		/// programmatically, not to dialog windows.
		///
		/// Example:
		///
		/// ```cpp
		/// wnd.on().wm_create([](wl::wm::Create& p) {
		///     // ...
		///     p.ret = 0;
		/// });
		/// ```
		///
		/// [`WM_CREATE`]: https://learn.microsoft.com/en-us/windows/win32/winmsg/wm-create
		void wm_create(Func1<wl::wm::Create&> cb);

		/// Handles the [`WM_INITDIALOG`] message.
		///
		/// Note that this message is sent only to dialog windows, not to windows
		/// created programmatically.
		///
		/// Example:
		///
		/// ```cpp
		/// wnd.on().wm_init_dialog([](wl::wm::InitDialog& p) {
		///     // ...
		///     p.ret = TRUE;
		/// });
		/// ```
		///
		/// [`WM_INITDIALOG`]: https://learn.microsoft.com/en-us/windows/win32/dlgbox/wm-initdialog
		void wm_init_dialog(Func1<wl::wm::InitDialog&> cb);

		/// Adds a callback to a [`WM_COMMAND`] message, to an specific
		/// notification code.
		///
		/// Example:
		///
		/// ```cpp
		/// wnd.on().wm_command(BTN_MAIN, BN_CLICKED, []() {
		///     // ...
		/// });
		/// ```
		///
		/// [`WM_COMMAND`]: https://learn.microsoft.com/en-us/windows/win32/menurc/wm-command
		void wm_command(WORD cmdId, WORD notifCode, Func0 cb);

		/// Adds a callback to a [`WM_COMMAND`] message, for notifications from
		/// both accelerator and menu events.
		///
		/// This is the most common case.
		///
		/// Example:
		///
		/// ```cpp
		/// wnd.on().wm_command(IDCANCEL, []() {
		///     // ...
		/// });
		/// ```
		///
		/// [`WM_COMMAND`]: https://learn.microsoft.com/en-us/windows/win32/menurc/wm-command
		void wm_command(WORD cmdId, Func0 cb);

		/// Adds a callback to a [`WM_NOTIFY`] message, to specific control ID and
		/// notification code.
		///
		/// This is a general method for custom notifications, always prefer using
		/// the specific control notification metods.
		///
		/// Example:
		///
		/// ```cpp
		/// wnd.on().wm_notify(LST_MAIN, LVN_BEGINDRAG, [](wl::wm::Notify& p) {
		///     // ...
		///     p.ret = 0;
		/// });
		/// ```
		///
		/// [`WM_NOTIFY`]: https://learn.microsoft.com/en-us/windows/win32/controls/wm-notify
		void wm_notify(WORD idFrom, int code, Func1<wl::wm::Notify&> cb);

		void wm_activate(Func1<wl::wm::Activate&> cb);
		void wm_activate_app(Func1<wl::wm::ActivateApp&> cb);
		void wm_child_activate(Func0 cb);
		void wm_close(Func0 cb);
		void wm_destroy(Func0 cb);
		void wm_display_change(Func1<wl::wm::DisplayChange&> cb);
		void wm_enable(Func1<wl::wm::Enable&> cb);
		void wm_end_session(Func1<wl::wm::EndSession&> cb);
		void wm_enter_size_move(Func0 cb);
		void wm_erase_bkgnd(Func1<wl::wm::EraseBkgnd&> cb);
		void wm_exit_size_move(Func0 cb);
		void wm_get_dlg_code(Func1<wl::wm::GetDlgCode&> cb);
		void wm_h_scroll(Func1<wl::wm::HScroll&> cb);
		void wm_init_menu_popup(Func1<wl::wm::InitMenuPopup&> cb);
		void wm_kill_focus(Func1<wl::wm::KillFocus&> cb);
		void wm_l_button_dbl_clk(Func1<wl::wm::LButtonDblClk&> cb);
		void wm_l_button_down(Func1<wl::wm::LButtonDown&> cb);
		void wm_l_button_up(Func1<wl::wm::LButtonUp&> cb);
		void wm_m_button_dbl_clk(Func1<wl::wm::MButtonDblClk&> cb);
		void wm_m_button_down(Func1<wl::wm::MButtonDown&> cb);
		void wm_m_button_up(Func1<wl::wm::MButtonUp&> cb);
		void wm_mouse_hover(Func1<wl::wm::MouseHover&> cb);
		void wm_mouse_move(Func1<wl::wm::MouseMove&> cb);
		void wm_move(Func1<wl::wm::Move&> cb);
		void wm_moving(Func1<wl::wm::Moving&> cb);
		void wm_nc_calc_size(Func1<wl::wm::NcCalcSize&> cb);
		void wm_nc_destroy(Func0 cb);
		void wm_nc_paint(Func1<wl::wm::NcPaint&> cb);
		void wm_paint(Func0 cb);
		void wm_power_broadcast(Func1<wl::wm::PowerBroadcast&> cb);
		void wm_query_open(Func0 cb);
		void wm_r_button_dbl_clk(Func1<wl::wm::RButtonDblClk&> cb);
		void wm_r_button_down(Func1<wl::wm::RButtonDown&> cb);
		void wm_r_button_up(Func1<wl::wm::RButtonUp&> cb);
		void wm_set_cursor(Func1<wl::wm::SetCursor&> cb);
		void wm_set_focus(Func1<wl::wm::SetFocus&> cb);
		void wm_show_window(Func1<wl::wm::ShowWindow&> cb);
		void wm_size(Func1<wl::wm::Size&> cb);
		void wm_sizing(Func1<wl::wm::Sizing&> cb);
		void wm_theme_changed(Func0 cb);
		void wm_time_change(Func0 cb);
		void wm_v_scroll(Func1<wl::wm::VScroll&> cb);
		void wm_window_pos_changed(Func1<wl::wm::WindowPosChanged&> cb);
		void wm_window_pos_changing(Func1<wl::wm::WindowPosChanging&> cb);

	private:
		[[nodiscard]] bool has_message() const;
		void clear_inis(); // WM_CREATE and WM_INITDIALOG
		void clear();
		void process_last(wl::wm::Msg &procMsg) const;

		bool _isDlg;
		std::vector<Msg> _inis{}; // WM_CREATE, WM_INITDIALOG
		std::vector<Msg> _msgs{};
		std::vector<Cmd> _cmds{}; // WM_COMMAND
		std::vector<Nfy> _nfys{}; // WM_NOTIFY

		friend _wl_internal::WndBase;
		friend _wl_internal::NativeCtrlBase;
	};

}
