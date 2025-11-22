#include "app_node.h"

#include "core/string/translation.h"
#include "core/string/ustring.h"
#include "core/version.h"

#include "scene/gui/box_container.h"
#include "scene/gui/container.h"
#include "scene/gui/label.h"
#include "scene/gui/panel.h"
#include "scene/gui/popup_menu.h"
#include "scene/gui/split_container.h"
#include "scene/gui/tab_container.h"

#include "scene/main/scene_tree.h"

#include "scene/theme/theme_db.h"

#include "servers/display_server.h"

#include "app/app_string_names.h"
#include "app/paths/app_paths.h"
#include "app/settings/app_settings.h"
#include "app/themes/app_theme_manager.h"

#include "app/core/io/file_system_access.h"

#include "app/file_manager/file_system_list.h"
#include "app/file_manager/file_system_tree.h"
#include "app/gui/app_tab_container.h"

AppNode *AppNode::singleton = nullptr;

void AppNode::_update_theme(bool p_skip_creation) {
	if (!p_skip_creation) {
		theme = AppThemeManager::generate_theme(theme);
		DisplayServer::set_early_window_clear_color_override(true, theme->get_color(SNAME("background"), AppStringName(App)));
	}

	Vector<Ref<Theme>> app_themes;
	app_themes.push_back(theme);
	app_themes.push_back(ThemeDB::get_singleton()->get_default_theme());

	ThemeContext *node_tc = ThemeDB::get_singleton()->get_theme_context(this);
	if (node_tc) {
		node_tc->set_themes(app_themes);
	} else {
		ThemeDB::get_singleton()->create_theme_context(this, app_themes);
	}

	Window *window = get_window();
	if (window) {
		ThemeContext *window_tc = ThemeDB::get_singleton()->get_theme_context(window);
		if (window_tc) {
			window_tc->set_themes(app_themes);
		} else {
			ThemeDB::get_singleton()->create_theme_context(window, app_themes);
		}
	}

	// Update styles.
	{
		gui_base->add_theme_style_override(SceneStringName(panel), theme->get_stylebox(SNAME("Background"), AppStringName(AppStyles)));
	}
}

int AppNode::_new_tab(AppTabContainer *p_parent) {
	int tab_index = p_parent->get_tab_count();
	FileSystemList *file_system_list = memnew(FileSystemList);
	file_system_list->connect("path_changed", callable_mp(this, &AppNode::_on_tab_path_changed));
	p_parent->add_child(file_system_list);
	file_system_controls.push_back(file_system_list);

	String path = file_system_list->get_current_path();
	String title = path.get_file();
	if (title.is_empty()) {
		title = path;
	}
	p_parent->set_tab_icon_max_width(tab_index, theme->get_constant(SNAME("class_icon_size"), AppStringName(App)));
	p_parent->set_tab_icon(tab_index, file_system_list->get_current_dir_icon());
	p_parent->set_tab_title(tab_index, title);
	p_parent->set_current_tab(tab_index);

	return tab_index;
}

// void AppNode::_on_tab_path_changed(const String &p_path) {
// 	String title = p_path.get_file();
// 	if (title.is_empty()) {
// 		title = p_path;
// 	}
// 	int tab_index = main_screen->get_current_tab();
// 	main_screen->set_tab_title(tab_index, title);
// }

void AppNode::_on_tab_path_changed(FileSystemControl *p_fs) {
	String title = p_fs->get_current_dir_name();
	int tab_index = main_screen->get_current_tab();
	main_screen->set_tab_icon(tab_index, p_fs->get_current_dir_icon());
	main_screen->set_tab_title(tab_index, title);
}

void AppNode::_on_tree_item_activated(const String &p_path, bool is_dir) {
	if (is_dir) {
		int tab_index = _new_tab(main_screen);
		FileSystemList *file_system_list = Object::cast_to<FileSystemList>(main_screen->get_tab_control(tab_index));
		file_system_list->set_current_path(p_path);
	} else {
		// TODO: FileSystemList::_item_dc_selected
	}
}

// void AppNode::_on_tree_item_selected(TreeItem *p_item) {
void AppNode::_on_tree_item_selected(const String &p_path, bool is_dir) {
	if (is_dir) {
		FileSystemList *file_system_list = Object::cast_to<FileSystemList>(main_screen->get_current_tab_control());
		// TODO: handle file item
		file_system_list->set_current_path(p_path);
	}
}

String AppNode::_get_config_path() const {
	return AppPaths::get_singleton()->get_config_dir().path_join("app_layout.cfg");
}

void AppNode::_save_layout() {
	if (!load_layout_done) {
		return;
	}
	Ref<ConfigFile> config;
	String config_path = _get_config_path();
	config.instantiate();
	// Load and amend existing config if it exists.
	config->load(config_path);

	for (auto control : file_system_controls) {
		print_line(control);
		control->save_layout_to_config(config, "docks");
	}

	// _save_open_scenes_to_config(config);
	// _save_central_editor_layout_to_config(config);
	// _save_window_settings_to_config(config, "EditorWindow");
	// editor_data.get_plugin_window_layout(config);

	config->save(config_path);
}

void AppNode::_load_layout() {
	// EditorProgress ep("loading_editor_layout", TTR("Loading editor"), 5);
	// ep.step(TTR("Loading editor layout..."), 0, true);
	Ref<ConfigFile> config;
	String config_path = _get_config_path();
	config.instantiate();
	Error err = config->load(config_path);
	if (err != OK) { // No config.
		// // If config is not found, expand the res:// folder and favorites by default.
		// TreeItem *root = FileSystemDock::get_singleton()->get_tree_control()->get_item_with_metadata("res://", 0);
		// if (root) {
		// 	root->set_collapsed(false);
		// }

		// TreeItem *favorites = FileSystemDock::get_singleton()->get_tree_control()->get_item_with_metadata("Favorites", 0);
		// if (favorites) {
		// 	favorites->set_collapsed(false);
		// }

		// if (overridden_default_layout >= 0) {
		// 	_layout_menu_option(overridden_default_layout);
		// }
	} else {
		// ep.step(TTR("Loading docks..."), 1, true);
		// editor_dock_manager->load_docks_from_config(config, "docks", true);
		for (auto control : file_system_controls) {
			control->load_layout_from_config(config, "docks");
		}

		// ep.step(TTR("Reopening scenes..."), 2, true);
		// _load_open_scenes_from_config(config);

		// ep.step(TTR("Loading central editor layout..."), 3, true);
		// _load_central_editor_layout_from_config(config);

		// ep.step(TTR("Loading plugin window layout..."), 4, true);
		// editor_data.set_plugin_window_layout(config);

		// ep.step(TTR("Editor layout ready."), 5, true);
	}
	load_layout_done = true;
}

void AppNode::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY: {
			_load_layout();

			// TODO: file_system_tree
			// Connect item_collapsed signal after loading the layout
			file_system_controls[0]->connect("item_collapsed", callable_mp(this, &AppNode::save_layout_delayed));
		} break;

		case NOTIFICATION_ENTER_TREE: {
			// Theme has already been created in the constructor, so we can skip that step.
			_update_theme(true);
		} break;

		case AppSettings::NOTIFICATION_APP_SETTINGS_CHANGED: {
			follow_system_theme = APP_GET("interface/theme/follow_system_theme");
			use_system_accent_color = APP_GET("interface/theme/use_system_accent_color");
		} break;
	}
}

void AppNode::save_layout_delayed() {
	layout_save_delay_timer->start();
}

AppNode::AppNode() {
	DEV_ASSERT(!singleton);
	singleton = this;

	// TODO: main.cpp DisplayServer::get_singleton()->window_set_title(appname);
	SceneTree::get_singleton()->get_root()->set_title(APP_VERSION_NAME);

	// File system.
	FileSystemAccess::create();

	// Load settings.
	if (!AppSettings::get_singleton()) {
		AppSettings::create();
	}

	// Define a minimum window size to prevent UI elements from overlapping or being cut off.
	Window *w = Object::cast_to<Window>(SceneTree::get_singleton()->get_root());
	if (w) {
		const Size2 minimum_size = Size2(1024, 600) * APP_SCALE;
		w->set_min_size(minimum_size); // Calling it this early doesn't sync the property with DS.
		DisplayServer::get_singleton()->window_set_min_size(minimum_size);
	}

	AppThemeManager::initialize();
	theme = AppThemeManager::generate_theme();
	DisplayServer::set_early_window_clear_color_override(true, theme->get_color(SNAME("background"), AppStringName(App)));

	// App layout.
	gui_base = memnew(Panel);
	add_child(gui_base);

	gui_base->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	gui_base->set_anchor(SIDE_RIGHT, Control::ANCHOR_END);
	gui_base->set_anchor(SIDE_BOTTOM, Control::ANCHOR_END);
	gui_base->set_end(Point2(0, 0));

	main_vbox = memnew(VBoxContainer);
	gui_base->add_child(main_vbox);
	main_vbox->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT, Control::PRESET_MODE_MINSIZE);

	title_bar = memnew(HBoxContainer);
	main_vbox->add_child(title_bar);

	left_hsplit = memnew(HSplitContainer);
	main_vbox->add_child(left_hsplit);
	left_hsplit->set_v_size_flags(Control::SIZE_EXPAND_FILL);

	left_split = memnew(SplitContainer);
	left_hsplit->add_child(left_split);

	right_hsplit = memnew(HSplitContainer);
	left_hsplit->add_child(right_hsplit);

	center_split = memnew(SplitContainer);
	right_hsplit->add_child(center_split);
	center_split->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	center_split->set_v_size_flags(Control::SIZE_EXPAND_FILL);

	main_screen = memnew(AppTabContainer);
	main_screen->set_new_tab_enabled(true);
	main_screen->connect("new_tab", callable_mp(this, &AppNode::_new_tab).bind(main_screen));
	center_split->add_child(main_screen);

	FileSystemTree *file_system_tree = memnew(FileSystemTree);
	file_system_tree->connect("item_activated", callable_mp(this, &AppNode::_on_tree_item_activated));
	file_system_tree->connect("item_selected", callable_mp(this, &AppNode::_on_tree_item_selected));
	left_split->add_child(file_system_tree);
	file_system_controls.push_back(file_system_tree);

	_new_tab(main_screen);

	layout_save_delay_timer = memnew(Timer);
	add_child(layout_save_delay_timer);
	layout_save_delay_timer->set_wait_time(0.5);
	layout_save_delay_timer->set_one_shot(true);
	layout_save_delay_timer->connect("timeout", callable_mp(this, &AppNode::_save_layout));

	set_process(true);

	set_process_shortcut_input(true);
}

AppNode::~AppNode() {
	AppSettings::destroy();
	AppThemeManager::finalize();
	FileSystemAccess::destroy();

	singleton = nullptr;
}
