#pragma once

#include "core/string/string_name.h"

class AppStringNames {
	inline static AppStringNames *singleton = nullptr;

public:
	static void create() { singleton = memnew(AppStringNames); }
	static void free() {
		memdelete(singleton);
		singleton = nullptr;
	}

	_FORCE_INLINE_ static AppStringNames *get_singleton() { return singleton; }

	const StringName App = StringName("App");
	const StringName AppFonts = StringName("AppFonts");
	const StringName AppIcons = StringName("AppIcons");
	const StringName AppStyles = StringName("AppStyles");
};

#define AppStringName(m_name) AppStringNames::get_singleton()->m_name
