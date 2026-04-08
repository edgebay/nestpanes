#include "address_bar.h"

#include "scene/gui/button.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/popup_menu.h"

void AddressBar::shortcut_input(const Ref<InputEvent> &p_event) {
	ERR_FAIL_COND(p_event.is_null());

	if (disable_shortcuts) {
		return;
	}

	if (p_event->is_pressed() && !p_event->is_echo() && is_visible_in_tree() && popup->activate_item_by_event(p_event, false)) {
		accept_event();
		return;
	}
}

void AddressBar::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_TRANSLATION_CHANGED:
		case NOTIFICATION_LAYOUT_DIRECTION_CHANGED: {
			popup->set_layout_direction((Window::LayoutDirection)get_layout_direction());
		} break;

		case NOTIFICATION_THEME_CHANGED: {
			popup_button->set_button_icon(get_theme_icon(SNAME("arrow"), SNAME("AddressBar")));
		} break;

		case NOTIFICATION_VISIBILITY_CHANGED: {
			if (!is_visible_in_tree()) {
				popup->hide();
			}
		} break;
	}
}

void AddressBar::_on_text_submitted(const String &p_path) {
	emit_signal(SceneStringName(text_submitted), p_path);
}

void AddressBar::_on_editing_toggled(bool p_toggled_on) {
	if (p_toggled_on) {
		if (!popup->is_visible()) {
			show_popup();
			// line_edit->grab_focus();
		}
	}
}

void AddressBar::_on_button_pressed() {
	if (popup->is_visible()) {
		popup->hide();
		return;
	}

	show_popup();
}

void AddressBar::_on_id_pressed(int p_id) {
	if (!is_inside_tree()) {
		return;
	}

	int index = popup->get_item_index(p_id);
	String path = popup->get_item_text(index);
	// print_line("Selected id: ", p_id, "index: ", index, "path: ", path);

	emit_signal(SceneStringName(item_selected), p_id);
}

void AddressBar::edit(bool p_hide_focus) {
	line_edit->edit(p_hide_focus);
}

void AddressBar::unedit() {
	line_edit->unedit();
}

bool AddressBar::is_editing() const {
	return line_edit->is_editing();
}

void AddressBar::set_text(String p_text) {
	line_edit->set_text(p_text);
}

String AddressBar::get_text() const {
	return line_edit->get_text();
}

void AddressBar::add_icon_item(const Ref<Texture2D> &p_icon, const String &p_label, int p_id) {
	popup->add_icon_item(p_icon, p_label, p_id);
}

void AddressBar::add_item(const String &p_label, int p_id) {
	popup->add_item(p_label, p_id);
}

void AddressBar::set_focused_item(int p_idx) {
	popup->set_focused_item(p_idx);
}

int AddressBar::get_focused_item() const {
	return popup->get_focused_item();
}

int AddressBar::get_item_count() const {
	return popup->get_item_count();
}

void AddressBar::remove_item(int p_idx) {
	popup->remove_item(p_idx);
}

void AddressBar::clear_items() {
	popup->clear();
}

void AddressBar::clear() {
	line_edit->clear();
}

PopupMenu *AddressBar::get_popup() const {
	return popup;
}

void AddressBar::show_popup() {
	if (!get_viewport()) {
		return;
	}

	Rect2 rect = get_screen_rect();
	rect.position.y += rect.size.height;
	if (get_viewport()->is_embedding_subwindows() && popup->get_force_native()) {
		Transform2D xform = get_viewport()->get_popup_base_transform_native();
		rect = xform.xform(rect);
	}
	rect.size.height = 0;
	popup->popup(rect);
}

void AddressBar::_bind_methods() {
	ADD_SIGNAL(MethodInfo("item_selected", PropertyInfo(Variant::INT, "index")));
	ADD_SIGNAL(MethodInfo("text_submitted", PropertyInfo(Variant::STRING, "new_text")));

	ADD_CLASS_DEPENDENCY("Button");
	ADD_CLASS_DEPENDENCY("LineEdit");
	ADD_CLASS_DEPENDENCY("PopupMenu");
}

AddressBar::AddressBar() {
	set_process_shortcut_input(true);

	line_edit = memnew(LineEdit);
	line_edit->set_structured_text_bidi_override(TextServer::STRUCTURED_TEXT_FILE);
	line_edit->set_select_all_on_focus(true);
	line_edit->set_context_menu_enabled(false);
	line_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	add_child(line_edit);
	line_edit->connect(SNAME("text_submitted"), callable_mp(this, &AddressBar::_on_text_submitted));
	// line_edit->connect(SNAME("editing_toggled"), callable_mp(this, &AddressBar::_on_editing_toggled));

	popup_button = memnew(Button);
	popup_button->set_toggle_mode(true);
	popup_button->set_action_mode(Button::ACTION_MODE_BUTTON_PRESS);
	popup_button->set_theme_type_variation(SceneStringName(FlatButton));
	add_child(popup_button);
	popup_button->connect(SNAME("pressed"), callable_mp(this, &AddressBar::_on_button_pressed));

	popup = memnew(PopupMenu);
	popup->set_shrink_width(false);
	add_child(popup, false, INTERNAL_MODE_FRONT);
	popup->connect("id_pressed", callable_mp(this, &AddressBar::_on_id_pressed));
	popup->connect("popup_hide", callable_mp(Object::cast_to<BaseButton>(popup_button), &BaseButton::set_pressed).bind(false));
}

AddressBar::~AddressBar() {
}
