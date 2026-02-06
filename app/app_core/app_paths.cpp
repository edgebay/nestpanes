#include "app_paths.h"

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/os/os.h"
#include "core/version.h"
#include "main/main.h"

AppPaths *AppPaths::singleton = nullptr;

bool AppPaths::are_paths_valid() const {
	return paths_valid;
}

String AppPaths::get_config_dir() const {
	return config_dir;
}

String AppPaths::get_cache_dir() const {
	return cache_dir;
}

String AppPaths::get_temp_dir() const {
	return temp_dir;
}

void AppPaths::create() {
	memnew(AppPaths);
}

void AppPaths::free() {
	ERR_FAIL_NULL(singleton);
	memdelete(singleton);
	singleton = nullptr;
}

void AppPaths::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_config_dir"), &AppPaths::get_config_dir);
	ClassDB::bind_method(D_METHOD("get_cache_dir"), &AppPaths::get_cache_dir);
}

AppPaths::AppPaths() {
	ERR_FAIL_COND(singleton != nullptr);
	singleton = this;

	// Paths.

	// @ref "application/config/custom_user_dir_name"
	String app_dir_name = String(APP_VERSION_NAME);

	String config_path = OS::get_singleton()->get_config_path();
	config_dir = config_path.path_join(app_dir_name);

	String cache_path = OS::get_singleton()->get_cache_path();
	if (cache_path == config_path) {
		cache_dir = config_dir.path_join("cache");
	} else {
		cache_dir = cache_path.path_join(app_dir_name);
	}

	temp_dir = OS::get_singleton()->get_temp_path();

	paths_valid = !config_path.is_empty();
	ERR_FAIL_COND_MSG(!paths_valid, "App config paths are invalid.");

	// Validate or create each dir and its relevant subdirectories.

	Ref<DirAccess> dir = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);

	// Config dir.
	{
		if (dir->change_dir(config_dir) != OK) {
			dir->make_dir_recursive(config_dir);
			if (dir->change_dir(config_dir) != OK) {
				ERR_PRINT("Could not create config directory: " + config_dir);
				paths_valid = false;
			}
		}
	}

	// Cache dir.
	{
		if (dir->change_dir(cache_dir) != OK) {
			dir->make_dir_recursive(cache_dir);
			if (dir->change_dir(cache_dir) != OK) {
				ERR_PRINT("Could not create cache directory: " + cache_dir);
			}
		}
	}

	// Temporary dir.
	{
		if (dir->change_dir(temp_dir) != OK) {
			dir->make_dir_recursive(temp_dir);
			if (dir->change_dir(temp_dir) != OK) {
				ERR_PRINT("Could not create temporary directory: " + temp_dir);
			}
		}
	}
}
