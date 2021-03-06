/*
 * Copyright (c) 1997, 1998 Hellmuth Michaelis. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *---------------------------------------------------------------------------
 *
 *	i4b_i4bdrv.c - i4b userland interface driver
 *	--------------------------------------------
 *
 *	$Id: i4b_i4bdrv.c,v 1.2 1999/01/12 11:05:01 eivind Exp $ 
 *
 *      last edit-date: [Sat Dec  5 18:35:02 1998]
 *
 *---------------------------------------------------------------------------*/

#include "i4b.h"
#include "i4bipr.h"

#if NI4B > 1
#error "only 1 (one) i4b device possible!"
#endif

#if NI4B > 0

#include <sys/param.h>

#if defined(__FreeBSD_version) && __FreeBSD_version >= 300001
#include <sys/ioccom.h>
#include <sys/malloc.h>
#include <sys/uio.h>
#else
#include <sys/ioctl.h>
#endif

#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/mbuf.h>
#include <sys/proc.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <net/if.h>

#ifdef __FreeBSD__
#include "opt_devfs.h"
#endif

#ifdef DEVFS
#include <sys/devfsext.h>
#endif

#ifdef __FreeBSD__
#include <machine/i4b_debug.h>
#include <machine/i4b_ioctl.h>
#include <machine/i4b_cause.h>
#else
#include <i4b/i4b_debug.h>
#include <i4b/i4b_ioctl.h>
#include <i4b/i4b_cause.h>
#endif

#include <i4b/include/i4b_l3l4.h>
#include <i4b/include/i4b_mbuf.h>
#include <i4b/include/i4b_global.h>

#include <i4b/layer4/i4b_l4.h>

#if defined(__FreeBSD__) && (!defined(__FreeBSD_version) || __FreeBSD_version < 300001)
/* do nothing */
#else
#include <sys/poll.h>
#endif

struct selinfo select_rd_info;

static struct ifqueue i4b_rdqueue;
static int openflag = 0;
static int selflag = 0;
static int readflag = 0;
#ifdef DEVFS
static void *devfs_token;
#endif

#ifndef __FreeBSD__
#define	PDEVSTATIC	/* - not static - */
PDEVSTATIC void i4battach __P((void));
PDEVSTATIC int i4bopen __P((dev_t dev, int flag, int fmt, struct proc *p));
PDEVSTATIC int i4bclose __P((dev_t dev, int flag, int fmt, struct proc *p));
PDEVSTATIC int i4bread __P((dev_t dev, struct uio *uio, int ioflag));
PDEVSTATIC int i4bioctl __P((dev_t dev, int cmd, caddr_t data, int flag, struct proc *p));
PDEVSTATIC int i4bpoll __P((dev_t dev, int events, struct proc *p));

#if defined (__OpenBSD__)
PDEVSTATIC int i4bselect(dev_t dev, int rw, struct proc *p);
#endif

#endif

#if BSD > 199306 && defined(__FreeBSD__)
#define PDEVSTATIC	static

PDEVSTATIC	d_open_t	i4bopen;
PDEVSTATIC	d_close_t	i4bclose;
PDEVSTATIC	d_read_t	i4bread;
PDEVSTATIC	d_ioctl_t	i4bioctl;
#if defined(__FreeBSD_version) && __FreeBSD_version >= 300001
PDEVSTATIC	d_poll_t	i4bpoll;
#else
PDEVSTATIC	d_select_t	i4bselect;
#endif

#define CDEV_MAJOR 60
static struct cdevsw i4b_cdevsw = 
	{ i4bopen,	i4bclose,	i4bread,	nowrite,
	  i4bioctl,	nostop,		nullreset,	nodevtotty,
#if defined(__FreeBSD_version) && __FreeBSD_version >= 300001
	  i4bpoll,	nommap,		NULL,	"i4b", NULL,	-1 };
#else
	  i4bselect,	nommap,		NULL,	"i4b", NULL,	-1 };
#endif

PDEVSTATIC void i4battach(void *);
PSEUDO_SET(i4battach, i4b_i4bdrv);

#endif /* __FreeBSD__ */

/*---------------------------------------------------------------------------*
 *	interface attach routine
 *---------------------------------------------------------------------------*/
PDEVSTATIC void
#ifdef __FreeBSD__
i4battach(void *dummy)
#else
i4battach()
#endif
{
#ifndef HACK_NO_PSEUDO_ATTACH_MSG
	printf("i4b: ISDN call control device attached\n");
#endif
	i4b_rdqueue.ifq_maxlen = IFQ_MAXLEN;
#ifdef DEVFS
	devfs_token = devfs_add_devswf(&i4b_cdevsw, 0, DV_CHR,
				       UID_ROOT, GID_WHEEL, 0600,
				       "i4b");
#endif
}

/*---------------------------------------------------------------------------*
 *	i4bopen - device driver open routine
 *---------------------------------------------------------------------------*/
PDEVSTATIC int
i4bopen(dev_t dev, int flag, int fmt, struct proc *p)
{
	int x;
	
	if(minor(dev))
		return(ENXIO);

	if(openflag)
		return(EBUSY);
	
	x = splimp();
	openflag = 1;
	i4b_l4_daemon_attached();
	splx(x);
	
	return(0);
}

/*---------------------------------------------------------------------------*
 *	i4bclose - device driver close routine
 *---------------------------------------------------------------------------*/
PDEVSTATIC int
i4bclose(dev_t dev, int flag, int fmt, struct proc *p)
{
	int x = splimp();	
	openflag = 0;
	i4b_l4_daemon_detached();
	i4b_Dcleanifq(&i4b_rdqueue);
	splx(x);
	return(0);
}

/*---------------------------------------------------------------------------*
 *	i4bread - device driver read routine
 *---------------------------------------------------------------------------*/
PDEVSTATIC int
i4bread(dev_t dev, struct uio *uio, int ioflag)
{
	struct mbuf *m;
	int x;
	int error = 0;

	if(minor(dev))
		return(ENODEV);

	while(IF_QEMPTY(&i4b_rdqueue))
	{
		x = splimp();
		readflag = 1;
		splx(x);
		tsleep((caddr_t) &i4b_rdqueue, (PZERO + 1) | PCATCH, "bird", 0);
	}

	x = splimp();

	IF_DEQUEUE(&i4b_rdqueue, m);

	splx(x);
		
	if(m && m->m_len)
		error = uiomove(m->m_data, m->m_len, uio);
	else
		error = EIO;
		
	if(m)
		i4b_Dfreembuf(m);

	return(error);
}

/*---------------------------------------------------------------------------*
 *	i4bioctl - device driver ioctl routine
 *---------------------------------------------------------------------------*/
PDEVSTATIC int
#if defined (__FreeBSD_version) && __FreeBSD_version >= 300003
i4bioctl(dev_t dev, u_long cmd, caddr_t data, int flag, struct proc *p)
#else
i4bioctl(dev_t dev, int cmd, caddr_t data, int flag, struct proc *p)
#endif
{
	call_desc_t *cd;
	int error = 0;
	
	if(minor(dev))
		return(ENODEV);

	switch(cmd)
	{
		/* cdid request, reserve cd and return cdid */

		case I4B_CDID_REQ:
		{
			msg_cdid_req_t *mir;			
			mir = (msg_cdid_req_t *)data;
			cd = reserve_cd();
			mir->cdid = cd->cdid;
			break;
		}
		
		/* connect request, dial out to remote */
		
		case I4B_CONNECT_REQ:
		{
			msg_connect_req_t *mcr;
			mcr = (msg_connect_req_t *)data;	/* setup ptr */

			if((cd = cd_by_cdid(mcr->cdid)) == NULL)/* get cd */
			{
				DBGL4(L4_ERR, "i4bioctl", ("I4B_CONNECT_REQ ioctl, cdid not found!\n")); 
				error = EINVAL;
				break;
			}

			cd->controller = mcr->controller;	/* fill cd */
			cd->bprot = mcr->bprot;
			cd->driver = mcr->driver;
			cd->driver_unit = mcr->driver_unit;
			cd->cr = get_rand_cr(ctrl_desc[cd->controller].unit);

			cd->unitlen_time  = mcr->unitlen_time;
			cd->idle_time     = mcr->idle_time;
			cd->earlyhup_time = mcr->earlyhup_time;

			cd->last_aocd_time = 0;
			if(mcr->unitlen_method == ULEN_METHOD_DYNAMIC)
				cd->aocd_flag = 1;
			else
				cd->aocd_flag = 0;
				
			cd->cunits = 0;

			cd->max_idle_time = 0;	/* this is outgoing */

			cd->dir = DIR_OUTGOING;
			
			DBGL4(L4_TIMO, "i4bioctl", ("I4B_CONNECT_REQ times, unitlen=%ld idle=%ld earlyhup=%ld\n",
					(long)cd->unitlen_time, (long)cd->idle_time, (long)cd->earlyhup_time));

			strcpy(cd->dst_telno, mcr->dst_telno);
			strcpy(cd->src_telno, mcr->src_telno);
			cd->display[0] = '\0';

			SET_CAUSE_TYPE(cd->cause_in, CAUSET_I4B);
			SET_CAUSE_VAL(cd->cause_in, CAUSE_I4B_NORMAL);
			
			switch(mcr->channel)
			{
				case CHAN_B1:
				case CHAN_B2:
					if(ctrl_desc[mcr->controller].bch_state[mcr->channel] != BCH_ST_FREE)
						SET_CAUSE_VAL(cd->cause_in, CAUSE_I4B_NOCHAN);
					break;

				case CHAN_ANY:
					if((ctrl_desc[mcr->controller].bch_state[CHAN_B1] != BCH_ST_FREE) &&
					   (ctrl_desc[mcr->controller].bch_state[CHAN_B2] != BCH_ST_FREE))
						SET_CAUSE_VAL(cd->cause_in, CAUSE_I4B_NOCHAN);
					break;

				default:
					SET_CAUSE_VAL(cd->cause_in, CAUSE_I4B_NOCHAN);
					break;
			}

			cd->channelid = mcr->channel;

			cd->isdntxdelay = mcr->txdelay;
			
			/* check whether we have a pointer. Seems like */
			/* this should be adequate. GJ 19.09.97 */
			if(ctrl_desc[cd->controller].N_CONNECT_REQUEST == NULL)
/*XXX*/				SET_CAUSE_VAL(cd->cause_in, CAUSE_I4B_NOCHAN);

			if((GET_CAUSE_VAL(cd->cause_in)) != CAUSE_I4B_NORMAL)
			{
				i4b_l4_disconnect_ind(cd);
				freecd_by_cd(cd);
			}
			else
			{
				(*ctrl_desc[cd->controller].N_CONNECT_REQUEST)(mcr->cdid);
			}
			break;
		}
		
		/* connect response, accept/reject/ignore incoming call */
		
		case I4B_CONNECT_RESP:
		{
			msg_connect_resp_t *mcrsp;
			
			mcrsp = (msg_connect_resp_t *)data;

			if((cd = cd_by_cdid(mcrsp->cdid)) == NULL)/* get cd */
			{
				DBGL4(L4_ERR, "i4bioctl", ("I4B_CONNECT_RESP ioctl, cdid not found!\n")); 
				error = EINVAL;
				break;
			}

			T400_stop(cd);

			cd->driver = mcrsp->driver;
			cd->driver_unit = mcrsp->driver_unit;
			cd->max_idle_time = mcrsp->max_idle_time;

			cd->unitlen_time = 0;	/* this is incoming */
			cd->idle_time = 0;
			cd->earlyhup_time = 0;

			cd->isdntxdelay = mcrsp->txdelay;			
			
			DBGL4(L4_TIMO, "i4bioctl", ("I4B_CONNECT_RESP max_idle_time set to %ld seconds\n", (long)cd->max_idle_time));

			(*ctrl_desc[cd->controller].N_CONNECT_RESPONSE)(mcrsp->cdid, mcrsp->response, mcrsp->cause);
			break;
		}
		
		/* disconnect request, actively terminate connection */
		
		case I4B_DISCONNECT_REQ:
		{
			msg_discon_req_t *mdr;
			
			mdr = (msg_discon_req_t *)data;

			if((cd = cd_by_cdid(mdr->cdid)) == NULL)/* get cd */
			{
				DBGL4(L4_ERR, "i4bioctl", ("I4B_DISCONNECT_REQ ioctl, cdid not found!\n")); 
				error = EINVAL;
				break;
			}

			/* preset causes with our cause */
			cd->cause_in = cd->cause_out = mdr->cause;
			
			(*ctrl_desc[cd->controller].N_DISCONNECT_REQUEST)(mdr->cdid, mdr->cause);
			break;
		}
		
		/* controller info request */

		case I4B_CTRL_INFO_REQ:
		{
			msg_ctrl_info_req_t *mcir;
			
			mcir = (msg_ctrl_info_req_t *)data;
			mcir->ncontroller = nctrl;

			if(mcir->controller > nctrl)
			{
				mcir->ctrl_type = -1;
				mcir->card_type = -1;
			}
			else
			{
				mcir->ctrl_type = 
					ctrl_desc[mcir->controller].ctrl_type;
				mcir->card_type = 
					ctrl_desc[mcir->controller].card_type;

				if(ctrl_desc[mcir->controller].ctrl_type == CTRL_PASSIVE)
					mcir->tei = ctrl_desc[mcir->controller].tei;
				else
					mcir->tei = -1;
			}
			break;
		}
		
		/* dial response */
		
		case I4B_DIALOUT_RESP:
		{
			msg_dialout_resp_t *mdrsp;
			
			mdrsp = (msg_dialout_resp_t *)data;

#if NI4BIPR > 0
			if(mdrsp->driver == BDRV_IPR)
			{
				drvr_link_t *dlt;
				dlt = ipr_ret_linktab(mdrsp->driver_unit);
				(*dlt->dial_response)(mdrsp->driver_unit, mdrsp->stat);
			}
#endif
			break;
		}
		
		/* update timeout value */
		
		case I4B_TIMEOUT_UPD:
		{
			msg_timeout_upd_t *mtu;
			int x;
			
			mtu = (msg_timeout_upd_t *)data;

			if((cd = cd_by_cdid(mtu->cdid)) == NULL)/* get cd */
			{
				DBGL4(L4_ERR, "i4bioctl", ("I4B_TIMEOUT_UPD ioctl, cdid not found!\n")); 
				error = EINVAL;
				break;
			}

			x = SPLI4B();
			cd->unitlen_time = mtu->unitlen_time;
			cd->idle_time = mtu->idle_time;
			cd->earlyhup_time = mtu->earlyhup_time;
			splx(x);
			break;
		}
			
		/* soft enable/disable interface */
		
		case I4B_UPDOWN_IND:
		{
			msg_updown_ind_t *mui;
			
			mui = (msg_updown_ind_t *)data;

#if NI4BIPR > 0
			if(mui->driver == BDRV_IPR)
			{
				drvr_link_t *dlt;
				dlt = ipr_ret_linktab(mui->driver_unit);
				(*dlt->updown_ind)(mui->driver_unit, mui->updown);
			}
#endif
			break;
		}
		
		/* send ALERT request */
		
		case I4B_ALERT_REQ:
		{
			msg_alert_req_t *mar;
			
			mar = (msg_alert_req_t *)data;

			if((cd = cd_by_cdid(mar->cdid)) == NULL)
			{
				DBGL4(L4_ERR, "i4bioctl", ("I4B_ALERT_REQ ioctl, cdid not found!\n")); 
				error = EINVAL;
				break;
			}

			T400_stop(cd);
			
			(*ctrl_desc[cd->controller].N_ALERT_REQUEST)(mar->cdid);

			break;
		}

		/* version/release number request */
		
		case I4B_VR_REQ:
                {
			msg_vr_req_t *mvr;

			mvr = (msg_vr_req_t *)data;

			mvr->version = VERSION;
			mvr->release = REL;
			mvr->step = STEP;			
			break;
		}
		
		/* Download request */

		case I4B_CTRL_DOWNLOAD:
		{
			struct isdn_dr_prot *prots = NULL, *prots2 = NULL;
			struct isdn_download_request *r =
				(struct isdn_download_request*)data;
			int i;

			if(!ctrl_desc[r->controller].N_DOWNLOAD)
			{
				error = ENODEV;
				goto download_done;
			}

			prots = malloc(r->numprotos * sizeof(struct isdn_dr_prot),
					M_DEVBUF, M_WAITOK);

			prots2 = malloc(r->numprotos * sizeof(struct isdn_dr_prot),
					M_DEVBUF, M_WAITOK);

			if(!prots || !prots2)
			{
				error = ENOMEM;
				goto download_done;
			}

			copyin(r->protocols, prots, r->numprotos * sizeof(struct isdn_dr_prot));

			for(i = 0; i < r->numprotos; i++)
			{
				prots2[i].microcode = malloc(prots[i].bytecount, M_DEVBUF, M_WAITOK);
				copyin(prots[i].microcode, prots2[i].microcode, prots[i].bytecount);
				prots2[i].bytecount = prots[i].bytecount; 
			}

			error = ctrl_desc[r->controller].N_DOWNLOAD(
						ctrl_desc[r->controller].unit,
						r->numprotos, prots2);

download_done:
			if(prots2)
			{
				for(i = 0; i < r->numprotos; i++)
				{
					if(prots2[i].microcode)
					{
						free(prots2[i].microcode, M_DEVBUF);
					}
				}
				free(prots2, M_DEVBUF);
			}

			if(prots)
			{
				free(prots, M_DEVBUF);
			}
			break;
		}

		/* Diagnostic request */

		case I4B_ACTIVE_DIAGNOSTIC:
		{
			struct isdn_diagnostic_request req, *r =
				(struct isdn_diagnostic_request*)data;

			req.in_param = req.out_param = NULL;

			if(!ctrl_desc[r->controller].N_DIAGNOSTICS)
			{
				error = ENODEV;
				goto diag_done;
			}

			memcpy(&req, r, sizeof(req));

			if(req.in_param_len)
			{
				req.in_param = malloc(r->in_param_len, M_DEVBUF, M_WAITOK);

				if(!req.in_param)
				{
					error = ENOMEM;
					goto diag_done;
				}
				copyin(r->in_param, req.in_param, req.in_param_len);
			}

			if(req.out_param_len)
			{
				req.out_param = malloc(r->out_param_len, M_DEVBUF, M_WAITOK);

				if(!req.out_param)
				{
					error = ENOMEM;
					goto diag_done;
				}
			}
			
			error = ctrl_desc[r->controller].N_DIAGNOSTICS(r->controller, &req);

			if(!error && req.out_param_len)
				copyout(req.out_param, r->out_param, req.out_param_len);

diag_done:
			if(req.in_param)
				free(req.in_param, M_DEVBUF);
				
			if(req.out_param)
				free(req.out_param, M_DEVBUF);

			break;
		}

		/* default */
		
		default:
			error = ENOTTY;
			break;
	}
	
	return(error);
}

#if (defined(__FreeBSD__) && \
        (!defined(__FreeBSD_version) || (__FreeBSD_version < 300001))) \
	|| defined (__OpenBSD__)

/*---------------------------------------------------------------------------*
 *	i4bselect - device driver select routine
 *---------------------------------------------------------------------------*/
PDEVSTATIC int
i4bselect(dev_t dev, int rw, struct proc *p)
{
	int x;
	
	if(minor(dev))
		return(ENODEV);

	switch(rw)
	{
		case FREAD:
			if(!IF_QEMPTY(&i4b_rdqueue))
				return(1);
			x = splimp();
			selrecord(p, &select_rd_info);
			selflag = 1;
			splx(x);
			return(0);
			break;

		case FWRITE:
			return(1);
			break;
	}
	return(0);
}

#else /* NetBSD and FreeBSD -current */

/*---------------------------------------------------------------------------*
 *	i4bpoll - device driver poll routine
 *---------------------------------------------------------------------------*/
PDEVSTATIC int
i4bpoll(dev_t dev, int events, struct proc *p)
{
	int x;
	
	if(minor(dev))
		return(ENODEV);

	if((events & POLLIN) || (events & POLLRDNORM))
	{
		if(!IF_QEMPTY(&i4b_rdqueue))
			return(1);

		x = splimp();
		selrecord(p, &select_rd_info);
		selflag = 1;
		splx(x);
		return(0);
	}
	else if((events & POLLOUT) || (events & POLLWRNORM))
	{
		return(1);
	}

	return(0);
}

#endif /* defined(__FreeBSD__) && __FreeBSD__ < 3 */

/*---------------------------------------------------------------------------*
 *	i4bputqueue - put message into queue to userland
 *---------------------------------------------------------------------------*/
void
i4bputqueue(struct mbuf *m)
{
	int x;
	
	if(!openflag)
	{
		i4b_Dfreembuf(m);
		return;
	}

	x = splimp();
	
	if(IF_QFULL(&i4b_rdqueue))
	{
		struct mbuf *m1;
		IF_DEQUEUE(&i4b_rdqueue, m1);
		i4b_Dfreembuf(m1);
		DBGL4(L4_ERR, "i4bputqueue", ("ERROR, queue full, removing entry!\n"));
	}

	IF_ENQUEUE(&i4b_rdqueue, m);

	splx(x);	

	if(readflag)
	{
		readflag = 0;
		wakeup((caddr_t) &i4b_rdqueue);
	}

	if(selflag)
	{
		selflag = 0;
		selwakeup(&select_rd_info);
	}
}

/*---------------------------------------------------------------------------*
 *	i4bputqueue_hipri - put message into front of queue to userland
 *---------------------------------------------------------------------------*/
void
i4bputqueue_hipri(struct mbuf *m)
{
	int x;
	
	if(!openflag)
	{
		i4b_Dfreembuf(m);
		return;
	}

	x = splimp();
	
	if(IF_QFULL(&i4b_rdqueue))
	{
		struct mbuf *m1;
		IF_DEQUEUE(&i4b_rdqueue, m1);
		i4b_Dfreembuf(m1);
		DBGL4(L4_ERR, "i4bputqueue", ("ERROR, queue full, removing entry!\n"));
	}

	IF_PREPEND(&i4b_rdqueue, m);

	splx(x);	

	if(readflag)
	{
		readflag = 0;
		wakeup((caddr_t) &i4b_rdqueue);
	}

	if(selflag)
	{
		selflag = 0;
		selwakeup(&select_rd_info);
	}
}

#if BSD > 199306 && defined(__FreeBSD__)
static i4b_devsw_installed = 0;

static void
i4b_drvinit(void *unused)
{
	dev_t dev;

	if( ! i4b_devsw_installed ) {
		dev = makedev(CDEV_MAJOR,0);
		cdevsw_add(&dev,&i4b_cdevsw,NULL);
		i4b_devsw_installed = 1;
    	}
}

SYSINIT(i4bdev,SI_SUB_DRIVERS,SI_ORDER_MIDDLE+CDEV_MAJOR,i4b_drvinit,NULL)
#endif /* __FreeBSD__ */
#endif /* NI4B > 0 */
