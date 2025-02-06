/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2025 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "web.h"
#if defined BITTY_OS_HTML
#	include "web_html.h"
#else /* BITTY_OS_HTML */
#	include "web_curl.h"
#endif /* BITTY_OS_HTML */
#include "web_civetweb.h"

/*
** {===========================================================================
** Macros and constants
*/

#if !BITTY_WEB_ENABLED
#	pragma message("Web disabled.")
#endif /* BITTY_WEB_ENABLED */

/* ===========================================================================} */

/*
** {===========================================================================
** Fetch
*/

#if BITTY_WEB_ENABLED

Fetch* Fetch::create(void) {
#if defined BITTY_OS_HTML
	FetchHtml* result = new FetchHtml();

	return result;
#else /* BITTY_OS_HTML */
	FetchCurl* result = new FetchCurl();

	return result;
#endif /* BITTY_OS_HTML */
}

void Fetch::destroy(Fetch* ptr) {
#if defined BITTY_OS_HTML
	FetchHtml* impl = static_cast<FetchHtml*>(ptr);
	delete impl;
#else /* BITTY_OS_HTML */
	FetchCurl* impl = static_cast<FetchCurl*>(ptr);
	delete impl;
#endif /* BITTY_OS_HTML */
}

#endif /* BITTY_WEB_ENABLED */

/* ===========================================================================} */

/*
** {===========================================================================
** Web
*/

#if BITTY_WEB_ENABLED

Web* Web::create(const char* type) {
#if defined BITTY_OS_HTML
	Web* result = nullptr;
	if (strcmp(type, "html") == 0 || strcmp(type, "default") == 0) {
		result = new WebHtml();
	} else {
		assert(false && "Unknown backend type.");
	}

	return result;
#else /* BITTY_OS_HTML */
	Web* result = nullptr;
	if (strcmp(type, "civetweb") == 0 || strcmp(type, "default") == 0) {
		result = new WebCivetWeb();
	} else {
		assert(false && "Unknown backend type.");
	}

	return result;
#endif /* BITTY_OS_HTML */
}

void Web::destroy(Web* ptr) {
#if defined BITTY_OS_HTML
	switch (ptr->type()) {
	case WebHtml::TYPE(): {
			WebHtml* impl = static_cast<WebHtml*>(ptr);
			delete impl;
		}

		break;
	default:
		assert(false && "Unknown backend type.");

		break;
	}
#else /* BITTY_OS_HTML */
	switch (ptr->type()) {
	case WebCivetWeb::TYPE(): {
			WebCivetWeb* impl = static_cast<WebCivetWeb*>(ptr);
			delete impl;
		}

		break;
	default:
		assert(false && "Unknown backend type.");

		break;
	}
#endif /* BITTY_OS_HTML */
}

#endif /* BITTY_WEB_ENABLED */

/* ===========================================================================} */
