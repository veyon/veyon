/* -- rates.c -- */

#include "x11vnc.h"
#include "xwrappers.h"
#include "scan.h"

int measure_speeds = 1;
int speeds_net_rate = 0;
int speeds_net_rate_measured = 0;
int speeds_net_latency = 0;
int speeds_net_latency_measured = 0;
int speeds_read_rate = 0;
int speeds_read_rate_measured = 0;


int get_cmp_rate(void);
int get_raw_rate(void);
void initialize_speeds(void);
int get_read_rate(void);
int link_rate(int *latency, int *netrate);
int get_net_rate(void);
int get_net_latency(void);
void measure_send_rates(int init);


static void measure_display_hook(rfbClientPtr cl);
static int get_rate(int which);
static int get_latency(void);


static void measure_display_hook(rfbClientPtr cl) {
	ClientData *cd = (ClientData *) cl->clientData;
	if (! cd) {
		return;
	}
	dtime0(&cd->timer);
}

static int get_rate(int which) {
	rfbClientIteratorPtr iter;
	rfbClientPtr cl;
	int irate, irate_min = 1;	/* 1 KB/sec */
	int irate_max = 100000;		/* 100 MB/sec */
	int count = 0;
	double slowest = -1.0, rate;
	static double save_rate = 1000 * NETRATE0;

	if (!screen) {
		return 0;
	}

	iter = rfbGetClientIterator(screen);
	while( (cl = rfbClientIteratorNext(iter)) ) {
		ClientData *cd = (ClientData *) cl->clientData;

		if (! cd) {
			continue;
		}
		if (cl->state != RFB_NORMAL) {
			continue;
		}
		if (cd->send_cmp_rate == 0.0 || cd->send_raw_rate == 0.0) {
			continue;
		}
		count++;
		
		if (which == 0) {
			rate = cd->send_cmp_rate;
		} else {
			rate = cd->send_raw_rate;
		}
		if (slowest == -1.0 || rate < slowest) {
			slowest = rate;
		}
		
	}
	rfbReleaseClientIterator(iter);

	if (! count) {
		return NETRATE0;
	}

	if (slowest == -1.0) {
		slowest = save_rate;
	} else {
		save_rate = slowest;
	}

	irate = (int) (slowest/1000.0);
	if (irate < irate_min) {
		irate = irate_min;
	}
	if (irate > irate_max) {
		irate = irate_max;
	}
if (0) fprintf(stderr, "get_rate(%d) %d %.3f/%.3f\n", which, irate, save_rate, slowest); 

	return irate;
}

static int get_latency(void) {
	rfbClientIteratorPtr iter;
	rfbClientPtr cl;
	int ilat, ilat_min = 1;	/* 1 ms */
	int ilat_max = 2000;	/* 2 sec */
	double slowest = -1.0, lat;
	static double save_lat = ((double) LATENCY0)/1000.0;
	int count = 0;
	
	if (!screen) {
		return 0;
	}

	iter = rfbGetClientIterator(screen);
	while( (cl = rfbClientIteratorNext(iter)) ) {
		ClientData *cd = (ClientData *) cl->clientData;
		
		if (! cd) {
			continue;
		}
		if (cl->state != RFB_NORMAL) {
			continue;
		}
		if (cd->latency == 0.0) {
			continue;
		}
		count++;

		lat = cd->latency;
		if (slowest == -1.0 || lat > slowest) {
			slowest = lat;
		}
	}
	rfbReleaseClientIterator(iter);

	if (! count) {
		return LATENCY0;
	}

	if (slowest == -1.0) {
		slowest = save_lat;
	} else {
		save_lat = slowest;
	}

	ilat = (int) (slowest * 1000.0);
	if (ilat < ilat_min) {
		ilat = ilat_min;
	}
	if (ilat > ilat_max) {
		ilat = ilat_max;
	}

	return ilat;
}

int get_cmp_rate(void) {
	return get_rate(0);
}

int get_raw_rate(void) {
	return get_rate(1);
}

void initialize_speeds(void) {
	char *s, *s_in, *p;
	int i;

	speeds_read_rate = 0;
	speeds_net_rate = 0;
	speeds_net_latency = 0;
	if (! speeds_str || *speeds_str == '\0') {
		s_in = strdup("");
	} else {
		s_in = strdup(speeds_str);
	}

	if (!strcmp(s_in, "modem")) {
		s = strdup("6,4,200");
	} else if (!strcmp(s_in, "dsl")) {
		s = strdup("6,100,50");
	} else if (!strcmp(s_in, "lan")) {
		s = strdup("6,5000,1");
	} else {
		s = strdup(s_in);
	}

	p = strtok(s, ",");
	i = 0;
	while (p) {
		double val;
		if (*p != '\0') {
			val = atof(p);
			if (i==0) {
				speeds_read_rate = (int) 1000000 * val;
			} else if (i==1) {
				speeds_net_rate = (int) 1000 * val;
			} else if (i==2) {
				speeds_net_latency = (int) val;
			}
		}
		i++;
		p = strtok(NULL, ",");
	}
	free(s);
	free(s_in);

	if (! speeds_read_rate) {
		int n = 0;
		double dt, timer;
		dtime0(&timer);
		if (fullscreen) {
			copy_image(fullscreen, 0, 0, 0, 0);
			n = fullscreen->bytes_per_line * fullscreen->height;
		} else if (scanline) {
			copy_image(scanline, 0, 0, 0, 0);
			n = scanline->bytes_per_line * scanline->height;
		}
		dt = dtime(&timer);
		if (n && dt > 0.0) {
			double rate = ((double) n) / dt;
			speeds_read_rate_measured = (int) (rate/1000000.0);
			if (speeds_read_rate_measured < 1) {
				speeds_read_rate_measured = 1;
			} else {
				rfbLog("fb read rate: %d MB/sec\n",
				    speeds_read_rate_measured);
			}
		}
	}
}

int get_read_rate(void) {
	if (speeds_read_rate) {
		return speeds_read_rate;
	}
	if (speeds_read_rate_measured) {
		return speeds_read_rate_measured;
	}
	return 0;
}

int link_rate(int *latency, int *netrate) {
	*latency = get_net_latency();
	*netrate = get_net_rate();

	if (speeds_str) {
		if (!strcmp(speeds_str, "modem")) {
			return LR_DIALUP;
		} else if (!strcmp(speeds_str, "dsl")) {
			return LR_BROADBAND;
		} else if (!strcmp(speeds_str, "lan")) {
			return LR_LAN;
		}
	}

	if (*latency == LATENCY0 && *netrate == NETRATE0)  {
		return LR_UNSET;
	} else if (*latency > 150 || *netrate < 20) {
		return LR_DIALUP;
	} else if (*latency > 50 || *netrate < 150) {
		return LR_BROADBAND;
	} else if (*latency < 10 && *netrate > 300) {
		return LR_LAN;
	} else {
		return LR_UNKNOWN;
	}
}

int get_net_rate(void) {
	int spm = speeds_net_rate_measured;
	if (speeds_net_rate) {
		return speeds_net_rate;
	}
	if (! spm || spm == NETRATE0) {
		speeds_net_rate_measured = get_cmp_rate();
	}
	if (speeds_net_rate_measured) {
		return speeds_net_rate_measured;
	}
	return 0;
}

int get_net_latency(void) {
	int spm = speeds_net_latency_measured;
	if (speeds_net_latency) {
		return speeds_net_latency;
	}
	if (! spm || spm == LATENCY0) {
		speeds_net_latency_measured = get_latency();
	}
	if (speeds_net_latency_measured) {
		return speeds_net_latency_measured;
	}
	return 0;
}

void measure_send_rates(int init) {
	double cmp_rate, raw_rate;
	static double now, start = 0.0;
	static rfbDisplayHookPtr orig_display_hook = NULL;
	double cmp_max = 1.0e+08;	/* 100 MB/sec */
	double cmp_min = 1000.0;	/* 9600baud */
	double lat_max = 5.0;		/* 5 sec */
	double lat_min = .0005;		/* 0.5 ms */
	int min_cmp = 10000, nclients;
	rfbClientIteratorPtr iter;
	rfbClientPtr cl;
	int db = 0, msg = 0;

db = 0;

	if (! measure_speeds) {
		return;
	}
	if (speeds_net_rate && speeds_net_latency) {
		return;
	}

	if (! orig_display_hook) {
		orig_display_hook = screen->displayHook;
	}

	if (start == 0.0) {
		dtime(&start);
	}
	dtime0(&now);
	now = now - start;

	nclients = 0;

	if (!screen) {
		return;
	}

	iter = rfbGetClientIterator(screen);
	while( (cl = rfbClientIteratorNext(iter)) ) {
		int defer, i, cbs, rbs;
		char *httpdir;
		double dt, dt1 = 0.0, dt2, dt3;
		double tm, spin_max = 15.0, spin_lat_max = 1.5;
		int got_t2 = 0, got_t3 = 0;
		ClientData *cd = (ClientData *) cl->clientData;

		if (! cd) {
			continue;
		}
		if (cd->send_cmp_rate > 0.0) {
			continue;
		}
		nclients++;


#if 0
		for (i=0; i<MAX_ENCODINGS; i++) {
			cbs += cl->bytesSent[i];
		}
		rbs = cl->rawBytesEquivalent;
#else
		cbs = rfbStatGetSentBytes(cl);
		rbs = rfbStatGetSentBytesIfRaw(cl);
#endif

		if (init) {

if (db) fprintf(stderr, "%d client num rects req: %d  mod: %d  cbs: %d  "
    "rbs: %d  dt1: %.3f  t: %.3f\n", init,
    (int) sraRgnCountRects(cl->requestedRegion),
    (int) sraRgnCountRects(cl->modifiedRegion), cbs, rbs, dt1, now);

			cd->timer = dnow();
			cd->cmp_bytes_sent = cbs;
			cd->raw_bytes_sent = rbs;
			continue;
		}

		/* first part of the bulk transfer of initial screen */
		dt1 = dtime(&cd->timer);

if (db) fprintf(stderr, "%d client num rects req: %d  mod: %d  cbs: %d  "
    "rbs: %d  dt1: %.3f  t: %.3f\n", init,
    (int) sraRgnCountRects(cl->requestedRegion),
    (int) sraRgnCountRects(cl->modifiedRegion), cbs, rbs, dt1, now);

		if (dt1 <= 0.0) {
			continue;
		}

		cbs = cbs - cd->cmp_bytes_sent;
		rbs = rbs - cd->raw_bytes_sent;

		if (cbs < min_cmp) {
			continue;
		}

		rfbPE(1000);
		if (sraRgnCountRects(cl->modifiedRegion)) {
			rfbPE(1000);
		}

		defer = screen->deferUpdateTime;
		httpdir = screen->httpDir;
		screen->deferUpdateTime = 0;
		screen->httpDir = NULL;

		/* mark a small rectangle: */
		mark_rect_as_modified(0, 0, 16, 16, 1);

		dtime0(&tm);

		dt2 = 0.0;
		dt3 = 0.0;

		if (dt1 < 0.25) {
			/* try to cut it down to avoid long pauses. */
			spin_max = 5.0;
		}

		/* when req1 = 1 mod1 == 0, end of 2nd part of bulk transfer */
		while (1) {
			int req0, req1, mod0, mod1;
			req0 = sraRgnCountRects(cl->requestedRegion);
			mod0 = sraRgnCountRects(cl->modifiedRegion);
			if (use_threads) {
				usleep(1000);
			} else {
				if (mod0) {
					rfbPE(1000);
				} else {
					rfbCFD(1000);
				}
			}
			dt = dtime(&tm);
			dt2 += dt;
			if (dt2 > spin_max) {
				break;
			}
			req1 = sraRgnCountRects(cl->requestedRegion);
			mod1 = sraRgnCountRects(cl->modifiedRegion);

if (db) fprintf(stderr, "dt2 calc: num rects req: %d/%d mod: %d/%d  "
    "fbu-sent: %d  dt: %.4f dt2: %.4f  tm: %.4f\n",
    req0, req1, mod0, mod1,
#if 0
    cl->framebufferUpdateMessagesSent,
#else
    rfbStatGetMessageCountSent(cl, rfbFramebufferUpdate),
#endif
    dt, dt2, tm);
			if (req1 != 0 && mod1 == 0) {
				got_t2 = 1;
				break;
			}
		}

		if (! got_t2) {
			dt2 = 0.0;
		} else {
			int tr, trm = 3;
			double dts[10];
			
			/*
			 * Note: since often select(2) cannot sleep
			 * less than 1/HZ (e.g. 10ms), the resolution
			 * of the latency may be messed up by something
			 * of this order.  Effect may occur on both ends,
			 * i.e. the viewer may not respond immediately.
			 */
		
			for (tr = 0; tr < trm; tr++) {
				usleep(5000);

				/* mark a 2nd small rectangle: */
				mark_rect_as_modified(0, 0, 16, 16, 1);
				i = 0;
				dtime0(&tm);
				dt3 = 0.0;

				/*
				 * when req1 > 0 and mod1 == 0, we say
				 * that is the "ping" time.
				 */
				while (1) {
					int req0, req1, mod0, mod1;

					req0 = sraRgnCountRects(
					    cl->requestedRegion);
					mod0 = sraRgnCountRects(
					    cl->modifiedRegion);

					if (i == 0) {
						rfbPE(0);
					} else {
						if (use_threads) {
							usleep(1000);
						} else {
							/* try to get it all */
							rfbCFD(1000*1000);
						}
					}
					dt = dtime(&tm);
					i++;

					dt3 += dt;
					if (dt3 > spin_lat_max) {
						break;
					}
					req1 = sraRgnCountRects(
					    cl->requestedRegion);

					mod1 = sraRgnCountRects(
					    cl->modifiedRegion);

if (db) fprintf(stderr, "dt3 calc: num rects req: %d/%d mod: %d/%d  "
    "fbu-sent: %d  dt: %.4f dt3: %.4f  tm: %.4f\n",
    req0, req1, mod0, mod1,
#if 0
    cl->framebufferUpdateMessagesSent,
#else
    rfbStatGetMessageCountSent(cl, rfbFramebufferUpdate),
#endif
    dt, dt3, tm);

					if (req1 != 0 && mod1 == 0) {
						dts[got_t3++] = dt3;
						break;
					}
				}
			}
		
			if (! got_t3) {
				dt3 = 0.0;
			} else {
				if (got_t3 == 1) {
					dt3 = dts[0];
				} else if (got_t3 == 2) {
					dt3 = dts[1];
				} else {
					if (dts[2] > 0.0) {
						double rat = dts[1]/dts[2];
						if (rat > 0.5 && rat < 2.0) {
							dt3 = dts[1]+dts[2];
							dt3 *= 0.5;
						} else {
							dt3 = dts[1];
						}
					} else {
						dt3 = dts[1];
					}
				}
			}
		}
		
		screen->deferUpdateTime = defer;
		screen->httpDir = httpdir;

		dt = dt1 + dt2;


		if (dt3 <= dt2/2.0) {
			/* guess only 1/2 a ping for reply... */
			dt = dt - dt3/2.0;
		}

		cmp_rate = cbs/dt;
		raw_rate = rbs/dt;

		if (cmp_rate > cmp_max) {
			cmp_rate = cmp_max;
		}
		if (cmp_rate <= cmp_min) {
			cmp_rate = cmp_min;
		}

		cd->send_cmp_rate = cmp_rate;
		cd->send_raw_rate = raw_rate;

		if (dt3 > lat_max) {
			dt3 = lat_max;
		}
		if (dt3 <= lat_min) {
			dt3 = lat_min;
		}

		cd->latency = dt3;
		
		rfbLog("client %d network rate %.1f KB/sec (%.1f eff KB/sec)\n",
		    cd->uid, cmp_rate/1000.0, raw_rate/1000.0);
		rfbLog("client %d latency:  %.1f ms\n", cd->uid, 1000.0*dt3);
		rfbLog("dt1: %.4f, dt2: %.4f dt3: %.4f bytes: %d\n",
		    dt1, dt2, dt3, cbs);
		msg = 1;
	}
	rfbReleaseClientIterator(iter);

	if (msg) {
		int link, latency, netrate;
		char *str = "error";

		link = link_rate(&latency, &netrate);
		if (link == LR_UNSET) {
			str = "LR_UNSET";
		} else if (link == LR_UNKNOWN) {
			str = "LR_UNKNOWN";
		} else if (link == LR_DIALUP) {
			str = "LR_DIALUP";
		} else if (link == LR_BROADBAND) {
			str = "LR_BROADBAND";
		} else if (link == LR_LAN) {
			str = "LR_LAN";
		}
		rfbLog("link_rate: %s - %d ms, %d KB/s\n", str, latency,
		    netrate);
	}

	if (init) {
		if (nclients) {
			screen->displayHook = measure_display_hook;
		}
	} else {
		screen->displayHook = orig_display_hook;
	}
}

