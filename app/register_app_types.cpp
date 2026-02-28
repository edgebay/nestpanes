#include "register_app_types.h"

#include "core/os/os.h"

#include "app/app_core/app_paths.h"
#include "app/app_string_names.h"
#include "app/settings/app_settings.h"

#include "app/gui/app_tab_container.h"
#include "app/gui/multi_split_container.h"

#include "app/gui/welcome_pane.h"

void register_app_types() {
	OS::get_singleton()->benchmark_begin_measure("App", "Register Types");

	AppStringNames::create();

	GDREGISTER_CLASS(AppPaths);
	GDREGISTER_CLASS(AppSettings);

	GDREGISTER_CLASS(AppTabContainer);
	GDREGISTER_CLASS(MultiSplitContainer);

	GDREGISTER_CLASS(PaneBase);
	GDREGISTER_CLASS(WelcomePane);

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
