#include "version_button.h"

#include "core/os/time.h"
#include "core/version.h"

String _get_version_string(VersionButton::VersionFormat p_format) {
	String main;
	switch (p_format) {
		case VersionButton::FORMAT_BASIC: {
			return APP_VERSION_FULL_CONFIG;
		} break;
		case VersionButton::FORMAT_WITH_BUILD: {
			main = "v" APP_VERSION_FULL_BUILD;
		} break;
		case VersionButton::FORMAT_WITH_NAME_AND_BUILD: {
			main = APP_VERSION_FULL_NAME;
		} break;
		default: {
			ERR_FAIL_V_MSG(APP_VERSION_FULL_NAME, "Unexpected format: " + itos(p_format));
		} break;
	}

	String hash = APP_VERSION_HASH;
	if (!hash.is_empty()) {
		hash = vformat(" [%s]", hash.left(9));
	}
	return main + hash;
}

void VersionButton::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_POSTINITIALIZE: {
			// This can't be done in the constructor because theme cache is not ready yet.
			set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
			set_text(_get_version_string(format));
		} break;

		case NOTIFICATION_TRANSLATION_CHANGED: {
			String build_date;
			if (APP_VERSION_TIMESTAMP > 0) {
				build_date = Time::get_singleton()->get_datetime_string_from_unix_time(APP_VERSION_TIMESTAMP, true) + " UTC";
			} else {
				build_date = RTR("(unknown)");
			}
			set_tooltip_text(vformat(RTR("Git commit date: %s\nClick to copy the version information."), build_date));
		} break;
	}
}

void VersionButton::pressed() {
	DisplayServer::get_singleton()->clipboard_set(_get_version_string(FORMAT_WITH_BUILD));
}

VersionButton::VersionButton(VersionFormat p_format) {
	format = p_format;
	set_underline_mode(LinkButton::UNDERLINE_MODE_ON_HOVER);
}
