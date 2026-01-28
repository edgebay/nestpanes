#include "register_app_types.h"

#include "core/os/os.h"

#include "app/app_core/app_paths.h"
#include "app/app_modules/settings/app_settings.h"
#include "app/app_string_names.h"

#include "app/gui/app_tab_container.h"
#include "app/gui/multi_split_container.h"

#include "app/gui/welcome_pane.h"

#include "app/app_modules/settings/gui/settings_pane.h"
#include "app/gui/inspector/object_inspector.h"
#include "app/gui/inspector/sectioned_inspector.h"

#include "app/app_modules/file_management/file_system_list.h"
#include "app/app_modules/file_management/file_system_tree.h"
#include "app/gui/file_system_control.h"

#include "app/app_modules/text_editing/text_editor.h"

void register_app_types() {
	OS::get_singleton()->benchmark_begin_measure("App", "Register Types");

	AppStringNames::create();

	GDREGISTER_CLASS(AppPaths);
	GDREGISTER_CLASS(AppSettings);

	GDREGISTER_CLASS(AppTabContainer);
	GDREGISTER_CLASS(MultiSplitContainer);
	GDREGISTER_CLASS(HMultiSplitContainer);
	GDREGISTER_CLASS(VMultiSplitContainer);

	GDREGISTER_CLASS(WelcomePane);

	GDREGISTER_CLASS(ObjectInspector);
	GDREGISTER_CLASS(SectionedInspector);
	GDREGISTER_CLASS(SettingsPane);

	GDREGISTER_CLASS(FileSystemControl);
	GDREGISTER_CLASS(FileSystemList);
	GDREGISTER_CLASS(FileSystemTree);

	GDREGISTER_CLASS(TextEditor);

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
