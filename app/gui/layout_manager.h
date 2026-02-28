#pragma once

#include "scene/main/node.h"

#include "core/io/config_file.h"

#define LEFT_SIDEBAR_NAME "left_sidebar"
#define CENTRAL_AREA_NAME "central_area"
#define RIGHT_SIDEBAR_NAME "right_sidebar"
// // TODO: ribbon
// #define RIBBON_NAME "ribbon"

class AppTabContainer;
class Control;
class MultiSplitContainer;
class PaneBase;
class PopupMenu;
class TabBar;

class Timer;

// TODO: Object?
class LayoutManager : public Node {
	GDCLASS(LayoutManager, Node);

public:
	enum MenuOptions {
		// Split menu.
		SPLIT_MENU_UP,
		SPLIT_MENU_DOWN,
		SPLIT_MENU_LEFT,
		SPLIT_MENU_RIGHT,
	};

private:
	static LayoutManager *singleton;

	Ref<ConfigFile> default_layout;
	bool load_layout_done = false;

	Control *gui_main = nullptr;
	Vector<Control *> areas;

	bool was_window_windowed_last = false;

	// Container.
	PopupMenu *popup_menu = nullptr;

	List<MultiSplitContainer *> split_containers;
	List<AppTabContainer *> tab_containers;

	AppTabContainer *current_tab_container = nullptr;
	AppTabContainer *prev_tab_container = nullptr;
	AppTabContainer *selected_tab_container = nullptr;

	Timer *layout_save_delay_timer = nullptr;

	void _menu_id_pressed(int p_option);
	void _select_tab_container(AppTabContainer *p_tab_container);
	void _tab_container_child_order_changed(AppTabContainer *p_tab_container);
	void _move_tab_control(TabBar *p_from_tab_bar, int p_from_index, AppTabContainer *p_to);
	void _on_drop_tab(int p_position, TabBar *p_from_tab_bar, int p_from_index, AppTabContainer *p_tab_container);
	void _on_tab_selected(int p_tab, AppTabContainer *p_tab_container);

	void _on_pane_title_changed(PaneBase *p_pane);
	void _on_pane_icon_changed(PaneBase *p_pane);
	void _on_pane_data_changed(PaneBase *p_pane);

	AppTabContainer *_split(AppTabContainer *p_from, int p_direction);

	void _set_tab_closable(MultiSplitContainer *p_split_container, bool p_closable);
	void _set_tabs_rearrange_group(MultiSplitContainer *p_split_container, int p_group_id);
	void _close_tab_control(int p_tab, AppTabContainer *p_tab_container);

	void _split_container_drag_ended();

	void _on_current_pane_changed(PaneBase *p_pane);
	void _update_current_pane(int p_tab, AppTabContainer *p_tab_container);

	// Layout.
	void _save_window_settings_to_config(Ref<ConfigFile> p_layout, const String &p_section);

	Dictionary _get_tab_container_data(AppTabContainer *p_tab_container);
	void _set_tab_container_data(AppTabContainer *p_tab_container, const Dictionary &p_data);
	Dictionary _get_split_container_data(MultiSplitContainer *p_split_container);
	void _set_split_container_data(MultiSplitContainer *p_split_container, const Dictionary &p_data);
	Dictionary _get_area_data(Control *p_area);
	void _set_area_data(Control *p_area, const Dictionary &p_data);
	Dictionary _get_main_data();
	void _set_main_data(const Dictionary &p_data);

	Array _get_children_data(Node *p_parent);
	void _save_area_to_config(Ref<ConfigFile> p_layout, Control *p_node);

	void _apply_layout_config(PaneBase *p_pane, const Dictionary &p_data);
	void _load_children(Node *p_parent, const Array &p_children);
	void _load_container(Node *p_parent, const Array &p_children);
	void _load_area_from_config(Ref<ConfigFile> p_layout, const String &p_name, Node *p_parent);

	void _setup_default_layout(Node *p_parent);

	Control *_create_main();
	Control *_create_area(const String &p_type = "SplitContainer");
	void _add_area(Control *p_area);
	Control *_get_area(const String &p_name) const;
	void _remove_area(Control *p_area);

	void _area_visibility_changed(const String &p_name, bool p_visible);
	void _layout_changed();

protected:
	static void _bind_methods();

public:
	static LayoutManager *get_singleton() { return singleton; }

	// Container.
	PopupMenu *get_popup() const;

	/// TODO: private
	AppTabContainer *create_tab_container(bool p_tab_closable = false, int p_group_id = -1);
	MultiSplitContainer *create_split_container();

	void set_current_tab_container(AppTabContainer *p_tab_container);
	void clear_current_tab_container();
	AppTabContainer *get_current_tab_container() const;
	AppTabContainer *get_prev_tab_container() const;

	void set_tab_closable(MultiSplitContainer *p_split_container, bool p_closable);
	bool get_tab_closable(MultiSplitContainer *p_split_container) const;

	void set_tabs_rearrange_group(MultiSplitContainer *p_split_container, int p_group_id);
	int get_tabs_rearrange_group(MultiSplitContainer *p_split_container) const;
	///

	void create_new_tab(const StringName &p_type = StringName(), AppTabContainer *p_tab_container = nullptr, const Callable &p_callback = Callable());
	void close_current_tab();
	void select_next_tab();
	void select_previous_tab();

	// Layout.
	Ref<ConfigFile> get_layout();

	void set_window_windowed(bool p_windowed);

	void save_layout_delayed();
	void save_layout();
	void load_layout(Node *p_parent);
	bool is_load_layout_done() const;

	void show_area(const String &p_name);
	void hide_area(const String &p_name);
	void toggle_area_visible(const String &p_name);
	bool is_area_visible(const String &p_name) const;

	LayoutManager();
	~LayoutManager();
};
