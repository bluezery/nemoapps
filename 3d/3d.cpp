#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <getopt.h>
#include <libgen.h>

#include <nemotool.h>
#include <nemotimer.h>
#include <nemoshow.h>

//#include <nemopoly.h>

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include "nemoutil.h"
#include "widgets.h"
#include "nemoui.h"

#include "showhelper.h"
#include "glhelper.h"
#include "meshhelper.h"
#include "fbohelper.h"

#define COL_ACTIVE      0x1EDCDCFF
#define COL_BASE        0x0F6E6EFF
#define COL_INACTIVE    0x094242FF
#define COL_BLINK       0xFF8C32FF

#define	NEMOMESH_SHADERS_MAX			(2)

typedef struct _Context Context;
struct _Context {
    bool lock;
    int state;
    struct nemoshow *show;
	struct nemotool *tool;

	struct nemocanvas *canvas;

    struct showone *mesh_canvas;

	int32_t width, height;
	float aspect;

	GLuint fbo, dbo;

	GLuint programs[NEMOMESH_SHADERS_MAX];

	GLuint program;
	GLuint umvp;
	GLuint uprojection;
	GLuint umodelview;
	GLuint ulight;
	GLuint ucolor;

	struct nemomatrix projection;

	struct nemomesh *mesh;
};

static const char *simple_vertex_shader =
"uniform mat4 mvp;\n"
"uniform mat4 projection;\n"
"uniform mat4 modelview;\n"
"attribute vec3 vertex;\n"
"attribute vec3 normal;\n"
"attribute vec3 diffuse;\n"
"void main() {\n"
"  gl_Position = mvp * vec4(vertex, 1.0);\n"
"}\n";

static const char *simple_fragment_shader =
"precision mediump float;\n"
"uniform vec4 light;\n"
"uniform vec4 color;\n"
"void main() {\n"
"  gl_FragColor = color;\n"
"}\n";

static const char *light_vertex_shader =
"uniform mat4 mvp;\n"
"uniform mat4 projection;\n"
"uniform mat4 modelview;\n"
"uniform vec4 light;\n"
"attribute vec3 vertex;\n"
"attribute vec3 normal;\n"
"attribute vec3 diffuse;\n"
"varying vec3 vnormal;\n"
"varying vec3 vlight;\n"
"varying vec3 vdiffuse;\n"
"void main() {\n"
"  gl_Position = mvp * vec4(vertex, 1.0);\n"
"  vlight = normalize(light.xyz - mat3(modelview) * vertex);\n"
"  vnormal = normalize(mat3(modelview) * normal);\n"
"  vdiffuse = diffuse;\n"
"}\n";

static const char *light_fragment_shader =
"precision mediump float;\n"
"uniform vec4 color;\n"
"varying vec3 vlight;\n"
"varying vec3 vnormal;\n"
"varying vec3 vdiffuse;\n"
"void main() {\n"
"  gl_FragColor = vec4(vdiffuse * max(dot(vlight, vnormal), 0.0), 1.0) * color;\n"
"}\n";

static GLuint nemomesh_create_shader(const char *fshader, const char *vshader)
{
	GLuint frag, vert;
	GLuint program;
	GLint status;

	frag = gl_compile_shader(GL_FRAGMENT_SHADER, 1, &fshader);
	vert = gl_compile_shader(GL_VERTEX_SHADER, 1, &vshader);

	program = glCreateProgram();
	glAttachShader(program, frag);
	glAttachShader(program, vert);
	glLinkProgram(program);

	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (!status) {
		char log[1000];
		GLsizei len;
		glGetProgramInfoLog(program, 1000, &len, log);
		fprintf(stderr, "Error: linking:\n%*s\n", len, log);
		exit(1);
	}

	glUseProgram(program);

	glBindAttribLocation(program, 0, "vertex");
	glBindAttribLocation(program, 1, "normal");
	glBindAttribLocation(program, 2, "diffuse");
	glLinkProgram(program);

	return program;
}

static void nemomesh_prepare_shader(Context *ctx, GLuint program)
{
	if (ctx->program == program)
		return;

	ctx->program = program;

	ctx->umvp = glGetUniformLocation(ctx->program, "mvp");
	ctx->uprojection = glGetUniformLocation(ctx->program, "projection");
	ctx->umodelview = glGetUniformLocation(ctx->program, "modelview");
	ctx->ulight = glGetUniformLocation(ctx->program, "light");
	ctx->ucolor = glGetUniformLocation(ctx->program, "color");
}

static void nemomesh_render_object(Context *ctx, struct nemomesh *mesh)
{
	struct nemomatrix matrix;

	matrix = mesh->modelview;
	nemomatrix_multiply(&matrix, &ctx->projection);

	glUniformMatrix4fv(ctx->umvp, 1, GL_FALSE, (GLfloat *)matrix.d);
	glUniformMatrix4fv(ctx->umodelview, 1, GL_FALSE, (GLfloat *)mesh->modelview.d);

	glBindVertexArray(mesh->varray);
	glDrawArrays(mesh->mode, 0, mesh->elements);
	glBindVertexArray(0);

	if (mesh->on_guides != 0) {
		glPointSize(10.0f);

		glBindVertexArray(mesh->garray);
		glDrawArrays(GL_LINES, 0, mesh->nguides);
		glBindVertexArray(0);
	}
}

static void nemomesh_render_scene(Context *ctx)
{
	struct nemovector light = { 1.0f, 1.0f, 1.0f, 1.0f };
	GLfloat color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

	glUseProgram(ctx->program);

	glBindFramebuffer(GL_FRAMEBUFFER, ctx->fbo);

	glViewport(0, 0, ctx->width, ctx->height);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUniformMatrix4fv(ctx->uprojection, 1, GL_FALSE, (GLfloat *)ctx->projection.d);
	glUniform4fv(ctx->ulight, 1, light.f);
	glUniform4fv(ctx->ucolor, 1, color);

	nemomesh_render_object(ctx, ctx->mesh);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glBindTexture(GL_TEXTURE_2D, 0);
}

#if 0
static void nemomesh_unproject(Context *ctx, struct nemomesh *mesh, float x, float y, float z, float *out)
{
	struct nemomatrix matrix;
	struct nemomatrix inverse;
	struct nemovector in;

	matrix = mesh->modelview;
	nemomatrix_multiply(&matrix, &ctx->projection);

	nemomatrix_invert(&inverse, &matrix);

	in.f[0] = x * 2.0f / ctx->width - 1.0f;
	in.f[1] = y * 2.0f / ctx->height - 1.0f;
	in.f[2] = z * 2.0f - 1.0f;
	in.f[3] = 1.0f;

	nemomatrix_transform(&inverse, &in);

	if (fabsf(in.f[3]) < 1e-6) {
		out[0] = 0.0f;
		out[1] = 0.0f;
		out[2] = 0.0f;
	} else {
		out[0] = in.f[0] / in.f[3];
		out[1] = in.f[1] / in.f[3];
		out[2] = in.f[2] / in.f[3];
	}
}

static int nemomesh_pick_object(Context *ctx, struct nemomesh *mesh, float x, float y)
{
	float near[3], far[3];
	float rayorg[3];
	float rayvec[3];
	float raylen;
	float mint, maxt;

	nemomesh_unproject(ctx, mesh, x, y, -1.0f, near);
	nemomesh_unproject(ctx, mesh, x, y, 1.0f, far);

	rayvec[0] = far[0] - near[0];
	rayvec[1] = far[1] - near[1];
	rayvec[2] = far[2] - near[2];

	raylen = sqrtf(rayvec[0] * rayvec[0] + rayvec[1] * rayvec[1] + rayvec[2] * rayvec[2]);

	rayvec[0] /= raylen;
	rayvec[1] /= raylen;
	rayvec[2] /= raylen;

	rayorg[0] = near[0];
	rayorg[1] = near[1];
	rayorg[2] = near[2];

	return nemopoly_cube_intersect(mesh->boundingbox, rayorg, rayvec, &mint, &maxt);
}
#endif

static void _mesh_event(NemoWidget *opengl, const char *id, void *info, void *userdata)
{
    struct nemoshow *show = nemowidget_get_show(opengl);
    struct showevent *event = (struct showevent *)info;

	Context *ctx = (Context *)userdata;
    RET_IF(!ctx->mesh || !ctx->mesh_canvas);

    if (nemoshow_event_is_down(show, event)) {
        if (ctx->lock) {
            if (ctx->state == 1) {
                // TODO: Use grab by using nemomesh_pick_object
                struct nemomesh *mesh = ctx->mesh;
                nemomesh_reset_quaternion(mesh,
                        ctx->width,
                        ctx->height,
                        nemoshow_event_get_x_on(event, 0),
                        nemoshow_event_get_y_on(event, 0));
            }
        }
    } else if (nemoshow_event_is_motion(show, event)) {
        if (ctx->lock) {
            if (ctx->state == 1) {
                struct nemomesh *mesh = ctx->mesh;
                nemomesh_update_quaternion(mesh,
                        ctx->width,
                        ctx->height,
                        nemoshow_event_get_x_on(event, 0),
                        nemoshow_event_get_y_on(event, 0));
            }
        }
    }

    if (nemoshow_event_is_single_click(show, event)) {
        struct nemomesh *mesh = ctx->mesh;

        if (nemoshow_event_is_double_taps(show, event)) {
            nemomesh_turn_off_guides(mesh);
        }

#if 0
        struct nemomesh *mesh = ctx->mesh;
        int plane = nemomesh_pick_object(ctx, mesh, event->x, event->y);

        if (plane != NEMOMETRO_NONE_PLANE) {
            if (mesh->mode == GL_TRIANGLES)
                nemomesh_prepare_buffer(mesh, GL_LINES, mesh->lines, mesh->nlines);
            else
                nemomesh_prepare_buffer(mesh, GL_TRIANGLES, mesh->meshes, mesh->nmeshes);

            nemowidget_dirty(opengl);
        }
#endif
    }
    nemoshow_dispatch_frame(show);
}

static void _mesh_frame(NemoWidget *opengl, const char *id, void *info, void *userdata)
{
    Context *ctx = (Context *)userdata;
    RET_IF(!ctx->mesh || !ctx->mesh_canvas);

	nemomesh_update_transform(ctx->mesh);
	nemomesh_render_scene(ctx);
}

static void _mesh_resize(NemoWidget *opengl, const char *id, void *info, void *userdata)
{
    NemoWidgetInfo_Resize *resize = (NemoWidgetInfo_Resize *)info;
	Context *ctx = (Context *)userdata;
    // FIXME: resize can be called before set content mesh canvas
    RET_IF(!ctx->mesh || !ctx->mesh_canvas);

    int width = resize->width;
    int height = resize->height;

	ctx->width = width;
	ctx->height = height;

	glDeleteFramebuffers(1, &ctx->fbo);
	glDeleteRenderbuffers(1, &ctx->dbo);

	fbo_prepare_context(
            nemoshow_canvas_get_texture(ctx->mesh_canvas),
			width, height,
			&ctx->fbo, &ctx->dbo);
    nemowidget_dirty(opengl);
}

static void _frame_clicked(NemoWidget *frame, const char *id, void *info, void *userdata)
{
    Context *ctx = (Context *)userdata;
    if (!info) return;

    if (!strcmp(id, "scale,clicked")) {
        ctx->state = 0;
    } else if (!strcmp(id, "rotate,clicked")) {
        ctx->state = 1;
    } else if (!strcmp(id, "move,clicked")) {
        ctx->state = 2;
    } else if (!strcmp(id, "lock,clicked")) {
        NemoWidget *win = nemowidget_get_top_widget(frame);
        if (ctx->lock) {
            ctx->lock = false;
            nemowidget_win_enable_move(win, 1);
            nemowidget_win_enable_scale(win, 2);
            nemowidget_win_enable_rotate(win, 2);
        } else {
            ctx->lock = true;
            nemowidget_win_enable_move(win, 0);
            nemowidget_win_enable_scale(win, 0);
            nemowidget_win_enable_rotate(win, 0);
        }
    } else if (!strcmp(id, "quit,clicked")) {
        NemoWidget *win = nemowidget_get_top_widget(frame);
        nemowidget_win_exit(win);
    }
    nemoshow_dispatch_frame(ctx->show);
}

static void _show_end(struct nemotimer *timer, void *userdata)
{
    Context *ctx = (Context *)userdata;

    _nemoshow_item_motion(ctx->mesh_canvas, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0,
            "alpha", 1.0f,
            NULL);
}

int main(int argc, char *argv[])
{
	struct option options[] = {
		{ "file",				required_argument,			NULL,		'f' },
		{ "path",				required_argument,			NULL,		'p' },
		{ "type",				required_argument,			NULL,		't' },
		{ 0 }
	};
	Context *ctx;
	struct nemomesh *mesh;
	char *filepath = NULL;
	char *basepath = NULL;
	int opt;

	while ((opt = getopt_long(argc, argv, "f:p:t:", options, NULL))) {
		if (opt == -1)
			break;

		switch (opt) {
			case 'f':
				filepath = strdup(optarg);
				break;

			case 'p':
				basepath = strdup(optarg);
				break;

			default:
				break;
		}
	}

	if (filepath == NULL) {
        ERR("Usage: nemo3d -f [filename]");
        return -1;
    }

    Config *base;
    base = config_load(PROJECT_NAME, APPNAME, CONFXML, argc, argv);

	ctx = (Context *)calloc(sizeof(Context), 1);
	ctx->width = base->width;
	ctx->height = base->height;
	ctx->aspect = (double)base->height/base->width;

    struct nemotool *tool = TOOL_CREATE();

    NemoWidget *win;
    win = nemowidget_create_win_base(tool, APPNAME, base);
    nemowidget_set_data(win, "context", ctx);

    ctx->show = nemowidget_get_show(win);

    NemoWidget *frame = nemowidget_create(&NEMOWIDGET_FRAME, win, ctx->width, ctx->height);
    nemowidget_frame_set_color(frame,
            COL_INACTIVE, COL_INACTIVE, COL_ACTIVE, COL_BLINK);
    nemowidget_append_callback(frame, "scale,clicked", _frame_clicked, ctx);
    nemowidget_append_callback(frame, "rotate,clicked", _frame_clicked, ctx);
    nemowidget_append_callback(frame, "move,clicked", _frame_clicked, ctx);
    nemowidget_append_callback(frame, "lock,clicked", _frame_clicked, ctx);
    nemowidget_append_callback(frame, "quit,clicked", _frame_clicked, ctx);
    nemowidget_show(frame, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0);

	nemomatrix_init_identity(&ctx->projection);
	nemomatrix_scale_xyz(&ctx->projection, ctx->aspect, -1.0f, ctx->aspect * -1.0f);
	nemomatrix_scale_xyz(&ctx->projection, 0.8f, 0.8f, 0.8f);

    NemoWidget *opengl;
    opengl = nemowidget_create_opengl(win, ctx->width, ctx->height);
    nemowidget_set_data(opengl, "context", ctx);
    nemowidget_append_callback(opengl, "resize", _mesh_resize, ctx);
    nemowidget_append_callback(opengl, "event", _mesh_event, ctx);
    nemowidget_append_callback(opengl, "frame", _mesh_frame, ctx);
    nemowidget_show(opengl, 0, 0, 0);
    nemowidget_set_content(frame, "main", opengl);

    struct showone *canvas;
    canvas = nemowidget_get_canvas(opengl);
    nemoshow_canvas_set_alpha(canvas, 0.0);
    //nemoshow_canvas_set_dispatch_redraw(canvas, _win_frame);
    ctx->mesh_canvas = canvas;

	ctx->programs[0] = nemomesh_create_shader(simple_fragment_shader, simple_vertex_shader);
	ctx->programs[1] = nemomesh_create_shader(light_fragment_shader, light_vertex_shader);
	nemomesh_prepare_shader(ctx, ctx->programs[1]);

	fbo_prepare_context(
            nemoshow_canvas_get_texture(ctx->mesh_canvas),
			ctx->width, ctx->height,
			&ctx->fbo, &ctx->dbo);

	if (basepath == NULL)
		basepath = os_get_file_path(filepath);

	ctx->mesh = mesh = nemomesh_create_object(filepath, basepath);
	nemomesh_prepare_buffer(mesh, GL_LINES, mesh->lines, mesh->nlines);

    struct nemotimer *timer;
    timer = nemotimer_create(tool);
    nemotimer_set_timeout(timer, 6000);
    nemotimer_set_callback(timer, _show_end);
    nemotimer_set_userdata(timer, ctx);

    nemoshow_dispatch_frame(ctx->show);

    nemowidget_show(win, 0, 0, 0);
    nemotool_run(tool);

	nemomesh_destroy_object(ctx->mesh);

	glDeleteFramebuffers(1, &ctx->fbo);
	glDeleteRenderbuffers(1, &ctx->dbo);

    nemowidget_destroy(win);
    TOOL_DESTROY(tool);
    free(ctx);

	return 0;
}
