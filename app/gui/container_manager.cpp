#include "container_manager.h"

#include "scene/gui/popup_menu.h"

#include "app/gui/app_tab_container.h"
#include "app/gui/multi_split_container.h"

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

	_split(selected_tab_container, direction);
	// TODO: copy tab

	selected_tab_container = nullptr;
}

void ContainerManager::_select_tab_container(AppTabContainer *p_tab_container) {
	selected_tab_container = p_tab_container;
}

void ContainerManager::_tab_container_emptied(AppTabContainer *p_tab_container) {
	if (tab_containers.size() == 1) {
		// Last tab container.
		return;
	}
	MultiSplitContainer *split_container = Object::cast_to<MultiSplitContainer>(p_tab_container->get_parent());
	if (split_container) {
		split_container->remove(p_tab_container);
		tab_containers.erase(p_tab_container);
		p_tab_container->queue_free();

		if (current_tab_container == p_tab_container) {
			current_tab_container = nullptr;
		}
	}
}

void ContainerManager::_on_drop_tab(int p_position, const Variant &p_data, AppTabContainer *p_tab_container) {
	print_line("drop tab: ", p_position, p_tab_container);
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
	// TODO: move tab
}

AppTabContainer *ContainerManager::_split(AppTabContainer *p_from, int p_direction) {
	MultiSplitContainer *split_container = Object::cast_to<MultiSplitContainer>(p_from->get_parent());
	AppTabContainer *tab_container = _create_tab_container();
	split_container->split(tab_container, p_from, (MultiSplitContainer::SplitDirection)p_direction);
	tab_container->set_owner(split_container->get_owner()); // Note: after add to scene tree
	return tab_container;
}

void ContainerManager::init_popup_menu(Node *p_parent) {
	ERR_FAIL_NULL(p_parent);

	popup_menu = memnew(PopupMenu);
	popup_menu->add_item(RTR("Split Up"), SPLIT_MENU_UP);
	popup_menu->add_item(RTR("Split Down"), SPLIT_MENU_DOWN);
	popup_menu->add_item(RTR("Split Left"), SPLIT_MENU_LEFT);
	popup_menu->add_item(RTR("Split Right"), SPLIT_MENU_RIGHT);
	popup_menu->connect(SceneStringName(id_pressed), callable_mp(this, &ContainerManager::_menu_id_pressed));
	p_parent->add_child(popup_menu);
}

PopupMenu *ContainerManager::get_popup() const {
	return popup_menu;
}

MultiSplitContainer *ContainerManager::_create_split_container(const String &p_name, Node *p_parent, Node *p_owner) {
	MultiSplitContainer *split_container = memnew(MultiSplitContainer);

	if (!p_name.is_empty()) {
		split_container->set_name(p_name);
	}

	if (p_parent) {
		p_parent->add_child(split_container);
		if (p_owner) {
			split_container->set_owner(p_owner);
		}
	}

	split_containers.push_back(split_container);
	return split_container;
}

AppTabContainer *ContainerManager::_create_tab_container() {
	AppTabContainer *tab_container = memnew(AppTabContainer);
	// tab_container->set_theme_type_variation("TabContainerOdd");	// TODO: theme
	tab_container->set_new_tab_enabled(true);
	tab_container->set_drag_to_rearrange_enabled(true);
	tab_container->set_tabs_rearrange_group(1);

	if (popup_menu) {
		tab_container->set_popup(popup_menu);
	}

	tab_container->connect("pre_popup_pressed", callable_mp(this, &ContainerManager::_select_tab_container).bind(tab_container));
	// tab_container->connect("new_tab", callable_mp(this, &ContainerManager::_new_tab).bind(tab_container));
	tab_container->connect("emptied", callable_mp(this, &ContainerManager::_tab_container_emptied).bind(tab_container));
	tab_container->connect("tab_dropped", callable_mp(this, &ContainerManager::_on_drop_tab).bind(tab_container));

	tab_containers.push_back(tab_container);
	current_tab_container = tab_container;

	return tab_container;
}

MultiSplitContainer *ContainerManager::create_container(const String &p_name, Node *p_parent, Node *p_owner, Node *p_child) {
	MultiSplitContainer *split_container = memnew(MultiSplitContainer);
	if (!p_name.is_empty()) {
		split_container->set_name(p_name);
	}

	AppTabContainer *tab_container = _create_tab_container();
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
		current_tab_container = p_tab_container;
	}
}

AppTabContainer *ContainerManager::get_current_tab_container() const {
	return current_tab_container;
}

ContainerManager::ContainerManager() {
}

ContainerManager::~ContainerManager() {
	split_containers.clear();
	tab_containers.clear();
}
