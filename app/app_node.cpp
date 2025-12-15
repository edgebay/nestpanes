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

#include "scene/resources/packed_scene.h"

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
#include "app/gui/multi_split_container.h"
#include "app/text_editor/text_editor.h"

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

void AppNode::_split_menu_id_pressed(int p_option) {
	if (!selected_tab_container) {
		return;
	}

	MultiSplitContainer::SplitDirection direction = MultiSplitContainer::SPLIT_RIGHT;
	switch (p_option) {
		case SPLIT_MENU_UP: {
			direction = MultiSplitContainer::SPLIT_UP;
		} break;

		case SPLIT_MENU_DOWN: {
			direction = MultiSplitContainer::SPLIT_DOWN;
		} break;

		case SPLIT_MENU_LEFT: {
			direction = MultiSplitContainer::SPLIT_LEFT;
		} break;

		case SPLIT_MENU_RIGHT: {
			direction = MultiSplitContainer::SPLIT_RIGHT;
		} break;

		default:
			return;
	}

	MultiSplitContainer *split_container = Object::cast_to<MultiSplitContainer>(selected_tab_container->get_parent());
	AppTabContainer *tab_container = _create_tab_container();
	split_container->split(tab_container, selected_tab_container, direction);
	tab_container->set_owner(gui_main); // Note: after add to scene tree
	_new_tab(tab_container);

	selected_tab_container = nullptr;
}

void AppNode::_select_tab_container(AppTabContainer *p_tab_container) {
	selected_tab_container = p_tab_container;
}

AppTabContainer *AppNode::_create_tab_container() {
	AppTabContainer *tab_container = memnew(AppTabContainer);
	tab_container->set_new_tab_enabled(true);
	tab_container->set_popup(split_menu);
	tab_container->connect("pre_popup_pressed", callable_mp(this, &AppNode::_select_tab_container).bind(tab_container));
	tab_container->connect("new_tab", callable_mp(this, &AppNode::_new_tab).bind(tab_container));
	tab_container->connect("emptied", callable_mp(this, &AppNode::_tab_container_emptied).bind(tab_container));
	tab_containers.push_back(tab_container);
	return tab_container;
}

int AppNode::_new_tab(AppTabContainer *p_parent) {
	int tab_index = p_parent->get_tab_count();

	FileSystemList *file_system_list = memnew(FileSystemList);
	file_system_list->connect("path_changed", callable_mp(this, &AppNode::_on_tab_path_changed));
	p_parent->add_child(file_system_list);
	file_system_list->set_owner(gui_main);
	file_system_lists.push_back(file_system_list);

	// Update tab.
	String path = file_system_list->get_current_path();
	String title = path.get_file();
	if (title.is_empty()) {
		title = path;
	}
	p_parent->set_tab_icon_max_width(tab_index, theme->get_constant(SNAME("class_icon_size"), AppStringName(App)));
	p_parent->set_tab_icon(tab_index, file_system_list->get_current_dir_icon());
	p_parent->set_tab_title(tab_index, title);
	p_parent->set_current_tab(tab_index);

	current_tab_container = p_parent;

	return tab_index;
}

int AppNode::_new_editor(AppTabContainer *p_parent, const String &p_path) {
	int tab_index = p_parent->get_tab_count();

	TextEditor *editor = memnew(TextEditor);
	// editor->connect("path_changed", callable_mp(this, &AppNode::_on_tab_path_changed));
	p_parent->add_child(editor);
	editor->set_owner(gui_main);
	// editors.push_back(editor);

	// Ref<TextFile> text_file = edited_res;
	// editor->get_text_editor()->set_text(text_file->get_text());
	editor->open_file(p_path);

	// Update tab.
	String path = p_path;
	String title = path.get_file();
	if (title.is_empty()) {
		title = path;
	}
	p_parent->set_tab_icon_max_width(tab_index, theme->get_constant(SNAME("class_icon_size"), AppStringName(App)));
	p_parent->set_tab_icon(tab_index, editor->get_app_theme_icon(SNAME("File")));
	p_parent->set_tab_title(tab_index, title);
	p_parent->set_current_tab(tab_index);

	current_tab_container = p_parent;

	return tab_index;
}

void AppNode::_tab_container_emptied(AppTabContainer *p_tab_container) {
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

// void AppNode::_on_tab_path_changed(const String &p_path) {
// 	AppTabContainer *tab_container = Object::cast_to<AppTabContainer>(p_fs->get_parent());
// 	if (tab_container) {
// 		String title = p_path.get_file();
// 		if (title.is_empty()) {
// 			title = p_path;
// 		}
// 		int tab_index = tab_container->get_current_tab();
// 		tab_container->set_tab_title(tab_index, title);
// 	}
// }

void AppNode::_on_tab_path_changed(FileSystemControl *p_fs) {
	AppTabContainer *tab_container = Object::cast_to<AppTabContainer>(p_fs->get_parent());
	if (tab_container) {
		String title = p_fs->get_current_dir_name();
		int tab_index = tab_container->get_current_tab();
		tab_container->set_tab_icon(tab_index, p_fs->get_current_dir_icon());
		tab_container->set_tab_title(tab_index, title);

		current_tab_container = tab_container;
	}
	_save_layout();
}

void AppNode::_on_tree_item_activated(const String &p_path, bool is_dir) {
	if (is_dir) {
		if (current_tab_container == nullptr) {
			return;
		}
		int tab_index = _new_tab(current_tab_container);
		FileSystemList *file_system_list = Object::cast_to<FileSystemList>(current_tab_container->get_tab_control(tab_index));
		file_system_list->set_current_path(p_path);
	} else {
		if (current_tab_container == nullptr) {
			return;
		}
		// TODO: FileSystemList::_item_dc_selected
		_new_editor(current_tab_container, p_path);
	}
}

// void AppNode::_on_tree_item_selected(TreeItem *p_item) {
void AppNode::_on_tree_item_selected(const String &p_path, bool is_dir) {
	if (is_dir) {
		if (current_tab_container == nullptr) {
			return;
		}
		FileSystemList *file_system_list = Object::cast_to<FileSystemList>(current_tab_container->get_current_tab_control());
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

	Ref<PackedScene> sdata;
	sdata.instantiate();
	Error err = sdata->pack(gui_main);
	if (err != OK) {
		// show_accept(TTR("Couldn't save scene. Likely dependencies (instances or inheritance) couldn't be satisfied."), TTR("OK"));
		return;
	}

	int flg = 0;
	flg |= ResourceSaver::FLAG_REPLACE_SUBRESOURCE_PATHS;

	String scene_path = _get_main_scene_path();
	err = ResourceSaver::save(sdata, scene_path, flg);
	// if (err == OK) {
	// 	scene->set_scene_file_path(ProjectSettings::get_singleton()->localize_path(p_file));
	// 	editor_data.set_scene_as_saved(idx);
	// 	editor_data.set_scene_modified_time(idx, FileAccess::get_modified_time(p_file));

	// 	editor_folding.save_scene_folding(scene, p_file);

	// 	_update_title();
	// 	scene_tabs->update_scene_tabs();
	// } else {
	// 	_dialog_display_save_error(p_file, err);
	// }

	Ref<ConfigFile> config;
	String config_path = _get_config_path();
	config.instantiate();
	// Load and amend existing config if it exists.
	config->load(config_path);

	for (auto control : file_system_trees) { // TODO: file_system_lists
		control->save_layout_to_config(config, "docks");
	}
	// for (int i = 0; i < file_system_controls.size(); i++) {
	// 	FileSystemControl *control = file_system_controls[i];
	// 	control->save_layout_to_config(config, "file_system_" + itos(i));
	// }

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
		for (auto control : file_system_trees) {
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

String AppNode::_get_main_scene_path() const {
	return AppPaths::get_singleton()->get_config_dir().path_join("app_scene.tscn");
}

Error AppNode::_parse_node(Node *p_node) {
	if (Object::cast_to<AppTabContainer>(p_node)) {
		AppTabContainer *container = Object::cast_to<AppTabContainer>(p_node);
		tab_containers.push_back(container);
	} else if (Object::cast_to<FileSystemTree>(p_node)) {
		FileSystemTree *control = Object::cast_to<FileSystemTree>(p_node);
		file_system_trees.push_back(control);
	} else if (Object::cast_to<FileSystemList>(p_node)) {
		FileSystemList *control = Object::cast_to<FileSystemList>(p_node);
		file_system_lists.push_back(control);
	}

	for (int i = 0; i < p_node->get_child_count(); i++) {
		Node *c = p_node->get_child(i);
		Error err = _parse_node(c);
		if (err) {
			tab_containers.clear();
			file_system_trees.clear();
			file_system_lists.clear();
			return err;
		}
	}

	return OK;
}

bool AppNode::_load_main_scene() {
	String scene_path = _get_main_scene_path();
	Ref<PackedScene> sdata = ResourceLoader::load(scene_path, "", ResourceFormatLoader::CACHE_MODE_REPLACE_DEEP);
	if (sdata.is_null()) {
		return false;
	}

	sdata->set_path(scene_path, true); // Take over path.

	Node *new_scene = sdata->instantiate(PackedScene::GEN_EDIT_STATE_DISABLED);
	if (!new_scene) {
		return false;
	}
	new_scene->set_scene_instance_state(Ref<SceneState>());

	Error err = _parse_node(new_scene);
	if (err == OK && !tab_containers.is_empty()) {
		for (auto container : tab_containers) {
			container->set_popup(split_menu);
			container->connect("pre_popup_pressed", callable_mp(this, &AppNode::_select_tab_container).bind(container));
			container->connect("new_tab", callable_mp(this, &AppNode::_new_tab).bind(container));
			container->connect("emptied", callable_mp(this, &AppNode::_tab_container_emptied).bind(container));

			// Update tab icon and text
			for (int i = 0; i < container->get_child_count(); i++) {
				Node *control = container->get_child(i);
				if (Object::cast_to<FileSystemList>(control)) {
					FileSystemList *file_system_list = Object::cast_to<FileSystemList>(control);
					int tab_index = container->get_tab_idx_from_control(file_system_list);
					String path = file_system_list->get_current_path();
					String title = path.get_file();
					if (title.is_empty()) {
						title = path;
					}
					container->set_tab_icon_max_width(tab_index, theme->get_constant(SNAME("class_icon_size"), AppStringName(App)));
					container->set_tab_icon(tab_index, file_system_list->get_current_dir_icon());
					container->set_tab_title(tab_index, title);
				}
			}
		}
		for (auto control : file_system_trees) {
			control->connect("item_activated", callable_mp(this, &AppNode::_on_tree_item_activated));
			control->connect("item_selected", callable_mp(this, &AppNode::_on_tree_item_selected));
			control->connect("item_collapsed", callable_mp(this, &AppNode::save_layout_delayed));
		}
		for (auto control : file_system_lists) {
			control->connect("path_changed", callable_mp(this, &AppNode::_on_tab_path_changed));
		}

		gui_main = new_scene;
		return true;
	}

	return false;
}

void AppNode::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY: {
			_load_layout();
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

	// Take up all screen.
	gui_base->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	gui_base->set_anchor(SIDE_RIGHT, Control::ANCHOR_END);
	gui_base->set_anchor(SIDE_BOTTOM, Control::ANCHOR_END);
	gui_base->set_end(Point2(0, 0));

	main_vbox = memnew(VBoxContainer);
	gui_base->add_child(main_vbox);
	main_vbox->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT, Control::PRESET_MODE_MINSIZE);

	title_bar = memnew(HBoxContainer);
	main_vbox->add_child(title_bar);

	split_menu = memnew(PopupMenu);
	split_menu->add_item(RTR("Split Up"), SPLIT_MENU_UP);
	split_menu->add_item(RTR("Split Down"), SPLIT_MENU_DOWN);
	split_menu->add_item(RTR("Split Left"), SPLIT_MENU_LEFT);
	split_menu->add_item(RTR("Split Right"), SPLIT_MENU_RIGHT);
	split_menu->connect(SceneStringName(id_pressed), callable_mp(this, &AppNode::_split_menu_id_pressed));
	gui_base->add_child(split_menu);

	if (_load_main_scene()) {
		main_vbox->add_child(gui_main);
	} else {
		HSplitContainer *left_hsplit = memnew(HSplitContainer);
		main_vbox->add_child(left_hsplit);
		left_hsplit->set_v_size_flags(Control::SIZE_EXPAND_FILL);

		SplitContainer *left_split = memnew(SplitContainer);
		left_hsplit->add_child(left_split);

		HSplitContainer *right_hsplit = memnew(HSplitContainer);
		left_hsplit->add_child(right_hsplit);
		right_hsplit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		right_hsplit->set_v_size_flags(Control::SIZE_EXPAND_FILL);

		gui_main = left_hsplit;

		MultiSplitContainer *center_split = memnew(MultiSplitContainer);
		right_hsplit->add_child(center_split);
		center_split->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		center_split->set_v_size_flags(Control::SIZE_EXPAND_FILL);

		left_split->set_owner(gui_main);
		right_hsplit->set_owner(gui_main);
		center_split->set_owner(gui_main);

		AppTabContainer *tab_container = _create_tab_container();
		center_split->split(tab_container);
		tab_container->set_owner(gui_main);
		_new_tab(tab_container);

		SplitContainer *right_split = memnew(SplitContainer);
		right_hsplit->add_child(right_split);

		// Controls
		FileSystemTree *file_system_tree = memnew(FileSystemTree);
		file_system_tree->connect("item_activated", callable_mp(this, &AppNode::_on_tree_item_activated));
		file_system_tree->connect("item_selected", callable_mp(this, &AppNode::_on_tree_item_selected));
		file_system_tree->connect("item_collapsed", callable_mp(this, &AppNode::save_layout_delayed));
		left_split->add_child(file_system_tree);
		file_system_tree->set_owner(gui_main);
		file_system_trees.push_back(file_system_tree);
	}

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
