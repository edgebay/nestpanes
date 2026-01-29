#include "container_manager.h"

#include "scene/gui/popup_menu.h"

#include "app/gui/app_tab_container.h"
#include "app/gui/multi_split_container.h"

#include "app/gui/pane_base.h"
#include "app/gui/pane_factory.h"

ContainerManager *ContainerManager::singleton = nullptr;

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

void ContainerManager::_new_tab(AppTabContainer *p_tab_container) {
	int tab_index = p_tab_container->get_tab_count();

	PaneBase *pane = PaneFactory::get_singleton()->create_pane(pane_type);
	p_tab_container->add_child(pane);
	pane->set_owner(p_tab_container->get_owner());
	pane->connect("title_changed", callable_mp(this, &ContainerManager::_on_pane_title_changed).bind(p_tab_container, pane));
	pane->connect("icon_changed", callable_mp(this, &ContainerManager::_on_pane_icon_changed).bind(p_tab_container, pane));

	// TODO: connect the pane's signal, should the handler function be placed in ContainerManager, PaneFactory, or elsewhere?
	// pane->connect("signal", callable_mp(this, &ContainerManager::_func).bind(p_tab_container, pane));
	// pane->connect("signal", callable_mp(PaneFactory::get_singleton(), &PaneFactory::func).bind(p_tab_container, pane));

	p_tab_container->set_tab_icon(tab_index, pane->get_icon());
	p_tab_container->set_tab_title(tab_index, pane->get_title());
	p_tab_container->set_current_tab(tab_index);
}

void ContainerManager::_tab_container_child_order_changed(AppTabContainer *p_tab_container) {
	if (p_tab_container->get_child_count(false) == 0) {
		MultiSplitContainer *split_container = Object::cast_to<MultiSplitContainer>(p_tab_container->get_parent());
		print_line("tab container emptied: ", p_tab_container, split_container);
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
					current_tab_container = nullptr;
				}
			}
		}
	}
}

void ContainerManager::_on_drop_tab(int p_position, const Variant &p_data, AppTabContainer *p_tab_container) {
	print_line("drop tab: ", p_position, p_tab_container);
	// Move tab.
	if (p_position == AppTabContainer::DropPosition::DROP_CENTER) {
		p_tab_container->move_tab(p_data, p_tab_container->get_tab_count());
		current_tab_container = p_tab_container;
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
	tab_container->move_tab(p_data, 0); // TODO: split_container::_create_sub_split changed from_path
}

void ContainerManager::_on_pane_title_changed(AppTabContainer *p_tab_container, PaneBase *p_pane) {
	int tab_index = p_pane->get_index(false);
	p_tab_container->set_tab_title(tab_index, p_pane->get_title());
}

void ContainerManager::_on_pane_icon_changed(AppTabContainer *p_tab_container, PaneBase *p_pane) {
	int tab_index = p_pane->get_index(false);
	p_tab_container->set_tab_icon(tab_index, p_pane->get_icon());
}

AppTabContainer *ContainerManager::_split(AppTabContainer *p_from, int p_direction) {
	MultiSplitContainer *split_container = Object::cast_to<MultiSplitContainer>(p_from->get_parent());
	AppTabContainer *tab_container = _create_tab_container();
	split_container->split(tab_container, p_from, (MultiSplitContainer::SplitDirection)p_direction);
	tab_container->set_owner(split_container->get_owner()); // Note: after add to scene tree

	current_tab_container = tab_container;

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
	tab_container->connect("new_tab", callable_mp(this, &ContainerManager::_new_tab).bind(tab_container));
	tab_container->connect("child_order_changed", callable_mp(this, &ContainerManager::_tab_container_child_order_changed).bind(tab_container));
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

void ContainerManager::new_tab() {
	if (current_tab_container) {
		_new_tab(current_tab_container);
	}
}

void ContainerManager::close_current_tab() {
	if (current_tab_container && current_tab_container->get_tab_count() > 0) {
		int tab = current_tab_container->get_current_tab();
		current_tab_container->close_tab(tab);
	}
}

ContainerManager::ContainerManager() {
	ERR_FAIL_NULL_MSG(PaneFactory::get_singleton(), "PaneFactory doesn't exist.");
	singleton = this;

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
