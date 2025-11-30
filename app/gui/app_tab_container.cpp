#include "app_tab_container.h"

#include "scene/gui/button.h"
#include "scene/gui/popup_menu.h"

#define CUSTOM_MINIMUM_SIZE Size2(170, 20) // * APP_SCALE

void AppTabContainer::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_DRAW: {
			_on_resized();
		} break;

		case NOTIFICATION_THEME_CHANGED: {
			// tabbar_panel->add_theme_style_override(SceneStringName(panel), get_theme_stylebox(SNAME("tabbar_background"), SNAME("TabContainer")));
			// _tab_bar->add_theme_constant_override("icon_max_width", get_theme_constant(SNAME("class_icon_size"), EditorStringName(Editor)));

			tab_add->set_button_icon(get_theme_icon(SNAME("Add"), SNAME("AppIcons")));
			tab_add->add_theme_color_override("icon_normal_color", Color(0.6f, 0.6f, 0.6f, 0.8f));

			// scene_tab_add_ph->set_custom_minimum_size(tab_add->get_minimum_size());
		} break;
	}
}

// void AppTabContainer::add_child_notify(Node *p_child) {
// 	TabContainer::add_child_notify(p_child);

// 	// _on_resized();
// }

// void AppTabContainer::move_child_notify(Node *p_child) {
// 	TabContainer::move_child_notify(p_child);

// 	_on_resized();
// }

void AppTabContainer::_menu_option_confirm(int p_option, bool p_confirmed) {
	if (!p_confirmed) { // FIXME: this may be a hack.
		current_menu_option = (MenuOptions)p_option;
	}

	switch (p_option) {
		case FILE_NEW_SCENE: {
			emit_signal(SNAME("new_tab"));
		} break;
	}
}

void AppTabContainer::_on_tab_closed(int p_tab) {
	Node *control = get_child(p_tab, false);
	print_line(itos(p_tab) + ", tab closed: ", control);
	if (control != nullptr) {
		remove_child(control);
		control->queue_free();
	}
	if (get_child_count(false) == 0) {
		emit_signal(SNAME("emptied"));
	}
}

void AppTabContainer::_on_resized() {
	TabBar *_tab_bar = get_tab_bar();
	const Size2 add_button_size = Size2(tab_add->get_size().x, _tab_bar->get_size().y);
	// if (_tab_bar->get_offset_buttons_visible()) {
	// 	// Move the add button to a fixed position.
	// 	if (tab_add->get_parent() == _tab_bar) {
	// 		_tab_bar->remove_child(tab_add);
	// 		scene_tab_add_ph->add_child(tab_add);
	// 		tab_add->set_rect(Rect2(Point2(), add_button_size));
	// 	}
	// } else {
	// 	// Move the add button to be after the last tab.
	// 	if (tab_add->get_parent() == scene_tab_add_ph) {
	// 		scene_tab_add_ph->remove_child(tab_add);
	// 		_tab_bar->add_child(tab_add);
	// 	}

	if (_tab_bar->get_tab_count() == 0) {
		tab_add->set_rect(Rect2(Point2(), add_button_size));
		return;
	}

	Rect2 last_tab = _tab_bar->get_tab_rect(_tab_bar->get_tab_count() - 1);
	int hsep = _tab_bar->get_theme_constant(SNAME("h_separation"));
	if (_tab_bar->is_layout_rtl()) {
		tab_add->set_rect(Rect2(Point2(last_tab.position.x - add_button_size.x - hsep, last_tab.position.y), add_button_size));
	} else {
		tab_add->set_rect(Rect2(Point2(last_tab.position.x + last_tab.size.width + hsep, last_tab.position.y), add_button_size));
	}
	// }
}

void AppTabContainer::_reposition_active_tab(int p_to_index) {
	// AppTabContainer::get_editor_data().move_edited_scene_to_index(p_to_index);
	// update_scene_tabs();

	_on_resized();
}

void AppTabContainer::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_new_tab_enabled", "new_tab_enabled"), &AppTabContainer::set_new_tab_enabled);
	ClassDB::bind_method(D_METHOD("get_new_tab_enabled"), &AppTabContainer::get_new_tab_enabled);

	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "new_tab_enabled"), "set_new_tab_enabled", "get_new_tab_enabled");

	ADD_SIGNAL(MethodInfo("new_tab"));
	ADD_SIGNAL(MethodInfo("emptied"));

	ADD_CLASS_DEPENDENCY("Button");
}

void AppTabContainer::set_new_tab_enabled(bool p_enabled) {
	if (new_tab_enabled == p_enabled) {
		return;
	}

	if (p_enabled) {
		get_tab_bar()->set_tab_close_display_policy(TabBar::CLOSE_BUTTON_SHOW_ALWAYS);
		tab_add->show();
	} else {
		get_tab_bar()->set_tab_close_display_policy(TabBar::CLOSE_BUTTON_SHOW_NEVER);
		tab_add->hide();
	}
	new_tab_enabled = p_enabled;
}

bool AppTabContainer::get_new_tab_enabled() const {
	return new_tab_enabled;
}

void AppTabContainer::trigger_menu_option(int p_option, bool p_confirmed) {
	_menu_option_confirm(p_option, p_confirmed);
}

AppTabContainer::AppTabContainer() {
	set_custom_minimum_size(CUSTOM_MINIMUM_SIZE);
	set_v_size_flags(Control::SIZE_EXPAND_FILL);

	// PopupMenu *tabs_menu = memnew(PopupMenu);
	// tabs_menu->add_item(RTR("Test"), 0);
	// add_child(tabs_menu, false, INTERNAL_MODE_FRONT);

	// // set_popup(dock_select_popup);
	// set_popup(tabs_menu);

	// connect("pre_popup_pressed", callable_mp(this, &AppNode::_dock_pre_popup).bind(i));
	set_drag_to_rearrange_enabled(true);
	set_tabs_rearrange_group(1);
	// connect("tab_changed", callable_mp(this, &AppNode::_dock_tab_changed));
	set_use_hidden_tabs_for_min_size(true);

	tab_add = memnew(Button);
	tab_add->set_flat(true);
	tab_add->set_tooltip_text(RTR("Add a new scene."));
	tab_add->connect(SceneStringName(pressed), callable_mp(this, &AppTabContainer::trigger_menu_option).bind(AppTabContainer::FILE_NEW_SCENE, false));
	tab_add->hide();

	TabBar *_tab_bar = get_tab_bar();
	// _tab_bar->set_tab_close_display_policy(TabBar::CLOSE_BUTTON_SHOW_ALWAYS);
	_tab_bar->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
	_tab_bar->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	_tab_bar->add_child(tab_add, false, INTERNAL_MODE_FRONT);

	_tab_bar->connect("tab_close_pressed", callable_mp(this, &AppTabContainer::_on_tab_closed));
	// _tab_bar->connect("active_tab_rearranged", callable_mp(this, &AppTabContainer::_reposition_active_tab));
	// _tab_bar->connect(SceneStringName(resized), callable_mp(this, &AppTabContainer::_on_resized), CONNECT_DEFERRED);
}

AppTabContainer::~AppTabContainer() {
}
