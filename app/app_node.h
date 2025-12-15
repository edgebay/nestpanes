#pragma once

#include "scene/main/node.h"

#include "core/templates/safe_refcount.h"
#include "scene/resources/theme.h"

class Control;
class HBoxContainer;
class VBoxContainer;

class AppTabContainer;
class FileSystemControl;
class FileSystemTree;
class FileSystemList;
class PopupMenu;

class Timer;

class AppNode : public Node {
	GDCLASS(AppNode, Node);

private:
	enum SplitMenu {
		SPLIT_MENU_UP,
		SPLIT_MENU_DOWN,
		SPLIT_MENU_LEFT,
		SPLIT_MENU_RIGHT
	};

	static AppNode *singleton;

	Control *gui_base = nullptr;
	Node *gui_main = nullptr;
	VBoxContainer *main_vbox = nullptr;

	HBoxContainer *title_bar = nullptr;

	PopupMenu *split_menu = nullptr;

	List<AppTabContainer *> tab_containers;
	AppTabContainer *current_tab_container = nullptr;
	AppTabContainer *selected_tab_container = nullptr;

	List<FileSystemTree *> file_system_trees;
	List<FileSystemList *> file_system_lists;

	Ref<Theme> theme;

	// Timer *system_theme_timer = nullptr;
	bool follow_system_theme = false;
	bool use_system_accent_color = false;
	bool last_dark_mode_state = false;
	Color last_system_base_color = Color(0, 0, 0, 0);
	Color last_system_accent_color = Color(0, 0, 0, 0);

	Timer *layout_save_delay_timer = nullptr;

	bool load_layout_done = false;

	void _update_theme(bool p_skip_creation = false);

	void _split_menu_id_pressed(int p_option);
	void _select_tab_container(AppTabContainer *p_tab_container);

	AppTabContainer *_create_tab_container();
	int _new_tab(AppTabContainer *p_parent); // TODO: new file list?
	int _new_editor(AppTabContainer *p_parent, const String &p_path);
	void _tab_container_emptied(AppTabContainer *p_tab_container);

	// void _on_tab_path_changed(const String &p_path);
	void _on_tab_path_changed(FileSystemControl *p_fs);
	void _on_tree_item_activated(const String &p_path, bool is_dir);
	// void _on_tree_item_selected(TreeItem *p_item);
	void _on_tree_item_selected(const String &p_path, bool is_dir);

	String _get_config_path() const;
	void _save_layout();
	void _load_layout();

	String _get_main_scene_path() const;
	Error _parse_node(Node *p_node);
	bool _load_main_scene();

protected:
	void _notification(int p_what);

public:
	static AppNode *get_singleton() { return singleton; }

	void save_layout_delayed();

	AppNode();
	~AppNode();
};
