#include "pane_base.h"

String PaneBase::_get_pane_title() const {
	return get_type();
}

Ref<Texture2D> PaneBase::_get_pane_icon() const {
	return Ref<Texture2D>();
}

void PaneBase::_on_active(bool p_active) {
	set_process_shortcut_input(p_active);
}

void PaneBase::_data_changed() {
	emit_signal(SNAME("data_changed"));
}

void PaneBase::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY: {
			// Trigger icon_changed to avoid invalid icons caused by unpropagated theme context.
			if (!pane_icon.is_valid()) {
				Ref<Texture2D> icon = _get_pane_icon();
				if (icon.is_valid()) {
					set_icon(icon);
				}
			}
		} break;
	}
}

PaneBase *PaneBase::get_control_parent_pane(Control *p_control) {
	ERR_FAIL_NULL_V(p_control, nullptr);

	PaneBase *pane = Object::cast_to<PaneBase>(p_control);
	if (pane) {
		return pane;
	}

	Control *parent = p_control->get_parent_control();
	while (parent) {
		PaneBase *pane = Object::cast_to<PaneBase>(parent);
		if (pane) {
			return pane;
		}
		parent = parent->get_parent_control();
	}

	return nullptr;
}

StringName PaneBase::get_type() const {
	return pane_type;
}

void PaneBase::set_title(const String &p_title) {
	if (pane_title != p_title) {
		pane_title = p_title;
		emit_signal(SNAME("title_changed"));
	}
}

String PaneBase::get_title() const {
	return pane_title.is_empty() ? _get_pane_title() : pane_title;
}

void PaneBase::set_icon(const Ref<Texture2D> &p_icon) {
	if (pane_icon != p_icon) {
		pane_icon = p_icon;
		emit_signal(SNAME("icon_changed"));
	}
}

Ref<Texture2D> PaneBase::get_icon() const {
	return !pane_icon.is_valid() ? _get_pane_icon() : pane_icon;
}

void PaneBase::set_active(bool p_active) {
	if (active == p_active) {
		return;
	}

	active = p_active;

	_on_active(active);
}

bool PaneBase::is_active() const {
	return active;
}

Dictionary PaneBase::get_config_data() {
	return Dictionary();
}

void PaneBase::apply_config_data(const Dictionary &p_dict) {
}

void PaneBase::_bind_methods() {
	ADD_SIGNAL(MethodInfo("title_changed"));
	ADD_SIGNAL(MethodInfo("icon_changed"));
	ADD_SIGNAL(MethodInfo("data_changed"));
}

PaneBase::PaneBase(const StringName &p_type) {
	pane_type = p_type;
}

PaneBase::~PaneBase() {
}
