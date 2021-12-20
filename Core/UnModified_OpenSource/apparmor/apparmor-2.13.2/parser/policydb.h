/*
 * Copyright 2012 Canonical Ltd.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __AA_POLICYDB_H
#define __AA_POLICYDB_H

/*
 * Class of private mediation types in the AppArmor policy db
 *
 * See libapparmor's apparmor.h for public mediation types
 */
#define AA_CLASS_COND		0
#define AA_CLASS_UNKNOWN	1
#define AA_CLASS_FILE		2
#define AA_CLASS_CAP		3
#define AA_CLASS_NET		4
#define AA_CLASS_RLIMITS	5
#define AA_CLASS_DOMAIN		6
#define AA_CLASS_MOUNT		7
#define AA_CLASS_NS_DOMAIN	8
#define AA_CLASS_PTRACE		9
#define AA_CLASS_SIGNAL		10

#define AA_CLASS_LABEL		16

/* defined in libapparmor's apparmor.h #define AA_CLASS_DBUS 32 */
#define AA_CLASS_X		33

#endif /* __AA_POLICYDB_H */
