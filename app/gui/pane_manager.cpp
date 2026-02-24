#include "pane_manager.h"

#include "scene/main/viewport.h"

#include "app/gui/pane_factory.h"

#include "app/app_modules/file_management/file_system.h"
#include "app/app_modules/file_management/gui/file_pane.h"
#include "app/app_modules/file_management/gui/navigation_pane.h"
// TODO: SettingsPane, WelcomePane
// #include "app/settings/gui/settings_pane.h"
// #include "app/gui/welcome_pane.h"

PaneManager *PaneManager::singleton = nullptr;

void PaneManager::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY: {
			Viewport *viewport = get_viewport();
			ERR_FAIL_NULL(viewport);
			viewport->connect("gui_focus_changed", callable_mp(this, &PaneManager::_gui_focus_changed));
		} break;
	}
}

void PaneManager::_gui_focus_changed(Control *p_control) {
	PaneBase *pane = PaneBase::get_control_parent_pane(p_control);
	if (!pane) {
		return;
	}

	set_current_pane(pane);
}

// File management.
void PaneManager::_on_navigation_pane_create(PaneBase *p_pane) {
	NavigationPane *pane = Object::cast_to<NavigationPane>(p_pane);
	if (pane) {
		pane->set_file_system(file_system);
		pane->connect("item_activated", callable_mp(this, &PaneManager::_on_tree_item_activated));
		pane->connect("item_selected", callable_mp(this, &PaneManager::_on_tree_item_selected));
	}
}

void PaneManager::_on_file_pane_create(PaneBase *p_pane) {
	FilePane *pane = Object::cast_to<FilePane>(p_pane);
	if (pane) {
		pane->set_file_system(file_system);
	}
}

void PaneManager::_on_tree_item_activated(const String &p_path, bool is_dir) {
	if (!is_dir) {
		// TODO: open_file()/run_file()
		OS::get_singleton()->shell_open(p_path);
	}
}

void PaneManager::_on_tree_item_selected(const String &p_path, bool is_dir) {
	if (is_dir) {
		// Current pane is NavigationPane.
		FilePane *pane = Object::cast_to<FilePane>(get_prev_pane());
		if (pane) {
			pane->set_path(p_path);
		}
	}
}

template <typename T>
void PaneManager::register_pane(const StringName &p_type, const Ref<Texture2D> &p_icon, const Callable &p_create_callback) {
	if (pane_map.has(p_type)) {
		return;
	}

	pane_map.insert(p_type, List<PaneBase *>());
	PaneFactory::register_pane<T>(p_type, p_icon, p_create_callback);
}

PaneBase *PaneManager::create(const StringName &p_type) {
	StringName type = p_type;
	if (type.is_empty()) {
		type = pane_type;
	}

	if (!pane_map.has(type)) {
		return nullptr;
	}

	PaneBase *pane = PaneFactory::create_pane(type);
	if (!pane) {
		return nullptr;
	}

	set_current_pane(pane);

	pane_map[type].push_back(pane);

	return pane;
}

void PaneManager::destroy(PaneBase *p_pane) {
	String type = p_pane->get_class_name();

	if (pane_map.has(type)) {
		pane_map[type].erase(p_pane);
	}

	if (current_pane == p_pane) {
		clear_current_pane();
	}
	p_pane->queue_free();
}

void PaneManager::set_current_pane(PaneBase *p_pane) {
	ERR_FAIL_NULL(p_pane);

	if (p_pane != current_pane) {
		if (current_pane) {
			current_pane->set_active(false);
			prev_pane = current_pane;
		}
		current_pane = p_pane;
		current_pane->set_active(true);

		emit_signal("pane_changed", p_pane);
	}
}

void PaneManager::clear_current_pane() {
	if (current_pane) {
		if (prev_pane == current_pane) {
			prev_pane = nullptr;
		}
		current_pane->set_active(false);
		current_pane = nullptr;
	}

	if (prev_pane) {
		current_pane = prev_pane;
		current_pane->set_active(true);
	}
}

PaneBase *PaneManager::get_current_pane() const {
	return current_pane;
}

PaneBase *PaneManager::get_prev_pane() const {
	return prev_pane;
}

void PaneManager::_bind_methods() {
	ADD_SIGNAL(MethodInfo("pane_changed", PropertyInfo(Variant::OBJECT, "pane")));
}

PaneManager::PaneManager() {
	singleton = this;

	// File management.
	file_system = memnew(FileSystem);
	add_child(file_system);

	register_pane<FilePane>(
			FilePane::get_class_static(),
			// theme->get_icon(SNAME("Folder"), SNAME("AppIcons")),
			Ref<Texture2D>(), // TODO
			callable_mp(this, &PaneManager::_on_file_pane_create));
	register_pane<NavigationPane>(
			NavigationPane::get_class_static(),
			// theme->get_icon(SNAME("Filesystem"), SNAME("AppIcons")),
			Ref<Texture2D>(),
			callable_mp(this, &PaneManager::_on_navigation_pane_create));

	// TODO: SettingsPane, WelcomePane
	// register_pane<SettingsPane>(
	// 		SettingsPane::get_class_static(),
	// 		theme->get_icon(SNAME("Settings"), SNAME("AppIcons"))); // TODO: icon
	// register_pane<WelcomePane>(
	// 		WelcomePane::get_class_static(),
	// 		theme->get_icon(SNAME("Welcome"), SNAME("AppIcons"))); // TODO: icon

	// TODO: set?
	pane_type = FilePane::get_class_static();
}

PaneManager::~PaneManager() {
	PaneFactory::clear();

	singleton = nullptr;
}
