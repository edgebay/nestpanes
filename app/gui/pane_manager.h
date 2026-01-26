#pragma once

#include "core/object/object.h"

#include "app/gui/pane_base.h"
#include "core/templates/hash_map.h"

// TODO: PaneFactory
class PaneManager : public Object {
	GDCLASS(PaneManager, Object);

public:
	typedef PaneBase *(*CreatePaneFunc)();
	struct PaneInfo {
		StringName type;
		Ref<Texture2D> icon;
		CreatePaneFunc create_func;
	};

private:
	static PaneManager *singleton;
	// static HashMap<StringName, CreatePaneFunc> create_func;
	static HashMap<StringName, PaneInfo> pane_map;

protected:
	template <typename T>
	static PaneBase *_create_pane() {
		return memnew(T);
	}

public:
	static PaneManager *get_singleton() { return singleton; }

	// template <typename T>
	// static void register_pane(const StringName &p_type) {
	// 	ERR_FAIL_COND_MSG(create_func.has(p_type), vformat("Pane already registered: '%s'.", p_type));
	// 	create_func.insert(p_type, _create_pane<T>);
	// }

	// static PaneBase *create_pane(const StringName &p_type) {
	// 	PaneBase *pane = nullptr;
	// 	if (create_func.has(p_type)) {
	// 		pane = create_func.get(p_type)();
	// 	}
	// 	return pane;
	// }

	// static void get_pane_list(List<StringName> *r_panes);

	template <typename T>
	static void register_pane(const StringName &p_type, const Ref<Texture2D> &p_icon) {
		ERR_FAIL_COND_MSG(pane_map.has(p_type), vformat("Pane already registered: '%s'.", p_type));
		PaneInfo info;
		info.type = p_type;
		info.icon = p_icon;
		info.create_func = _create_pane<T>;

		pane_map[p_type] = info;
	}

	static PaneBase *create_pane(const StringName &p_type) {
		PaneBase *pane = nullptr;
		if (pane_map.has(p_type)) {
			pane = pane_map.get(p_type).create_func();
		}
		return pane;
	}

	static void get_pane_list(List<PaneInfo> *r_panes);
	static bool get_pane_info(const StringName &p_type, PaneInfo *r_info);

	PaneManager();
	~PaneManager();
};
