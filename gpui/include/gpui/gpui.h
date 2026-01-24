#pragma once

/// GPUI - A C++ port of Zed's GPU-accelerated UI framework.
///
/// GPUI provides a declarative, reactive UI framework with:
/// - Flexbox/Grid layout (via Taffy-compatible layout engine)
/// - GPU-accelerated rendering
/// - Entity-based state management with observers
/// - Focus management and keyboard event dispatch
/// - Platform abstraction for macOS, Windows, and Linux
///
/// Basic usage:
/// ```cpp
/// #include <gpui/gpui.h>
///
/// struct MyApp {
///     int counter = 0;
///
///     gpui::Div render(gpui::Window& window, gpui::Context<MyApp>& cx) {
///         return gpui::div()
///             .flex().flex_col().items_center().justify_center()
///             .size(gpui::Length(gpui::DefiniteLength::from_fraction(1.0f)))
///             .bg(gpui::rgb(0x1e1e2e))
///             .child(
///                 gpui::text("Hello, GPUI!")
///                     .text_color(gpui::white())
///                     .text_2xl()
///                     .font_bold()
///             );
///     }
/// };
///
/// int main() {
///     gpui::Application app;
///     app.run([](gpui::App& cx) {
///         auto view = gpui::new_view(cx, MyApp{});
///         cx.open_window(gpui::WindowOptions{}, [&](gpui::Window& w, gpui::App& cx) {
///             // Set up root view
///         });
///     });
///     return 0;
/// }
/// ```

// Dependencies
#include <gpui/collections.h>
#include <gpui/refineable.h>
#include <gpui/util.h>
#include <gpui/scheduler.h>
#include <gpui/http_client.h>

// Core modules
#include <gpui/geometry.h>
#include <gpui/color.h>
#include <gpui/input.h>
#include <gpui/action.h>
#include <gpui/scene.h>
#include <gpui/style.h>
#include <gpui/styled.h>
#include <gpui/text_system.h>
#include <gpui/platform.h>
#include <gpui/executor.h>
#include <gpui/element.h>
#include <gpui/app.h>
#include <gpui/window.h>
#include <gpui/view.h>

// Built-in elements
#include <gpui/elements/div.h>
#include <gpui/elements/text.h>

namespace gpui {

/// Prelude - commonly used types brought into scope.
/// Users can write `using namespace gpui::prelude;` for convenience.
namespace prelude {

using gpui::App;
using gpui::Application;
using gpui::Window;
using gpui::Entity;
using gpui::Context;
using gpui::View;

using gpui::Div;
using gpui::TextElement;
using gpui::div;
using gpui::text;

using gpui::Pixels;
using gpui::Rems;
using gpui::Rgba;
using gpui::Hsla;

using gpui::rgb;
using gpui::rgba;
using gpui::hsla;
using gpui::black;
using gpui::white;
using gpui::red;
using gpui::green;
using gpui::blue;
using gpui::transparent;

using gpui::WindowOptions;
using gpui::WindowHandle;

using gpui::Subscription;
using gpui::FocusHandle;

using gpui::MouseButton;
using gpui::MouseDownEvent;
using gpui::MouseUpEvent;
using gpui::MouseMoveEvent;
using gpui::ScrollWheelEvent;
using gpui::KeyDownEvent;
using gpui::KeyUpEvent;
using gpui::ClickEvent;
using gpui::Keystroke;
using gpui::Modifiers;

namespace literals = gpui::literals;

} // namespace prelude

} // namespace gpui
