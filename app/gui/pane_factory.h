#pragma once

#include "core/object/object.h"

#include "app/gui/pane_base.h"
#include "core/templates/hash_map.h"

class PaneFactory : public Object {
	GDCLASS(PaneFactory, Object);

public:
	typedef PaneBase *(*CreatePaneFunc)();
	struct PaneInfo {
		StringName type;
		Ref<Texture2D> icon;
		CreatePaneFunc create_func;
		Callable create_callback;
	};

private:
	static PaneFactory *singleton;
	static HashMap<StringName, PaneInfo> pane_map;

protected:
	template <typename T>
	static PaneBase *_create_pane() {
		return memnew(T);
	}

public:
	static PaneFactory *get_singleton() { return singleton; }

	template <typename T>
	static void register_pane(const StringName &p_type, const Ref<Texture2D> &p_icon, const Callable &p_create_callback = Callable(), CreatePaneFunc p_create_func = nullptr) {
		ERR_FAIL_COND_MSG(pane_map.has(p_type), vformat("Pane already registered: '%s'.", p_type));
		PaneInfo info;
		info.type = p_type;
		info.icon = p_icon;
		info.create_func = p_create_func ? p_create_func : _create_pane<T>;
		info.create_callback = p_create_callback;

		pane_map[p_type] = info;
	}

	static PaneBase *create_pane(const StringName &p_type) {
		PaneBase *pane = nullptr;
		if (pane_map.has(p_type)) {
			const PaneInfo &info = pane_map.get(p_type);
			pane = info.create_func();
			if (pane && info.create_callback.is_valid()) {
				info.create_callback.call(pane);
			}
		}
		return pane;
	}

	static void get_pane_list(List<PaneInfo> *r_panes);
	static bool get_pane_info(const StringName &p_type, PaneInfo *r_info);

	PaneFactory();
	~PaneFactory();
};
