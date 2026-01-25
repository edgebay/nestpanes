#include "pane_manager.h"

PaneManager *PaneManager::singleton = nullptr;
HashMap<StringName, PaneManager::CreatePaneFunc> PaneManager::create_func;

PaneManager::PaneManager() {
	singleton = this;
}

PaneManager::~PaneManager() {
	create_func.clear();

	singleton = nullptr;
}
