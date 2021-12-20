/* SPDX-License-Identifier: LGPL-2.1+ */
/***
  Copyright © 2014 Intel Corporation. All rights reserved.
***/

#include <net/ethernet.h>
#include <net/if.h>

#include "alloc-util.h"
#include "conf-parser.h"
#include "netdev/bridge.h"
#include "netlink-util.h"
#include "networkd-fdb.h"
#include "networkd-manager.h"
#include "util.h"
#include "vlan-util.h"

#define STATIC_FDB_ENTRIES_PER_NETWORK_MAX 1024U

/* create a new FDB entry or get an existing one. */
int fdb_entry_new_static(
                Network *network,
                const char *filename,
                unsigned section_line,
                FdbEntry **ret) {

        _cleanup_(network_config_section_freep) NetworkConfigSection *n = NULL;
        _cleanup_(fdb_entry_freep) FdbEntry *fdb_entry = NULL;
        _cleanup_free_ struct ether_addr *mac_addr = NULL;
        int r;

        assert(network);
        assert(ret);
        assert(!!filename == (section_line > 0));

        /* search entry in hashmap first. */
        if (filename) {
                r = network_config_section_new(filename, section_line, &n);
                if (r < 0)
                        return r;

                fdb_entry = hashmap_get(network->fdb_entries_by_section, n);
                if (fdb_entry) {
                        *ret = TAKE_PTR(fdb_entry);

                        return 0;
                }
        }

        if (network->n_static_fdb_entries >= STATIC_FDB_ENTRIES_PER_NETWORK_MAX)
                return -E2BIG;

        /* allocate space for MAC address. */
        mac_addr = new0(struct ether_addr, 1);
        if (!mac_addr)
                return -ENOMEM;

        /* allocate space for and FDB entry. */
        fdb_entry = new(FdbEntry, 1);
        if (!fdb_entry)
                return -ENOMEM;

        /* init FDB structure. */
        *fdb_entry = (FdbEntry) {
                .network = network,
                .mac_addr = TAKE_PTR(mac_addr),
        };

        LIST_PREPEND(static_fdb_entries, network->static_fdb_entries, fdb_entry);
        network->n_static_fdb_entries++;

        if (filename) {
                fdb_entry->section = TAKE_PTR(n);

                r = hashmap_ensure_allocated(&network->fdb_entries_by_section, &network_config_hash_ops);
                if (r < 0)
                        return r;

                r = hashmap_put(network->fdb_entries_by_section, fdb_entry->section, fdb_entry);
                if (r < 0)
                        return r;
        }

        /* return allocated FDB structure. */
        *ret = TAKE_PTR(fdb_entry);

        return 0;
}

static int set_fdb_handler(sd_netlink *rtnl, sd_netlink_message *m, Link *link) {
        int r;

        assert(link);

        r = sd_netlink_message_get_errno(m);
        if (r < 0 && r != -EEXIST)
                log_link_error_errno(link, r, "Could not add FDB entry: %m");

        return 1;
}

/* send a request to the kernel to add a FDB entry in its static MAC table. */
int fdb_entry_configure(Link *link, FdbEntry *fdb_entry) {
        _cleanup_(sd_netlink_message_unrefp) sd_netlink_message *req = NULL;
        sd_netlink *rtnl;
        int r;
        uint8_t flags;
        Bridge *bridge;

        assert(link);
        assert(link->network);
        assert(link->manager);
        assert(fdb_entry);

        rtnl = link->manager->rtnl;
        bridge = BRIDGE(link->network->bridge);

        /* create new RTM message */
        r = sd_rtnl_message_new_neigh(rtnl, &req, RTM_NEWNEIGH, link->ifindex, PF_BRIDGE);
        if (r < 0)
                return rtnl_log_create_error(r);

        if (bridge)
                flags = NTF_MASTER;
        else
                flags = NTF_SELF;

        r = sd_rtnl_message_neigh_set_flags(req, flags);
        if (r < 0)
                return rtnl_log_create_error(r);

        /* only NUD_PERMANENT state supported. */
        r = sd_rtnl_message_neigh_set_state(req, NUD_NOARP | NUD_PERMANENT);
        if (r < 0)
                return rtnl_log_create_error(r);

        r = sd_netlink_message_append_ether_addr(req, NDA_LLADDR, fdb_entry->mac_addr);
        if (r < 0)
                return rtnl_log_create_error(r);

        /* VLAN Id is optional. We'll add VLAN Id only if it's specified. */
        if (0 != fdb_entry->vlan_id) {
                r = sd_netlink_message_append_u16(req, NDA_VLAN, fdb_entry->vlan_id);
                if (r < 0)
                        return rtnl_log_create_error(r);
        }

        /* send message to the kernel to update its internal static MAC table. */
        r = netlink_call_async(rtnl, NULL, req, set_fdb_handler,
                               link_netlink_destroy_callback, link);
        if (r < 0)
                return log_link_error_errno(link, r, "Could not send rtnetlink message: %m");

        link_ref(link);

        return 0;
}

/* remove and FDB entry. */
void fdb_entry_free(FdbEntry *fdb_entry) {
        if (!fdb_entry)
                return;

        if (fdb_entry->network) {
                LIST_REMOVE(static_fdb_entries, fdb_entry->network->static_fdb_entries, fdb_entry);
                assert(fdb_entry->network->n_static_fdb_entries > 0);
                fdb_entry->network->n_static_fdb_entries--;

                if (fdb_entry->section)
                        hashmap_remove(fdb_entry->network->fdb_entries_by_section, fdb_entry->section);
        }

        network_config_section_free(fdb_entry->section);
        free(fdb_entry->mac_addr);
        free(fdb_entry);
}

/* parse the HW address from config files. */
int config_parse_fdb_hwaddr(
                const char *unit,
                const char *filename,
                unsigned line,
                const char *section,
                unsigned section_line,
                const char *lvalue,
                int ltype,
                const char *rvalue,
                void *data,
                void *userdata) {

        Network *network = userdata;
        _cleanup_(fdb_entry_freep) FdbEntry *fdb_entry = NULL;
        int r;

        assert(filename);
        assert(section);
        assert(lvalue);
        assert(rvalue);
        assert(data);

        r = fdb_entry_new_static(network, filename, section_line, &fdb_entry);
        if (r < 0)
                return log_oom();

        /* read in the MAC address for the FDB table. */
        r = sscanf(rvalue, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
                   &fdb_entry->mac_addr->ether_addr_octet[0],
                   &fdb_entry->mac_addr->ether_addr_octet[1],
                   &fdb_entry->mac_addr->ether_addr_octet[2],
                   &fdb_entry->mac_addr->ether_addr_octet[3],
                   &fdb_entry->mac_addr->ether_addr_octet[4],
                   &fdb_entry->mac_addr->ether_addr_octet[5]);

        if (r != ETHER_ADDR_LEN) {
                log_syntax(unit, LOG_ERR, filename, line, 0, "Not a valid MAC address, ignoring assignment: %s", rvalue);
                return 0;
        }

        fdb_entry = NULL;

        return 0;
}

/* parse the VLAN Id from config files. */
int config_parse_fdb_vlan_id(
                const char *unit,
                const char *filename,
                unsigned line,
                const char *section,
                unsigned section_line,
                const char *lvalue,
                int ltype,
                const char *rvalue,
                void *data,
                void *userdata) {

        Network *network = userdata;
        _cleanup_(fdb_entry_freep) FdbEntry *fdb_entry = NULL;
        int r;

        assert(filename);
        assert(section);
        assert(lvalue);
        assert(rvalue);
        assert(data);

        r = fdb_entry_new_static(network, filename, section_line, &fdb_entry);
        if (r < 0)
                return log_oom();

        r = config_parse_vlanid(unit, filename, line, section,
                                section_line, lvalue, ltype,
                                rvalue, &fdb_entry->vlan_id, userdata);
        if (r < 0)
                return r;

        fdb_entry = NULL;

        return 0;
}
