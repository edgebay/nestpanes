#include "welcome_pane.h"

#include "app/gui/app_control.h"

String WelcomePane::_get_pane_title() const {
	return RTR("Welcome");
}

Ref<Texture2D> WelcomePane::_get_pane_icon() const {
	return get_app_theme_icon(SNAME("File")); // TODO: static?
}

WelcomePane::WelcomePane() :
		PaneBase(get_class_static()) {
}
