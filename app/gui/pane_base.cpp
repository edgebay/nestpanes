#include "pane_base.h"

StringName PaneBase::get_type() const {
	return type;
}

void PaneBase::set_title(const String &p_title) {
	title = p_title;
}

String PaneBase::get_title() const {
	return title;
}

void PaneBase::set_icon(const Ref<Texture2D> &p_icon) {
	icon = p_icon;
}

Ref<Texture2D> PaneBase::get_icon() const {
	return icon;
}

PaneBase::PaneBase(const StringName &p_type, const String &p_title, const Ref<Texture2D> &p_icon) {
	type = p_type;

	title = p_title;
	icon = p_icon;
}
