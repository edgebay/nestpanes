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
class VBoxContainer;

class ContainerManager;
class PaneManager;

class FileSystemControl;
class FileSystemTree;
class FileSystemList;
class PopupMenu;

class Timer;

class AppAbout;

class AppNode : public Node {
	GDCLASS(AppNode, Node);

public:
	enum MenuOptions {
		// Scene menu.
		SCENE_NEW_SCENE,
		SCENE_NEW_INHERITED_SCENE,
		SCENE_OPEN_SCENE,
		SCENE_OPEN_PREV,
		SCENE_OPEN_RECENT,
		SCENE_SAVE_SCENE,
		SCENE_SAVE_AS_SCENE,
		SCENE_SAVE_ALL_SCENES,
		SCENE_MULTI_SAVE_AS_SCENE,
		SCENE_QUICK_OPEN,
		SCENE_QUICK_OPEN_SCENE,
		SCENE_QUICK_OPEN_SCRIPT,
		SCENE_UNDO,
		SCENE_REDO,
		SCENE_RELOAD_SAVED_SCENE,
		SCENE_CLOSE,
		SCENE_QUIT,

		FILE_EXPORT_MESH_LIBRARY,

		// // Project menu.
		// PROJECT_OPEN_SETTINGS,
		// PROJECT_FIND_IN_FILES,
		// PROJECT_VERSION_CONTROL,
		// PROJECT_EXPORT,
		// PROJECT_PACK_AS_ZIP,
		// PROJECT_INSTALL_ANDROID_SOURCE,
		// PROJECT_OPEN_USER_DATA_FOLDER,
		// PROJECT_RELOAD_CURRENT_PROJECT,
		// PROJECT_QUIT_TO_PROJECT_MANAGER,

		// TOOLS_ORPHAN_RESOURCES,
		// TOOLS_BUILD_PROFILE_MANAGER,
		// TOOLS_PROJECT_UPGRADE,
		// TOOLS_CUSTOM,

		// VCS_METADATA,
		// VCS_SETTINGS,

		// // Editor menu.
		// EDITOR_OPEN_SETTINGS,
		// EDITOR_COMMAND_PALETTE,
		// EDITOR_TAKE_SCREENSHOT,
		// EDITOR_TOGGLE_FULLSCREEN,
		// EDITOR_OPEN_DATA_FOLDER,
		// EDITOR_OPEN_CONFIG_FOLDER,
		// EDITOR_MANAGE_FEATURE_PROFILES,
		// EDITOR_MANAGE_EXPORT_TEMPLATES,
		// EDITOR_CONFIGURE_FBX_IMPORTER,

		// LAYOUT_SAVE,
		// LAYOUT_DELETE,
		// LAYOUT_DEFAULT,

		// Help menu.
		HELP_SEARCH,
		HELP_DOCS,
		HELP_FORUM,
		HELP_COMMUNITY,
		HELP_COPY_SYSTEM_INFO,
		HELP_REPORT_A_BUG,
		HELP_SUGGEST_A_FEATURE,
		HELP_SEND_DOCS_FEEDBACK,
		HELP_ABOUT,
		HELP_SUPPORT_GODOT_DEVELOPMENT,

		// // Update spinner menu.
		// SPINNER_UPDATE_CONTINUOUSLY,
		// SPINNER_UPDATE_WHEN_CHANGED,
		// SPINNER_UPDATE_SPINNER_HIDE,

		// // Non-menu options.
		// SCENE_TAB_CLOSE,
		// SAVE_AND_RUN,
		// SAVE_AND_RUN_MAIN_SCENE,
		// RESOURCE_SAVE,
		// RESOURCE_SAVE_AS,
		// SETTINGS_PICK_MAIN_SCENE,
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
	PaneManager *pane_manager = nullptr;

	Control *menu_btn_spacer = nullptr;
	MenuButton *main_menu_button = nullptr; // TODO: Remove?
	MenuBar *main_menu_bar = nullptr;

	PopupMenu *file_menu = nullptr;
	PopupMenu *view_menu = nullptr;
	// PopupMenu *settings_menu = nullptr;
	PopupMenu *help_menu = nullptr;
	// PopupMenu *tool_menu = nullptr;

	// PopupMenu *recent_scenes = nullptr;
	// String _recent_scene;
	// List<String> prev_closed_scenes;
	// String defer_load_scene;
	// Node *_last_instantiated_scene = nullptr;

	AppAbout *about = nullptr;

	List<AppTabContainer *> tab_containers;

	List<FileSystemTree *> file_system_trees;
	List<FileSystemList *> file_system_lists;

	Ref<Theme> theme;

	bool exiting = false;
	bool dimmed = false;

	// Timer *system_theme_timer = nullptr;
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
	void _menu_confirm_current();
	void _menu_option_confirm(int p_option, bool p_confirmed);

	void _toggle_left_sidebar();
	void _toggle_right_sidebar();

	int _new_tab(AppTabContainer *p_parent); // TODO: new file list?
	int _new_editor(AppTabContainer *p_parent, const String &p_path);

	// void _on_tab_path_changed(const String &p_path);
	void _on_tab_path_changed(FileSystemControl *p_fs);
	void _on_tree_item_activated(const String &p_path, bool is_dir);
	// void _on_tree_item_selected(TreeItem *p_item);
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
