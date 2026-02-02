#include "app_tab_container.h"

#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/panel.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/popup_menu.h"
#include "scene/gui/texture_rect.h"

#include "scene/theme/theme_db.h"

// TODO: app/gui/* should not depend on app/app_modules/*
#include "app/app_modules/settings/app_settings.h"
#include "app/app_string_names.h"
#include "app/gui/app_control.h"
#include "app/themes/app_scale.h"

DropOverlay::DropPosition DropOverlay::_get_position(const Point2 &p_point) const {
	DropPosition position = DropPosition::DROP_CENTER;
	Size2 size = get_size();

	// TODO: 3.0
	double h_margin = size.width / 3.0;
	double v_margin = size.height / 3.0;

	if (h_margin < 1 || v_margin < 1) {
		return position;
	}

	int32_t px = p_point.x;
	int32_t py = p_point.y;
	if (px < h_margin) {
		position = DropPosition::DROP_LEFT;
	} else if (px > (size.width - h_margin)) {
		position = DropPosition::DROP_RIGHT;
	} else {
		if (py < v_margin) {
			position = DropPosition::DROP_UP;
		} else if (py > (size.height - v_margin)) {
			position = DropPosition::DROP_DOWN;
		} else {
			// center
		}
	}

	return position;
}

void DropOverlay::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_MOUSE_ENTER: {
			is_hovering = true;
			queue_redraw();
			print_line("DropOverlay enter", this);
		} break;

		case NOTIFICATION_MOUSE_EXIT: {
			is_hovering = false;
			queue_redraw();
			print_line("DropOverlay exit", this);
		} break;

		case NOTIFICATION_DRAW: {
			if (is_hovering) {
				Size2 size = get_size();
				double h_margin = size.width / 3.0;
				double v_margin = size.height / 3.0;
				switch (drop_position) {
					case DropPosition::DROP_UP:
						draw_style_box(get_theme_stylebox(SNAME("hovered"), SNAME("Tree")), Rect2(Point2(), Size2(size.width, v_margin)));
						break;
					case DropPosition::DROP_DOWN:
						draw_style_box(get_theme_stylebox(SNAME("hovered"), SNAME("Tree")), Rect2(Point2(0, size.height - v_margin), Size2(size.width, v_margin)));
						break;
					case DropPosition::DROP_LEFT:
						draw_style_box(get_theme_stylebox(SNAME("hovered"), SNAME("Tree")), Rect2(Point2(), Size2(h_margin, size.height)));
						break;
					case DropPosition::DROP_RIGHT:
						draw_style_box(get_theme_stylebox(SNAME("hovered"), SNAME("Tree")), Rect2(Point2(size.width - h_margin, 0), Size2(h_margin, size.height)));
						break;
					case DropPosition::DROP_CENTER:
						draw_style_box(get_theme_stylebox(SNAME("hovered"), SNAME("Tree")), Rect2(Point2(), size));
						break;

					default:
						break;
				}
			}
		} break;
	}
}

bool DropOverlay::can_drop_data(const Point2 &p_point, const Variant &p_data) const {
	Dictionary d = p_data;
	if (!d.has("type")) {
		return false;
	}

	if (String(d["type"]) == "tab_container_tab") {
		return true;
	}

	return false;
}

void DropOverlay::drop_data(const Point2 &p_point, const Variant &p_data) {
	Dictionary d = p_data;
	if (!d.has("type")) {
		return;
	}

	if (String(d["type"]) == "tab_container_tab") {
		emit_signal(SNAME("dropped"), p_point, p_data, drop_position);
	}
}

void DropOverlay::gui_input(const Ref<InputEvent> &p_event) {
	ERR_FAIL_COND(p_event.is_null());

	Ref<InputEventMouseMotion> mm = p_event;

	if (mm.is_valid()) {
		DropPosition current_drop_position = DropPosition::DROP_CENTER;
		if (position_detection) {
			current_drop_position = _get_position(mm->get_position());
		}

		// print_line("in_parent_pos: ", get_transform().xform(mm->get_position()), mm->get_position(), current_drop_position, drop_position);
		if (current_drop_position != drop_position) {
			drop_position = current_drop_position;
			queue_redraw();
		}
	}
}

void DropOverlay::enable_position_detection(bool p_enable) {
	position_detection = p_enable;
}

bool DropOverlay::get_position_detection() const {
	return position_detection;
}

void DropOverlay::_bind_methods() {
	ADD_SIGNAL(MethodInfo("dropped", PropertyInfo(Variant::VECTOR2, "point"), PropertyInfo(Variant::DICTIONARY, "data"), PropertyInfo(Variant::INT, "position")));
}

Rect2 AppTabContainer::_get_tab_rect() const {
	Rect2 rect;
	if (tabs_visible && get_tab_count() > 0) {
		rect = Rect2(theme_cache.tabbar_style->get_offset(), tabbar_panel->get_size());
		rect.position.x += is_layout_rtl() ? theme_cache.menu_icon->get_width() : theme_cache.side_margin;
	}

	return rect;
}

int AppTabContainer::_get_tab_height() const {
	int height = 0;
	if (tabs_visible && get_tab_count() > 0) {
		height = tabbar_panel->get_minimum_size().height + theme_cache.tabbar_style->get_margin(SIDE_TOP) + theme_cache.tabbar_style->get_margin(SIDE_BOTTOM);
	}

	return height;
}

void AppTabContainer::gui_input(const Ref<InputEvent> &p_event) {
	ERR_FAIL_COND(p_event.is_null());

	Ref<InputEventMouseButton> mb = p_event;

	Popup *popup = get_popup();

	if (mb.is_valid() && mb->is_pressed() && mb->get_button_index() == MouseButton::LEFT) {
		Point2 pos = mb->get_position();
		real_t content_height = get_size().height - _get_tab_height();

		Rect2 popup_rect = _get_tab_rect();
		popup_rect.position.x += is_layout_rtl() ? -theme_cache.menu_icon->get_width() : popup_rect.size.x;
		popup_rect.position.y += tabs_position == POSITION_BOTTOM ? content_height : 0;
		popup_rect.size.x = theme_cache.menu_icon->get_width();

		// Click must be on tabs in the tab header area.
		if (!tabs_visible || pos.y < popup_rect.position.y || pos.y >= popup_rect.position.y + popup_rect.size.y) {
			return;
		}

		// Handle menu button.
		if (popup) {
			if (popup_rect.has_point(pos)) {
				emit_signal(SNAME("pre_popup_pressed"));

				Vector2 popup_pos = get_screen_position();
				popup_pos.x += popup_rect.position.x + (is_layout_rtl() ? 0 : popup_rect.size.x - popup->get_size().width);
				popup_pos.y += popup_rect.position.y + popup_rect.size.y / 2.0;
				if (tabs_position == POSITION_BOTTOM) {
					popup_pos.y -= popup->get_size().height;
					popup_pos.y -= theme_cache.menu_icon->get_height() / 2.0;
				} else {
					popup_pos.y += theme_cache.menu_icon->get_height() / 2.0;
				}

				popup->set_position(popup_pos);
				popup->popup();
				return;
			}
		}
	}

	Ref<InputEventMouseMotion> mm = p_event;

	if (mm.is_valid()) {
		Point2 pos = mm->get_position();
		real_t content_height = get_size().height - _get_tab_height();

		Rect2 popup_rect = _get_tab_rect();
		popup_rect.position.x += is_layout_rtl() ? -theme_cache.menu_icon->get_width() : popup_rect.size.x;
		popup_rect.position.y += tabs_position == POSITION_BOTTOM ? content_height : 0;
		popup_rect.size.x = theme_cache.menu_icon->get_width();

		// Mouse must be on tabs in the tab header area.
		if (!tabs_visible || pos.y < popup_rect.position.y || pos.y >= popup_rect.position.y + popup_rect.size.y) {
			if (menu_hovered) {
				menu_hovered = false;
				queue_redraw();
			}
			return;
		}

		if (popup) {
			if (popup_rect.has_point(pos)) {
				if (!menu_hovered) {
					menu_hovered = true;
					queue_redraw();
					return;
				}
			} else if (menu_hovered) {
				menu_hovered = false;
				queue_redraw();
			}

			if (menu_hovered) {
				return;
			}
		}
	}
}

void AppTabContainer::unhandled_key_input(const Ref<InputEvent> &p_event) {
	if (!tab_preview_panel->is_visible()) {
		return;
	}

	Ref<InputEventKey> k = p_event;
	if (k.is_valid() && k->is_action_pressed(SNAME("ui_cancel"), false, true)) {
		tab_preview_panel->hide();
	}
}

void AppTabContainer::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			// TODO: tab name, setup_current_tab
			// If some nodes happen to be renamed outside the tree, the tab names need to be updated manually.
			if (get_tab_count() > 0) {
				_refresh_tab_names();
			}

			if (setup_current_tab >= -1) {
				set_current_tab(setup_current_tab);
				setup_current_tab = -2;
			}
		} break;

		case NOTIFICATION_READY:
		case NOTIFICATION_RESIZED: {
			_update_margins();
		} break;

		case NOTIFICATION_DRAW: {
			RID canvas = get_canvas_item();
			Size2 size = get_size();
			Rect2 tabbar_rect = _get_tab_rect();

			// Draw only the tab area if the header is hidden.
			if (!tabs_visible) {
				theme_cache.panel_style->draw(canvas, Rect2(0, 0, size.width, size.height));
				return;
			}

			int header_height = _get_tab_height();
			int header_voffset = int(tabs_position == POSITION_BOTTOM) * (size.height - header_height);

			// Draw background for the tabbar.
			theme_cache.tabbar_style->draw(canvas, Rect2(0, header_voffset, size.width, header_height));
			// Draw the background for the tab's content.
			theme_cache.panel_style->draw(canvas, Rect2(0, int(tabs_position == POSITION_TOP) * header_height, size.width, size.height - header_height));

			// Draw the popup menu.
			if (get_popup()) {
				int x = is_layout_rtl() ? tabbar_rect.position.x - theme_cache.menu_icon->get_width() : tabbar_rect.position.x + tabbar_rect.size.x;
				header_voffset += tabbar_rect.position.y;

				if (menu_hovered) {
					theme_cache.menu_hl_icon->draw(get_canvas_item(), Point2(x, header_voffset + (tabbar_rect.size.y - theme_cache.menu_hl_icon->get_height()) / 2));
				} else {
					theme_cache.menu_icon->draw(get_canvas_item(), Point2(x, header_voffset + (tabbar_rect.size.y - theme_cache.menu_icon->get_height()) / 2));
				}
			}

			_on_tab_resized();
		} break;

		case NOTIFICATION_VISIBILITY_CHANGED: {
			if (!is_visible()) {
				return;
			}

			updating_visibility = true;

			// As the visibility change notification will be triggered for all children soon after,
			// beat it to the punch and make sure that the correct node is the only one visible first.
			// Otherwise, it can prevent a tab change done right before this container was made visible.
			Vector<Control *> controls = _get_tab_controls();
			int current = setup_current_tab > -2 ? setup_current_tab : get_current_tab();
			for (int i = 0; i < controls.size(); i++) {
				controls[i]->set_visible(i == current);
			}

			updating_visibility = false;
		} break;

		case EditorSettings::NOTIFICATION_EDITOR_SETTINGS_CHANGED: {
			if (EditorSettings::get_singleton()->check_changed_settings_in_group("interface/tabs")) {
				// tab_bar->set_tab_close_display_policy((TabBar::CloseButtonDisplayPolicy)EDITOR_GET("interface/tabs/display_close_button").operator int());
				tab_bar->set_max_tab_width(int(EDITOR_GET("interface/tabs/maximum_width")) * APP_SCALE);
				_on_tab_resized();
			}
		} break;

		case NOTIFICATION_TRANSLATION_CHANGED:
		case NOTIFICATION_LAYOUT_DIRECTION_CHANGED: {
			theme_changing = true;
			callable_mp(this, &AppTabContainer::_on_theme_changed).call_deferred(); // Wait until all changed theme.
		} break;
		case NOTIFICATION_THEME_CHANGED: {
			theme_changing = true;
			callable_mp(this, &AppTabContainer::_on_theme_changed).call_deferred(); // Wait until all changed theme.

			// tabbar_panel->add_theme_style_override(SceneStringName(panel), get_theme_stylebox(SNAME("tabbar_background"), SNAME("AppTabContainer")));
			tabbar_panel->add_theme_style_override(SceneStringName(panel), theme_cache.tabbar_style);
			tab_bar->add_theme_constant_override("icon_max_width", get_theme_constant(SNAME("class_icon_size"), EditorStringName(Editor)));

			tab_add->set_button_icon(get_theme_icon(SNAME("Add"), SNAME("AppIcons")));
			tab_add->add_theme_color_override("icon_normal_color", Color(0.6f, 0.6f, 0.6f, 0.8f));

			tab_add_ph->set_custom_minimum_size(tab_add->get_minimum_size());
		} break;

		case NOTIFICATION_DRAG_BEGIN: {
			Dictionary drop_data = get_viewport()->gui_get_drag_data();

			// Check if we are dragging a tab.
			const String type = drop_data.get("type", "");
			print_line("drag type: ", type, drop_overlay->get_rect());
			if (type == "tab_container_tab") { // TODO: type
				split_dragging = true;
				drop_overlay->set_mouse_filter(MOUSE_FILTER_PASS);

				NodePath from_path = drop_data.get("from_path", NodePath());
				NodePath tab_bar_path = get_tab_bar()->get_path();
				print_line("path: ", from_path, tab_bar_path);
				if (from_path == tab_bar_path && get_child_count(false) == 1) {
					drop_overlay->enable_position_detection(false);
				} else {
					drop_overlay->enable_position_detection(true);
				}
			}
		} break;

		case NOTIFICATION_DRAG_END: {
			if (split_dragging) {
				split_dragging = false;
				drop_overlay->set_mouse_filter(MOUSE_FILTER_IGNORE);
			}
		} break;
	}
}

void AppTabContainer::_menu_option_confirm(int p_option, bool p_confirmed) {
	// TODO
	if (!p_confirmed) { // FIXME: this may be a hack.
		current_menu_option = (MenuOptions)p_option;
	}

	switch (p_option) {
		case FILE_NEW_SCENE: {
			emit_signal(SNAME("new_tab"));
		} break;
	}
}

void AppTabContainer::_update_context_menu() {
	// #define DISABLE_LAST_OPTION_IF(m_condition)                   \
	// 	if (m_condition) {                                        \
	// 		tab_bar_context_menu->set_item_disabled(-1, true); \
	// 	}

	// 	tab_bar_context_menu->clear();
	// 	tab_bar_context_menu->reset_size();

	// 	int tab_id = tab_bar->get_hovered_tab();
	// 	bool no_root_node = !EditorNode::get_editor_data().get_edited_scene_root(tab_id);

	tab_bar_context_menu->add_shortcut(ED_GET_SHORTCUT("app/new_scene"), AppTabContainer::FILE_NEW_SCENE);
	// 	if (tab_id >= 0) {
	// 		tab_bar_context_menu->add_shortcut(ED_GET_SHORTCUT("editor/save_scene"), EditorNode::SCENE_SAVE_SCENE);
	// 		DISABLE_LAST_OPTION_IF(no_root_node);
	// 		tab_bar_context_menu->add_shortcut(ED_GET_SHORTCUT("editor/save_scene_as"), EditorNode::SCENE_SAVE_AS_SCENE);
	// 		DISABLE_LAST_OPTION_IF(no_root_node);
	// 	}

	// 	bool can_save_all_scenes = false;
	// 	for (int i = 0; i < EditorNode::get_editor_data().get_edited_scene_count(); i++) {
	// 		if (!EditorNode::get_editor_data().get_scene_path(i).is_empty() && EditorNode::get_editor_data().get_edited_scene_root(i)) {
	// 			can_save_all_scenes = true;
	// 			break;
	// 		}
	// 	}
	// 	tab_bar_context_menu->add_shortcut(ED_GET_SHORTCUT("editor/save_all_scenes"), EditorNode::SCENE_SAVE_ALL_SCENES);
	// 	DISABLE_LAST_OPTION_IF(!can_save_all_scenes);

	// 	if (tab_id >= 0) {
	// 		tab_bar_context_menu->add_separator();
	// 		tab_bar_context_menu->add_item(RTR("Show in FileSystem"), SCENE_SHOW_IN_FILESYSTEM);
	// 		DISABLE_LAST_OPTION_IF(!ResourceLoader::exists(EditorNode::get_editor_data().get_scene_path(tab_id)));
	// 		tab_bar_context_menu->add_item(RTR("Play This Scene"), SCENE_RUN);
	// 		DISABLE_LAST_OPTION_IF(no_root_node);

	// 		tab_bar_context_menu->add_separator();
	// 		tab_bar_context_menu->add_shortcut(ED_GET_SHORTCUT("editor/close_scene"), EditorNode::SCENE_CLOSE);
	// 		tab_bar_context_menu->set_item_text(-1, RTR("Close Tab"));
	// 		tab_bar_context_menu->add_shortcut(ED_GET_SHORTCUT("editor/reopen_closed_scene"), EditorNode::SCENE_OPEN_PREV);
	// 		tab_bar_context_menu->set_item_text(-1, RTR("Undo Close Tab"));
	// 		DISABLE_LAST_OPTION_IF(!EditorNode::get_singleton()->has_previous_closed_scenes());
	// 		tab_bar_context_menu->add_item(RTR("Close Other Tabs"), SCENE_CLOSE_OTHERS);
	// 		DISABLE_LAST_OPTION_IF(EditorNode::get_editor_data().get_edited_scene_count() <= 1);
	// 		tab_bar_context_menu->add_item(RTR("Close Tabs to the Right"), SCENE_CLOSE_RIGHT);
	// 		DISABLE_LAST_OPTION_IF(EditorNode::get_editor_data().get_edited_scene_count() == tab_id + 1);
	// 		tab_bar_context_menu->add_item(RTR("Close All Tabs"), SCENE_CLOSE_ALL);

	// 		const PackedStringArray paths = { EditorNode::get_editor_data().get_scene_path(tab_id) };
	// 		EditorContextMenuPluginManager::get_singleton()->add_options_from_plugins(tab_bar_context_menu, EditorContextMenuPlugin::CONTEXT_SLOT_SCENE_TABS, paths);
	// 	} else {
	// 		EditorContextMenuPluginManager::get_singleton()->add_options_from_plugins(tab_bar_context_menu, EditorContextMenuPlugin::CONTEXT_SLOT_SCENE_TABS, {});
	// 	}
	// #undef DISABLE_LAST_OPTION_IF

	// last_hovered_tab = tab_id;
}

void AppTabContainer::_custom_menu_option(int p_option) {
}

void AppTabContainer::_reposition_active_tab(int p_to_index) {
	// AppTabContainer::get_editor_data().move_edited_scene_to_index(p_to_index);
	// update_scene_tabs();

	_on_tab_resized();
}

void AppTabContainer::_on_theme_changed() {
	if (!theme_changing) {
		return;
	}

	tab_bar->begin_bulk_theme_override();

	tab_bar->add_theme_style_override(SNAME("tab_unselected"), theme_cache.tab_unselected_style);
	tab_bar->add_theme_style_override(SNAME("tab_hovered"), theme_cache.tab_hovered_style);
	tab_bar->add_theme_style_override(SNAME("tab_selected"), theme_cache.tab_selected_style);
	tab_bar->add_theme_style_override(SNAME("tab_disabled"), theme_cache.tab_disabled_style);
	tab_bar->add_theme_style_override(SNAME("tab_focus"), theme_cache.tab_focus_style);

	tab_bar->add_theme_icon_override(SNAME("increment"), theme_cache.increment_icon);
	tab_bar->add_theme_icon_override(SNAME("increment_highlight"), theme_cache.increment_hl_icon);
	tab_bar->add_theme_icon_override(SNAME("decrement"), theme_cache.decrement_icon);
	tab_bar->add_theme_icon_override(SNAME("decrement_highlight"), theme_cache.decrement_hl_icon);
	tab_bar->add_theme_icon_override(SNAME("drop_mark"), theme_cache.drop_mark_icon);
	tab_bar->add_theme_color_override(SNAME("drop_mark_color"), theme_cache.drop_mark_color);

	tab_bar->add_theme_color_override(SNAME("font_selected_color"), theme_cache.font_selected_color);
	tab_bar->add_theme_color_override(SNAME("font_hovered_color"), theme_cache.font_hovered_color);
	tab_bar->add_theme_color_override(SNAME("font_unselected_color"), theme_cache.font_unselected_color);
	tab_bar->add_theme_color_override(SNAME("font_disabled_color"), theme_cache.font_disabled_color);
	tab_bar->add_theme_color_override(SNAME("font_outline_color"), theme_cache.font_outline_color);

	tab_bar->add_theme_font_override(SceneStringName(font), theme_cache.tab_font);
	tab_bar->add_theme_font_size_override(SceneStringName(font_size), theme_cache.tab_font_size);

	tab_bar->add_theme_constant_override(SNAME("h_separation"), theme_cache.icon_separation);
	tab_bar->add_theme_constant_override(SNAME("tab_separation"), theme_cache.tab_separation);
	tab_bar->add_theme_constant_override(SNAME("icon_max_width"), theme_cache.icon_max_width);
	tab_bar->add_theme_constant_override(SNAME("outline_size"), theme_cache.outline_size);

	tab_bar->end_bulk_theme_override();

	_update_margins();
	if (get_tab_count() > 0) {
		_repaint();
	} else {
		update_minimum_size();
	}
	queue_redraw();

	theme_changing = false;
}

void AppTabContainer::_repaint() {
	Vector<Control *> controls = _get_tab_controls();
	int current = get_current_tab();

	int tab_height = _get_tab_height();
	float top_margin = theme_cache.tabbar_style->get_margin(SIDE_TOP);
	float bottom_margin = theme_cache.tabbar_style->get_margin(SIDE_BOTTOM);

	// Move the TabBar to the top or bottom.
	// Don't change the left and right offsets since the TabBar will resize and may change tab offset.
	if (tabs_position == POSITION_BOTTOM) {
		tabbar_panel->set_anchor_and_offset(SIDE_BOTTOM, 1.0, -bottom_margin);
		tabbar_panel->set_anchor_and_offset(SIDE_TOP, 1.0, top_margin - tab_height);

		drop_overlay->set_anchor_and_offset(SIDE_TOP, 0.0, 0);
		drop_overlay->set_anchor_and_offset(SIDE_BOTTOM, 1.0, tab_height);
	} else {
		tabbar_panel->set_anchor_and_offset(SIDE_TOP, 0.0, top_margin);
		tabbar_panel->set_anchor_and_offset(SIDE_BOTTOM, 0.0, tab_height - bottom_margin);

		drop_overlay->set_anchor_and_offset(SIDE_TOP, 0.0, tab_height);
		drop_overlay->set_anchor_and_offset(SIDE_BOTTOM, 1.0, 0);
	}

	updating_visibility = true;
	for (int i = 0; i < controls.size(); i++) {
		Control *c = controls[i];

		if (i == current) {
			c->show();
			c->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);

			if (tabs_visible) {
				if (tabs_position == POSITION_BOTTOM) {
					c->set_offset(SIDE_BOTTOM, -tab_height);
				} else {
					c->set_offset(SIDE_TOP, tab_height);
				}
			}

			c->set_offset(SIDE_TOP, c->get_offset(SIDE_TOP) + theme_cache.panel_style->get_margin(SIDE_TOP));
			c->set_offset(SIDE_LEFT, c->get_offset(SIDE_LEFT) + theme_cache.panel_style->get_margin(SIDE_LEFT));
			c->set_offset(SIDE_RIGHT, c->get_offset(SIDE_RIGHT) - theme_cache.panel_style->get_margin(SIDE_RIGHT));
			c->set_offset(SIDE_BOTTOM, c->get_offset(SIDE_BOTTOM) - theme_cache.panel_style->get_margin(SIDE_BOTTOM));
		} else {
			c->hide();
		}
	}
	updating_visibility = false;

	update_minimum_size();
}

void AppTabContainer::_update_margins() {
	// Directly check for validity, to avoid errors when quitting.
	bool has_popup = popup_obj_id.is_valid();

	int left_margin = theme_cache.tabbar_style->get_margin(SIDE_LEFT);
	int right_margin = theme_cache.tabbar_style->get_margin(SIDE_RIGHT);

	if (is_layout_rtl()) {
		SWAP(left_margin, right_margin);
	}

	if (has_popup) {
		right_margin += theme_cache.menu_icon->get_width();
	}

	if (get_tab_count() == 0) {
		tabbar_panel->set_offset(SIDE_LEFT, left_margin);
		tabbar_panel->set_offset(SIDE_RIGHT, -right_margin);
		return;
	}

	switch (get_tab_alignment()) {
		case TabBar::ALIGNMENT_LEFT: {
			tabbar_panel->set_offset(SIDE_LEFT, left_margin + theme_cache.side_margin);
			tabbar_panel->set_offset(SIDE_RIGHT, -right_margin);
		} break;

		case TabBar::ALIGNMENT_CENTER: {
			tabbar_panel->set_offset(SIDE_LEFT, left_margin);
			tabbar_panel->set_offset(SIDE_RIGHT, -right_margin);
		} break;

		case TabBar::ALIGNMENT_RIGHT: {
			tabbar_panel->set_offset(SIDE_LEFT, left_margin);

			if (has_popup) {
				tabbar_panel->set_offset(SIDE_RIGHT, -right_margin);
				return;
			}

			int first_tab_pos = tab_bar->get_tab_rect(0).position.x;
			Rect2 last_tab_rect = tab_bar->get_tab_rect(get_tab_count() - 1);
			int total_tabs_width = left_margin + right_margin + last_tab_rect.position.x - first_tab_pos + last_tab_rect.size.width;

			// Calculate if all the tabs would still fit if the margin was present.
			if (get_clip_tabs() && (tab_bar->get_offset_buttons_visible() || (get_tab_count() > 1 && (total_tabs_width + theme_cache.side_margin) > get_size().width))) {
				tabbar_panel->set_offset(SIDE_RIGHT, -right_margin);
			} else {
				tabbar_panel->set_offset(SIDE_RIGHT, -right_margin - theme_cache.side_margin);
			}
		} break;

		case TabBar::ALIGNMENT_MAX:
			break; // Can't happen, but silences warning.
	}
}

void AppTabContainer::_on_mouse_exited() {
	if (menu_hovered) {
		menu_hovered = false;
		queue_redraw();
	}
}

Vector<Control *> AppTabContainer::_get_tab_controls() const {
	Vector<Control *> controls;
	for (int i = 0; i < get_child_count(false); i++) {
		Control *control = as_sortable_control(get_child(i, false), SortableVisibilityMode::IGNORE);
		if (!control || children_removing.has(control)) {
			continue;
		}

		controls.push_back(control);
	}

	return controls;
}

Variant AppTabContainer::_get_drag_data_fw(const Point2 &p_point, Control *p_from_control) {
	if (!drag_to_rearrange_enabled) {
		return Variant();
	}
	return tab_bar->_handle_get_drag_data("tab_container_tab", p_point);
}

bool AppTabContainer::_can_drop_data_fw(const Point2 &p_point, const Variant &p_data, Control *p_from_control) const {
	if (!drag_to_rearrange_enabled) {
		return false;
	}
	return tab_bar->_handle_can_drop_data("tab_container_tab", p_point, p_data);
}

void AppTabContainer::_drop_data_fw(const Point2 &p_point, const Variant &p_data, Control *p_from_control) {
	if (!drag_to_rearrange_enabled) {
		return;
	}
	return tab_bar->_handle_drop_data("tab_container_tab", p_point, p_data, callable_mp(this, &AppTabContainer::_drag_move_tab), callable_mp(this, &AppTabContainer::_drag_move_tab_from));
}

void AppTabContainer::_drag_move_tab(int p_from_index, int p_to_index) {
	move_child(get_tab_control(p_from_index), get_tab_control(p_to_index)->get_index(false));
}

void AppTabContainer::_drag_move_tab_from(TabBar *p_from_tabbar, int p_from_index, int p_to_index) {
	// TODO: handle layout: tab_container/tabbar_panel/tabbar_container
	AppTabContainer *from_tab_container = _get_control_parent_tab_container(p_from_tabbar);
	if (!from_tab_container) {
		return;
	}
	move_tab_from_tab_container(from_tab_container, p_from_index, p_to_index);
}

void AppTabContainer::_on_drop_data(const Point2 &p_point, const Variant &p_data, int p_position) {
	print_line("on drop data: ", p_position);
	Dictionary d = p_data;
	if (!d.has("type")) {
		return;
	}

	if (String(d["type"]) == "tab_container_tab") {
		NodePath from_path = d["from_path"];
		Node *from_node = get_node(from_path);
		if (p_position == DropPosition::DROP_CENTER && from_node == get_tab_bar()) {
			return;
		}
		emit_signal(SNAME("tab_dropped"), p_position, p_data);
	}
}

bool AppTabContainer::_is_internal_child(Node *p_node) const {
	return (p_node == tabbar_panel || p_node == drop_overlay);
}

AppTabContainer *AppTabContainer::_get_control_parent_tab_container(Control *p_control) {
	{
		AppTabContainer *tab_container = Object::cast_to<AppTabContainer>(p_control);
		if (tab_container) {
			return tab_container;
		}
	}

	Control *parent = p_control->get_parent_control();
	while (parent) {
		AppTabContainer *tab_container = Object::cast_to<AppTabContainer>(parent);
		if (tab_container) {
			return tab_container;
		}
		parent = parent->get_parent_control();
	}
	return nullptr;
}

void AppTabContainer::move_tab_from_tab_container(AppTabContainer *p_from, int p_from_index, int p_to_index) {
	ERR_FAIL_NULL(p_from);
	ERR_FAIL_INDEX(p_from_index, p_from->get_tab_count());
	ERR_FAIL_INDEX(p_to_index, get_tab_count() + 1);

	// Get the tab properties before they get erased by the child removal.
	String tab_title = p_from->get_tab_title(p_from_index);
	String tab_tooltip = p_from->get_tab_tooltip(p_from_index);
	Ref<Texture2D> tab_icon = p_from->get_tab_icon(p_from_index);
	bool tab_disabled = p_from->is_tab_disabled(p_from_index);
	bool tab_hidden = p_from->is_tab_hidden(p_from_index);
	Variant tab_metadata = p_from->get_tab_metadata(p_from_index);
	int tab_icon_max_width = p_from->get_tab_bar()->get_tab_icon_max_width(p_from_index);

	Control *moving_tabc = p_from->get_tab_control(p_from_index);
	p_from->remove_child(moving_tabc);
	add_child(moving_tabc, true);
	// TODO: owner, use reparent()?

	if (p_to_index < 0 || p_to_index > get_tab_count() - 1) {
		p_to_index = get_tab_count() - 1;
	}
	move_child(moving_tabc, get_tab_control(p_to_index)->get_index(false));

	set_tab_title(p_to_index, tab_title);
	set_tab_tooltip(p_to_index, tab_tooltip);
	set_tab_icon(p_to_index, tab_icon);
	set_tab_disabled(p_to_index, tab_disabled);
	set_tab_hidden(p_to_index, tab_hidden);
	set_tab_metadata(p_to_index, tab_metadata);
	get_tab_bar()->set_tab_icon_max_width(p_to_index, tab_icon_max_width);

	if (!is_tab_disabled(p_to_index)) {
		set_current_tab(p_to_index);
	}
}

void AppTabContainer::move_tab(const Variant &p_data, int p_to_index) {
	Dictionary d = p_data;
	if (!d.has("type")) {
		return;
	}

	if (String(d["type"]) == "tab_container_tab") {
		NodePath from_path = d["from_path"];
		Node *from_node = get_node(from_path);
		TabBar *from_tab_bar = Object::cast_to<TabBar>(from_node);
		// TODO: handle layout: tab_container/tabbar_panel/tabbar_container
		AppTabContainer *p_from = _get_control_parent_tab_container(from_tab_bar);
		if (!p_from) {
			return;
		}
		print_line("move: ", from_node, p_from);
		print_line(from_path);

		int tab_from_id = d["tab_index"];

		move_tab_from_tab_container(p_from, tab_from_id, p_to_index);
	}
}

void AppTabContainer::_on_tab_clicked(int p_tab) {
	emit_signal(SNAME("tab_clicked"), p_tab);
}

void AppTabContainer::_on_tab_closed(int p_tab) {
	close_tab(p_tab);
}

void AppTabContainer::_on_tab_hovered(int p_tab) {
	emit_signal(SNAME("tab_hovered"), p_tab);

	if (!bool(EDITOR_GET("interface/tabs/show_thumbnail_on_hover"))) {
		return;
	}

	// TODO: preview
	// // Currently the tab previews are displayed under the running game process when embed.
	// // Right now, the easiest technique to fix that is to prevent displaying the tab preview
	// // when the user is in the Game View.
	// if (EditorNode::get_singleton()->get_editor_main_screen()->get_selected_index() == EditorMainScreen::EDITOR_GAME && EditorRunBar::get_singleton()->is_playing()) {
	// 	return;
	// }

	// int current_tab = tab_bar->get_current_tab();

	// if (p_tab == current_tab || p_tab < 0) {
	// 	tab_preview_panel->hide();
	// } else {
	// 	String path = EditorNode::get_editor_data().get_scene_path(p_tab);
	// 	if (!path.is_empty()) {
	// 		EditorResourcePreview::get_singleton()->queue_resource_preview(path, this, "_tab_preview_done", p_tab);
	// 	}
	// }
}

void AppTabContainer::_on_tab_exit() {
	tab_preview_panel->hide();
}

void AppTabContainer::_on_tab_changed(int p_tab) {
	callable_mp(this, &AppTabContainer::_repaint).call_deferred();
	queue_redraw();

	tab_preview_panel->hide();

	emit_signal(SNAME("tab_changed"), p_tab);
}

void AppTabContainer::_on_tab_input(const Ref<InputEvent> &p_input) {
	Ref<InputEventMouseButton> mb = p_input;

	if (mb.is_valid()) {
		if (mb->get_button_index() == MouseButton::LEFT && mb->is_double_click()) {
			int tab_buttons = 0;
			if (tab_bar->get_offset_buttons_visible()) {
				tab_buttons = get_theme_icon(SNAME("increment"), SNAME("TabBar"))->get_width() + get_theme_icon(SNAME("decrement"), SNAME("TabBar"))->get_width();
			}

			if ((is_layout_rtl() && mb->get_position().x > tab_buttons) || (!is_layout_rtl() && mb->get_position().x < tab_bar->get_size().width - tab_buttons)) {
				trigger_menu_option(AppTabContainer::FILE_NEW_SCENE, true);
			}
		}
		if (mb->get_button_index() == MouseButton::RIGHT && mb->is_pressed()) {
			// Context menu.
			_update_context_menu();

			tab_bar_context_menu->set_position(tab_bar->get_screen_position() + mb->get_position());
			tab_bar_context_menu->reset_size();
			tab_bar_context_menu->popup();
		}
	}
}

void AppTabContainer::_on_tab_resized() {
	const Size2 add_button_size = tab_add->is_visible() ? Size2(tab_add->get_size().x, tab_bar->get_size().y) : Size2(0, tab_bar->get_size().y);
	if (tab_bar->get_offset_buttons_visible()) {
		// Move the add button to a fixed position.
		if (tab_add->get_parent() == tab_bar) {
			tab_bar->remove_child(tab_add);
			tab_add_ph->add_child(tab_add);
			tab_add->set_rect(Rect2(Point2(), add_button_size));
		}
	} else {
		// Move the add button to be after the last tab.
		if (tab_add->get_parent() == tab_add_ph) {
			tab_add_ph->remove_child(tab_add);
			tab_bar->add_child(tab_add);
		}

		if (tab_bar->get_tab_count() == 0) {
			tab_add->set_rect(Rect2(Point2(), add_button_size));
			return;
		}

		Rect2 last_tab = tab_bar->get_tab_rect(tab_bar->get_tab_count() - 1);
		int hsep = tab_bar->get_theme_constant(SNAME("h_separation"));
		if (tab_bar->is_layout_rtl()) {
			tab_add->set_rect(Rect2(Point2(last_tab.position.x - add_button_size.x - hsep, last_tab.position.y), add_button_size));
		} else {
			tab_add->set_rect(Rect2(Point2(last_tab.position.x + last_tab.size.width + hsep, last_tab.position.y), add_button_size));
		}
	}
}

void AppTabContainer::_on_tab_selected(int p_tab) {
	if (p_tab != get_previous_tab()) {
		callable_mp(this, &AppTabContainer::_repaint).call_deferred();
	}

	emit_signal(SNAME("tab_selected"), p_tab);
}

void AppTabContainer::_on_tab_button_pressed(int p_tab) {
	emit_signal(SNAME("tab_button_pressed"), p_tab);
}

void AppTabContainer::_on_active_tab_rearranged(int p_tab) {
	emit_signal(SNAME("active_tab_rearranged"), p_tab);
}

void AppTabContainer::_on_tab_visibility_changed(Control *p_child) {
	if (updating_visibility) {
		return;
	}
	int tab_index = get_tab_idx_from_control(p_child);
	if (tab_index == -1) {
		return;
	}
	// Only allow one tab to be visible.
	bool made_visible = p_child->is_visible();
	updating_visibility = true;

	if (!made_visible && get_current_tab() == tab_index) {
		if (get_deselect_enabled() || get_tab_count() == 0) {
			// Deselect.
			set_current_tab(-1);
		} else if (get_tab_count() == 1) {
			// Only tab, cannot deselect.
			p_child->show();
		} else {
			// Set a different tab to be the current tab.
			bool selected = select_next_available();
			if (!selected) {
				selected = select_previous_available();
			}
			if (!selected) {
				// No available tabs, deselect.
				set_current_tab(-1);
			}
		}
	} else if (made_visible && get_current_tab() != tab_index) {
		set_current_tab(tab_index);
	}

	updating_visibility = false;
}

void AppTabContainer::_tab_preview_done(const String &p_path, const Ref<Texture2D> &p_preview, const Ref<Texture2D> &p_small_preview, const Variant &p_udata) {
	int p_tab = p_udata;
	if (p_preview.is_valid()) {
		tab_preview->set_texture(p_preview);

		Rect2 rect = tab_bar->get_tab_rect(p_tab);
		rect.position += tab_bar->get_global_position();
		tab_preview_panel->set_global_position(rect.position + Vector2(0, rect.size.height));
		tab_preview_panel->show();
	}
}

void AppTabContainer::_refresh_tab_indices() {
	Vector<Control *> controls = _get_tab_controls();
	for (int i = 0; i < controls.size(); i++) {
		controls[i]->set_meta("_tab_index", i);
	}
}

void AppTabContainer::_refresh_tab_names() {
	Vector<Control *> controls = _get_tab_controls();
	for (int i = 0; i < controls.size(); i++) {
		// TODO: use meta name
		if (!controls[i]->has_meta("_tab_name") && String(controls[i]->get_name()) != get_tab_title(i)) {
			tab_bar->set_tab_title(i, controls[i]->get_name());
		}
	}
}

void AppTabContainer::add_child_notify(Node *p_child) {
	Container::add_child_notify(p_child);

	if (_is_internal_child(p_child)) {
		return;
	}

	Control *c = as_sortable_control(p_child, SortableVisibilityMode::IGNORE);
	if (!c) {
		return;
	}
	c->hide();

	tab_bar->add_tab(p_child->get_name());
	c->set_meta("_tab_index", tab_bar->get_tab_count() - 1);

	_update_margins();
	if (get_tab_count() == 1) {
		queue_redraw();
	}

	p_child->connect("renamed", callable_mp(this, &AppTabContainer::_refresh_tab_names));
	p_child->connect(SceneStringName(visibility_changed), callable_mp(this, &AppTabContainer::_on_tab_visibility_changed).bind(c));

	// TabBar won't emit the "tab_changed" signal when not inside the tree.
	if (!is_inside_tree()) {
		callable_mp(this, &AppTabContainer::_repaint).call_deferred();
	}
}

void AppTabContainer::move_child_notify(Node *p_child) {
	Container::move_child_notify(p_child);

	if (_is_internal_child(p_child)) {
		return;
	}

	Control *c = as_sortable_control(p_child, SortableVisibilityMode::IGNORE);
	if (c) {
		tab_bar->move_tab(c->get_meta("_tab_index"), get_tab_idx_from_control(c));
	}

	_refresh_tab_indices();
}

void AppTabContainer::remove_child_notify(Node *p_child) {
	Container::remove_child_notify(p_child);

	if (_is_internal_child(p_child)) {
		return;
	}

	Control *c = as_sortable_control(p_child, SortableVisibilityMode::IGNORE);
	if (!c) {
		return;
	}

	int idx = get_tab_idx_from_control(c);

	// As the child hasn't been removed yet, keep track of it so when the "tab_changed" signal is fired it can be ignored.
	children_removing.push_back(c);

	tab_bar->remove_tab(idx);
	_refresh_tab_indices();

	children_removing.erase(c);

	_update_margins();
	if (get_tab_count() == 0) {
		queue_redraw();
	}

	p_child->remove_meta("_tab_index");
	p_child->remove_meta("_tab_name");
	p_child->disconnect("renamed", callable_mp(this, &AppTabContainer::_refresh_tab_names));
	p_child->disconnect(SceneStringName(visibility_changed), callable_mp(this, &AppTabContainer::_on_tab_visibility_changed));

	// TabBar won't emit the "tab_changed" signal when not inside the tree.
	if (!is_inside_tree()) {
		callable_mp(this, &AppTabContainer::_repaint).call_deferred();
	}
}

TabBar *AppTabContainer::get_tab_bar() const {
	return tab_bar;
}

int AppTabContainer::get_tab_count() const {
	return tab_bar->get_tab_count();
}

void AppTabContainer::set_current_tab(int p_current) {
	if (!is_inside_tree()) {
		setup_current_tab = p_current;
		return;
	}

	tab_bar->set_current_tab(p_current);
}

int AppTabContainer::get_current_tab() const {
	return tab_bar->get_current_tab();
}

int AppTabContainer::get_previous_tab() const {
	return tab_bar->get_previous_tab();
}

bool AppTabContainer::select_previous_available() {
	return tab_bar->select_previous_available();
}

bool AppTabContainer::select_next_available() {
	return tab_bar->select_next_available();
}

void AppTabContainer::set_deselect_enabled(bool p_enabled) {
	tab_bar->set_deselect_enabled(p_enabled);
}

bool AppTabContainer::get_deselect_enabled() const {
	return tab_bar->get_deselect_enabled();
}

Control *AppTabContainer::get_tab_control(int p_idx) const {
	Vector<Control *> controls = _get_tab_controls();
	if (p_idx >= 0 && p_idx < controls.size()) {
		return controls[p_idx];
	} else {
		return nullptr;
	}
}

Control *AppTabContainer::get_current_tab_control() const {
	return get_tab_control(tab_bar->get_current_tab());
}

int AppTabContainer::get_tab_idx_at_point(const Point2 &p_point) const {
	return tab_bar->get_tab_idx_at_point(p_point);
}

int AppTabContainer::get_tab_idx_from_control(Control *p_child) const {
	ERR_FAIL_NULL_V(p_child, -1);
	ERR_FAIL_COND_V(p_child->get_parent() != this, -1);

	Vector<Control *> controls = _get_tab_controls();
	for (int i = 0; i < controls.size(); i++) {
		if (controls[i] == p_child) {
			return i;
		}
	}

	return -1;
}

void AppTabContainer::set_tab_alignment(TabBar::AlignmentMode p_alignment) {
	if (tab_bar->get_tab_alignment() == p_alignment) {
		return;
	}

	tab_bar->set_tab_alignment(p_alignment);
	_update_margins();
}

TabBar::AlignmentMode AppTabContainer::get_tab_alignment() const {
	return tab_bar->get_tab_alignment();
}

void AppTabContainer::set_tabs_position(TabPosition p_tabs_position) {
	ERR_FAIL_INDEX(p_tabs_position, POSITION_MAX);
	if (p_tabs_position == tabs_position) {
		return;
	}
	tabs_position = p_tabs_position;

	tab_bar->set_tab_style_v_flip(tabs_position == POSITION_BOTTOM);

	callable_mp(this, &AppTabContainer::_repaint).call_deferred();
	queue_redraw();
}

AppTabContainer::TabPosition AppTabContainer::get_tabs_position() const {
	return tabs_position;
}

void AppTabContainer::set_tab_focus_mode(Control::FocusMode p_focus_mode) {
	tab_bar->set_focus_mode(p_focus_mode);
}

Control::FocusMode AppTabContainer::get_tab_focus_mode() const {
	return tab_bar->get_focus_mode();
}

void AppTabContainer::set_clip_tabs(bool p_clip_tabs) {
	tab_bar->set_clip_tabs(p_clip_tabs);
}

bool AppTabContainer::get_clip_tabs() const {
	return tab_bar->get_clip_tabs();
}

void AppTabContainer::set_tabs_visible(bool p_visible) {
	if (p_visible == tabs_visible) {
		return;
	}

	tabs_visible = p_visible;
	tab_bar->set_visible(tabs_visible);

	callable_mp(this, &AppTabContainer::_repaint).call_deferred();
	queue_redraw();
}

bool AppTabContainer::are_tabs_visible() const {
	return tabs_visible;
}

void AppTabContainer::set_tab_title(int p_tab, const String &p_title) {
	Control *child = get_tab_control(p_tab);
	ERR_FAIL_NULL(child);

	if (tab_bar->get_tab_title(p_tab) == p_title) {
		return;
	}

	tab_bar->set_tab_title(p_tab, p_title);

	if (p_title == child->get_name()) {
		child->remove_meta("_tab_name");
	} else {
		child->set_meta("_tab_name", p_title);
	}

	_repaint();
	queue_redraw();
}

String AppTabContainer::get_tab_title(int p_tab) const {
	return tab_bar->get_tab_title(p_tab);
}

void AppTabContainer::set_tab_tooltip(int p_tab, const String &p_tooltip) {
	tab_bar->set_tab_tooltip(p_tab, p_tooltip);
}

String AppTabContainer::get_tab_tooltip(int p_tab) const {
	return tab_bar->get_tab_tooltip(p_tab);
}

void AppTabContainer::set_tab_icon(int p_tab, const Ref<Texture2D> &p_icon) {
	if (tab_bar->get_tab_icon(p_tab) == p_icon) {
		return;
	}

	tab_bar->set_tab_icon(p_tab, p_icon);

	_update_margins();
	_repaint();
	queue_redraw();
}

Ref<Texture2D> AppTabContainer::get_tab_icon(int p_tab) const {
	return tab_bar->get_tab_icon(p_tab);
}

void AppTabContainer::set_tab_icon_max_width(int p_tab, int p_width) {
	if (tab_bar->get_tab_icon_max_width(p_tab) == p_width) {
		return;
	}

	tab_bar->set_tab_icon_max_width(p_tab, p_width);

	_update_margins();
	_repaint();
	queue_redraw();
}

int AppTabContainer::get_tab_icon_max_width(int p_tab) const {
	return tab_bar->get_tab_icon_max_width(p_tab);
}

void AppTabContainer::set_tab_disabled(int p_tab, bool p_disabled) {
	if (tab_bar->is_tab_disabled(p_tab) == p_disabled) {
		return;
	}

	tab_bar->set_tab_disabled(p_tab, p_disabled);

	_update_margins();
	if (!get_clip_tabs()) {
		update_minimum_size();
	}
}

bool AppTabContainer::is_tab_disabled(int p_tab) const {
	return tab_bar->is_tab_disabled(p_tab);
}

void AppTabContainer::set_tab_hidden(int p_tab, bool p_hidden) {
	Control *child = get_tab_control(p_tab);
	ERR_FAIL_NULL(child);

	if (tab_bar->is_tab_hidden(p_tab) == p_hidden) {
		return;
	}

	tab_bar->set_tab_hidden(p_tab, p_hidden);
	child->hide();

	_update_margins();
	if (!get_clip_tabs()) {
		update_minimum_size();
	}
	callable_mp(this, &AppTabContainer::_repaint).call_deferred();
}

bool AppTabContainer::is_tab_hidden(int p_tab) const {
	return tab_bar->is_tab_hidden(p_tab);
}

void AppTabContainer::set_tab_metadata(int p_tab, const Variant &p_metadata) {
	tab_bar->set_tab_metadata(p_tab, p_metadata);
}

Variant AppTabContainer::get_tab_metadata(int p_tab) const {
	return tab_bar->get_tab_metadata(p_tab);
}

void AppTabContainer::set_new_tab_enabled(bool p_enabled) {
	if (new_tab_enabled == p_enabled) {
		return;
	}

	if (p_enabled) {
		get_tab_bar()->set_tab_close_display_policy(TabBar::CLOSE_BUTTON_SHOW_ALWAYS);
		tab_add->show();
		tab_add_ph->show();
	} else {
		get_tab_bar()->set_tab_close_display_policy(TabBar::CLOSE_BUTTON_SHOW_NEVER);
		tab_add->hide();
		tab_add_ph->hide();
	}
	new_tab_enabled = p_enabled;
}

bool AppTabContainer::get_new_tab_enabled() const {
	return new_tab_enabled;
}

Size2 AppTabContainer::get_minimum_size() const {
	Size2 ms;

	if (tabs_visible) {
		ms = tab_bar->get_minimum_size();
		ms.width += theme_cache.tabbar_style->get_margin(SIDE_LEFT) + theme_cache.tabbar_style->get_margin(SIDE_RIGHT);
		ms.height += theme_cache.tabbar_style->get_margin(SIDE_TOP) + theme_cache.tabbar_style->get_margin(SIDE_BOTTOM);

		if (!get_clip_tabs()) {
			if (get_popup()) {
				ms.width += theme_cache.menu_icon->get_width();
			}

			if (theme_cache.side_margin > 0 && get_tab_alignment() != TabBar::ALIGNMENT_CENTER &&
					(get_tab_alignment() != TabBar::ALIGNMENT_RIGHT || !get_popup())) {
				ms.width += theme_cache.side_margin;
			}
		}
	}

	Vector<Control *> controls = _get_tab_controls();
	Size2 largest_child_min_size;
	for (int i = 0; i < controls.size(); i++) {
		Control *c = controls[i];

		if (!c->is_visible() && !use_hidden_tabs_for_min_size) {
			continue;
		}

		Size2 cms = c->get_combined_minimum_size();
		largest_child_min_size = largest_child_min_size.max(cms);
	}
	ms.height += largest_child_min_size.height;

	Size2 panel_ms = theme_cache.panel_style->get_minimum_size();

	ms.width = MAX(ms.width, largest_child_min_size.width + panel_ms.width);
	ms.height += panel_ms.height;

	return ms;
}

void AppTabContainer::set_popup(Node *p_popup) {
	bool had_popup = get_popup();

	Popup *popup = Object::cast_to<Popup>(p_popup);
	ObjectID popup_id = popup ? popup->get_instance_id() : ObjectID();
	if (popup_obj_id == popup_id) {
		return;
	}
	popup_obj_id = popup_id;

	if (had_popup != bool(popup)) {
		queue_redraw();
		_update_margins();
		if (!get_clip_tabs()) {
			update_minimum_size();
		}
	}
}

Popup *AppTabContainer::get_popup() const {
	if (popup_obj_id.is_valid()) {
		Popup *popup = ObjectDB::get_instance<Popup>(popup_obj_id);
		if (popup) {
			return popup;
		} else {
#ifdef DEBUG_ENABLED
			ERR_PRINT("Popup assigned to AppTabContainer is gone!");
#endif
			popup_obj_id = ObjectID();
		}
	}

	return nullptr;
}

void AppTabContainer::trigger_menu_option(int p_option, bool p_confirmed) {
	_menu_option_confirm(p_option, p_confirmed);
}

// TODO: remove drag_to_rearrange_enabled?
void AppTabContainer::set_drag_to_rearrange_enabled(bool p_enabled) {
	drag_to_rearrange_enabled = p_enabled;
}

bool AppTabContainer::get_drag_to_rearrange_enabled() const {
	return drag_to_rearrange_enabled;
}

void AppTabContainer::set_tabs_rearrange_group(int p_group_id) {
	tab_bar->set_tabs_rearrange_group(p_group_id);
}

int AppTabContainer::get_tabs_rearrange_group() const {
	return tab_bar->get_tabs_rearrange_group();
}

void AppTabContainer::set_use_hidden_tabs_for_min_size(bool p_use_hidden_tabs) {
	if (use_hidden_tabs_for_min_size == p_use_hidden_tabs) {
		return;
	}

	use_hidden_tabs_for_min_size = p_use_hidden_tabs;
	update_minimum_size();
}

bool AppTabContainer::get_use_hidden_tabs_for_min_size() const {
	return use_hidden_tabs_for_min_size;
}

Vector<int> AppTabContainer::get_allowed_size_flags_horizontal() const {
	return Vector<int>();
}

Vector<int> AppTabContainer::get_allowed_size_flags_vertical() const {
	return Vector<int>();
}

void AppTabContainer::close_tab(int p_tab) {
	emit_signal("tab_closed", p_tab);

	Node *control = get_child(p_tab, false);
	if (control != nullptr) {
		remove_child(control);
		control->queue_free();
	}
}

void AppTabContainer::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_new_tab_enabled", "new_tab_enabled"), &AppTabContainer::set_new_tab_enabled);
	ClassDB::bind_method(D_METHOD("get_new_tab_enabled"), &AppTabContainer::get_new_tab_enabled);

	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "new_tab_enabled"), "set_new_tab_enabled", "get_new_tab_enabled");

	ADD_SIGNAL(MethodInfo("active_tab_rearranged", PropertyInfo(Variant::INT, "idx_to")));
	ADD_SIGNAL(MethodInfo("tab_changed", PropertyInfo(Variant::INT, "tab")));
	ADD_SIGNAL(MethodInfo("tab_clicked", PropertyInfo(Variant::INT, "tab")));
	ADD_SIGNAL(MethodInfo("tab_closed", PropertyInfo(Variant::INT, "tab")));
	ADD_SIGNAL(MethodInfo("tab_hovered", PropertyInfo(Variant::INT, "tab")));
	ADD_SIGNAL(MethodInfo("tab_selected", PropertyInfo(Variant::INT, "tab")));
	ADD_SIGNAL(MethodInfo("tab_button_pressed", PropertyInfo(Variant::INT, "tab")));
	ADD_SIGNAL(MethodInfo("pre_popup_pressed"));

	ADD_SIGNAL(MethodInfo("new_tab"));
	ADD_SIGNAL(MethodInfo("tab_dropped", PropertyInfo(Variant::INT, "position"), PropertyInfo(Variant::DICTIONARY, "data")));

	// ADD_PROPERTY(PropertyInfo(Variant::INT, "tab_alignment", PROPERTY_HINT_ENUM, "Left,Center,Right"), "set_tab_alignment", "get_tab_alignment");
	// ADD_PROPERTY(PropertyInfo(Variant::INT, "current_tab", PROPERTY_HINT_RANGE, "-1,4096,1"), "set_current_tab", "get_current_tab");
	// ADD_PROPERTY(PropertyInfo(Variant::INT, "tabs_position", PROPERTY_HINT_ENUM, "Top,Bottom"), "set_tabs_position", "get_tabs_position");
	// ADD_PROPERTY(PropertyInfo(Variant::BOOL, "clip_tabs"), "set_clip_tabs", "get_clip_tabs");
	// ADD_PROPERTY(PropertyInfo(Variant::BOOL, "tabs_visible"), "set_tabs_visible", "are_tabs_visible");
	// ADD_PROPERTY(PropertyInfo(Variant::BOOL, "drag_to_rearrange_enabled"), "set_drag_to_rearrange_enabled", "get_drag_to_rearrange_enabled");
	// ADD_PROPERTY(PropertyInfo(Variant::INT, "tabs_rearrange_group"), "set_tabs_rearrange_group", "get_tabs_rearrange_group");
	// ADD_PROPERTY(PropertyInfo(Variant::BOOL, "use_hidden_tabs_for_min_size"), "set_use_hidden_tabs_for_min_size", "get_use_hidden_tabs_for_min_size");
	// ADD_PROPERTY(PropertyInfo(Variant::INT, "tab_focus_mode", PROPERTY_HINT_ENUM, "None,Click,All"), "set_tab_focus_mode", "get_tab_focus_mode");
	// ADD_PROPERTY(PropertyInfo(Variant::BOOL, "deselect_enabled"), "set_deselect_enabled", "get_deselect_enabled");

	ADD_CLASS_DEPENDENCY("TabBar");
	ADD_CLASS_DEPENDENCY("Button");

	BIND_ENUM_CONSTANT(POSITION_TOP);
	BIND_ENUM_CONSTANT(POSITION_BOTTOM);
	BIND_ENUM_CONSTANT(POSITION_MAX);

	BIND_THEME_ITEM(Theme::DATA_TYPE_CONSTANT, AppTabContainer, side_margin);
	BIND_THEME_ITEM(Theme::DATA_TYPE_CONSTANT, AppTabContainer, tab_separation);

	BIND_THEME_ITEM_CUSTOM(Theme::DATA_TYPE_STYLEBOX, AppTabContainer, panel_style, "panel");
	BIND_THEME_ITEM_CUSTOM(Theme::DATA_TYPE_STYLEBOX, AppTabContainer, tabbar_style, "tabbar_background");

	BIND_THEME_ITEM_CUSTOM(Theme::DATA_TYPE_ICON, AppTabContainer, menu_icon, "menu");
	BIND_THEME_ITEM_CUSTOM(Theme::DATA_TYPE_ICON, AppTabContainer, menu_hl_icon, "menu_highlight");

	// TabBar overrides.
	BIND_THEME_ITEM(Theme::DATA_TYPE_CONSTANT, AppTabContainer, icon_separation);
	BIND_THEME_ITEM(Theme::DATA_TYPE_CONSTANT, AppTabContainer, icon_max_width);

	BIND_THEME_ITEM_CUSTOM(Theme::DATA_TYPE_STYLEBOX, AppTabContainer, tab_unselected_style, "tab_unselected");
	BIND_THEME_ITEM_CUSTOM(Theme::DATA_TYPE_STYLEBOX, AppTabContainer, tab_hovered_style, "tab_hovered");
	BIND_THEME_ITEM_CUSTOM(Theme::DATA_TYPE_STYLEBOX, AppTabContainer, tab_selected_style, "tab_selected");
	BIND_THEME_ITEM_CUSTOM(Theme::DATA_TYPE_STYLEBOX, AppTabContainer, tab_disabled_style, "tab_disabled");
	BIND_THEME_ITEM_CUSTOM(Theme::DATA_TYPE_STYLEBOX, AppTabContainer, tab_focus_style, "tab_focus");

	BIND_THEME_ITEM_CUSTOM(Theme::DATA_TYPE_ICON, AppTabContainer, increment_icon, "increment");
	BIND_THEME_ITEM_CUSTOM(Theme::DATA_TYPE_ICON, AppTabContainer, increment_hl_icon, "increment_highlight");
	BIND_THEME_ITEM_CUSTOM(Theme::DATA_TYPE_ICON, AppTabContainer, decrement_icon, "decrement");
	BIND_THEME_ITEM_CUSTOM(Theme::DATA_TYPE_ICON, AppTabContainer, decrement_hl_icon, "decrement_highlight");
	BIND_THEME_ITEM_CUSTOM(Theme::DATA_TYPE_ICON, AppTabContainer, drop_mark_icon, "drop_mark");
	BIND_THEME_ITEM(Theme::DATA_TYPE_COLOR, AppTabContainer, drop_mark_color);

	BIND_THEME_ITEM(Theme::DATA_TYPE_COLOR, AppTabContainer, font_selected_color);
	BIND_THEME_ITEM(Theme::DATA_TYPE_COLOR, AppTabContainer, font_hovered_color);
	BIND_THEME_ITEM(Theme::DATA_TYPE_COLOR, AppTabContainer, font_unselected_color);
	BIND_THEME_ITEM(Theme::DATA_TYPE_COLOR, AppTabContainer, font_disabled_color);
	BIND_THEME_ITEM(Theme::DATA_TYPE_COLOR, AppTabContainer, font_outline_color);

	BIND_THEME_ITEM_CUSTOM(Theme::DATA_TYPE_FONT, AppTabContainer, tab_font, "font");
	BIND_THEME_ITEM_CUSTOM(Theme::DATA_TYPE_FONT_SIZE, AppTabContainer, tab_font_size, "font_size");
	BIND_THEME_ITEM(Theme::DATA_TYPE_CONSTANT, AppTabContainer, outline_size);
}

// TODO: settings string "interface/tabs/xxx"
AppTabContainer::AppTabContainer() {
	set_process_unhandled_key_input(true);

	tabbar_panel = memnew(PanelContainer);
	add_child(tabbar_panel, false, INTERNAL_MODE_FRONT);
	tabbar_panel->set_anchors_and_offsets_preset(Control::PRESET_TOP_WIDE);

	tabbar_container = memnew(HBoxContainer);
	tabbar_panel->add_child(tabbar_container);
	// add_child(tabbar_container, false, INTERNAL_MODE_FRONT);
	// tabbar_container->set_use_parent_material(true);
	// tabbar_container->set_anchors_and_offsets_preset(Control::PRESET_TOP_WIDE);

	tab_bar = memnew(TabBar);
	SET_DRAG_FORWARDING_GCDU(tab_bar, AppTabContainer);
	tabbar_container->add_child(tab_bar);
	tab_bar->set_select_with_rmb(true);
	tab_bar->set_max_tab_width(int(EDITOR_GET("interface/tabs/maximum_width")) * APP_SCALE);
	tab_bar->set_drag_to_rearrange_enabled(true); // TODO: handle set_drag_to_rearrange_enabled
	tab_bar->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
	tab_bar->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	// tab_bar->set_use_parent_material(true);
	tab_bar->set_anchors_and_offsets_preset(Control::PRESET_TOP_WIDE);
	tab_bar->connect("tab_changed", callable_mp(this, &AppTabContainer::_on_tab_changed));
	tab_bar->connect("tab_clicked", callable_mp(this, &AppTabContainer::_on_tab_clicked));
	tab_bar->connect("tab_hovered", callable_mp(this, &AppTabContainer::_on_tab_hovered));
	tab_bar->connect("tab_selected", callable_mp(this, &AppTabContainer::_on_tab_selected));
	tab_bar->connect("tab_button_pressed", callable_mp(this, &AppTabContainer::_on_tab_button_pressed));
	tab_bar->connect("tab_close_pressed", callable_mp(this, &AppTabContainer::_on_tab_closed));
	tab_bar->connect(SceneStringName(mouse_exited), callable_mp(this, &AppTabContainer::_on_tab_exit));
	tab_bar->connect(SceneStringName(gui_input), callable_mp(this, &AppTabContainer::_on_tab_input));
	tab_bar->connect("active_tab_rearranged", callable_mp(this, &AppTabContainer::_on_active_tab_rearranged));
	tab_bar->connect(SceneStringName(resized), callable_mp(this, &AppTabContainer::_on_tab_resized), CONNECT_DEFERRED);

	tab_bar_context_menu = memnew(PopupMenu);
	tabbar_container->add_child(tab_bar_context_menu);
	tab_bar_context_menu->connect(SceneStringName(id_pressed), callable_mp(this, &AppTabContainer::trigger_menu_option).bind(false));
	// tab_bar_context_menu->connect(SceneStringName(id_pressed), callable_mp(this, &AppTabContainer::_custom_menu_option));

	tab_add = memnew(Button);
	tab_add->set_flat(true);
	tab_add->set_tooltip_text(RTR("Add a new scene.")); // TODO: scene -> pane
	tab_bar->add_child(tab_add);
	tab_add->connect(SceneStringName(pressed), callable_mp(this, &AppTabContainer::trigger_menu_option).bind(AppTabContainer::FILE_NEW_SCENE, false));
	tab_add->hide();

	tab_add_ph = memnew(Control);
	tab_add_ph->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	tab_add_ph->set_custom_minimum_size(tab_add->get_minimum_size());
	tabbar_container->add_child(tab_add_ph);

	// new_tab_enabled false
	tab_bar->set_tab_close_display_policy(TabBar::CLOSE_BUTTON_SHOW_NEVER);
	tab_add->hide();
	tab_add_ph->hide();

	// On-hover tab preview.

	Control *tab_preview_anchor = memnew(Control);
	tab_preview_anchor->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	tabbar_container->add_child(tab_preview_anchor);

	tab_preview_panel = memnew(Panel);
	tab_preview_panel->set_size(Size2(100, 100) * APP_SCALE);
	tab_preview_panel->hide();
	tab_preview_panel->set_self_modulate(Color(1, 1, 1, 0.7));
	tab_preview_anchor->add_child(tab_preview_panel);

	tab_preview = memnew(TextureRect);
	tab_preview->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
	tab_preview->set_size(Size2(96, 96) * APP_SCALE);
	tab_preview->set_position(Point2(2, 2) * APP_SCALE);
	tab_preview_panel->add_child(tab_preview);

	drop_overlay = memnew(DropOverlay);
	drop_overlay->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	drop_overlay->set_mouse_filter(MOUSE_FILTER_IGNORE);
	// drop_overlay->set_visible(false);
	add_child(drop_overlay, false, INTERNAL_MODE_BACK);
	// drop_overlay->set_anchors_and_offsets_preset(Control::PRESET_BOTTOM_WIDE);
	drop_overlay->connect("dropped", callable_mp(this, &AppTabContainer::_on_drop_data));

	connect(SceneStringName(mouse_exited), callable_mp(this, &AppTabContainer::_on_mouse_exited));
}

AppTabContainer::~AppTabContainer() {
}
