#include "welcome_pane.h"

#include "app/gui/app_control.h"

// WelcomePane::WelcomePane() :
// 		PaneBase("Welcome", "Welcome", get_app_theme_icon(SNAME("File"))) {
// }

String WelcomePane::_get_pane_title() const {
	return "Welcome";
}

Ref<Texture2D> WelcomePane::_get_pane_icon() const {
	return get_app_theme_icon(SNAME("File")); // TODO: static?
}

const StringName &WelcomePane::get_type_static() {
	static StringName _type_static = StringName("WelcomePane");
	return _type_static;
}

WelcomePane::WelcomePane() :
		PaneBase(get_type_static()) {
}
