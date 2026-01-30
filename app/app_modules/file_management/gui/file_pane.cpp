#include "file_pane.h"

#include "app/gui/app_control.h"

String FilePane::_get_pane_title() const {
	return "Folder";
}

Ref<Texture2D> FilePane::_get_pane_icon() const {
	return get_app_theme_icon(SNAME("Folder"));
}

FilePane::FilePane() :
		PaneBase(get_class_static()) {
}
