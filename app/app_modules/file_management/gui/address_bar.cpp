#include "address_bar.h"

#include "scene/gui/popup_menu.h"

void AddressBar::_notification(int p_what) {
	// switch (p_what) {
	// 	case NOTIFICATION_ENTER_TREE: {
	// 		connect("editing_toggled", callable_mp(this, &AddressBar::_on_editing_toggled));
	// 		// connect(SNAME("editing_toggled"), callable_mp(this, &AddressBar::_on_editing_toggled));
	// 	} break;
	// }
}

void AddressBar::_selected(int p_which) {
	_select(p_which, true);
}

void AddressBar::_select(int p_which, bool p_emit) {
	ERR_FAIL_INDEX(p_which, popup->get_item_count());

	// TODO: Remove
	for (int i = 0; i < popup->get_item_count(); i++) {
		popup->set_item_checked(i, i == p_which);
	}

	String path = popup->get_item_text(p_which);
	set_text(path);

	if (is_inside_tree() && p_emit) {
		emit_signal(SceneStringName(item_selected), path);
	}
}

void AddressBar::_on_editing_toggled(bool p_toggled_on) {
	// if (p_toggled_on) {
	// 	if (!popup->is_visible()) {
	// 		show_popup();
	// 	}
	// }
}

void AddressBar::add_icon_item(const Ref<Texture2D> &p_icon, const String &p_label, int p_id) {
	popup->add_icon_radio_check_item(p_icon, p_label, p_id);
}

void AddressBar::add_item(const String &p_label, int p_id) {
	popup->add_radio_check_item(p_label, p_id);
}

void AddressBar::remove_item(int p_idx) {
	popup->remove_item(p_idx);
}

void AddressBar::clear() {
	LineEdit::clear();

	// TODO
	popup->clear();
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

	ADD_CLASS_DEPENDENCY("PopupMenu");
}

AddressBar::AddressBar() {
	// popup = memnew(PopupMenu);
	// popup->hide();
	// add_child(popup, false, INTERNAL_MODE_FRONT);
	// popup->connect("index_pressed", callable_mp(this, &AddressBar::_selected));
}

AddressBar::~AddressBar() {
}
