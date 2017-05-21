#ifndef	__FBO_HELPER_H__
#define	__FBO_HELPER_H__

#include <nemoconfig.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <GLES2/gl2.h>

extern int fbo_prepare_context(GLuint tex, int32_t width, int32_t height, GLuint *fbo, GLuint *dbo);

#ifdef __cplusplus
}
#endif

#endif
