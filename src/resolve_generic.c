/*
 * resolve_generic.c - support code for the resolver
 */
#include "resolve.h"

/*
 * Set up for a resolver.
 */
resolve	*
resolve_init(opts *opts, sccs *s)
{
	resolve	*rs = calloc(1, sizeof(*rs));
	char	buf[MAXKEY];

	rs->opts = opts;
	rs->s = s;
	sccs_sdelta(rs->s, sccs_ino(rs->s), buf);
	rs->key = strdup(buf);
	rs->d = sccs_getrev(s, "+", 0, 0);
	assert(rs->d);
	rs->dname = name2sccs(rs->d->pathname);
	if (rs->snames = getnames(sccs_Xfile(rs->s, 'm'), 'm')) {
		rs->gnames         = calloc(1, sizeof(names));
		rs->gnames->local  = sccs2name(rs->snames->local);
		rs->gnames->gca    = sccs2name(rs->snames->gca);
		rs->gnames->remote = sccs2name(rs->snames->remote);
	}
	rs->revs = getnames(sccs_Xfile(rs->s, 'r'), 'r');
	unless (rs->pager = getenv("PAGER")) rs->pager = "more";
	unless (rs->editor = getenv("EDITOR")) rs->editor = "vi";
	return (rs);
}

void
resolve_free(resolve *rs)
{
	if (rs->gnames) freenames(rs->gnames, 1);
	if (rs->snames) freenames(rs->snames, 1);
	if (rs->revs) freenames(rs->revs, 1);
	free(rs->dname);
	sccs_free(rs->s);
	free(rs->key);
	bzero(rs, sizeof(*rs));
	free(rs);
}

int
resolve_loop(char *name, resolve *rs, rfuncs *rf)
{
	int	i, ret;
	char	buf[100];

	rs->funcs = rf;
	if (rs->opts->debug) {
		fprintf(stderr, "%s\n", name);
		fprintf(stderr, "\tsccs: %s\n", rs->s->gfile);
		fprintf(stderr, "\ttot: %s@%s\n", rs->d->pathname, rs->d->rev);
	}
	rs->n = 0;
	while (1) {
		fprintf(stderr, "%s>> ", rs->prompt);
		getline(0, buf, sizeof(buf));
		unless (buf[0]) strcpy(buf, "?");

again:		/* 100 tries for the same file means we're hosed.  */
		if (++rs->n == 100) return (ELOOP);
		for (i = 0; rf[i].spec && !streq(rf[i].spec, buf); i++);
		unless (rf[i].spec) {
			strcpy(buf, "?");
			goto again;
		}
		if (rs->opts->debug) {
			fprintf(stderr,
			    "[%s] Calling %s on %s\n", buf, rf[i].name, name);
		}
		if (ret = rf[i].func(rs)) {
			if (rs->opts->debug) {
				fprintf(stderr,
				    "%s returns %d\n", rf[i].name, ret);
			}
			return (ret);
		}
		if (rs->opts->debug) {
			fprintf(stderr, "%s returns 0\n", rf[i].name);
		}
	}
}