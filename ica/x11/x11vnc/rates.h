#ifndef _X11VNC_RATES_H
#define _X11VNC_RATES_H

/* -- rates.h -- */

extern int measure_speeds;
extern int speeds_net_rate;
extern int speeds_net_rate_measured;
extern int speeds_net_latency;
extern int speeds_net_latency_measured;
extern int speeds_read_rate;
extern int speeds_read_rate_measured;

extern int get_cmp_rate(void);
extern int get_raw_rate(void);
extern void initialize_speeds(void);
extern int get_read_rate(void);
extern int link_rate(int *latency, int *netrate);
extern int get_net_rate(void);
extern int get_net_latency(void);
extern void measure_send_rates(int init);

#endif /* _X11VNC_RATES_H */
