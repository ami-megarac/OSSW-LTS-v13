/*
 *   Copyright (c) 2004, 2005, 2006 NOVELL (All rights reserved)
 *   Copyright (c) 2014 Canonical, Ltd. (All rights reserved)
 *
 *    The mod_apparmor module is licensed under the terms of the GNU
 *    Lesser General Public License, version 2.1. Please see the file
 *    COPYING.LGPL.
 *
 * mod_apparmor - (apache 2.0.x)
 * Author: Steve Beattie <steve@nxnw.org>
 *
 * This currently only implements change_hat functionality, but could be
 * extended for other stuff we decide to do.
 */

#include "ap_config.h"
#include "httpd.h"
#include "http_config.h"
#include "http_request.h"
#include "http_log.h"
#include "http_main.h"
#include "http_protocol.h"
#include "util_filter.h"
#include "apr.h"
#include "apr_strings.h"
#include "apr_lib.h"

#include <sys/apparmor.h>
#include <unistd.h>

/* #define DEBUG */

/* should the following be configurable? */
#define DEFAULT_HAT "HANDLING_UNTRUSTED_INPUT"
#define DEFAULT_URI_HAT "DEFAULT_URI"

/* Compatibility with apache 2.2 */
#if AP_SERVER_MAJORVERSION_NUMBER == 2 && AP_SERVER_MINORVERSION_NUMBER < 3
  #define APLOG_TRACE1 APLOG_DEBUG
  server_rec *ap_server_conf = NULL;
#endif

#ifdef APLOG_USE_MODULE
  APLOG_USE_MODULE(apparmor);
#endif
module AP_MODULE_DECLARE_DATA apparmor_module;

static unsigned long magic_token = 0;
static int inside_default_hat = 0;

typedef struct {
        const char *hat_name;
        char *path;
} apparmor_dir_cfg;

typedef struct {
        const char *hat_name;
        int is_initialized;
} apparmor_srv_cfg;

/* aa_init() gets invoked in the post_config stage of apache.
 * Unfortunately, apache reads its config once when it starts up, then
 * it re-reads it when goes into its restart loop, where it starts it's
 * children. This means we cannot call change_hat here, as the modules
 * memory will be wiped out, and the magic_token will be lost, so apache
 * wouldn't be able to change_hat back out. */
static int
aa_init(apr_pool_t *p, apr_pool_t *plog, apr_pool_t *ptemp, server_rec *s)
{
    apr_file_t *file;
    apr_size_t size = sizeof(magic_token);
    int ret;

    ret = apr_file_open (&file, "/dev/urandom", APR_READ, APR_OS_DEFAULT, p);
    if (!ret) {
        apr_file_read(file, (void *) &magic_token, &size);
        apr_file_close(file);
    } else {
        ap_log_error(APLOG_MARK, APLOG_ERR, errno, ap_server_conf,
                     "Failed to open /dev/urandom");
    }
    ap_log_error(APLOG_MARK, APLOG_TRACE1, 0, ap_server_conf,
                 "Opened /dev/urandom successfully");

    return OK;
}

/* As each child starts up, we'll change_hat into a default hat, mostly
 * to protect ourselves from bugs in parsing network input, but before
 * we change_hat to the uri specific hat. */
static void
aa_child_init(apr_pool_t *p, server_rec *s)
{
    int ret;

    ap_log_error(APLOG_MARK, APLOG_TRACE1, 0, ap_server_conf,
                 "init: calling change_hat with '%s'", DEFAULT_HAT);
    ret = aa_change_hat(DEFAULT_HAT, magic_token);
    if (ret < 0) {
        ap_log_error(APLOG_MARK, APLOG_ERR, errno, ap_server_conf,
                     "Failed to change_hat to '%s'", DEFAULT_HAT);
    } else {
        inside_default_hat = 1;
    }
}

static void
debug_dump_uri(request_rec *r)
{
    apr_uri_t *uri = &r->parsed_uri;
    if (uri)
        ap_log_rerror(APLOG_MARK, APLOG_TRACE1, 0, r, "Dumping uri info "
                      "scheme='%s' host='%s' path='%s' query='%s' fragment='%s'",
                      uri->scheme, uri->hostname, uri->path, uri->query,
                      uri->fragment);
    else
        ap_log_rerror(APLOG_MARK, APLOG_TRACE1, 0, r, "Asked to dump NULL uri");

}

/*
   aa_enter_hat will attempt to change_hat in the following order:
   (1) to a hatname in a location directive
   (2) to the server name or a defined per-server default
   (3) to the server name + "-" + uri
   (4) to the uri
   (5) to DEFAULT_URI
   (6) back to the parent profile
*/
static int
aa_enter_hat(request_rec *r)
{
    int aa_ret = -1;
    apparmor_dir_cfg *dcfg = (apparmor_dir_cfg *)
                    ap_get_module_config(r->per_dir_config, &apparmor_module);
    apparmor_srv_cfg *scfg = (apparmor_srv_cfg *)
                    ap_get_module_config(r->server->module_config, &apparmor_module);
    const char *aa_hat_array[6] = { NULL, NULL, NULL, NULL, NULL, NULL };
    int i = 0;
    char *aa_label, *aa_mode, *aa_hat;
    const char *vhost_uri;

    debug_dump_uri(r);
    ap_log_rerror(APLOG_MARK, APLOG_TRACE1, 0, r, "aa_enter_hat (%s) n:0x%lx p:0x%lx main:0x%lx",
                  dcfg->path, (unsigned long) r->next, (unsigned long) r->prev,
                  (unsigned long) r->main);

    /* We only call change_hat for the main request, not subrequests */
    if (r->main)
        return OK;

    if (inside_default_hat) {
        aa_change_hat(NULL, magic_token);
        inside_default_hat = 0;
    }

    if (dcfg != NULL && dcfg->hat_name != NULL) {
        ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                      "[dcfg] adding hat '%s' to aa_change_hat vector", dcfg->hat_name);
        aa_hat_array[i++] = dcfg->hat_name;
    }

    if (scfg) {
        ap_log_rerror(APLOG_MARK, APLOG_TRACE1, 0, r, "Dumping scfg info: "
                      "scfg='0x%lx' scfg->hat_name='%s'",
                      (unsigned long) scfg, scfg->hat_name);
    } else {
        ap_log_rerror(APLOG_MARK, APLOG_TRACE1, 0, r, "scfg is null");
    }
    if (scfg != NULL) {
        if (scfg->hat_name != NULL) {
            ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                          "[scfg] adding hat '%s' to aa_change_hat vector", scfg->hat_name);
            aa_hat_array[i++] = scfg->hat_name;
        } else {
            ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                          "[scfg] adding server_name '%s' to aa_change_hat vector",
                          r->server->server_hostname);
            aa_hat_array[i++] = r->server->server_hostname;
        }

        vhost_uri = apr_pstrcat(r->pool, r->server->server_hostname, "-", r->uri, NULL);
        ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                      "[vhost+uri] adding vhost+uri '%s' to aa_change_hat vector", vhost_uri);
        aa_hat_array[i++] = vhost_uri;
    }

    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                  "[uri] adding uri '%s' to aa_change_hat vector", r->uri);
    aa_hat_array[i++] = r->uri;

    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                  "[default] adding '%s' to aa_change_hat vector", DEFAULT_URI_HAT);
    aa_hat_array[i++] = DEFAULT_URI_HAT;

    aa_ret = aa_change_hatv(aa_hat_array, magic_token);
    if (aa_ret < 0) {
        ap_log_rerror(APLOG_MARK, APLOG_WARNING, errno, r, "aa_change_hatv call failed");
    }

    /* Check to see if a defined AAHatName or AADefaultHatName would
     * apply, but wasn't the hat we landed up in; report a warning if
     * that's the case. */
    aa_ret = aa_getcon(&aa_label, &aa_mode);
    if (aa_ret < 0) {
        ap_log_rerror(APLOG_MARK, APLOG_WARNING, errno, r, "aa_getcon call failed");
    } else {
        ap_log_rerror(APLOG_MARK, APLOG_TRACE1, 0, r,
                      "AA checks: aa_getcon result is '%s', mode '%s'", aa_label, aa_mode);
        /* TODO: use libapparmor get hat_name fn here once it is implemented */
        aa_hat = strstr(aa_label, "//");
        if (aa_hat != NULL && strcmp(aa_mode, "enforce") == 0) {
            aa_hat += 2;  /* skip "//" */
            ap_log_rerror(APLOG_MARK, APLOG_TRACE1, 0, r,
                          "AA checks: apache is in hat '%s', mode '%s'", aa_hat, aa_mode);
            if (dcfg != NULL && dcfg->hat_name != NULL) {
                if (strcmp(aa_hat, dcfg->hat_name) != 0)
                    ap_log_rerror(APLOG_MARK, APLOG_WARNING, 0, r,
                        "AAHatName '%s' applies, but does not appear to be a hat in the apache apparmor policy",
                        dcfg->hat_name);
            } else if (scfg != NULL && scfg->hat_name != NULL) {
                if (strcmp(aa_hat, scfg->hat_name) != 0 &&
                    strcmp(aa_hat, r->uri) != 0)
                    ap_log_rerror(APLOG_MARK, APLOG_WARNING, 0, r,
                        "AADefaultHatName '%s' applies, but does not appear to be a hat in the apache apparmor policy",
                        scfg->hat_name);
            }
        }
        free(aa_label);
    }

    return OK;
}

static int
aa_exit_hat(request_rec *r)
{
    int aa_ret;
    apparmor_dir_cfg *dcfg = (apparmor_dir_cfg *)
                    ap_get_module_config(r->per_dir_config, &apparmor_module);
    /* apparmor_srv_cfg *scfg = (apparmor_srv_cfg *)
                    ap_get_module_config(r->server->module_config, &apparmor_module); */
    ap_log_rerror(APLOG_MARK, APLOG_TRACE1, 0, r, "exiting change_hat: dir hat %s dir path %s",
                  dcfg->hat_name, dcfg->path);

    /* can convert the following back to aa_change_hat() when the
     * aa_change_hat() bug addressed in trunk commit 2329 lands in most
     * system libapparmors */
    aa_change_hatv(NULL, magic_token);

    aa_ret = aa_change_hat(DEFAULT_HAT, magic_token);
    if (aa_ret < 0) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, errno, r,
                      "Failed to change_hat to '%s'", DEFAULT_HAT);
    } else {
        inside_default_hat = 1;
    }

    return OK;
}

static const char *
aa_cmd_ch_path(cmd_parms *cmd, void *mconfig, const char *parm1)
{
    ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, ap_server_conf, "directory config change hat %s",
                 parm1 ? parm1 : "DEFAULT");
    apparmor_dir_cfg *dcfg = mconfig;
    if (parm1 != NULL) {
        dcfg->hat_name = parm1;
    } else {
        dcfg->hat_name = "DEFAULT";
    }
    return NULL;
}

static int path_warn_once;

static const char *
immunix_cmd_ch_path(cmd_parms *cmd, void *mconfig, const char *parm1)
{
    if (path_warn_once == 0) {
        ap_log_error(APLOG_MARK, APLOG_NOTICE, 0, ap_server_conf, "ImmHatName is "
                     "deprecated, please use AAHatName instead");
        path_warn_once = 1;
    }
    return aa_cmd_ch_path(cmd, mconfig, parm1);
}

static const char *
aa_cmd_ch_srv(cmd_parms *cmd, void *mconfig, const char *parm1)
{
    ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, ap_server_conf, "server config change hat %s",
                            parm1 ? parm1 : "DEFAULT");
    apparmor_srv_cfg *scfg = (apparmor_srv_cfg *)
            ap_get_module_config(cmd->server->module_config, &apparmor_module);
    if (parm1 != NULL) {
        scfg->hat_name = parm1;
    } else {
        scfg->hat_name = "DEFAULT";
    }
    return NULL;
}

static int srv_warn_once;

static const char *
immunix_cmd_ch_srv(cmd_parms *cmd, void *mconfig, const char *parm1)
{
    if (srv_warn_once == 0) {
        ap_log_error(APLOG_MARK, APLOG_NOTICE, 0, ap_server_conf, "ImmDefaultHatName is "
                     "deprecated, please use AADefaultHatName instead");
        srv_warn_once = 1;
    }
    return aa_cmd_ch_srv(cmd, mconfig, parm1);
}

static void *
aa_create_dir_config(apr_pool_t *p, char *path)
{
    apparmor_dir_cfg *newcfg = (apparmor_dir_cfg *) apr_pcalloc(p, sizeof(*newcfg));

    ap_log_error(APLOG_MARK, APLOG_TRACE1, 0, ap_server_conf,
                 "aa_create_dir_cfg (%s)", path ? path : ":no path:");
    if (newcfg == NULL) {
        ap_log_error(APLOG_MARK, APLOG_ERR, 0, ap_server_conf,
                     "aa_create_dir_config: couldn't alloc dir config");
        return NULL;
    }
    newcfg->path = apr_pstrdup(p, path ? path : ":no path:");

    return newcfg;
}

/* XXX: Should figure out an appropriate action to take here, if any

static void *
aa_merge_dir_config(apr_pool_t *p, void *parent, void *child)
{
    apparmor_dir_cfg *newcfg = (apparmor_dir_cfg *) apr_pcalloc(p, sizeof(*newcfg));

    ap_log_error(APLOG_MARK, APLOG_TRACE1, 0, ap_server_conf, "in immunix_merge_dir ()");
    if (newcfg == NULL)
            return NULL;

    return newcfg;
}
*/

static void *
aa_create_srv_config(apr_pool_t *p, server_rec *srv)
{
    apparmor_srv_cfg *newcfg = (apparmor_srv_cfg *) apr_pcalloc(p, sizeof(*newcfg));

    ap_log_error(APLOG_MARK, APLOG_TRACE1, 0, ap_server_conf,
                 "in aa_create_srv_config");
    if (newcfg == NULL) {
        ap_log_error(APLOG_MARK, APLOG_ERR, 0, ap_server_conf,
                     "aa_create_srv_config: couldn't alloc srv config");
        return NULL;
    }

    return newcfg;
}


static const command_rec mod_apparmor_cmds[] = {

    AP_INIT_TAKE1(
        "ImmHatName",
        immunix_cmd_ch_path,
        NULL,
        ACCESS_CONF,
        ""
    ),
    AP_INIT_TAKE1(
        "ImmDefaultHatName",
        immunix_cmd_ch_srv,
        NULL,
        RSRC_CONF,
        ""
    ),
    AP_INIT_TAKE1(
        "AAHatName",
        aa_cmd_ch_path,
        NULL,
        ACCESS_CONF,
        ""
    ),
    AP_INIT_TAKE1(
        "AADefaultHatName",
        aa_cmd_ch_srv,
        NULL,
        RSRC_CONF,
        ""
    ),
    { NULL }
};

static void
register_hooks(apr_pool_t *p)
{
    ap_hook_post_config(aa_init, NULL, NULL, APR_HOOK_MIDDLE);
    ap_hook_child_init(aa_child_init, NULL, NULL, APR_HOOK_MIDDLE);
    ap_hook_access_checker(aa_enter_hat, NULL, NULL, APR_HOOK_FIRST);
    /* ap_hook_post_read_request(aa_enter_hat, NULL, NULL, APR_HOOK_FIRST); */
    ap_hook_log_transaction(aa_exit_hat, NULL, NULL, APR_HOOK_LAST);
}

module AP_MODULE_DECLARE_DATA apparmor_module = {
    STANDARD20_MODULE_STUFF,
    aa_create_dir_config,        /* dir config creater */
    NULL,                        /* dir merger --- default is to override */
    /* immunix_merge_dir_config, */        /* dir merger --- default is to override */
    aa_create_srv_config,        /* server config */
    NULL,                        /* merge server config */
    mod_apparmor_cmds,           /* command table */
    register_hooks               /* register hooks */
};
