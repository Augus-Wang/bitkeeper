/*
 * Stolen from get.c and stripped down.
 *
 * Copyright (c) 1997-2001 L.W.McVoy
 */
#include "system.h"
#include "sccs.h"
#include "range.h"
WHATSTR("@(#)%K%");

typedef struct	{
	char	*key;	/* if set, we have the whole key */
	char	*user;	/* if set, we look for user matches */
	char	*host;	/* if set, we look for host.domain matches */
	char	*email;	/* if set, we look for u@h.d matches */
	char	*path;	/* if set, we look for pathname matches */
	time_t	utc;	/* if set, we look for UTC matches */
	sum_t	cksum;	/* if set, we look for cksum matches */
	char	*random;/* if set, look for random bits matche s*/
	u32	pkey:1;	/* if set, print the key as well */
} look;
private void	findkey(sccs *s, look l);
private	time_t	utc(char *date);

int
findkey_main(int ac, char **av)
{
	sccs	*s;
	int	expand = 2, errors = 0, c;
	project	*proj = 0;
	look	look;
	char	*name;
	RANGE_DECL;

	if (ac == 2 && streq("--help", av[1])) {
		system("bk help findkey");
		return (1);
	}

	/* user@host.domain|path/name|UTC|CKSUM|RANDOM
	 * -b random
	 * -c cksum
	 * -e user@host.domain
	 * -h host.domain
	 * -p path
	 * -t utc
	 * -u user
	 *
	 * Usage: findkey [opts | key] [files]
	 */
	bzero(&look, sizeof(look));
	while ((c = getopt(ac, av, "b;c;e;h;kp;r;t;u;")) != -1) {
		switch (c) {
		    case 'b': look.random = optarg; break;
		    case 'c': look.cksum = atoi(optarg); break;
		    case 'e': look.email = optarg; break;
		    case 'h': look.host = optarg; break;
		    case 'k': look.pkey = 1; break;
		    case 'p': look.path = optarg; break;
		    case 't': look.utc = utc(optarg); break;
		    case 'u': look.user = optarg; break;
		    RANGE_OPTS('d', 'r');
		    default:
usage:			system("bk help -s findkey");
			return (1);
		}
	}
	unless (look.random || look.cksum ||
	    look.email || look.host || look.path || look.utc || look.user) {
	    	unless (av[optind] && strchr(av[optind], '|')) {
			fprintf(stderr, "%s: missing key\n", av[0]);
			return (1);
		}
		look.key = av[optind++];
	}
	if (look.email && !strchr(look.email, '@')) {
		fprintf(stderr, "%s: email address must have @ sign.\n", look.email);
		return (1);
	}
	name = sfileFirst("findkey", &av[optind], 0);
	for (; name; name = sfileNext()) {
		unless (s = sccs_init(name, INIT_SAVEPROJ, proj)) continue;
		unless (proj) proj = s->proj;
		unless (s->tree) {
			sccs_free(s);
			continue;
		}
		RANGE("findkey", s, 3, 0);
		findkey(s, look);
next:		sccs_free(s);
	}
	sfileDone();
	if (proj) proj_free(proj);
	return (errors);
}

/*
 * If it is a 99/03/09 style data, just pass it in.
 * Otherwise convert it and pass it in.
 */
private time_t
utc(char *date)
{
	int	year, mon, day, min, hour, sec;
	char	buf[100];

	if (strchr(date, '/')) return (sccs_date2time(date, 0));
	unless (sscanf(date, "%4d%02d%02d%02d%02d%02d",
	    &year, &mon, &day, &min, &hour, &sec) == 6) {
	    	fprintf(stderr, "Bad date format %s\n", date);
		exit(1);
	}
	sprintf(buf,
	    "%d/%02d/%02d %02d:%02d:%02d", year, mon, day, min, hour, sec);
	return (sccs_date2time(buf, 0));
}

private void
findkey(sccs *s, look l)
{
	delta	*d;
	char	key[MAXKEY];

	for (d = s->table; d; d = d->next) {
		unless (d->flags & D_SET) continue;
		/* continues if no match, print if we get through all */
		if (l.key) {
			sccs_sdelta(s, d, key);
			unless (streq(key, l.key)) continue;
		} 
		if (l.random) unless (streq(d->random, l.random)) continue;
		if (l.cksum) unless (d->sum == l.cksum) continue;
		if (l.utc) unless (d->date == l.utc) continue;
		if (l.path) unless (streq(d->pathname, l.path)) continue;
		if (l.user) unless (streq(d->user, l.user)) continue;
		if (l.host) unless (streq(d->hostname, l.host)) continue;
		if (l.email) {
			char	*at = strchr(l.email, '@');

			*at = 0;
			unless (streq(d->user, l.email) &&
			    streq(d->hostname, at+1)) {
				*at = '@';
				continue;
			}
			*at = '@';
		}
		printf("%s|%s", s->gfile, d->rev);
		if (l.pkey) {
			sccs_sdelta(s, d, key);
			printf("\t%s", key);
		}
		printf("\n");
	}
}