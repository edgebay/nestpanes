#include "pane_manager.h"

PaneManager *PaneManager::singleton = nullptr;
// HashMap<StringName, PaneManager::CreatePaneFunc> PaneManager::create_func;

// void PaneManager::get_pane_list(List<StringName> *r_panes) {
// 	for (const KeyValue<StringName, CreatePaneFunc> &E : create_func) {
// 		r_panes->push_back(E.key);
// 	}

// 	// r_panes->sort_custom<StringName::AlphCompare>();	// TODO: sort
// }

HashMap<StringName, PaneManager::PaneInfo> PaneManager::pane_map;

void PaneManager::get_pane_list(List<PaneInfo> *r_panes) {
	for (KeyValue<StringName, PaneInfo> &E : pane_map) {
		r_panes->push_back(E.value);
	}
}

bool PaneManager::get_pane_info(const StringName &p_type, PaneInfo *r_info) {
	if (pane_map.has(p_type)) {
		PaneInfo info = pane_map[p_type];
		if (r_info) {
			*r_info = info;
		}
		return true;
	}
	return false;
}

PaneManager::PaneManager() {
	singleton = this;
}

PaneManager::~PaneManager() {
	pane_map.clear();

	singleton = nullptr;
}
