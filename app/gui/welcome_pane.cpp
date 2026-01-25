#include "welcome_pane.h"

#include "app/gui/app_control.h"

WelcomePane::WelcomePane() :
		PaneBase("Welcome", "Welcome", get_app_theme_icon(SNAME("File"))) {
}