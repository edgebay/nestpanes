#include "pane_base.h"

void PaneBase::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			title = _get_pane_title();
			icon = _get_pane_icon();
		} break;
	}
}

String PaneBase::_get_pane_title() const {
	return get_type();
}

Ref<Texture2D> PaneBase::_get_pane_icon() const {
	return Ref<Texture2D>();
}

StringName PaneBase::get_type() const {
	return type;
}

void PaneBase::set_title(const String &p_title) {
	if (title != p_title) {
		title = p_title;
		// emit_signal(SNAME("title_changed"), title);
		emit_signal(SNAME("title_changed"));
	}
}

String PaneBase::get_title() const {
	return title;
}

void PaneBase::set_icon(const Ref<Texture2D> &p_icon) {
	if (icon != p_icon) {
		icon = p_icon;
		// emit_signal(SNAME("icon_changed"), p_icon);
		emit_signal(SNAME("icon_changed"));
	}
}

Ref<Texture2D> PaneBase::get_icon() const {
	return icon;
}

void PaneBase::_bind_methods() {
	// ADD_SIGNAL(MethodInfo("title_changed", PropertyInfo(Variant::STRING, "title")));
	// ADD_SIGNAL(MethodInfo("icon_changed", PropertyInfo(Variant::OBJECT, "icon")));
	ADD_SIGNAL(MethodInfo("title_changed"));
	ADD_SIGNAL(MethodInfo("icon_changed"));
}

PaneBase::PaneBase(const StringName &p_type) {
	type = p_type;
}

PaneBase::~PaneBase() {
}
