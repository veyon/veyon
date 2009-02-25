#ifndef _X11VNC_XDAMAGE_H
#define _X11VNC_XDAMAGE_H

/* -- xdamage.h -- */

#if LIBVNCSERVER_HAVE_LIBXDAMAGE
extern Damage xdamage;
#endif
extern int use_xdamage;
extern int xdamage_present;
extern int xdamage_max_area;
extern double xdamage_memory;
extern int xdamage_tile_count, xdamage_direct_count;
extern double xdamage_scheduled_mark;
extern sraRegionPtr xdamage_scheduled_mark_region;
extern sraRegionPtr *xdamage_regions;
extern int xdamage_ticker;
extern int XD_skip, XD_tot, XD_des;

extern void add_region_xdamage(sraRegionPtr new_region);
extern void clear_xdamage_mark_region(sraRegionPtr markregion, int flush);
extern int collect_xdamage(int scancnt, int call);
extern int xdamage_hint_skip(int y);
extern void initialize_xdamage(void);
extern void create_xdamage_if_needed(void);
extern void destroy_xdamage_if_needed(void);
extern void check_xdamage_state(void);

#endif /* _X11VNC_XDAMAGE_H */
