/*
*         OpenPBS (Portable Batch System) v2.3 Software License
*
* Copyright (c) 1999-2000 Veridian Information Solutions, Inc.
* All rights reserved.
*
* ---------------------------------------------------------------------------
* For a license to use or redistribute the OpenPBS software under conditions
* other than those described below, or to purchase support for this software,
* please contact Veridian Systems, PBS Products Department ("Licensor") at:
*
*    www.OpenPBS.org  +1 650 967-4675                  sales@OpenPBS.org
*                        877 902-4PBS (US toll-free)
* ---------------------------------------------------------------------------
*
* This license covers use of the OpenPBS v2.3 software (the "Software") at
* your site or location, and, for certain users, redistribution of the
* Software to other sites and locations.  Use and redistribution of
* OpenPBS v2.3 in source and binary forms, with or without modification,
* are permitted provided that all of the following conditions are met.
* After December 31, 2001, only conditions 3-6 must be met:
*
* 1. Commercial and/or non-commercial use of the Software is permitted
*    provided a current software registration is on file at www.OpenPBS.org.
*    If use of this software contributes to a publication, product, or
*    service, proper attribution must be given; see www.OpenPBS.org/credit.html
*
* 2. Redistribution in any form is only permitted for non-commercial,
*    non-profit purposes.  There can be no charge for the Software or any
*    software incorporating the Software.  Further, there can be no
*    expectation of revenue generated as a consequence of redistributing
*    the Software.
*
* 3. Any Redistribution of source code must retain the above copyright notice
*    and the acknowledgment contained in paragraph 6, this list of conditions
*    and the disclaimer contained in paragraph 7.
*
* 4. Any Redistribution in binary form must reproduce the above copyright
*    notice and the acknowledgment contained in paragraph 6, this list of
*    conditions and the disclaimer contained in paragraph 7 in the
*    documentation and/or other materials provided with the distribution.
*
* 5. Redistributions in any form must be accompanied by information on how to
*    obtain complete source code for the OpenPBS software and any
*    modifications and/or additions to the OpenPBS software.  The source code
*    must either be included in the distribution or be available for no more
*    than the cost of distribution plus a nominal fee, and all modifications
*    and additions to the Software must be freely redistributable by any party
*    (including Licensor) without restriction.
*
* 6. All advertising materials mentioning features or use of the Software must
*    display the following acknowledgment:
*
*     "This product includes software developed by NASA Ames Research Center,
*     Lawrence Livermore National Laboratory, and Veridian Information
*     Solutions, Inc.
*     Visit www.OpenPBS.org for OpenPBS software support,
*     products, and information."
*
* 7. DISCLAIMER OF WARRANTY
*
* THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND. ANY EXPRESS
* OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
* OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT
* ARE EXPRESSLY DISCLAIMED.
*
* IN NO EVENT SHALL VERIDIAN CORPORATION, ITS AFFILIATED COMPANIES, OR THE
* U.S. GOVERNMENT OR ANY OF ITS AGENCIES BE LIABLE FOR ANY DIRECT OR INDIRECT,
* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
* OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
* LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
* EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
* This license will be governed by the laws of the Commonwealth of Virginia,
* without reference to its choice of law rules.
*/
/*
 * node_attr_def is the array of "pbs_attribute" definitions for a node.
 * Each legal node "pbs_attribute" is defined here
 */

#include <pbs_config.h>		/* the master config generated by configure */

#include "pbs_ifl.h"
#include "list_link.h"
#include "attribute.h"

/* External Functions Referenced */

/*
* extern int decode_state(struct pbs_attribute *patr, char *nam, char *rn, char *val);
* extern int decode_props(struct pbs_attribute *patr, char *nam, char *rn, char *val);
* extern int encode_state      (struct pbs_attribute *patr, tlist_head *ph, char *anm, char *rmn, int mode);
* extern int encode_props      (struct pbs_attribute *patr, tlist_head *ph, char *anm, char *rmn, int mode);
* extern int set_node_state    (pbs_attribute *patr, pbs_attribute *new, enum batch_op);
* extern int set_node_props    (pbs_attribute *patr, pbs_attribute *new, enum batch_op);
* extern int node_state        (pbs_attribute *patr, void *pobj, int actmode);
* extern int node_np_action    (pbs_attribute *patr, void *pobj, int actmode);
* extern int node_prop_list    (pbs_attribute *patr, void *pobj, int actmode);
* extern void free_null      (pbs_attribute *patr);
* extern void free_prop_attr   (pbs_attribute *patr);
* extern int  comp_null      (pbs_attribute *patr, pbs_attribute *with);
*/


/*
 * The entries for each pbs_attribute are (see attribute.h):
 * name,
 * decode function,
 * encode function,
 * set function,
 * compare function,
 * free value space function,
 * action function,
 * access permission flags,
 * value type
 */

attribute_def node_attr_def[] =
  {

	/* ND_ATR_state */
    { (char *)ATTR_NODE_state, /* "state" */
    decode_state,
    encode_state,
    set_node_state,
    comp_null,
    free_null,
    node_state,
    NO_USER_SET,
    ATR_TYPE_SHORT,
    PARENT_TYPE_NODE,
    },

    /* ND_ATR_power_state */
    { (char *) ATTR_NODE_power_state, /* "power_state" */
    decode_power_state,
    encode_power_state,
    set_power_state,
    comp_null,
    free_null,
    node_power_state,
    MGR_ONLY_SET,
    ATR_TYPE_SHORT,
    PARENT_TYPE_NODE,
    },

	/* ND_ATR_np */
	{ (char *)ATTR_NODE_np,  /* "np" */
	  decode_l,
	  encode_l,
	  set_l,
	  comp_null,
	  free_null,
	  node_np_action,
	  NO_USER_SET,
	  ATR_TYPE_LONG,
	  PARENT_TYPE_NODE,
	},
	/* ND_ATR_properties */
	{ (char *)ATTR_NODE_properties, /* "properties" */
	  decode_arst,
	  encode_arst,
	  set_arst,
	  comp_null,
	  free_arst,
	  node_prop_list,
	  MGR_ONLY_SET,
	  ATR_TYPE_ARST,
	  PARENT_TYPE_NODE,
	},
  
	/* ND_ATR_ntype */
	{ (char *)ATTR_NODE_ntype, /* "ntype" */
	  decode_ntype,
	  encode_ntype,
	  set_node_ntype,
	  comp_null,
	  free_null,
	  node_ntype,
	  NO_USER_SET,
	  ATR_TYPE_SHORT,
	  PARENT_TYPE_NODE,
	},
  
	/* ND_ATR_jobs */
	{   (char *)ATTR_NODE_jobs,         /* "jobs" */
		decode_null,
		encode_jobs,
		set_null,
		comp_null,
		free_null,
		NULL_FUNC,
		ATR_DFLAG_RDACC,
		ATR_TYPE_JINFOP,
		PARENT_TYPE_NODE,
	},
  
	/* ND_ATR_status */
	{  (char *)ATTR_NODE_status,
	   decode_arst,
	   encode_arst,
	   set_arst,
	   comp_null,
	   free_arst,
	   node_status_list,
	   MGR_ONLY_SET,
	   ATR_TYPE_ARST,
	   PARENT_TYPE_NODE,
	},
  
	/* ND_ATR_note */
	{ (char *)ATTR_NODE_note, /* "note" */
	  decode_str,
	  encode_str,
	  set_note_str,
	  comp_str,
	  free_str,
	  node_note,
	  NO_USER_SET,
	  ATR_TYPE_STR,
	  PARENT_TYPE_NODE,
	},
	/* ND_ATR_mom_port */
	{ (char *)ATTR_NODE_mom_port,  /* "mom_service_port" */
	  decode_l,
	  encode_l,
	  set_l,
	  comp_null,
	  free_null,
	  node_mom_port_action,
	  NO_USER_SET,
	  ATR_TYPE_LONG,
	  PARENT_TYPE_NODE,
	},
	/* ND_ATR_mom_rm_port */
	{ (char *)ATTR_NODE_mom_rm_port,  /* "mom_manager_port" */
	  decode_l,
	  encode_l,
	  set_l,
	  comp_null,
	  free_null,
	  node_mom_rm_port_action,
	  NO_USER_SET,
	  ATR_TYPE_LONG,
	  PARENT_TYPE_NODE,
	},
  /* ND_ATR_num_node_boards */
  { (char *)ATTR_NODE_num_node_boards, /* "num_node_boards" */
    decode_l,
    encode_l,
    set_l,
    comp_null,
    free_null,
    node_numa_action,
    NO_USER_SET | ATR_DFLAG_NOSTAT,
    ATR_TYPE_LONG,
    PARENT_TYPE_NODE,
  },
  /* ND_ATR_numa_str */
  { (char *)ATTR_NODE_numa_str, /* "node_board_str" */
    decode_str,
    encode_str,
    set_str,
    comp_str,
    free_str,
    numa_str_action,
    NO_USER_SET | ATR_DFLAG_NOSTAT,
    ATR_TYPE_STR,
    PARENT_TYPE_NODE,
  },
  /* ND_ATR_gpus */
  { (char *)ATTR_NODE_gpus,    /* "gpus" */
    decode_l,
    encode_l,
    set_l,
    comp_null,
    free_null,
    node_gpus_action,
    NO_USER_SET,
    ATR_TYPE_LONG,
    PARENT_TYPE_NODE,
  },
  /* ND_ATR_gpus_str */
  { (char *)ATTR_NODE_gpus_str, /* "numa_gpu_node_str" */
    decode_str,
    encode_str,
    set_str,
    comp_str,
    free_str,
    gpu_str_action,
    NO_USER_SET | ATR_DFLAG_NOSTAT,
    ATR_TYPE_STR,
    PARENT_TYPE_NODE,
  },
  /* ND_ATR_gpustatus */
  {(char *)ATTR_NODE_gpustatus,		/* "gpu_status" */
   decode_arst,
   encode_arst,
   set_arst,
   comp_null,
   free_arst,
   node_gpustatus_list,
   MGR_ONLY_SET,
   ATR_TYPE_ARST,
   PARENT_TYPE_NODE,
   },
  /* ND_ATR_mics */
  { (char *)ATTR_NODE_mics,    /* "mics" */
    decode_l,
    encode_l,
    set_l,
    comp_null,
    free_null,
    node_mics_action,
    NO_USER_SET,
    ATR_TYPE_LONG,
    PARENT_TYPE_NODE,
  },
  /* ND_ATR_micstatus */
  {(char *)ATTR_NODE_micstatus,   /* "mic_status" */
   decode_arst,
   encode_arst,
   set_arst,
   comp_null,
   free_arst,
   node_micstatus_list,
   MGR_ONLY_SET,
   ATR_TYPE_ARST,
   PARENT_TYPE_NODE,
#ifndef PENABLE_LINUX_CGROUPS
  }
#else
  },
	/* ND_ATR_total_sockets */
	{ (char *)ATTR_NODE_total_sockets,  /* "total_sockets" */
	  decode_l,
	  encode_l,
	  set_l,
	  comp_null,
	  free_null,
	  node_total_socket_action,
	  NO_USER_SET,
	  ATR_TYPE_LONG,
	  PARENT_TYPE_NODE,
	},
 	/* ND_ATR_total_chips */
	{ (char *)ATTR_NODE_total_chips,  /* "total_chips" */
	  decode_l,
	  encode_l,
	  set_l,
	  comp_null,
	  free_null,
	  node_total_chip_action,
	  NO_USER_SET,
	  ATR_TYPE_LONG,
	  PARENT_TYPE_NODE,
	},
 	/* ND_ATR_total_cores */
	{ (char *)ATTR_NODE_total_cores,  /* "total_cores" */
	  decode_l,
	  encode_l,
	  set_l,
	  comp_null,
	  free_null,
	  node_total_core_action,
	  NO_USER_SET,
	  ATR_TYPE_LONG,
	  PARENT_TYPE_NODE,
	},
 	/* ND_ATR_total_threads */
	{ (char *)ATTR_NODE_total_threads,  /* "total_threads" */
	  decode_l,
	  encode_l,
	  set_l,
	  comp_null,
	  free_null,
	  node_total_thread_action,
	  NO_USER_SET,
	  ATR_TYPE_LONG,
	  PARENT_TYPE_NODE,
	},
 	/* ND_ATR_available_sockets */
	{ (char *)ATTR_NODE_available_sockets,  /* "available_sockets" */
	  decode_l,
	  encode_l,
	  set_l,
	  comp_null,
	  free_null,
	  node_available_socket_action,
	  NO_USER_SET,
	  ATR_TYPE_LONG,
	  PARENT_TYPE_NODE,
	},
 	/* ND_ATR_available_chips */
	{ (char *)ATTR_NODE_available_chips,  /* "available_chips" */
	  decode_l,
	  encode_l,
	  set_l,
	  comp_null,
	  free_null,
	  node_available_chip_action,
	  NO_USER_SET,
	  ATR_TYPE_LONG,
	  PARENT_TYPE_NODE,
	},
 	/* ND_ATR_available_cores */
	{ (char *)ATTR_NODE_available_cores,  /* "available_cores" */
	  decode_l,
	  encode_l,
	  set_l,
	  comp_null,
	  free_null,
	  node_available_core_action,
	  NO_USER_SET,
	  ATR_TYPE_LONG,
	  PARENT_TYPE_NODE,
	},
 	/* ND_ATR_available_threads */
	{ (char *)ATTR_NODE_available_threads,  /* "available_threads" */
	  decode_l,
	  encode_l,
	  set_l,
	  comp_null,
	  free_null,
	  node_available_thread_action,
	  NO_USER_SET,
	  ATR_TYPE_LONG,
	  PARENT_TYPE_NODE,
	}
#endif
  };
