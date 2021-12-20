/*
 *   Copyright (c) 2010
 *   Canonical, Ltd. (All rights reserved)
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of version 2 of the GNU General Public
 *   License published by the Free Software Foundation.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, contact Novell, Inc. or Canonical
 *   Ltd.
 */

/**
 * The mount command, its mix of options and flags, its permissions and
 * mapping are a mess.
 *      mount [-lhV]
 *
 *      mount -a [-fFnrsvw] [-t vfstype] [-O optlist]
 *
 *      mount [-fnrsvw] [-o option[,option]...]  device|dir
 *
 *      mount [-fnrsvw] [-t vfstype] [-o options] device dir
 *
 *----------------------------------------------------------------------
 * Mount flags of no interest for apparmor mediation
 * -a, --all
 * -F fork for simultaneous mount
 * -f fake, do everything except that actual system call
 * -h --help
 * -i, --internal-only
 * -n mount without writing in /etc/mtab
 * -O <optlist> limits what is auto mounted
 * -p, --pass-fd num
 * -s     Tolerate sloppy mount options
 * -U uuid
 * -V --version
 * --no-canonicalize
 *
 *----------------------------------------------------------------------
 * what do we do with these
 * -l	list?
 * -L <label> label
 * -v --verbose		deprecated
 *
 *----------------------------------------------------------------------
 * Filesystem type
 * -t <vfstype>
 *    vfstype=<vfstype>
 *
 *----------------------------------------------------------------------
 * Mount Flags/options (-o --options)
 *  -o option[,option]
 * 
 * The Linux kernel has 32 fs - independent mount flags, that mount command
 * is responsible for stripping out and mapping to a 32 bit flags field.
 * The mount commands mapping is documented below.
 *
 * Unfortunately we can not directly use this mapping as we need to be able
 * represent, whether none, 1 or both options of a flag can be present for
 * example
 *    ro, and rw information is stored in a single bit.  But we need 2 bits
 *    of information.
 *    ro - the mount can only be readonly
 *    rw - the mount can only be rw
 *    ro/rw - the mount can be either ro/rw
 *    the fourth state of neither ro/rw does not exist, but still we need
 *    >1 bit to represent the possible choices
 *
 * The fs specific mount options are passed into the kernel as a string
 * to be interpreted by the filesystem.
 *
 *
 * #define MS_RDONLY	 1		Mount read-only
 *	ro -r --read-only	[source] dest
 *	rw -w
 * #define MS_NOSUID	 2		Ignore suid and sgid bits
 *	nosuid
 *	suid
 * #define MS_NODEV	 4		Disallow access to device special files
 *	nodev
 *	dev
 * #define MS_NOEXEC	 8		Disallow program execution
 *	noexec
 *	exec
 * #define MS_SYNCHRONOUS	16	Writes are synced at once
 *	sync
 *	async
 * #define MS_REMOUNT	32		Alter flags of a mounted FS
 *	remount			source dest
 * #define MS_MANDLOCK	64		Allow mandatory locks on an FS
 *	mand
 *	nomand
 * #define MS_DIRSYNC	128		Directory modifications are synchronous
 *	dirsync
 * #define MS_NOATIME	1024		Do not update access times
 *	noatime
 *	atime
 * #define MS_NODIRATIME	2048	Do not update directory access times
 *	nodiratime
 *	diratime
 * #define MS_BIND		4096
 *	--bind -B		source dest
 * #define MS_MOVE		8192
 *	--move -M		source dest
 * #define MS_REC		16384
 *	--rbind -R		source dest
 *	--make-rshared		dest
 *	--make-rslave		dest
 *	--make-rprivate		dest
 *	--make-runbindable	dest
 * #define MS_VERBOSE	32768		MS_VERBOSE is deprecated
 * #define MS_SILENT	32768
 *	silent
 *	load
 * #define MS_POSIXACL	(1<<16)		VFS does not apply the umask
 *	acl
 *	noacl
 * #define MS_UNBINDABLE	(1<<17)	change to unbindable
 *	--make-unbindable	dest
 * #define MS_PRIVATE	(1<<18)		change to private
 *	--make-private		dest
 * #define MS_SLAVE	(1<<19)		change to slave
 *	--make-slave		dest
 * #define MS_SHARED	(1<<20)		change to shared
 *	--make-shared		dest
 * #define MS_RELATIME	(1<<21)		Update atime relative to mtime/ctime
 *	relatime
 *	norelatime
 * #define MS_KERNMOUNT	(1<<22)		this is a kern_mount call
 * #define MS_I_VERSION	(1<<23)		Update inode I_version field
 *	iversion
 *	noiversion
 * #define MS_STRICTATIME	(1<<24)	Always perform atime updates
 *	strictatime
 *	nostrictatime
 * #define MS_NOSEC	(1<<28)
 * #define MS_BORN		(1<<29)
 * #define MS_ACTIVE	(1<<30)
 * #define MS_NOUSER	(1<<31)
 *	nouser
 *	user
 *
 * other mount options of interest
 *
 *   selinux
 *     context=<context>
 *     fscontext=<context>
 *     defcontext=<context>,
 *     rootcontext=<context>
 *
 *   defaults -> rw, suid,  dev,  exec,  auto,  nouser,  async
 *   owner -> implies nosuid  and  nodev
 *   users -> implies noexec, nosuid, and nodev
 *
 *----------------------------------------------------------------------
 * AppArmor mount rules
 *
 * AppArmor mount rules try to leverage mount syntax within apparmor syntax
 * this can not be done entirely but it is largely covered.
 *
 * The general mount syntax is
 * [audit] [deny] [owner] mount [conds]* [source] [ -> [conds] path],
 * [audit] [deny] remount [conds]* [path],
 * [audit] [deny] umount [conds]* [path],
 *
 * Note: leading owner option applies owner condition to both sours and dest
 *       path.
 *
 * where [conds] can be
 * fstype=<expr>
 * options=<expr>
 * owner[=<expr>]
 *
 * <expr> := <re> | '(' (<re>[,])+ ')'
 *
 * If a condition is not specified then it is assumed to match all possible
 * entries for it.  ie. a missing fstype means all fstypes are matched.
 * However if a condition is specified then the rule only grants permission
 * for mounts matching the specified pattern.
 *
 * Examples.
 * mount,		# allow any mount
 * mount /dev/foo,	# allow mounting of /dev/foo anywhere
 * mount options=ro /dev/foo,  #allow mounting /dev/foo as read only
 * mount options=(ro,foo) /dev/foo,
 * mount options=ro options=foo /dev/foo,
 * mount fstype=overlayfs options=(rw,upperdir=/tmp/upper/,lowerdir=/) overlay -> /mnt/
 *
 *----------------------------------------------------------------------
 * pivotroot
 *   pivotroot [oldroot=<value>] <path> -> <profile>
 *   pivotroot <path> -> {  }
 *
 *----------------------------------------------------------------------
 * chroot
 *   chroot <path> -> <profile>
 *   chroot <path> -> {  }
 *
 *----------------------------------------------------------------------
 * AppArmor mount rule encoding
 *
 * TODO:
 *   add semantic checking of options against specified filesytem types
 *   to catch mount options that can't be covered.
 *
 *
 */


#include <stdlib.h>
#include <string.h>
#include <linux/limits.h>
#include <iostream>

#include "parser.h"
#include "policydb.h"
#include "profile.h"
#include "mount.h"

struct mnt_keyword_table {
	const char *keyword;
	unsigned int set;
	unsigned int clear;
};

static struct mnt_keyword_table mnt_opts_table[] = {
	{"ro",			MS_RDONLY, 0},
	{"r",			MS_RDONLY, 0},
	{"read-only",		MS_RDONLY, 0},
	{"rw",			0, MS_RDONLY},
	{"w",			0, MS_RDONLY},
	{"suid",		0, MS_NOSUID},
	{"nosuid",		MS_NOSUID, 0},
	{"dev",			0, MS_NODEV},
	{"nodev",		MS_NODEV, 0},
	{"exec",		0, MS_NOEXEC},
	{"noexec",		MS_NOEXEC, 0},
	{"sync",		MS_SYNC, 0},
	{"async",		0, MS_SYNC},
	{"remount",		MS_REMOUNT, 0},
	{"mand",		MS_MAND, 0},
	{"nomand",		0, MS_MAND},
	{"dirsync",		MS_DIRSYNC, 0},
	{"atime",		0, MS_NOATIME},
	{"noatime",		MS_NOATIME, 0},
	{"diratime",		0, MS_NODIRATIME},
	{"nodiratime",		MS_NODIRATIME, 0},
	{"bind",		MS_BIND, 0},
	{"B",			MS_BIND, 0},
	{"move",		MS_MOVE, 0},
	{"M",			MS_MOVE, 0},
	{"rbind",		MS_RBIND, 0},
	{"R",			MS_RBIND, 0},
	{"verbose",		MS_VERBOSE, 0},
	{"silent",		MS_SILENT, 0},
	{"loud",		0, MS_SILENT},
	{"acl",			MS_ACL, 0},
	{"noacl",		0, MS_ACL},
	{"unbindable",		MS_UNBINDABLE, 0},
	{"make-unbindable",	MS_UNBINDABLE, 0},
	{"runbindable",		MS_RUNBINDABLE, 0},
	{"make-runbindable",	MS_RUNBINDABLE, 0},
	{"private",		MS_PRIVATE, 0},
	{"make-private",	MS_PRIVATE, 0},
	{"rprivate",		MS_RPRIVATE, 0},
	{"make-rprivate",	MS_RPRIVATE, 0},
	{"slave",		MS_SLAVE, 0},
	{"make-slave",		MS_SLAVE, 0},
	{"rslave",		MS_RSLAVE, 0},
	{"make-rslave",		MS_RSLAVE, 0},
	{"shared",		MS_SHARED, 0},
	{"make-shared",		MS_SHARED, 0},
	{"rshared",		MS_RSHARED, 0},
	{"make-rshared",	MS_RSHARED, 0},

	{"relatime",		MS_RELATIME, 0},
	{"norelatime",		0, MS_NORELATIME},
	{"iversion",		MS_IVERSION, 0},
	{"noiversion",		0, MS_IVERSION},
	{"strictatime",		MS_STRICTATIME, 0},
	{"user",		0, (unsigned int) MS_NOUSER},
	{"nouser",		(unsigned int) MS_NOUSER, 0},

	{NULL, 0, 0}
};

static struct mnt_keyword_table mnt_conds_table[] = {
	{"options", MNT_SRC_OPT, MNT_COND_OPTIONS},
	{"option", MNT_SRC_OPT, MNT_COND_OPTIONS},
	{"fstype", MNT_SRC_OPT | MNT_DST_OPT, MNT_COND_FSTYPE},
	{"vfstype", MNT_SRC_OPT | MNT_DST_OPT, MNT_COND_FSTYPE},

	{NULL, 0, 0}
};

static int find_mnt_keyword(struct mnt_keyword_table *table, const char *name)
{
	int i;
	for (i = 0; table[i].keyword; i++) {
		if (strcmp(name, table[i].keyword) == 0)
			return i;
	}

	return -1;
}

int is_valid_mnt_cond(const char *name, int src)
{
	int i;
	i = find_mnt_keyword(mnt_conds_table, name);
	if (i != -1)
		return (mnt_conds_table[i].set & src);
	return -1;
}

static unsigned int extract_flags(struct value_list **list, unsigned int *inv)
{
	unsigned int flags = 0;
	*inv = 0;

	struct value_list *entry, *tmp, *prev = NULL;
	list_for_each_safe(*list, entry, tmp) {
		int i;
		i = find_mnt_keyword(mnt_opts_table, entry->value);
		if (i != -1) {
			flags |= mnt_opts_table[i].set;
			*inv |= mnt_opts_table[i].clear;
			PDEBUG(" extracting mount flag %s req: 0x%x inv: 0x%x"
			       " => req: 0x%x inv: 0x%x\n",
			       entry->value, mnt_opts_table[i].set,
			       mnt_opts_table[i].clear, flags, *inv);
			if (prev)
				prev->next = tmp;
			if (entry == *list)
				*list = tmp;
			entry->next = NULL;
			free_value_list(entry);
		} else
			prev = entry;
	}

	return flags;
}

static struct value_list *extract_fstype(struct cond_entry **conds)
{
	struct value_list *list = NULL;

	struct cond_entry *entry, *tmp, *prev = NULL;

	list_for_each_safe(*conds, entry, tmp) {
		if (strcmp(entry->name, "fstype") == 0 ||
		    strcmp(entry->name, "vfstype") == 0) {
			PDEBUG("  extracting fstype\n");
			list_remove_at(*conds, prev, entry);
			list_append(entry->vals, list);
			list = entry->vals;
			entry->vals = NULL;
			free_cond_entry(entry);
		} else
			prev = entry;
	}

	return list;
}

static struct value_list *extract_options(struct cond_entry **conds, int eq)
{
	struct value_list *list = NULL;

	struct cond_entry *entry, *tmp, *prev = NULL;

	list_for_each_safe(*conds, entry, tmp) {
		if ((strcmp(entry->name, "options") == 0 ||
		     strcmp(entry->name, "option") == 0) &&
		    entry->eq == eq) {
			list_remove_at(*conds, prev, entry);
			PDEBUG("  extracting option %s\n", entry->name);
			list_append(entry->vals, list);
			list = entry->vals;
			entry->vals = NULL;
			free_cond_entry(entry);
		} else
			prev = entry;
	}

	return list;
}

mnt_rule::mnt_rule(struct cond_entry *src_conds, char *device_p,
		   struct cond_entry *dst_conds unused, char *mnt_point_p,
		   int allow_p):
	mnt_point(mnt_point_p), device(device_p), trans(NULL), opts(NULL),
	flags(0), inv_flags(0), audit(0), deny(0)
{
	/* FIXME: dst_conds are ignored atm */
	dev_type = extract_fstype(&src_conds);

	if (src_conds) {
		struct value_list *list = extract_options(&src_conds, 0);

		opts = extract_options(&src_conds, 1);
		if (opts)
			flags = extract_flags(&opts, &inv_flags);

		if (list) {
			unsigned int tmpflags, tmpinv_flags = 0;

			tmpflags = extract_flags(&list, &tmpinv_flags);
			/* these flags are optional so set both */
			tmpflags |= tmpinv_flags;
			tmpinv_flags |= tmpflags;

			flags |= tmpflags;
			inv_flags |= tmpinv_flags;

			if (opts)
				list_append(opts, list);
			else if (list)
				opts = list;
		}
	}

	if (allow_p & AA_DUMMY_REMOUNT) {
		allow_p = AA_MAY_MOUNT;
		flags |= MS_REMOUNT;
		inv_flags = 0;
	} else if (!(flags | inv_flags)) {
		/* no flag options, and not remount, allow everything */
		flags = MS_ALL_FLAGS;
		inv_flags = MS_ALL_FLAGS;
	}

	allow = allow_p;

	if (src_conds) {
		PERROR("  unsupported mount conditions\n");
		exit(1);
	}
	if (opts) {
		PERROR("  unsupported mount options\n");
		exit(1);
	}
}

ostream &mnt_rule::dump(ostream &os)
{
	if (allow & AA_MAY_MOUNT)
		os << "mount";
	else if (allow & AA_MAY_UMOUNT)
		os << "umount";
	else if (allow & AA_MAY_PIVOTROOT)
		os << "pivotroot";
	else
		os << "error: unknonwn mount perm";

	os << " (0x" << hex << flags << " - 0x" << inv_flags << ") ";
	if (dev_type) {
		os << " type=";
		print_value_list(dev_type);
	}
	if (opts) {
		os << " options=";
		print_value_list(opts);
	}
	if (device)
		os << " " << device;
	if (mnt_point)
		os << " -> " << mnt_point;
	if (trans)
		os << " -> " << trans;

	const char *prefix = deny ? "deny" : "";
	os << " " << prefix << "(0x" << hex << allow << "/0x" << audit << ")";
	os << ",\n";

	return os;
}

/* does not currently support expansion of vars in options */
int mnt_rule::expand_variables(void)
{
	int error = 0;

	error = expand_entry_variables(&mnt_point);
	if (error)
		return error;
	error = expand_entry_variables(&device);
	if (error)
		return error;
	error = expand_entry_variables(&trans);
	if (error)
		return error;

	return 0;
}

static int build_mnt_flags(char *buffer, int size, unsigned int flags,
			   unsigned int inv_flags)
{
	char *p = buffer;
	int i, len = 0;

	if (flags == MS_ALL_FLAGS) {
		/* all flags are optional */
		len = snprintf(p, size, "%s", default_match_pattern);
		if (len < 0 || len >= size)
			return FALSE;
		return TRUE;
	}
	for (i = 0; i <= 31; ++i) {
		if ((flags & inv_flags) & (1 << i))
			len = snprintf(p, size, "(\\x%02x|)", i + 1);
		else if (flags & (1 << i))
			len = snprintf(p, size, "\\x%02x", i + 1);
		else	/* no entry = not set */
			continue;

		if (len < 0 || len >= size)
			return FALSE;
		p += len;
		size -= len;
	}

	/* this needs to go once the backend is updated. */
	if (buffer == p) {
		/* match nothing - use impossible 254 as regex parser doesn't
		 * like the empty string
		 */
		if (size < 9)
			return FALSE;

		strcpy(p, "(\\xfe|)");
	}

	return TRUE;
}

static int build_mnt_opts(std::string& buffer, struct value_list *opts)
{
	struct value_list *ent;
	pattern_t ptype;
	int pos;

	if (!opts) {
		buffer.append(default_match_pattern);
		return TRUE;
	}

	list_for_each(opts, ent) {
		ptype = convert_aaregex_to_pcre(ent->value, 0, glob_default, buffer, &pos);
		if (ptype == ePatternInvalid)
			return FALSE;

		if (ent->next)
			buffer.append(",");
	}

	return TRUE;
}

/* do we want to warn once/profile or just once per compile?? */
static void warn_once(const char *name)
{
	static const char *warned_name = NULL;

	if ((warnflags & WARN_RULE_NOT_ENFORCED) && warned_name != name) {
		cerr << "Warning from profile " << name << " (";
		if (current_filename)
			cerr << current_filename;
		else
			cerr << "stdin";
		cerr << ") mount rules not enforced\n";
		warned_name = name;
	}
}

int mnt_rule::gen_policy_re(Profile &prof)
{
	std::string mntbuf;
	std::string devbuf;
	std::string typebuf;
	char flagsbuf[PATH_MAX + 3];
	std::string optsbuf;
	char class_mount_hdr[64];
	const char *vec[5];
	int count = 0;
	unsigned int tmpflags, tmpinv_flags;

	if (!kernel_supports_mount) {
		warn_once(prof.name);
		return RULE_NOT_SUPPORTED;
	}

	sprintf(class_mount_hdr, "\\x%02x", AA_CLASS_MOUNT);

	/* a single mount rule may result in multiple matching rules being
	 * created in the backend to cover all the possible choices
	 */

	if ((allow & AA_MAY_MOUNT) && (flags & MS_REMOUNT)
	    && !device && !dev_type) {
		int tmpallow;
		/* remount can't be conditional on device and type */
		/* rule class single byte header */
		mntbuf.assign(class_mount_hdr);
		if (mnt_point) {
			/* both device && mnt_point or just mnt_point */
			if (!convert_entry(mntbuf, mnt_point))
				goto fail;
			vec[0] = mntbuf.c_str();
		} else {
			if (!convert_entry(mntbuf, device))
				goto fail;
			vec[0] = mntbuf.c_str();
		}
		/* skip device */
		vec[1] = default_match_pattern;
		/* skip type */
		vec[2] = default_match_pattern;

		tmpflags = flags;
		tmpinv_flags = inv_flags;
		if (tmpflags != MS_ALL_FLAGS)
			tmpflags &= MS_REMOUNT_FLAGS;
		if (tmpinv_flags != MS_ALL_FLAGS)
			tmpflags &= MS_REMOUNT_FLAGS;
		if (!build_mnt_flags(flagsbuf, PATH_MAX, tmpflags, tmpinv_flags))
			goto fail;
		vec[3] = flagsbuf;

		if (opts)
			tmpallow = AA_MATCH_CONT;
		else
			tmpallow = allow;

		/* rule for match without required data || data MATCH_CONT */
		if (!prof.policy.rules->add_rule_vec(deny, tmpallow,
					      audit | AA_AUDIT_MNT_DATA, 4,
					      vec, dfaflags))
			goto fail;
		count++;

		if (opts) {
			/* rule with data match required */
			optsbuf.clear();
			if (!build_mnt_opts(optsbuf, opts))
				goto fail;
			vec[4] = optsbuf.c_str();
			if (!prof.policy.rules->add_rule_vec(deny, allow,
						      audit | AA_AUDIT_MNT_DATA,
						      5, vec, dfaflags))
				goto fail;
			count++;
		}
	}
	if ((allow & AA_MAY_MOUNT) && (flags & MS_BIND)
	    && !dev_type && !opts) {
		/* bind mount rules can't be conditional on dev_type or data */
		/* rule class single byte header */
		mntbuf.assign(class_mount_hdr);
		if (!convert_entry(mntbuf, mnt_point))
			goto fail;
		vec[0] = mntbuf.c_str();
		if (!clear_and_convert_entry(devbuf, device))
			goto fail;
		vec[1] = devbuf.c_str();
		/* skip type */
		vec[2] = default_match_pattern;

		tmpflags = flags;
		tmpinv_flags = inv_flags;
		if (tmpflags != MS_ALL_FLAGS)
			tmpflags &= MS_BIND_FLAGS;
		if (tmpinv_flags != MS_ALL_FLAGS)
			tmpflags &= MS_BIND_FLAGS;
		if (!build_mnt_flags(flagsbuf, PATH_MAX, tmpflags, tmpinv_flags))
			goto fail;
		vec[3] = flagsbuf;
		if (!prof.policy.rules->add_rule_vec(deny, allow, audit, 4, vec,
						    dfaflags))
			goto fail;
		count++;
	}
	if ((allow & AA_MAY_MOUNT) &&
	    (flags & (MS_UNBINDABLE | MS_PRIVATE | MS_SLAVE | MS_SHARED))
	    && !device && !dev_type && !opts) {
		/* change type base rules can not be conditional on device,
		 * device type or data
		 */
		/* rule class single byte header */
		mntbuf.assign(class_mount_hdr);
		if (!convert_entry(mntbuf, mnt_point))
			goto fail;
		vec[0] = mntbuf.c_str();
		/* skip device and type */
		vec[1] = default_match_pattern;
		vec[2] = default_match_pattern;

		tmpflags = flags;
		tmpinv_flags = inv_flags;
		if (tmpflags != MS_ALL_FLAGS)
			tmpflags &= MS_MAKE_FLAGS;
		if (tmpinv_flags != MS_ALL_FLAGS)
			tmpflags &= MS_MAKE_FLAGS;
		if (!build_mnt_flags(flagsbuf, PATH_MAX, tmpflags, tmpinv_flags))
			goto fail;
		vec[3] = flagsbuf;
		if (!prof.policy.rules->add_rule_vec(deny, allow, audit, 4, vec,
						    dfaflags))
			goto fail;
		count++;
	}
	if ((allow & AA_MAY_MOUNT) && (flags & MS_MOVE)
	    && !dev_type && !opts) {
		/* mount move rules can not be conditional on dev_type,
		 * or data
		 */
		/* rule class single byte header */
		mntbuf.assign(class_mount_hdr);
		if (!convert_entry(mntbuf, mnt_point))
			goto fail;
		vec[0] = mntbuf.c_str();
		if (!clear_and_convert_entry(devbuf, device))
			goto fail;
		vec[1] = devbuf.c_str();
		/* skip type */
		vec[2] = default_match_pattern;

		tmpflags = flags;
		tmpinv_flags = inv_flags;
		if (tmpflags != MS_ALL_FLAGS)
			tmpflags &= MS_MOVE_FLAGS;
		if (tmpinv_flags != MS_ALL_FLAGS)
			tmpflags &= MS_MOVE_FLAGS;
		if (!build_mnt_flags(flagsbuf, PATH_MAX, tmpflags, tmpinv_flags))
			goto fail;
		vec[3] = flagsbuf;
		if (!prof.policy.rules->add_rule_vec(deny, allow, audit, 4, vec,
						    dfaflags))
			goto fail;
		count++;
	}
	if ((allow & AA_MAY_MOUNT) &&
	    (flags | inv_flags) & ~MS_CMDS) {
		int tmpallow;
		/* generic mount if flags are set that are not covered by
		 * above commands
		 */
		/* rule class single byte header */
		mntbuf.assign(class_mount_hdr);
		if (!convert_entry(mntbuf, mnt_point))
			goto fail;
		vec[0] = mntbuf.c_str();
		if (!clear_and_convert_entry(devbuf, device))
			goto fail;
		vec[1] = devbuf.c_str();
		typebuf.clear();
		if (!build_list_val_expr(typebuf, dev_type))
			goto fail;
		vec[2] = typebuf.c_str();

		tmpflags = flags;
		tmpinv_flags = inv_flags;
		if (tmpflags != MS_ALL_FLAGS)
			tmpflags &= ~MS_CMDS;
		if (tmpinv_flags != MS_ALL_FLAGS)
			tmpinv_flags &= ~MS_CMDS;
		if (!build_mnt_flags(flagsbuf, PATH_MAX, tmpflags, tmpinv_flags))
			goto fail;
		vec[3] = flagsbuf;

		if (opts)
			tmpallow = AA_MATCH_CONT;
		else
			tmpallow = allow;

		/* rule for match without required data || data MATCH_CONT */
		if (!prof.policy.rules->add_rule_vec(deny, tmpallow,
					      audit | AA_AUDIT_MNT_DATA, 4,
					      vec, dfaflags))
			goto fail;
		count++;

		if (opts) {
			/* rule with data match required */
			optsbuf.clear();
			if (!build_mnt_opts(optsbuf, opts))
				goto fail;
			vec[4] = optsbuf.c_str();
			if (!prof.policy.rules->add_rule_vec(deny, allow,
						      audit | AA_AUDIT_MNT_DATA,
						      5, vec, dfaflags))
				goto fail;
			count++;
		}
	}
	if (allow & AA_MAY_UMOUNT) {
		/* rule class single byte header */
		mntbuf.assign(class_mount_hdr);
		if (!convert_entry(mntbuf, mnt_point))
			goto fail;
		vec[0] = mntbuf.c_str();
		if (!prof.policy.rules->add_rule_vec(deny, allow, audit, 1, vec,
						    dfaflags))
			goto fail;
		count++;
	}
	if (allow & AA_MAY_PIVOTROOT) {
		/* rule class single byte header */
		mntbuf.assign(class_mount_hdr);
		if (!convert_entry(mntbuf, mnt_point))
			goto fail;
		vec[0] = mntbuf.c_str();
		if (!clear_and_convert_entry(devbuf, device))
			goto fail;
		vec[1] = devbuf.c_str();
		if (!prof.policy.rules->add_rule_vec(deny, allow, audit, 2, vec,
						    dfaflags))
			goto fail;
		count++;
	}

	if (!count)
		/* didn't actually encode anything */
		goto fail;

	return RULE_OK;

fail:
	PERROR("Encoding of mount rule failed\n");
	return RULE_ERROR;
}

void mnt_rule::post_process(Profile &prof)
{
	if (trans) {
		unsigned int mode = 0;
		int n = add_entry_to_x_table(&prof, trans);
		if (!n) {
			PERROR("Profile %s has too many specified profile transitions.\n", prof.name);
			exit(1);
		}

		if (allow & AA_USER_EXEC)
			mode |= SHIFT_MODE(n << 10, AA_USER_SHIFT);
		if (allow & AA_OTHER_EXEC)
			mode |= SHIFT_MODE(n << 10, AA_OTHER_SHIFT);
		allow = ((allow & ~AA_ALL_EXEC_MODIFIERS) |
				(mode & AA_ALL_EXEC_MODIFIERS));

		trans = NULL;
	}
}


