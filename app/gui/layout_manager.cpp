#include "layout_manager.h"

#include "app/settings/app_settings.h"

#include "app/gui/app_tab_container.h"
#include "app/gui/container_manager.h"
#include "app/gui/multi_split_container.h"
#include "app/gui/pane_base.h"
#include "app/gui/pane_manager.h"

#include "app/app_modules/file_management/gui/file_pane.h"
#include "app/app_modules/file_management/gui/navigation_pane.h"

LayoutManager *LayoutManager::singleton = nullptr;

Ref<ConfigFile> LayoutManager::get_layout() {
	return default_layout;
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
			// child_data = tab_container->get_config_data();	// TODO
			is_container = true;
			child_children = _get_children_data(child);
		} else if (Object::cast_to<MultiSplitContainer>(child)) {
			MultiSplitContainer *split_container = Object::cast_to<MultiSplitContainer>(child);
			child_type = "SplitContainer";
			// child_data = split_container->get_config_data();	// TODO
			child_data["vertical"] = split_container->is_vertical();
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
	ContainerManager *container_manager = ContainerManager::get_singleton();

	MultiSplitContainer *split_container = Object::cast_to<MultiSplitContainer>(p_area);
	if (split_container) {
		p_layout->set_value(section, "type", "SplitContainer");

		Dictionary container_data;
		container_data["visible"] = split_container->is_visible();
		container_data["vertical"] = split_container->is_vertical();
		container_data["tab_closable"] = container_manager->get_tab_closable(split_container);
		container_data["group_id"] = container_manager->get_tabs_rearrange_group(split_container);
		p_layout->set_value(section, "data", container_data);
		// p_layout->set_value(section, "data", split_container->get_config_data());

		p_layout->set_value(section, "children", _get_children_data(p_area));
	}
}

void LayoutManager::_load_children(Node *p_parent, const Array &p_children) {
	Container *parent = nullptr;
	if (Object::cast_to<MultiSplitContainer>(p_parent)) {
		parent = Object::cast_to<MultiSplitContainer>(p_parent);
	} else if (Object::cast_to<AppTabContainer>(p_parent)) {
		parent = Object::cast_to<AppTabContainer>(p_parent);
	} else {
		return;
	}

	ContainerManager *container_manager = ContainerManager::get_singleton();
	for (int i = 0; i < p_children.size(); i++) {
		Dictionary dict = p_children[i];
		String child_type = dict.get("type", "");
		if (child_type.is_empty()) {
			continue;
		}

		Dictionary child_data = dict.get("data", Dictionary());
		Container *container = nullptr;
		if (child_type == "TabContainer") {
			AppTabContainer *tab_container = container_manager->create_tab_container();
			parent->add_child(tab_container);
			container = tab_container;
		} else if (child_type == "SplitContainer") {
			MultiSplitContainer *split_container = container_manager->create_split_container();

			split_container->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
			split_container->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			split_container->set_v_size_flags(Control::SIZE_EXPAND_FILL);

			bool vertical = child_data.get("vertical", split_container->is_vertical());
			split_container->set_vertical(vertical);

			parent->add_child(split_container);
			container = split_container;
		} else {
			PaneBase *pane = PaneManager::get_singleton()->create(child_type);
			if (!pane) {
				continue;
			}
			parent->add_child(pane);
		}

		if (container && dict.has("children")) {
			Array child_children = dict["children"];
			_load_children(container, child_children);
		}
	}
}

void LayoutManager::_load_area_from_config(Ref<ConfigFile> p_layout, const String &p_name, Node *p_parent) {
	String section = p_name;

	if (!p_layout->has_section(section) || !p_layout->has_section_key(section, "type")) {
		return;
	}

	ContainerManager *container_manager = ContainerManager::get_singleton();

	String type = p_layout->get_value(section, "type");
	if (type == "SplitContainer") {
		MultiSplitContainer *area = container_manager->create_split_container();
		area->set_name(p_name);
		p_parent->add_child(area);

		Array children = p_layout->get_value(section, "children");
		_load_children(area, children);

		Dictionary container_data = p_layout->get_value(section, "data", Dictionary());
		area->set_visible(container_data.get("visible", area->is_visible()));
		area->set_vertical(container_data.get("vertical", area->is_vertical()));

		container_manager->set_tab_closable(area, container_data.get("tab_closable", false)); // TODO: default
		container_manager->set_tabs_rearrange_group(area, container_data.get("group_id", -1)); // TODO: default

		add_area(area);
	}
}

void LayoutManager::_setup_default_layout(Node *p_parent) {
	ContainerManager *container_manager = ContainerManager::get_singleton();
	AppTabContainer *tab_container = nullptr;

	// Left sidebar.
	MultiSplitContainer *left_sidebar = container_manager->create_split_container();
	p_parent->add_child(left_sidebar);
	left_sidebar->set_name(LEFT_SIDEBAR_NAME);

	tab_container = container_manager->create_tab_container();
	left_sidebar->add_child(tab_container);

	container_manager->new_tab(NavigationPane::get_class_static(), tab_container); // TODO
	add_area(left_sidebar);

	// Central content area.
	MultiSplitContainer *central_area = container_manager->create_split_container();
	p_parent->add_child(central_area);
	central_area->set_name(CENTRAL_AREA_NAME);
	central_area->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	central_area->set_v_size_flags(Control::SIZE_EXPAND_FILL);

	tab_container = container_manager->create_tab_container();
	central_area->add_child(tab_container);

	container_manager->set_tab_closable(central_area, true);
	container_manager->set_tabs_rearrange_group(central_area, 1);

	container_manager->new_tab(FilePane::get_class_static(), tab_container); // TODO
	add_area(central_area);

	// Right sidebar.
	MultiSplitContainer *right_sidebar = container_manager->create_split_container();
	p_parent->add_child(right_sidebar);
	right_sidebar->set_name(RIGHT_SIDEBAR_NAME);
	right_sidebar->hide();
	add_area(right_sidebar);
}

void LayoutManager::save_layout() {
	// Save default_layout.
	for (Control *area : areas) {
		_save_area_to_config(default_layout, area);
	}

	default_layout->save(AppSettings::get_layouts_config());
}

void LayoutManager::load_layout(Node *p_parent) {
	Error err = default_layout->load(AppSettings::get_layouts_config());
	if (err != OK) { // No default_layout.
		_setup_default_layout(p_parent);
	} else {
		_load_area_from_config(default_layout, LEFT_SIDEBAR_NAME, p_parent);
		_load_area_from_config(default_layout, CENTRAL_AREA_NAME, p_parent);
		_load_area_from_config(default_layout, RIGHT_SIDEBAR_NAME, p_parent);
	}
}

void LayoutManager::add_area(Control *p_area) {
	areas.push_back(p_area);
}

Control *LayoutManager::get_area(const String &p_name) const {
	for (Control *area : areas) {
		if (area->get_name() == p_name) {
			return area;
		}
	}
	return nullptr;
}

void LayoutManager::remove_area(Control *p_area) {
	areas.erase(p_area);
}

LayoutManager::LayoutManager() {
	default_layout.instantiate();

	singleton = this;
}

LayoutManager::~LayoutManager() {
	areas.clear();

	singleton = nullptr;
}
