#include "pane_factory.h"

PaneFactory *PaneFactory::singleton = nullptr;

HashMap<StringName, PaneFactory::PaneInfo> PaneFactory::pane_map;

void PaneFactory::get_pane_list(List<PaneInfo> *r_panes) {
	for (KeyValue<StringName, PaneInfo> &E : pane_map) {
		r_panes->push_back(E.value);
	}
}

bool PaneFactory::get_pane_info(const StringName &p_type, PaneInfo *r_info) {
	if (pane_map.has(p_type)) {
		PaneInfo info = pane_map[p_type];
		if (r_info) {
			*r_info = info;
		}
		return true;
	}
	return false;
}

PaneFactory::PaneFactory() {
	singleton = this;
}

PaneFactory::~PaneFactory() {
	pane_map.clear();

	singleton = nullptr;
}
