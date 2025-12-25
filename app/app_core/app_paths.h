#pragma once

#include "core/object/class_db.h"
#include "core/object/object.h"
#include "core/string/ustring.h"

class AppPaths : public Object {
	GDCLASS(AppPaths, Object)

	bool paths_valid = false; // If config dir can't be created, this is false.
	String config_dir; // APPDATA, default AppData\Roaming
	String cache_dir; // LOCALAPPDATA, default AppData\Local
	String temp_dir; // default AppData\Local\Temp

	static AppPaths *singleton;

protected:
	static void _bind_methods();

public:
	bool are_paths_valid() const;

	String get_config_dir() const;
	String get_cache_dir() const;
	String get_temp_dir() const;

	static AppPaths *get_singleton() {
		return singleton;
	}

	static void create();
	static void free();

	AppPaths();
};
