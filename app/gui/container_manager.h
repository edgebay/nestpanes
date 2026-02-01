#pragma once

#include "core/object/object.h"

class AppTabContainer;
class MultiSplitContainer;
class Node;
class PaneBase;
class PopupMenu;

class ContainerManager : public Object {
	GDCLASS(ContainerManager, Object);

public:
	enum MenuOptions {
		// Split menu.
		SPLIT_MENU_UP,
		SPLIT_MENU_DOWN,
		SPLIT_MENU_LEFT,
		SPLIT_MENU_RIGHT,
	};

	// typedef Control *(*NewTabFunc)();

private:
	static ContainerManager *singleton;

	// HashMap<String, NewTabFunc> func_map;

	PopupMenu *popup_menu = nullptr;

	List<MultiSplitContainer *> split_containers;
	List<AppTabContainer *> tab_containers;

	AppTabContainer *current_tab_container = nullptr;
	AppTabContainer *selected_tab_container = nullptr;

	StringName pane_type = "";

	AppTabContainer *_create_tab_container();

	void _menu_id_pressed(int p_option);
	void _select_tab_container(AppTabContainer *p_tab_container);
	void _new_tab(AppTabContainer *p_tab_container);
	void _tab_container_child_order_changed(AppTabContainer *p_tab_container);
	void _on_drop_tab(int p_position, const Variant &p_data, AppTabContainer *p_tab_container);

	void _on_pane_title_changed(PaneBase *p_pane);
	void _on_pane_icon_changed(PaneBase *p_pane);

	AppTabContainer *_split(AppTabContainer *p_from, int p_direction);

	// protected:
	// void _notification(int p_what);
	// static void _bind_methods();

public:
	static ContainerManager *get_singleton() { return singleton; }

	// int register_new_tab_function(const String &p_type, NewTabFunc p_function);

	void init_popup_menu(Node *p_parent);
	// void set_popup(PopupMenu *p_popup);
	PopupMenu *get_popup() const;

	MultiSplitContainer *create_container(const String &p_name = "", Node *p_parent = nullptr, Node *p_owner = nullptr, Node *p_child = nullptr);

	void set_current_tab_container(AppTabContainer *p_tab_container);
	AppTabContainer *get_current_tab_container() const;

	void new_tab();
	void close_current_tab();

	ContainerManager();
	~ContainerManager();
};
