#ifndef BACKWARDCOMPATIBILITY_H
#define BACKWARDCOMPATIBILITY_H

extern "C" {
#include <libavcodec/avcodec.h>
}

//These macros was prefixed with AV_ in ffmeg and broke API compatibility
//The backward compatible aliases was dropped from the trunk, so I put them back here
#ifndef AV_CODEC_CAP_TRUNCATED
	#define AV_CODEC_CAP_TRUNCATED CODEC_CAP_TRUNCATED
	#define AV_CODEC_FLAG_TRUNCATED CODEC_FLAG_TRUNCATED
	#define AV_CODEC_FLAG_PSNR CODEC_FLAG_PSNR
	#define AV_CODEC_CAP_PARAM_CHANGE CODEC_CAP_PARAM_CHANGE
	#define AV_CODEC_FLAG2_CHUNKS CODEC_FLAG2_CHUNKS
#endif

#endif // BACKWARDCOMPATIBILITY_H
