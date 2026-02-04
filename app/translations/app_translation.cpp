#include "app_translation.h"

#include "core/io/compression.h"
#include "core/io/file_access_memory.h"
#include "core/io/translation_loader_po.h"
#include "core/string/translation_server.h"

#include "app/translations/app_translations.gen.h"

Vector<String> get_locales() {
	Vector<String> locales;

	const AppTranslationList *tl = _app_translations;
	while (tl->data) {
		const String &locale = tl->lang;
		locales.push_back(locale);

		tl++;
	}

	return locales;
}

void load_translations(const String &p_locale) {
	// Use the default main_domain.
	const Ref<TranslationDomain> domain = TranslationServer::get_singleton()->get_or_add_domain(StringName());

	const AppTranslationList *tl = _app_translations;
	while (tl->data) {
		if (tl->lang == p_locale) {
			Vector<uint8_t> data;
			data.resize(tl->uncomp_size);
			const int64_t ret = Compression::decompress(data.ptrw(), tl->uncomp_size, tl->data, tl->comp_size, Compression::MODE_DEFLATE);
			ERR_FAIL_COND_MSG(ret == -1, "Compressed file is corrupt.");

			Ref<FileAccessMemory> fa;
			fa.instantiate();
			fa->open_custom(data.ptr(), data.size());

			Ref<Translation> tr = TranslationLoaderPO::load_translation(fa);

			if (tr.is_valid()) {
				tr->set_locale(tl->lang);
				domain->add_translation(tr);
				break;
			}
		}

		tl++;
	}
}
