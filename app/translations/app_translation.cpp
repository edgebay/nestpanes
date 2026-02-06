#include "app_translation.h"

#include "core/io/compression.h"
#include "core/io/file_access_memory.h"
#include "core/io/translation_loader_po.h"
#include "core/string/translation_server.h"

#include "app/translations/app_translations.gen.h"

Vector<String> get_locales() {
	Vector<String> locales;

	for (const AppTranslationList *etl = _app_translations; etl->data; etl++) {
		const String &locale = etl->lang;
		locales.push_back(locale);
	}

	return locales;
}

static void _load(const Ref<TranslationDomain> p_domain, const String &p_locale, const AppTranslationList *p_etl) {
	for (const AppTranslationList *etl = p_etl; etl->data; etl++) {
		if (etl->lang == p_locale) {
			LocalVector<uint8_t> data;
			data.resize_uninitialized(etl->uncomp_size);
			const int64_t ret = Compression::decompress(data.ptr(), etl->uncomp_size, etl->data, etl->comp_size, Compression::MODE_DEFLATE);
			ERR_FAIL_COND_MSG(ret == -1, "Compressed file is corrupt.");

			Ref<FileAccessMemory> fa;
			fa.instantiate();
			fa->open_custom(data.ptr(), data.size());

			Ref<Translation> tr = TranslationLoaderPO::load_translation(fa);
			if (tr.is_valid()) {
				tr->set_locale(etl->lang);
				p_domain->add_translation(tr);
				break;
			}
		}
	}
}

void load_translations(const String &p_locale) {
	Ref<TranslationDomain> domain;

	domain = TranslationServer::get_singleton()->get_main_domain();
	domain->clear();
	_load(domain, p_locale, _app_translations);
}
