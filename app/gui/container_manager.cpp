#include "container_manager.h"

#include "scene/gui/popup_menu.h"

#include "app/gui/app_tab_container.h"
#include "app/gui/multi_split_container.h"

#include "app/gui/pane_base.h"
#include "app/gui/pane_factory.h"

ContainerManager *ContainerManager::singleton = nullptr;

void ContainerManager::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY: {
			Viewport *viewport = get_viewport();
			ERR_FAIL_NULL(viewport);
			viewport->connect("gui_focus_changed", callable_mp(this, &ContainerManager::_gui_focus_changed));
		} break;
	}
}

void ContainerManager::_menu_id_pressed(int p_option) {
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
	new_tab(pane_type, tab_container);

	selected_tab_container = nullptr;
}

void ContainerManager::_select_tab_container(AppTabContainer *p_tab_container) {
	selected_tab_container = p_tab_container;
}

void ContainerManager::_new_tab(const StringName &p_pane_type, AppTabContainer *p_tab_container) {
	int tab_index = p_tab_container->get_tab_count();

	StringName type = p_pane_type;
	if (type.is_empty()) {
		type = pane_type;
	}
	PaneBase *pane = PaneFactory::get_singleton()->create_pane(type);
	if (!pane) {
		return;
	}
	p_tab_container->add_child(pane);
	pane->set_owner(p_tab_container->get_owner());
	pane->connect("title_changed", callable_mp(this, &ContainerManager::_on_pane_title_changed).bind(pane));
	pane->connect("icon_changed", callable_mp(this, &ContainerManager::_on_pane_icon_changed).bind(pane));

	// TODO: Update the current tab container when the pane gains focus.

	p_tab_container->set_tab_icon(tab_index, pane->get_icon());
	p_tab_container->set_tab_title(tab_index, pane->get_title());
	p_tab_container->set_current_tab(tab_index);

	set_current_tab_container(p_tab_container);
}

void ContainerManager::_tab_container_child_order_changed(AppTabContainer *p_tab_container) {
	if (p_tab_container->get_child_count(false) == 0) {
		MultiSplitContainer *split_container = Object::cast_to<MultiSplitContainer>(p_tab_container->get_parent());
		if (split_container) {
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
}

void ContainerManager::_on_drop_tab(int p_position, TabBar *p_from_tab_bar, int p_from_index, AppTabContainer *p_tab_container) {
	// Move tab.
	if (p_position == AppTabContainer::DropPosition::DROP_CENTER) {
		p_tab_container->move_tab_from(p_from_tab_bar, p_from_index);
		set_current_tab_container(p_tab_container);
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
	tab_container->move_tab_from(p_from_tab_bar, p_from_index);
}

void ContainerManager::_on_pane_title_changed(PaneBase *p_pane) {
	AppTabContainer *tab_container = Object::cast_to<AppTabContainer>(p_pane->get_parent());
	int tab_index = p_pane->get_index(false);
	tab_container->set_tab_title(tab_index, p_pane->get_title());
}

void ContainerManager::_on_pane_icon_changed(PaneBase *p_pane) {
	AppTabContainer *tab_container = Object::cast_to<AppTabContainer>(p_pane->get_parent());
	int tab_index = p_pane->get_index(false);
	tab_container->set_tab_icon(tab_index, p_pane->get_icon());
}

AppTabContainer *ContainerManager::_split(AppTabContainer *p_from, int p_direction) {
	MultiSplitContainer *split_container = Object::cast_to<MultiSplitContainer>(p_from->get_parent());
	bool tab_closable = get_tab_closable(split_container);
	int group_id = get_tabs_rearrange_group(split_container);

	AppTabContainer *tab_container = _create_tab_container(tab_closable, group_id);
	split_container->split(tab_container, p_from, (MultiSplitContainer::SplitDirection)p_direction);
	tab_container->set_owner(split_container->get_owner()); // Note: after add to scene tree

	set_current_tab_container(tab_container);

	return tab_container;
}

PopupMenu *ContainerManager::get_popup() const {
	return popup_menu;
}

AppTabContainer *ContainerManager::_create_tab_container(bool p_tab_closable, int p_group_id) {
	AppTabContainer *tab_container = memnew(AppTabContainer);
	// tab_container->set_theme_type_variation("TabContainerOdd");	// TODO: theme
	tab_container->set_new_tab_enabled(p_tab_closable);
	tab_container->set_drag_to_rearrange_enabled(true);
	tab_container->set_tabs_rearrange_group(p_group_id);

	// TODO: split, new tab
	// if (popup_menu) {
	// 	tab_container->set_popup(popup_menu);
	// }

	tab_container->connect("pre_popup_pressed", callable_mp(this, &ContainerManager::_select_tab_container).bind(tab_container));
	tab_container->connect("new_tab", callable_mp(this, &ContainerManager::_new_tab).bind("", tab_container));
	tab_container->connect("child_order_changed", callable_mp(this, &ContainerManager::_tab_container_child_order_changed).bind(tab_container));
	tab_container->connect("tab_dropped", callable_mp(this, &ContainerManager::_on_drop_tab).bind(tab_container));

	tab_containers.push_back(tab_container);
	set_current_tab_container(tab_container);

	return tab_container;
}

MultiSplitContainer *ContainerManager::create_container(const String &p_name, Node *p_parent, Node *p_owner, Node *p_child) {
	MultiSplitContainer *split_container = memnew(MultiSplitContainer);
	bool tab_closable = false;
	int group_id = -1;
	split_container->set_meta("tab_closable", tab_closable);
	split_container->set_meta("group_id", group_id);
	if (!p_name.is_empty()) {
		split_container->set_name(p_name);
	}

	AppTabContainer *tab_container = _create_tab_container(tab_closable, group_id);
	split_container->split(tab_container);
	if (p_child) {
		tab_container->add_child(p_child);
	}

	if (p_parent) {
		p_parent->add_child(split_container);

		if (p_owner) {
			split_container->set_owner(p_owner);
			tab_container->set_owner(p_owner);
			if (p_child) {
				p_child->set_owner(p_owner); // TODO
			}
		}
	}

	split_containers.push_back(split_container);
	return split_container;
}

void ContainerManager::set_current_tab_container(AppTabContainer *p_tab_container) {
	if (p_tab_container && p_tab_container != current_tab_container) {
		if (current_tab_container != nullptr) {
			prev_tab_container = current_tab_container;
		}
		current_tab_container = p_tab_container;
	}
}

void ContainerManager::clear_current_tab_container() {
	if (prev_tab_container == current_tab_container) {
		prev_tab_container = nullptr;
	}
	current_tab_container = nullptr;
}

AppTabContainer *ContainerManager::get_current_tab_container() const {
	return current_tab_container;
}

AppTabContainer *ContainerManager::get_prev_tab_container() const {
	return prev_tab_container;
}

void ContainerManager::new_tab() {
	if (pane_type.is_empty()) {
		return;
	}

	// TODO: Fix
	AppTabContainer *tab_container = nullptr;
	if (current_tab_container && get_tab_closable(Object::cast_to<MultiSplitContainer>(current_tab_container->get_parent()))) {
		tab_container = current_tab_container;
	} else if (prev_tab_container && get_tab_closable(Object::cast_to<MultiSplitContainer>(prev_tab_container->get_parent()))) {
		tab_container = prev_tab_container;
	} else {
		return;
	}

	callable_mp(this, &ContainerManager::_new_tab).call_deferred(pane_type, tab_container);
}

void ContainerManager::new_tab(const StringName &p_pane_type, AppTabContainer *p_tab_container) {
	AppTabContainer *tab_container = nullptr;
	if (p_tab_container) {
		tab_container = p_tab_container;
	} else if (current_tab_container) {
		tab_container = current_tab_container;
	} else {
		return;
	}

	if (!p_pane_type.is_empty()) {
		pane_type = p_pane_type;
	} else if (pane_type.is_empty()) {
		return;
	}

	callable_mp(this, &ContainerManager::_new_tab).call_deferred(pane_type, tab_container);
}

void ContainerManager::close_current_tab() {
	// TODO: Fix
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

	int tab = tab_container->get_current_tab();
	tab_container->close_tab(tab);
}

void ContainerManager::_set_tab_closable(MultiSplitContainer *p_split_container, bool p_closable) {
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

void ContainerManager::_set_tabs_rearrange_group(MultiSplitContainer *p_split_container, int p_group_id) {
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

void ContainerManager::_gui_focus_changed(Control *p_control) {
	AppTabContainer *tab_container = AppTabContainer::get_control_parent_tab_container(p_control);
	if (!tab_container) {
		return;
	}

	set_current_tab_container(tab_container);
}

void ContainerManager::set_tab_closable(MultiSplitContainer *p_split_container, bool p_closable) {
	bool tab_closable = p_split_container->get_meta("tab_closable");
	if (tab_closable == p_closable) {
		return;
	}

	p_split_container->set_meta("tab_closable", p_closable);
	_set_tab_closable(p_split_container, p_closable);
}

bool ContainerManager::get_tab_closable(MultiSplitContainer *p_split_container) const {
	MultiSplitContainer *split_container = MultiSplitContainer::get_root_split_container(p_split_container);
	return split_container->get_meta("tab_closable");
}

void ContainerManager::set_tabs_rearrange_group(MultiSplitContainer *p_split_container, int p_group_id) {
	int group_id = p_split_container->get_meta("group_id");
	if (group_id == p_group_id) {
		return;
	}

	p_split_container->set_meta("group_id", p_group_id);
	_set_tabs_rearrange_group(p_split_container, p_group_id);
}

int ContainerManager::get_tabs_rearrange_group(MultiSplitContainer *p_split_container) const {
	MultiSplitContainer *split_container = MultiSplitContainer::get_root_split_container(p_split_container);
	return split_container->get_meta("group_id");
}

ContainerManager::ContainerManager() {
	ERR_FAIL_NULL_MSG(PaneFactory::get_singleton(), "PaneFactory doesn't exist.");
	singleton = this;

	popup_menu = memnew(PopupMenu);
	popup_menu->add_item(RTR("Split Up"), SPLIT_MENU_UP);
	popup_menu->add_item(RTR("Split Down"), SPLIT_MENU_DOWN);
	popup_menu->add_item(RTR("Split Left"), SPLIT_MENU_LEFT);
	popup_menu->add_item(RTR("Split Right"), SPLIT_MENU_RIGHT);
	popup_menu->connect(SceneStringName(id_pressed), callable_mp(this, &ContainerManager::_menu_id_pressed));
	add_child(popup_menu);

	List<PaneFactory::PaneInfo> panes;
	PaneFactory::get_singleton()->get_pane_list(&panes);
	if (!panes.is_empty()) {
		const PaneFactory::PaneInfo &info = panes.get(0);
		pane_type = info.type;
	}
}

ContainerManager::~ContainerManager() {
	split_containers.clear();
	tab_containers.clear();

	singleton = nullptr;
}
