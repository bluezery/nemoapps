#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <getopt.h>
#include <libgen.h>
#include <time.h>

#include <ao/ao.h>

#include <nemotool.h>
#include <nemotimer.h>
#include <nemoshow.h>

#include <nemoplay.h>

#include "color.h"
#include "log.h"
#include "nemoutil.h"
#include "config.h"
#include "image.h"
#include "widgets.h"
#include "nemoui.h"
#include ""
#include "nemohelper.h"

#include <freerdp/freerdp.h>
#include <freerdp/channels/channels.h>
#include <freerdp/gdi/gdi.h>
#include <freerdp/client/cmdline.h>
#include <freerdp/utils/passphrase.h>

#include <winpr/thread.h>
#include <winpr/synch.h>
#include <winpr/handle.h>
#include </home/root/Work/FreeRDP/winpr/libwinpr/handle/handle.h>

NemoWidget *g_win;

typedef struct _MyContext MyContext;
struct _MyContext {
    rdpContext ctx;
    NemoWidget *win;

    NemoWidget *pixman;
};

static BOOL pre_connect(freerdp *rdp)
{
    if (freerdp_channels_pre_connect(rdp->context->channels, rdp)
            != CHANNEL_RC_OK) {
        return FALSE;
    }

    MyContext *myctx = (MyContext *)rdp->context;
    if (!myctx) return FALSE;
    myctx->win = g_win;

    return TRUE;
}

static BOOL begin_paint(rdpContext *ctx)
{
    ctx->gdi->primary->hdc->hwnd->invalid->null = 1;
    return TRUE;
}

static BOOL end_paint(rdpContext *ctx)
{
    ERR("");
    rdpGdi * gdi = ctx->gdi;
    if (gdi->primary->hdc->hwnd->invalid->null)
        return TRUE;

    if (gdi->primary->hdc->hwnd->invalid->null)
        return TRUE;

    INT32 x, y;
    UINT32 w, h;

    x = gdi->primary->hdc->hwnd->invalid->x;
    y = gdi->primary->hdc->hwnd->invalid->y;
    w = gdi->primary->hdc->hwnd->invalid->w;
    h = gdi->primary->hdc->hwnd->invalid->h;

    MyContext *myctx = (MyContext *)ctx;

    char *data;
    data = nemowidget_get_buffer(myctx->pixman, NULL, NULL, NULL);
    if (!data) return FALSE;

    int i = 0;
	for (i = 0; i < h; i++)
	{
		memcpy(data + ((i+y)*(gdi->width*4)) + x*4,
		       gdi->primary_buffer + ((i+y)*(gdi->width*4)) + x*4,
		       w*4);
	}
    nemowidget_dirty(myctx->pixman);

    return TRUE;
}

static void _frame(NemoWidget *widget, const char *id, void *info, void *data)
{
    ERR("");
    freerdp *rdp = data;
    MyContext *myctx = (MyContext *)rdp->context;
    //if (context->haveDamage && !wl_end_paint(rdp->context))
    //end_paint(rdp->context);
}

static void _idle(void *data)
{
    freerdp *rdp = data;

	if (freerdp_shall_disconnect(rdp)) {
        ERR("freerdp shall disconnect!!");
        return;
    }

    MyContext *myctx = (MyContext *)rdp->context;

    HANDLE handles[64];
    DWORD count;
    count = freerdp_get_event_handles(rdp->context, &handles[1], 63);
    if (!count) {
        ERR("failed get FreeRDP fd");
        return ;
    }

    DWORD status;
    status = WaitForMultipleObjects(count+1, handles, FALSE, INFINITE);
    if (WAIT_FAILED == status)
    {
        printf("%s: WaitForMultipleObjects failed\n", __FUNCTION__);
        return;
    }
    ERR("%d", count);
    ULONG Type;
    WINPR_HANDLE *Object;
    int i = 0;

    if (!freerdp_check_event_handles(rdp->context)) {
        ERR("freerdp check event handles");
        return;
    }


    //if (!freerdp_get_fds(rdp,
            /*
    for (i = 0; i < count + 1; i++) {
        int fd = winpr_Handle_getFd(&handles[i]);
        if (fd < 0) {
            ERR("get fd failed");
            return -1;
        }
        ERR("%d", fd);
    }
    */


    struct nemotool *tool = nemowidget_get_tool(myctx->win);
    nemotool_dispatch_idle(tool, _idle, rdp);
}

static BOOL post_connect(freerdp *rdp)
{
    ERR("");
    if (!gdi_init(rdp, CLRCONV_ALPHA | CLRCONV_INVERT | CLRBUF_16BPP | CLRBUF_32BPP, NULL))
        return FALSE;

    rdpGdi *gdi = rdp->context->gdi;
    if (!gdi) return FALSE;

    MyContext *myctx = (MyContext *)rdp->context;

    myctx->pixman = nemowidget_create_pixman(myctx->win,
            gdi->width, gdi->height);
    nemowidget_append_callback(myctx->pixman, "frame", _frame, rdp);

    int h, stride;
    char *buffer = nemowidget_get_buffer(myctx->pixman, NULL, &h, &stride);
    memset(buffer, 0xff, h * stride);

    nemowidget_show(myctx->pixman, 0, 0, 0);

    rdp->update->BeginPaint = begin_paint;
    rdp->update->EndPaint = end_paint;

    return (freerdp_channels_post_connect(rdp->context->channels, rdp)
            == CHANNEL_RC_OK);
}

static void post_disconncect(freerdp *rdp)
{
    if (!rdp) return;
    if (!rdp->context) return;

    MyContext *myctx = (MyContext *)rdp->context;
    gdi_free(rdp);
    if (myctx->pixman)
        nemowidget_destroy(myctx->pixman);
}

static BOOL authenticate_raw(freerdp *rdp, BOOL gateway, char **username, char **password, char **domain)
{
	static const size_t password_size = 512;
	const char* auth[] =
	{
		"Username: ",
		"Domain:   ",
		"Password: "
	};
	const char* gw[] =
	{
		"GatewayUsername: ",
		"GatewayDomain:   ",
		"GatewayPassword: "
	};
	const char** prompt = (gateway) ? gw : auth;

	if (!username || !password || !domain)
		return FALSE;

	if (!*username)
	{
		size_t username_size = 0;
		LOG("%s", prompt[0]);
		if (GetLine(username, &username_size, stdin) < 0)
		{
			ERR("GetLine returned %s [%d]", strerror(errno), errno);
			goto fail;
		}

		if (*username)
		{
			*username = StrSep(username, "\r");
			*username = StrSep(username, "\n");
		}
	}

	if (!*domain)
	{
		size_t domain_size = 0;
		LOG("%s", prompt[1]);
		if (GetLine(domain, &domain_size, stdin) < 0)
		{
			ERR("GetLine returned %s [%d]", strerror(errno), errno);
			goto fail;
		}

		if (*domain)
		{
			*domain = StrSep(domain, "\r");
			*domain = StrSep(domain, "\n");
		}
	}

	if (!*password)
	{
		*password = calloc(password_size, sizeof(char));
		if (!*password)
			goto fail;

		if (freerdp_passphrase_read(prompt[2], *password, password_size,
			rdp->settings->CredentialsFromStdin) == NULL)
			goto fail;
	}

	return TRUE;

fail:
	free(*username);
	free(*domain);
	free(*password);

	*username = NULL;
	*domain = NULL;
	*password = NULL;

	return FALSE;
}

BOOL authenticate(freerdp *rdp, char **username, char **password, char **domain)
{
    return authenticate_raw(rdp, FALSE, username, password, domain);
}

BOOL gateway_authenticate(freerdp *rdp, char **username, char **password, char **domain)
{
    return authenticate_raw(rdp, TRUE, username, password, domain);
}

static DWORD accept_certificate(rdpSettings* settings)
{
	char answer;

	if (settings->CredentialsFromStdin)
		return 0;

	while (1)
	{
		printf("Do you trust the above certificate? (Y/T/N) ");
		answer = fgetc(stdin);

		if (feof(stdin))
		{
			printf("\nError: Could not read answer from stdin.");
			if (settings->CredentialsFromStdin)
				printf(" - Run without parameter \"--from-stdin\" to set trust.");
			printf("\n");
			return 0;
		}

		switch(answer)
		{
			case 'y':
			case 'Y':
				return 1;
			case 't':
			case 'T':
				return 2;
			case 'n':
			case 'N':
				return 0;
			default:
				break;
		}
		printf("\n");
	}

	return 0;
}

DWORD verify_certificate(freerdp* rdp, const char* common_name,
				   const char* subject, const char* issuer,
				   const char* fingerprint, BOOL host_mismatch)
{
	printf("Certificate details:\n");
	printf("\tSubject: %s\n", subject);
	printf("\tIssuer: %s\n", issuer);
	printf("\tThumbprint: %s\n", fingerprint);
	printf("The above X.509 certificate could not be verified, possibly because you do not have\n"
		"the CA certificate in your certificate store, or the certificate has expired.\n"
		"Please look at the documentation on how to create local certificate store for a private CA.\n");

	return accept_certificate(rdp->settings);
}

DWORD verify_changed_certificate(freerdp* rdp, const char* common_name,
					   const char* subject, const char* issuer,
					   const char* fingerprint,
					   const char* old_subject, const char* old_issuer,
					   const char* old_fingerprint)
{
	printf("!!! Certificate has changed !!!\n");
	printf("\n");
	printf("New Certificate details:\n");
	printf("\tSubject: %s\n", subject);
	printf("\tIssuer: %s\n", issuer);
	printf("\tThumbprint: %s\n", fingerprint);
	printf("\n");
	printf("Old Certificate details:\n");
	printf("\tSubject: %s\n", old_subject);
	printf("\tIssuer: %s\n", old_issuer);
	printf("\tThumbprint: %s\n", old_fingerprint);
	printf("\n");
	printf("The above X.509 certificate does not match the certificate used for previous connections.\n"
		"This may indicate that the certificate has been tampered with.\n"
		"Please contact the administrator of the RDP server and clarify.\n");

	return accept_certificate(rdp->settings);
}

static BOOL context_new(freerdp *rdp, rdpContext *ctx)
{
    if (!(ctx->channels = freerdp_channels_new()))
        return FALSE;
    return TRUE;
}

static void context_free(freerdp *rdp, rdpContext *ctx)
{
    if (ctx && ctx->channels) {
        freerdp_channels_close(ctx->channels, rdp);
        freerdp_channels_free(ctx->channels);
        ctx->channels = NULL;
    }
}

int main(int argc, char *argv[])
{
    Config *base;
    base = config_load(PROJECT_NAME, APPNAME, CONFXML, argc, argv);

    struct nemotool *tool = TOOL_CREATE();
    NemoWidget *win = nemowidget_create_win_base(tool, APPNAME, base);
    nemowidget_win_load_scene_base(win);
    nemowidget_show(win, 0, 0, 0);
    g_win = win;

    freerdp *rdp;
    rdp = freerdp_new();
    if (!rdp) {
        ERR("freerdp_new() failed");
        return -1;
    }

    struct nemotool *tool;

    rdp->PreConnect = pre_connect;
    rdp->PostConnect = post_connect;
    rdp->PostDisconnect = post_disconncect;
    rdp->Authenticate = authenticate;
    rdp->GatewayAuthenticate = gateway_authenticate;
    rdp->VerifyCertificate = verify_certificate;
    rdp->VerifyChangedCertificate = verify_changed_certificate;
    rdp->ContextSize = sizeof(MyContext);
    rdp->ContextNew = context_new;
    rdp->ContextFree = context_free;

    if (!freerdp_context_new(rdp)) {
        ERR("freerdp_context_new() failed");
        return -1;
    }

    nemotool_dispatch_idle(tool, _idle, rdp);

    if (freerdp_client_settings_parse_command_line
            (rdp->settings, argc, argv, FALSE) < 0) {
        ERR("freerdp_client_settings_parse_command_line() failed");
        return -1;
    }

    if (!freerdp_client_load_addins(rdp->context->channels, rdp->settings)) {
        ERR("freerdp_client_load_addins() failed");
        return -1;
    }

    if (!freerdp_connect(rdp))
    {
        ERR("freerdp_connect() failed\n");
        return -1;
    }

    nemotool_run(tool);

	freerdp_channels_disconnect(rdp->context->channels, rdp);
	freerdp_disconnect(rdp);

    freerdp_context_free(rdp);
    freerdp_free(rdp);

    nemowidget_destroy(win);
    TOOL_DESTROY(tool);
    config_unload(base);

    return 0;
}
