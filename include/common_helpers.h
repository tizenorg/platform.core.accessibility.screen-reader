#ifndef COMMON_HELPERS_H_
#define COMMON_HELPERS_H_

#include <dlog.h>

#ifdef DEBUG
	#define PLOG(fmt, ...) \
		fprintf(stderr, "<D> %s(%d) --> ", __FUNCTION__, __LINE__); \
		fprintf(stderr, fmt, ##__VA_ARGS__);\
		fprintf(stderr, "\n");\
		LOGD(fmt, ##__VA_ARGS__);

	#define PLOGD(fmt, ...) \
		PLOG(fmt, ##__VA_ARGS__)

	#define PLOGW(fmt, ...) \
		fprintf(stderr, "<W> %s(%d) --> ", __FUNCTION__, __LINE__); \
		fprintf(stderr, fmt, ##__VA_ARGS__);\
		fprintf(stderr, "\n");\
		LOGD(fmt, ##__VA_ARGS__);

	#define PLOGE(fmt, ...) \
		fprintf(stderr, "<E> %s(%d) --> ", __FUNCTION__, __LINE__); \
		fprintf(stderr, fmt, ##__VA_ARGS__);\
		fprintf(stderr, "\n");\
		LOGD(fmt, ##__VA_ARGS__);

	#define PLOGI(fmt, ...) \
		fprintf(stderr, "<I> %s(%d) --> ", __FUNCTION__, __LINE__); \
		fprintf(stderr, fmt, ##__VA_ARGS__);\
		fprintf(stderr, "\n");\
		LOGD(fmt, ##__VA_ARGS__);

#else

	#define PLOG(fmt, ...) \
		LOGD(fmt, ##__VA_ARGS__);

	#define PLOGD(fmt, ...) \
		PLOG(fmt, ##__VA_ARGS__);

	#define PLOGW(fmt, ...) \
		LOGW(fmt, ##__VA_ARGS__);

	#define PLOGE(fmt, ...) \
		LOGE(fmt, ##__VA_ARGS__);

	#define PLOGI(fmt, ...) \
		LOGI(fmt, ##__VA_ARGS__);\

#endif

#endif /* COMMON_HELPERS_H_ */
