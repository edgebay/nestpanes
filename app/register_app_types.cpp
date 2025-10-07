#include "register_app_types.h"

#include "core/os/os.h"

#include "app/app_string_names.h"

void register_app_types() {
	OS::get_singleton()->benchmark_begin_measure("App", "Register Types");

	AppStringNames::create();

	OS::get_singleton()->benchmark_end_measure("App", "Register Types");
}

void unregister_app_types() {
	OS::get_singleton()->benchmark_begin_measure("App", "Unregister Types");

	AppStringNames::free();

	OS::get_singleton()->benchmark_end_measure("App", "Unregister Types");
}
