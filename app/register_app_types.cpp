#include "register_app_types.h"

#include "core/os/os.h"

#include "app/app_string_names.h"
#include "app/paths/app_paths.h"
#include "app/settings/app_settings.h"

void register_app_types() {
	OS::get_singleton()->benchmark_begin_measure("App", "Register Types");

	AppStringNames::create();

	GDREGISTER_CLASS(AppPaths);
	GDREGISTER_CLASS(AppSettings);

	OS::get_singleton()->benchmark_end_measure("App", "Register Types");
}

void unregister_app_types() {
	OS::get_singleton()->benchmark_begin_measure("App", "Unregister Types");

	if (AppPaths::get_singleton()) {
		AppPaths::free();
	}
	AppStringNames::free();

	OS::get_singleton()->benchmark_end_measure("App", "Unregister Types");
}
