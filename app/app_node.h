#pragma once

#include "scene/main/node.h"

#include "core/templates/safe_refcount.h"
#include "scene/resources/theme.h"

class AppTabContainer;
class Control;
class HBoxContainer;
class MenuBar;
class MenuButton;
class MultiSplitContainer;
class PopupMenu;
class VBoxContainer;

class ContainerManager;
class PaneFactory;
class PaneBase;

class FileSystem;

class Timer;

class AppAbout;

class AppNode : public Node {
	GDCLASS(AppNode, Node);

public:
	enum MenuOptions {
		// File menu.
		FILE_NEW_TAB,
		FILE_CLOSE_TAB,
		FILE_QUIT,

		// View menu.
		VIEW_LEFT_SIDEBAR,

		// Help menu.
		HELP_SEARCH,
		HELP_DOCS,
		HELP_FORUM,
		HELP_COMMUNITY,
		HELP_COPY_SYSTEM_INFO,
		HELP_GITHUB,
		HELP_REPORT_A_BUG,
		HELP_SUGGEST_A_FEATURE,
		HELP_SEND_DOCS_FEEDBACK,
		HELP_ABOUT,
		HELP_SUPPORT_GODOT_DEVELOPMENT,
	};

private:
	static AppNode *singleton;

	Control *gui_base = nullptr;
	Node *gui_main = nullptr;
	VBoxContainer *main_vbox = nullptr;

	HBoxContainer *title_bar = nullptr;
	VBoxContainer *ribbon = nullptr;
	MultiSplitContainer *left_sidebar = nullptr;
	MultiSplitContainer *central_area = nullptr;
	MultiSplitContainer *right_sidebar = nullptr;

	ContainerManager *container_manager = nullptr;
	PaneFactory *pane_factory = nullptr;

	Control *menu_btn_spacer = nullptr;
	MenuButton *main_menu_button = nullptr; // TODO: Remove?
	MenuBar *main_menu_bar = nullptr;

	PopupMenu *file_menu = nullptr;
	PopupMenu *view_menu = nullptr;
	PopupMenu *help_menu = nullptr;

	AppAbout *about = nullptr;

	List<AppTabContainer *> tab_containers;

	Ref<Theme> theme;

	FileSystem *file_system;

	bool exiting = false;
	bool dimmed = false;

	bool follow_system_theme = false;
	bool use_system_accent_color = false;
	bool last_dark_mode_state = false;
	Color last_system_base_color = Color(0, 0, 0, 0);
	Color last_system_accent_color = Color(0, 0, 0, 0);

	Timer *layout_save_delay_timer = nullptr;

	bool load_layout_done = false;

	void _update_theme(bool p_skip_creation = false);

	void _check_system_theme_changed();

	void _menu_option(int p_option);
	void _menu_option_confirm(int p_option, bool p_confirmed);

	void _toggle_left_sidebar();
	void _toggle_right_sidebar();

	void _on_navigation_pane_create(PaneBase *p_pane);
	void _on_file_pane_create(PaneBase *p_pane);

	void _on_tree_item_activated(const String &p_path, bool is_dir);
	void _on_tree_item_selected(const String &p_path, bool is_dir);

	void _exit(int p_exit_code);

	virtual void shortcut_input(const Ref<InputEvent> &p_event) override;

	String _get_config_path() const;
	void _save_layout();
	void _load_layout();

	String _get_main_scene_path() const;
	Error _parse_node(Node *p_node);
	bool _load_main_scene();

	void _update_main_menu_type();
	void _add_to_main_menu(const String &p_name, PopupMenu *p_menu);
	void _init_main_menu();

protected:
	void _notification(int p_what);

public:
	static AppNode *get_singleton() { return singleton; }

	void save_layout_delayed();

	AppNode();
	~AppNode();
};
