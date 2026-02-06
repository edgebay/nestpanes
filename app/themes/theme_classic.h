#pragma once

#include "app/themes/app_theme_manager.h"

class ThemeClassic {
public:
	static void populate_shared_styles(const Ref<AppTheme> &p_theme, AppThemeManager::ThemeConfiguration &p_config);
	static void populate_standard_styles(const Ref<AppTheme> &p_theme, AppThemeManager::ThemeConfiguration &p_config);
	static void populate_app_styles(const Ref<AppTheme> &p_theme, AppThemeManager::ThemeConfiguration &p_config);
};
