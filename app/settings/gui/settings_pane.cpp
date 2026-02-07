#include "settings_pane.h"

#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/check_button.h"
#include "scene/gui/label.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/tab_container.h"
#include "scene/gui/texture_rect.h"
#include "scene/gui/tree.h"

#include "app/app_modules/settings/gui/settings_inspector_plugin.h"
#include "app/gui/app_control.h"
#include "app/gui/inspector/object_inspector.h"
#include "app/gui/inspector/sectioned_inspector.h"

#include "app/app_modules/settings/app_settings.h"

String SettingsPane::_get_pane_title() const {
	return "Settings";
}

Ref<Texture2D> SettingsPane::_get_pane_icon() const {
	return get_app_theme_icon(SNAME("File")); // TODO: static?
}

void SettingsPane::_tabs_tab_changed(int p_tab) {
}

void SettingsPane::_advanced_toggled(bool p_button_pressed) {
}

void SettingsPane::_shortcut_button_pressed(Object *p_item, int p_column, int p_idx, MouseButton p_button) {
}

void SettingsPane::_shortcut_cell_double_clicked() {
}

void SettingsPane::_app_restart_request() {
}

void SettingsPane::_app_restart() {
}

void SettingsPane::_app_restart_close() {
}

void SettingsPane::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY: {
			if (!EditorSettings::get_singleton()) {
				return;
			}

			// EditorSettings::get_singleton()->update_text_editor_themes_list(); // Make sure we have an up to date list of themes.

			// _update_dynamic_property_hints();

			inspector->edit(EditorSettings::get_singleton());
			inspector->get_inspector()->update_tree();

			// _update_shortcuts();
			// set_process_shortcut_input(true);

			// // Restore valid window bounds or pop up at default size.
			// Rect2 saved_size = EditorSettings::get_singleton()->get_project_metadata("dialog_bounds", "editor_settings", Rect2());
			// if (saved_size != Rect2()) {
			// 	popup(saved_size);
			// } else {
			// 	popup_centered_clamped(Size2(900, 700) * EDSCALE, 0.8);
			// }

			// _focus_current_search_box();
		} break;
	}
}

SettingsPane::SettingsPane() :
		PaneBase(get_class_static()) {
	tabs = memnew(TabContainer);
	tabs->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	tabs->set_theme_type_variation("TabContainerOdd");
	tabs->connect("tab_changed", callable_mp(this, &SettingsPane::_tabs_tab_changed));
	add_child(tabs);

	// General Tab

	tab_general = memnew(VBoxContainer);
	tabs->add_child(tab_general);
	tab_general->set_name(TTRC("General"));

	HBoxContainer *hbc = memnew(HBoxContainer);
	hbc->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	tab_general->add_child(hbc);

	search_box = memnew(LineEdit);
	search_box->set_placeholder(TTRC("Filter Settings"));
	search_box->set_accessibility_name(TTRC("Filter Settings"));
	search_box->set_virtual_keyboard_show_on_focus(false);
	search_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	hbc->add_child(search_box);

	advanced_switch = memnew(CheckButton(TTRC("Advanced Settings")));
	hbc->add_child(advanced_switch);

	// TODO: settings string
	bool use_advanced = EDITOR_DEF("_editor_settings_advanced_mode", false);
	advanced_switch->set_pressed(use_advanced);
	advanced_switch->connect(SceneStringName(toggled), callable_mp(this, &SettingsPane::_advanced_toggled));

	inspector = memnew(SectionedInspector);
	inspector->get_inspector()->set_use_filter(true);
	// inspector->get_inspector()->set_mark_unsaved(false);
	inspector->register_search_box(search_box);
	inspector->register_advanced_toggle(advanced_switch);
	inspector->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	tab_general->add_child(inspector);
	// inspector->get_inspector()->connect("property_edited", callable_mp(this, &EditorSettingsDialog::_settings_property_edited));
	// inspector->get_inspector()->connect("restart_requested", callable_mp(this, &EditorSettingsDialog::_editor_restart_request));

	restart_container = memnew(PanelContainer);
	tab_general->add_child(restart_container);
	HBoxContainer *restart_hb = memnew(HBoxContainer);
	restart_container->add_child(restart_hb);
	restart_icon = memnew(TextureRect);
	restart_icon->set_v_size_flags(Control::SIZE_SHRINK_CENTER);
	restart_hb->add_child(restart_icon);
	restart_label = memnew(Label);
	restart_label->set_focus_mode(Control::FOCUS_ACCESSIBILITY);
	restart_label->set_text(TTRC("The editor must be restarted for changes to take effect."));
	restart_hb->add_child(restart_label);
	restart_hb->add_spacer();
	Button *restart_button = memnew(Button);
	restart_button->connect(SceneStringName(pressed), callable_mp(this, &SettingsPane::_app_restart));
	restart_hb->add_child(restart_button);
	restart_button->set_text(TTRC("Save & Restart"));
	restart_close_button = memnew(Button);
	restart_close_button->set_accessibility_name(TTRC("Close"));
	restart_close_button->set_flat(true);
	restart_close_button->connect(SceneStringName(pressed), callable_mp(this, &SettingsPane::_app_restart_close));
	restart_hb->add_child(restart_close_button);
	restart_container->hide();

	// Shortcuts Tab

	tab_shortcuts = memnew(VBoxContainer);

	tabs->add_child(tab_shortcuts);
	tab_shortcuts->set_name(TTRC("Shortcuts"));

	shortcuts = memnew(Tree);
	shortcuts->set_accessibility_name(TTRC("Shortcuts"));
	shortcuts->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	shortcuts->set_columns(2);
	shortcuts->set_hide_root(true);
	shortcuts->set_column_titles_visible(true);
	shortcuts->set_column_title(0, TTRC("Name"));
	shortcuts->set_column_title(1, TTRC("Binding"));
	shortcuts->connect("button_clicked", callable_mp(this, &SettingsPane::_shortcut_button_pressed));
	shortcuts->connect("item_activated", callable_mp(this, &SettingsPane::_shortcut_cell_double_clicked));
	tab_shortcuts->add_child(shortcuts);

	// SET_DRAG_FORWARDING_GCD(shortcuts, SettingsPane);

	Ref<SettingsInspectorPlugin> plugin;
	plugin.instantiate();
	plugin->inspector = inspector;
	EditorInspector::add_inspector_plugin(plugin);
}
