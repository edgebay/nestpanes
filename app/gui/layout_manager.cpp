#include "layout_manager.h"

#include "app/settings/app_settings.h"

#include "app/gui/app_tab_container.h"
#include "app/gui/multi_split_container.h"
#include "app/gui/pane_base.h"
#include "app/gui/pane_manager.h"
#include "scene/gui/popup_menu.h"

#include "app/app_modules/file_management/gui/file_pane.h"
#include "app/app_modules/file_management/gui/navigation_pane.h"

LayoutManager *LayoutManager::singleton = nullptr;

// Container.
void LayoutManager::_menu_id_pressed(int p_option) {
	if (!selected_tab_container) {
		return;
	}

	MultiSplitContainer::SplitDirection direction = MultiSplitContainer::SPLIT_RIGHT;
	switch (p_option) {
		case SPLIT_MENU_UP:
			direction = MultiSplitContainer::SPLIT_UP;
			break;
		case SPLIT_MENU_DOWN:
			direction = MultiSplitContainer::SPLIT_DOWN;
			break;
		case SPLIT_MENU_LEFT:
			direction = MultiSplitContainer::SPLIT_LEFT;
			break;
		case SPLIT_MENU_RIGHT:
			direction = MultiSplitContainer::SPLIT_RIGHT;
			break;

		default:
			return;
	}

	AppTabContainer *tab_container = _split(selected_tab_container, direction);
	create_new_tab("", tab_container); // TODO: type

	selected_tab_container = nullptr;
}

void LayoutManager::_select_tab_container(AppTabContainer *p_tab_container) {
	selected_tab_container = p_tab_container;
}

void LayoutManager::_tab_container_child_order_changed(AppTabContainer *p_tab_container) {
	if (p_tab_container->get_child_count(false) == 0) {
		MultiSplitContainer *split_container = Object::cast_to<MultiSplitContainer>(p_tab_container->get_parent());
		if (!split_container) {
			return;
		}

		// The last tab container in the main split container.
		if (split_containers.find(split_container) && split_container->get_child_count(false) == 1) {
			// TODO: new tab?
			// p_tab_container->add_child();
		} else {
			split_container->remove(p_tab_container);
			tab_containers.erase(p_tab_container);
			p_tab_container->queue_free();

			if (current_tab_container == p_tab_container) {
				clear_current_tab_container();
			}
		}
	}
}

void LayoutManager::_move_tab_control(TabBar *p_from_tab_bar, int p_from_index, AppTabContainer *p_to) {
	AppTabContainer *tab_container = AppTabContainer::get_control_parent_tab_container(p_from_tab_bar);
	if (!tab_container) {
		return;
	}

	int to_index = p_to->get_tab_count();
	p_to->move_tab_from_tab_container(tab_container, p_from_index, to_index);

	set_current_tab_container(p_to);
}

void LayoutManager::_on_drop_tab(int p_position, TabBar *p_from_tab_bar, int p_from_index, AppTabContainer *p_tab_container) {
	// Move tab.
	if (p_position == AppTabContainer::DropPosition::DROP_CENTER) {
		_move_tab_control(p_from_tab_bar, p_from_index, p_tab_container);
		return;
	}

	// Split.
	MultiSplitContainer::SplitDirection direction = MultiSplitContainer::SPLIT_RIGHT;
	switch (p_position) {
		case AppTabContainer::DropPosition::DROP_UP:
			direction = MultiSplitContainer::SPLIT_UP;
			break;
		case AppTabContainer::DropPosition::DROP_DOWN:
			direction = MultiSplitContainer::SPLIT_DOWN;
			break;
		case AppTabContainer::DropPosition::DROP_LEFT:
			direction = MultiSplitContainer::SPLIT_LEFT;
			break;
		case AppTabContainer::DropPosition::DROP_RIGHT:
			direction = MultiSplitContainer::SPLIT_RIGHT;
			break;

		default:
			return;
	}
	AppTabContainer *tab_container = _split(p_tab_container, direction);
	_move_tab_control(p_from_tab_bar, p_from_index, tab_container);
}

void LayoutManager::_on_pane_title_changed(PaneBase *p_pane) {
	AppTabContainer *tab_container = Object::cast_to<AppTabContainer>(p_pane->get_parent());
	int tab_index = p_pane->get_index(false);
	tab_container->set_tab_title(tab_index, p_pane->get_title());
}

void LayoutManager::_on_pane_icon_changed(PaneBase *p_pane) {
	AppTabContainer *tab_container = Object::cast_to<AppTabContainer>(p_pane->get_parent());
	int tab_index = p_pane->get_index(false);
	tab_container->set_tab_icon(tab_index, p_pane->get_icon());
}

void LayoutManager::_on_pane_data_changed(PaneBase *p_pane) {
	save_layout();
}

AppTabContainer *LayoutManager::_split(AppTabContainer *p_from, int p_direction) {
	MultiSplitContainer *split_container = Object::cast_to<MultiSplitContainer>(p_from->get_parent());
	bool tab_closable = get_tab_closable(split_container);
	int group_id = get_tabs_rearrange_group(split_container);

	AppTabContainer *tab_container = create_tab_container(tab_closable, group_id);
	split_container->split(tab_container, p_from, (MultiSplitContainer::SplitDirection)p_direction);

	set_current_tab_container(tab_container);

	return tab_container;
}

void LayoutManager::_on_current_pane_changed(PaneBase *p_pane) {
	if (!load_layout_done) {
		return;
	}

	AppTabContainer *tab_container = AppTabContainer::get_control_parent_tab_container(p_pane);
	if (!tab_container) {
		return;
	}

	set_current_tab_container(tab_container);
}

PopupMenu *LayoutManager::get_popup() const {
	return popup_menu;
}

AppTabContainer *LayoutManager::create_tab_container(bool p_tab_closable, int p_group_id) {
	AppTabContainer *tab_container = memnew(AppTabContainer);
	tab_container->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	tab_container->set_v_size_flags(Control::SIZE_EXPAND_FILL);

	// tab_container->set_theme_type_variation("TabContainerOdd");	// TODO: theme
	tab_container->set_new_tab_enabled(p_tab_closable);
	tab_container->set_drag_to_rearrange_enabled(true);
	tab_container->set_tabs_rearrange_group(p_group_id);

	// TODO: split, new tab
	// if (popup_menu) {
	// 	tab_container->set_popup(popup_menu);
	// }

	tab_container->connect("pre_popup_pressed", callable_mp(this, &LayoutManager::_select_tab_container).bind(tab_container));
	tab_container->connect("new_tab", callable_mp(this, &LayoutManager::create_new_tab).bind("", tab_container, Callable())); // TODO: type
	tab_container->connect("child_order_changed", callable_mp(this, &LayoutManager::_tab_container_child_order_changed).bind(tab_container));
	tab_container->connect("tab_dropped", callable_mp(this, &LayoutManager::_on_drop_tab).bind(tab_container));
	tab_container->connect("tab_closed", callable_mp(this, &LayoutManager::_close_tab_control).bind(tab_container));

	tab_containers.push_back(tab_container);
	set_current_tab_container(tab_container);

	return tab_container;
}

MultiSplitContainer *LayoutManager::create_split_container() {
	MultiSplitContainer *split_container = memnew(MultiSplitContainer);
	split_container->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);

	split_containers.push_back(split_container);

	return split_container;
}

void LayoutManager::set_current_tab_container(AppTabContainer *p_tab_container) {
	if (p_tab_container && p_tab_container != current_tab_container) {
		if (current_tab_container != nullptr) {
			prev_tab_container = current_tab_container;
		}
		current_tab_container = p_tab_container;
	}
}

void LayoutManager::clear_current_tab_container() {
	if (prev_tab_container == current_tab_container) {
		prev_tab_container = nullptr;
	}
	current_tab_container = nullptr;
}

AppTabContainer *LayoutManager::get_current_tab_container() const {
	return current_tab_container;
}

AppTabContainer *LayoutManager::get_prev_tab_container() const {
	return prev_tab_container;
}

void LayoutManager::_set_tab_closable(MultiSplitContainer *p_split_container, bool p_closable) {
	TypedArray<Node> children = p_split_container->get_children(false);
	if (children.is_empty()) {
		return;
	}

	for (Variant &c : children) {
		AppTabContainer *tab_container = Object::cast_to<AppTabContainer>(c);
		if (tab_container) {
			tab_container->set_new_tab_enabled(p_closable);
			continue;
		}

		MultiSplitContainer *split_container = Object::cast_to<MultiSplitContainer>(c);
		if (split_container) {
			_set_tab_closable(split_container, p_closable);
			continue;
		}
	}
}

void LayoutManager::_set_tabs_rearrange_group(MultiSplitContainer *p_split_container, int p_group_id) {
	TypedArray<Node> children = p_split_container->get_children(false);
	if (children.is_empty()) {
		return;
	}

	for (Variant &c : children) {
		AppTabContainer *tab_container = Object::cast_to<AppTabContainer>(c);
		if (tab_container) {
			tab_container->set_tabs_rearrange_group(p_group_id);
			continue;
		}

		MultiSplitContainer *split_container = Object::cast_to<MultiSplitContainer>(c);
		if (split_container) {
			_set_tabs_rearrange_group(split_container, p_group_id);
			continue;
		}
	}
}

void LayoutManager::set_tab_closable(MultiSplitContainer *p_split_container, bool p_closable) {
	bool tab_closable = p_split_container->get_meta("tab_closable");
	if (tab_closable == p_closable) {
		return;
	}

	p_split_container->set_meta("tab_closable", p_closable);
	_set_tab_closable(p_split_container, p_closable);
}

bool LayoutManager::get_tab_closable(MultiSplitContainer *p_split_container) const {
	ERR_FAIL_NULL_V(p_split_container, false);
	MultiSplitContainer *split_container = MultiSplitContainer::get_root_split_container(p_split_container);
	return split_container->get_meta("tab_closable");
}

void LayoutManager::set_tabs_rearrange_group(MultiSplitContainer *p_split_container, int p_group_id) {
	int group_id = p_split_container->get_meta("group_id");
	if (group_id == p_group_id) {
		return;
	}

	p_split_container->set_meta("group_id", p_group_id);
	_set_tabs_rearrange_group(p_split_container, p_group_id);
}

int LayoutManager::get_tabs_rearrange_group(MultiSplitContainer *p_split_container) const {
	ERR_FAIL_NULL_V(p_split_container, -1);
	MultiSplitContainer *split_container = MultiSplitContainer::get_root_split_container(p_split_container);
	return split_container->get_meta("group_id");
}

void LayoutManager::create_new_tab(const StringName &p_type, AppTabContainer *p_tab_container, const Callable &p_callback) {
	AppTabContainer *tab_container = nullptr;

	if (p_tab_container) {
		tab_container = p_tab_container;
	}
	// TODO
	else if (current_tab_container && get_tab_closable(Object::cast_to<MultiSplitContainer>(current_tab_container->get_parent()))) {
		tab_container = current_tab_container;
	} else if (prev_tab_container && get_tab_closable(Object::cast_to<MultiSplitContainer>(prev_tab_container->get_parent()))) {
		tab_container = prev_tab_container;
	} else {
		return;
	}

	PaneBase *pane = PaneManager::get_singleton()->create(p_type);
	if (!pane) {
		return;
	}

	int tab_index = tab_container->get_tab_count();
	tab_container->add_child(pane);
	pane->connect("title_changed", callable_mp(this, &LayoutManager::_on_pane_title_changed).bind(pane));
	pane->connect("icon_changed", callable_mp(this, &LayoutManager::_on_pane_icon_changed).bind(pane));
	pane->connect("data_changed", callable_mp(this, &LayoutManager::_on_pane_data_changed).bind(pane));

	tab_container->set_tab_icon(tab_index, pane->get_icon());
	tab_container->set_tab_title(tab_index, pane->get_title());
	tab_container->set_current_tab(tab_index);

	set_current_tab_container(tab_container);

	if (p_callback.is_valid()) {
		p_callback.call(pane);
	}
}

void LayoutManager::_close_tab_control(int p_tab, AppTabContainer *p_tab_container) {
	Control *control = p_tab_container->get_tab_control(p_tab);
	if (!control) {
		return;
	}

	p_tab_container->remove_child(control);

	PaneBase *pane = Object::cast_to<PaneBase>(control);
	if (!pane) {
		control->queue_free();
		return;
	}
	PaneManager::get_singleton()->destroy(pane);
}

void LayoutManager::close_current_tab() {
	// TODO: use current pane
	AppTabContainer *tab_container = nullptr;
	if (current_tab_container &&
			current_tab_container->get_tab_count() > 0 &&
			get_tab_closable(Object::cast_to<MultiSplitContainer>(current_tab_container->get_parent()))) {
		tab_container = current_tab_container;
	} else if (prev_tab_container &&
			prev_tab_container->get_tab_count() > 0 &&
			get_tab_closable(Object::cast_to<MultiSplitContainer>(prev_tab_container->get_parent()))) {
		tab_container = prev_tab_container;
	} else {
		return;
	}

	set_current_tab_container(tab_container);

	int tab = tab_container->get_current_tab();
	_close_tab_control(tab, tab_container);

	// Note: Do not set_current_tab_container after _close_tab_control! The current_tab_container may be cleared.
}

void LayoutManager::select_next_tab() {
	int tab_count = current_tab_container ? current_tab_container->get_tab_count() : 0;
	if (tab_count > 0) {
		int next_tab = current_tab_container->get_current_tab() + 1;
		next_tab %= tab_count;
		current_tab_container->set_current_tab(next_tab);
	}
}

void LayoutManager::select_previous_tab() {
	int tab_count = current_tab_container ? current_tab_container->get_tab_count() : 0;
	if (tab_count > 0) {
		int next_tab = current_tab_container->get_current_tab() - 1;
		next_tab = next_tab >= 0 ? next_tab : tab_count - 1;
		current_tab_container->set_current_tab(next_tab);
	}
}

// Layout.
Ref<ConfigFile> LayoutManager::get_layout() {
	return default_layout;
}

void LayoutManager::set_window_windowed(bool p_windowed) {
	was_window_windowed_last = p_windowed;
}

void LayoutManager::_save_window_settings_to_config(Ref<ConfigFile> p_layout, const String &p_section) {
	Window *w = get_window();
	if (w) {
		p_layout->set_value(p_section, "screen", w->get_current_screen());

		Window::Mode mode = w->get_mode();
		switch (mode) {
			case Window::MODE_WINDOWED:
				p_layout->set_value(p_section, "mode", "windowed");
				p_layout->set_value(p_section, "size", w->get_size());
				break;
			case Window::MODE_FULLSCREEN:
			case Window::MODE_EXCLUSIVE_FULLSCREEN:
				p_layout->set_value(p_section, "mode", "fullscreen");
				break;
			case Window::MODE_MINIMIZED:
				if (was_window_windowed_last) {
					p_layout->set_value(p_section, "mode", "windowed");
					p_layout->set_value(p_section, "size", w->get_size());
				} else {
					p_layout->set_value(p_section, "mode", "maximized");
				}
				break;
			default:
				p_layout->set_value(p_section, "mode", "maximized");
				break;
		}

		p_layout->set_value(p_section, "position", w->get_position());
	}
}

Dictionary LayoutManager::_get_tab_container_data(AppTabContainer *p_tab_container) {
	Dictionary d;
	d["current_tab"] = p_tab_container->get_current_tab();
	return d;
}

void LayoutManager::_set_tab_container_data(AppTabContainer *p_tab_container, const Dictionary &p_data) {
	int index = p_data.get("current_tab", p_tab_container->get_current_tab());
	p_tab_container->set_current_tab(index);

	PaneBase *pane = Object::cast_to<PaneBase>(p_tab_container->get_tab_control(index));
	if (pane) {
		PaneManager::get_singleton()->set_current_pane(pane);
	}
}

Dictionary LayoutManager::_get_split_container_data(MultiSplitContainer *p_split_container) {
	Dictionary d;
	d["visible"] = p_split_container->is_visible();
	d["vertical"] = p_split_container->is_vertical();
	return d;
}

void LayoutManager::_set_split_container_data(MultiSplitContainer *p_split_container, const Dictionary &p_data) {
	p_split_container->set_visible(p_data.get("visible", p_split_container->is_visible()));
	p_split_container->set_vertical(p_data.get("vertical", p_split_container->is_vertical()));
}

Dictionary LayoutManager::_get_area_data(Control *p_area) {
	Dictionary d;
	MultiSplitContainer *split_container = Object::cast_to<MultiSplitContainer>(p_area);
	if (split_container) {
		d = _get_split_container_data(split_container);

		d["size_flags_horizontal"] = split_container->get_h_size_flags();
		d["size_flags_vertical"] = split_container->get_v_size_flags();
		d["split_offsets"] = split_container->get_split_offsets();
		d["tab_closable"] = get_tab_closable(split_container);
		d["group_id"] = get_tabs_rearrange_group(split_container);
	}
	return d;
}

void LayoutManager::_set_area_data(Control *p_area, const Dictionary &p_data) {
	MultiSplitContainer *split_container = Object::cast_to<MultiSplitContainer>(p_area);
	if (split_container) {
		_set_split_container_data(split_container, p_data);

		split_container->set_h_size_flags(p_data.get("size_flags_horizontal", split_container->get_h_size_flags()));
		split_container->set_v_size_flags(p_data.get("size_flags_vertical", split_container->get_v_size_flags()));
		split_container->set_split_offsets(p_data.get("split_offsets", split_container->get_split_offsets()));
		set_tab_closable(split_container, p_data.get("tab_closable", get_tab_closable(split_container)));
		set_tabs_rearrange_group(split_container, p_data.get("group_id", get_tabs_rearrange_group(split_container)));
	}
}

Dictionary LayoutManager::_get_main_data() {
	Dictionary d;
	HSplitContainer *hsplit = Object::cast_to<HSplitContainer>(gui_main);
	d["split_offsets"] = hsplit->get_split_offsets();
	return d;
}

void LayoutManager::_set_main_data(const Dictionary &p_data) {
	HSplitContainer *hsplit = Object::cast_to<HSplitContainer>(gui_main);
	hsplit->set_split_offsets(p_data.get("split_offsets", hsplit->get_split_offsets()));
}

Array LayoutManager::_get_children_data(Node *p_parent) {
	Array children;

	for (int i = 0; i < p_parent->get_child_count(false); i++) {
		Node *child = p_parent->get_child(i, false);

		String child_name = child->get_name();
		String child_type = child->get_class_name();
		Dictionary child_data;
		bool is_container = false;
		Array child_children;

		if (Object::cast_to<AppTabContainer>(child)) {
			AppTabContainer *tab_container = Object::cast_to<AppTabContainer>(child);
			child_type = "TabContainer";
			child_data = _get_tab_container_data(tab_container);

			is_container = true;
			child_children = _get_children_data(child);
		} else if (Object::cast_to<MultiSplitContainer>(child)) {
			MultiSplitContainer *split_container = Object::cast_to<MultiSplitContainer>(child);
			child_type = "SplitContainer";
			child_data = _get_split_container_data(split_container);

			is_container = true;
			child_children = _get_children_data(child);
		} else {
			PaneBase *pane = Object::cast_to<PaneBase>(child);
			child_data = pane->get_config_data();
		}

		Dictionary dict;
		dict["name"] = child_name;
		dict["type"] = child_type;
		dict["data"] = child_data;
		if (is_container) {
			dict["children"] = child_children;
		}
		children.push_back(dict);
	}

	return children;
}

void LayoutManager::_save_area_to_config(Ref<ConfigFile> p_layout, Control *p_area) {
	String section = p_area->get_name();

	MultiSplitContainer *split_container = Object::cast_to<MultiSplitContainer>(p_area);
	if (split_container) {
		p_layout->set_value(section, "type", "SplitContainer");
		p_layout->set_value(section, "data", _get_area_data(split_container));
		p_layout->set_value(section, "children", _get_children_data(p_area));
	}
}

void LayoutManager::_apply_layout_config(PaneBase *p_pane, const Dictionary &p_data) {
	ERR_FAIL_NULL(p_pane);
	p_pane->apply_config_data(p_data);
}

void LayoutManager::_load_children(Node *p_parent, const Array &p_children) {
	AppTabContainer *parent = Object::cast_to<AppTabContainer>(p_parent);
	if (!parent) {
		return;
	}

	for (int i = 0; i < p_children.size(); i++) {
		Dictionary dict = p_children[i];
		String child_type = dict.get("type", "");
		if (child_type.is_empty()) {
			continue;
		}
		Dictionary child_data = dict.get("data", Dictionary());
		if (child_data.is_empty()) {
			create_new_tab(child_type, parent);
		} else {
			create_new_tab(child_type, parent, callable_mp(this, &LayoutManager::_apply_layout_config).bind(child_data));
		}
	}
}

void LayoutManager::_load_container(Node *p_parent, const Array &p_children) {
	MultiSplitContainer *parent = Object::cast_to<MultiSplitContainer>(p_parent);
	if (!parent) {
		return;
	}

	for (int i = 0; i < p_children.size(); i++) {
		Dictionary dict = p_children[i];
		String child_type = dict.get("type", "");
		Dictionary child_data = dict.get("data", Dictionary());

		if (child_type == "TabContainer") {
			bool tab_closable = get_tab_closable(parent);
			int group_id = get_tabs_rearrange_group(parent);
			AppTabContainer *tab_container = create_tab_container(tab_closable, group_id);
			parent->add_child(tab_container);

			if (dict.has("children")) {
				Array child_children = dict["children"];
				_load_children(tab_container, child_children);
			}

			_set_tab_container_data(tab_container, child_data);
		} else if (child_type == "SplitContainer") {
			MultiSplitContainer *split_container = create_split_container();
			parent->add_child(split_container);

			if (dict.has("children")) {
				Array child_children = dict["children"];
				_load_container(split_container, child_children);
			}

			_set_split_container_data(split_container, child_data);
		}
	}
}

void LayoutManager::_load_area_from_config(Ref<ConfigFile> p_layout, const String &p_name, Node *p_parent) {
	String section = p_name;

	if (!p_layout->has_section(section) || !p_layout->has_section_key(section, "type")) {
		return;
	}

	String type = p_layout->get_value(section, "type");
	if (type == "SplitContainer") {
		MultiSplitContainer *area = Object::cast_to<MultiSplitContainer>(_create_area(type));
		area->set_name(p_name);
		p_parent->add_child(area);

		_load_container(area, p_layout->get_value(section, "children", Array()));

		_set_area_data(area, p_layout->get_value(section, "data", Dictionary()));
	}
}

void LayoutManager::_setup_default_layout(Node *p_parent) {
	AppTabContainer *tab_container = nullptr;

	// Left sidebar.
	MultiSplitContainer *left_sidebar = Object::cast_to<MultiSplitContainer>(_create_area());
	p_parent->add_child(left_sidebar);
	left_sidebar->set_name(LEFT_SIDEBAR_NAME);

	tab_container = create_tab_container();
	left_sidebar->add_child(tab_container);

	create_new_tab(NavigationPane::get_class_static(), tab_container);

	// Central content area.
	bool tab_closable = true;
	int group_id = 1;
	MultiSplitContainer *central_area = Object::cast_to<MultiSplitContainer>(_create_area());
	p_parent->add_child(central_area);
	central_area->set_name(CENTRAL_AREA_NAME);
	central_area->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	central_area->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	set_tab_closable(central_area, tab_closable);
	set_tabs_rearrange_group(central_area, group_id);

	tab_container = create_tab_container(tab_closable, group_id);
	central_area->add_child(tab_container);

	create_new_tab(FilePane::get_class_static(), tab_container);

	// Right sidebar.
	MultiSplitContainer *right_sidebar = Object::cast_to<MultiSplitContainer>(_create_area());
	p_parent->add_child(right_sidebar);
	right_sidebar->set_name(RIGHT_SIDEBAR_NAME);
	right_sidebar->hide();
}

void LayoutManager::save_layout() {
	if (!load_layout_done) {
		return;
	}

	String section = "main";
	String type = gui_main->get_class_name();
	default_layout->set_value(section, "type", type);
	default_layout->set_value(section, "data", _get_main_data());

	// Save default_layout.
	for (Control *area : areas) {
		_save_area_to_config(default_layout, area);
	}

	_save_window_settings_to_config(default_layout, "window");

	default_layout->save(AppSettings::get_layouts_config());
}

void LayoutManager::load_layout(Node *p_parent) {
	if (!gui_main) {
		gui_main = _create_main();
		p_parent->add_child(gui_main); // TODO: change parent?
	}

	if (!popup_menu) {
		popup_menu = memnew(PopupMenu);
		popup_menu->add_item(RTR("Split Up"), SPLIT_MENU_UP);
		popup_menu->add_item(RTR("Split Down"), SPLIT_MENU_DOWN);
		popup_menu->add_item(RTR("Split Left"), SPLIT_MENU_LEFT);
		popup_menu->add_item(RTR("Split Right"), SPLIT_MENU_RIGHT);
		popup_menu->connect(SceneStringName(id_pressed), callable_mp(this, &LayoutManager::_menu_id_pressed));
		gui_main->add_child(popup_menu);
	}

	Error err = default_layout->load(AppSettings::get_layouts_config());
	if (err != OK) { // No default_layout.
		_setup_default_layout(gui_main);
	} else {
		_load_area_from_config(default_layout, LEFT_SIDEBAR_NAME, gui_main);
		_load_area_from_config(default_layout, CENTRAL_AREA_NAME, gui_main);
		_load_area_from_config(default_layout, RIGHT_SIDEBAR_NAME, gui_main);

		String section = "main";
		// String type = default_layout->get_value(section, "type");
		_set_main_data(default_layout->get_value(section, "data", Dictionary()));
	}

	load_layout_done = true;
}

bool LayoutManager::is_load_layout_done() const {
	return load_layout_done;
}

Control *LayoutManager::_create_main() {
	HSplitContainer *hsplit = memnew(HSplitContainer);
	hsplit->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	hsplit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	hsplit->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	return hsplit;
}

Control *LayoutManager::_create_area(const String &p_type) {
	if (p_type == "SplitContainer") {
		MultiSplitContainer *split_container = create_split_container();

		bool tab_closable = false;
		int group_id = -1;
		split_container->set_meta("tab_closable", tab_closable);
		split_container->set_meta("group_id", group_id);

		_add_area(split_container);

		return split_container;
	}
	return nullptr;
}

void LayoutManager::_add_area(Control *p_area) {
	areas.push_back(p_area);
}

Control *LayoutManager::_get_area(const String &p_name) const {
	for (Control *area : areas) {
		if (area->get_name() == p_name) {
			return area;
		}
	}
	return nullptr;
}

void LayoutManager::_remove_area(Control *p_area) {
	areas.erase(p_area);
}

void LayoutManager::show_area(const String &p_name) {
	Control *area = _get_area(p_name);
	if (!area) {
		return;
	}

	if (!area->is_visible()) {
		area->show();
		emit_signal("area_visibility_changed", p_name, true);
	}
}

void LayoutManager::hide_area(const String &p_name) {
	Control *area = _get_area(p_name);
	if (!area) {
		return;
	}

	if (area->is_visible()) {
		area->hide();
		emit_signal("area_visibility_changed", p_name, false);
	}
}

void LayoutManager::toggle_area_visible(const String &p_name) {
	Control *area = _get_area(p_name);
	if (!area) {
		return;
	}

	bool visible = area->is_visible();
	area->set_visible(!visible);
	emit_signal("area_visibility_changed", p_name, !visible);
}

bool LayoutManager::is_area_visible(const String &p_name) const {
	Control *area = _get_area(p_name);
	if (!area) {
		return false;
	}

	return area->is_visible();
}

void LayoutManager::_bind_methods() {
	ADD_SIGNAL(MethodInfo("area_visibility_changed", PropertyInfo(Variant::STRING, "name"), PropertyInfo(Variant::BOOL, "visible")));
}

LayoutManager::LayoutManager() {
	ERR_FAIL_NULL_MSG(PaneManager::get_singleton(), "PaneManager doesn't exist.");

	singleton = this;

	default_layout.instantiate();

	PaneManager::get_singleton()->connect("pane_changed", callable_mp(this, &LayoutManager::_on_current_pane_changed));
}

LayoutManager::~LayoutManager() {
	split_containers.clear();
	tab_containers.clear();
	areas.clear();

	singleton = nullptr;
}
