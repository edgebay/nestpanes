#pragma once

#include "scene/main/node.h"

#include "core/io/config_file.h"

#define LEFT_SIDEBAR_NAME "left_sidebar"
#define CENTRAL_AREA_NAME "central_area"
#define RIGHT_SIDEBAR_NAME "right_sidebar"

// class AppTabContainer;
class Control;
// class MultiSplitContainer;
// class PaneBase;
// class PopupMenu;
// class TabBar;

// TODO: Object?
class LayoutManager : public Node {
	GDCLASS(LayoutManager, Node);

	// public:
	// 	enum MenuOptions {
	// 		// Split menu.
	// 		SPLIT_MENU_UP,
	// 		SPLIT_MENU_DOWN,
	// 		SPLIT_MENU_LEFT,
	// 		SPLIT_MENU_RIGHT,
	// 	};

private:
	static LayoutManager *singleton;

	Ref<ConfigFile> default_layout;

	Vector<Control *> areas;

	Array _get_children_data(Node *p_parent);
	void _save_area_to_config(Ref<ConfigFile> p_layout, Control *p_node);
	void _load_children(Node *p_parent, const Array &p_children);
	void _load_area_from_config(Ref<ConfigFile> p_layout, const String &p_name, Node *p_parent);
	void _setup_default_layout(Node *p_parent);

public:
	static LayoutManager *get_singleton() { return singleton; }

	Ref<ConfigFile> get_layout();

	void save_layout();
	void load_layout(Node *p_parent);

	void add_area(Control *p_area);
	Control *get_area(const String &p_name) const;
	void remove_area(Control *p_area);

	LayoutManager();
	~LayoutManager();
};
