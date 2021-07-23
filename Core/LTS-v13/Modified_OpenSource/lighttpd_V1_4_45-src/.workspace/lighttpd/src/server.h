#ifndef _SERVER_H_
#define _SERVER_H_
#include "first.h"

#include "base.h"
#include "ncml.h"
#include "featuredef.h"

extern SERVICE_CONF_STRUCT g_serviceconf;
extern CoreFeatures_T      g_corefeatures;

int config_read(server *srv, const char *fn);
int config_set_defaults(server *srv);
int  ReInitialiseSSL(server *srv);
#endif
