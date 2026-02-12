#pragma once

#include "scene/main/node.h"

class AppTabContainer;
class Control;
class MultiSplitContainer;
class PaneBase;
class PopupMenu;
class TabBar;

class ContainerManager : public Node {
	GDCLASS(ContainerManager, Node);

public:
	enum MenuOptions {
		// Split menu.
		SPLIT_MENU_UP,
		SPLIT_MENU_DOWN,
		SPLIT_MENU_LEFT,
		SPLIT_MENU_RIGHT,
	};

private:
	static ContainerManager *singleton;

	PopupMenu *popup_menu = nullptr;

	List<MultiSplitContainer *> split_containers;
	List<AppTabContainer *> tab_containers;

	AppTabContainer *current_tab_container = nullptr;
	AppTabContainer *prev_tab_container = nullptr;
	AppTabContainer *selected_tab_container = nullptr;

	AppTabContainer *_create_tab_container(bool p_tab_closable, int p_group_id);

	void _menu_id_pressed(int p_option);
	void _select_tab_container(AppTabContainer *p_tab_container);
	void _new_tab(const StringName &p_pane_type, AppTabContainer *p_tab_container, const Callable &p_callback);
	void _tab_container_child_order_changed(AppTabContainer *p_tab_container);
	void _on_drop_tab(int p_position, TabBar *p_from_tab_bar, int p_from_index, AppTabContainer *p_tab_container);

	void _on_pane_title_changed(PaneBase *p_pane);
	void _on_pane_icon_changed(PaneBase *p_pane);

	AppTabContainer *_split(AppTabContainer *p_from, int p_direction);

	void _set_tab_closable(MultiSplitContainer *p_split_container, bool p_closable);
	void _set_tabs_rearrange_group(MultiSplitContainer *p_split_container, int p_group_id);

	void _gui_focus_changed(Control *p_control);

protected:
	void _notification(int p_what);

public:
	static ContainerManager *get_singleton() { return singleton; }

	PopupMenu *get_popup() const;

	MultiSplitContainer *create_container(const String &p_name = "", Node *p_parent = nullptr, Node *p_owner = nullptr, Node *p_child = nullptr);

	void set_current_tab_container(AppTabContainer *p_tab_container);
	void clear_current_tab_container();
	AppTabContainer *get_current_tab_container() const;
	AppTabContainer *get_prev_tab_container() const;

	void new_tab();
	void new_tab(const StringName &p_pane_type, AppTabContainer *p_tab_container = nullptr, const Callable &p_callback = Callable());
	void close_current_tab();

	void set_tab_closable(MultiSplitContainer *p_split_container, bool p_closable);
	bool get_tab_closable(MultiSplitContainer *p_split_container) const;

	void set_tabs_rearrange_group(MultiSplitContainer *p_split_container, int p_group_id);
	int get_tabs_rearrange_group(MultiSplitContainer *p_split_container) const;

	ContainerManager();
	~ContainerManager();
};
