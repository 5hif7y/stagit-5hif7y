#ifdef _WIN32
  #include "wincompat.h"
#else
  #include <err.h>
  #include <unistd.h>
#endif
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <git2.h>

static git_repository *repo;

static const char *relpath = "";

static char description[255] = "Repositories";
static char *name = "";
static char owner[255];

/* Handle read or write errors for a FILE * stream */
void
checkfileerror(FILE *fp, const char *name, int mode)
{
	if (mode == 'r' && ferror(fp))
		errx(1, "read error: %s", name);
	else if (mode == 'w' && (fflush(fp) || ferror(fp)))
		errx(1, "write error: %s", name);
}

void
joinpath(char *buf, size_t bufsiz, const char *path, const char *path2)
{
	int r;

	r = snprintf(buf, bufsiz, "%s%s%s",
		path, path[0] && path[strlen(path) - 1] != '/' ? "/" : "", path2);
	if (r < 0 || (size_t)r >= bufsiz)
		errx(1, "path truncated: '%s%s%s'",
			path, path[0] && path[strlen(path) - 1] != '/' ? "/" : "", path2);
}

/* Percent-encode, see RFC3986 section 2.1. */
void
percentencode(FILE *fp, const char *s, size_t len)
{
	static char tab[] = "0123456789ABCDEF";
	unsigned char uc;
	size_t i;

	for (i = 0; *s && i < len; s++, i++) {
		uc = *s;
		/* NOTE: do not encode '/' for paths or ",-." */
		if (uc < ',' || uc >= 127 || (uc >= ':' && uc <= '@') ||
		    uc == '[' || uc == ']') {
			putc('%', fp);
			putc(tab[(uc >> 4) & 0x0f], fp);
			putc(tab[uc & 0x0f], fp);
		} else {
			putc(uc, fp);
		}
	}
}

/* Escape characters below as HTML 2.0 / XML 1.0. */
void
xmlencode(FILE *fp, const char *s, size_t len)
{
	size_t i;

	for (i = 0; *s && i < len; s++, i++) {
		switch(*s) {
		case '<':  fputs("&lt;",   fp); break;
		case '>':  fputs("&gt;",   fp); break;
		case '\'': fputs("&#39;" , fp); break;
		case '&':  fputs("&amp;",  fp); break;
		case '"':  fputs("&quot;", fp); break;
		default:   putc(*s, fp);
		}
	}
}

void
printtimeshort(FILE *fp, const git_time *intime)
{
	struct tm *intm;
	time_t t;
	char out[32];

	t = (time_t)intime->time;
	if (!(intm = gmtime(&t)))
		return;
	strftime(out, sizeof(out), "%Y-%m-%d %H:%M", intm);
	fputs(out, fp);
}

void
writeheader(FILE *fp)
{
	fputs("<!DOCTYPE html>\n"
		"<html>\n<head>\n"
		"<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\" />\n"
		"<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\" />\n"
		"<title>", fp);
	xmlencode(fp, description, strlen(description));
	fprintf(fp, "</title>\n<link rel=\"icon\" type=\"image/png\" href=\"%sfavicon.png\" />\n", relpath);
	fprintf(fp, "<link rel=\"stylesheet\" type=\"text/css\" href=\"%sstyle.css\" />\n", relpath);
	fputs("</head>\n<body>\n", fp);
	fprintf(fp, "<table>\n<tr><td><img src=\"%slogo.png\" alt=\"\" width=\"32\" height=\"32\" /></td>\n"
	        "<td><h1>", relpath);
	xmlencode(fp, description, strlen(description));
	fputs("</h1></td></tr><tr><td></td><td>\n"
		"</td></tr>\n</table>\n<hr/>\n<div id=\"content\">\n"
		"<table id=\"index\"><thead>\n"
		"<tr><td><b>Name</b></td><td><b>Description</b></td><td><b>Owner</b></td>"
		"<td><b>Last commit</b></td></tr>"
		"</thead><tbody>\n", fp);
}

void
writefooter(FILE *fp)
{
	fputs("</tbody>\n</table>\n</div>\n</body>\n</html>\n", fp);
}

int
writelog(FILE *fp)
{
	git_commit *commit = NULL;
	const git_signature *author;
	git_revwalk *w = NULL;
	git_oid id;
	char *stripped_name = NULL, *p;
	int ret = 0;

	git_revwalk_new(&w, repo);
	git_revwalk_push_head(w);

	if (git_revwalk_next(&id, w) ||
	    git_commit_lookup(&commit, repo, &id)) {
		ret = -1;
		goto err;
	}

	author = git_commit_author(commit);

	/* strip .git suffix */
	if (!(stripped_name = strdup(name)))
		err(1, "strdup");
	if ((p = strrchr(stripped_name, '.')))
		if (!strcmp(p, ".git"))
			*p = '\0';

	fputs("<tr><td><a href=\"", fp);
	percentencode(fp, stripped_name, strlen(stripped_name));
	fputs("/log.html\">", fp);
	xmlencode(fp, stripped_name, strlen(stripped_name));
	fputs("</a></td><td>", fp);
	xmlencode(fp, description, strlen(description));
	fputs("</td><td>", fp);
	xmlencode(fp, owner, strlen(owner));
	fputs("</td><td>", fp);
	if (author)
		printtimeshort(fp, &(author->when));
	fputs("</td></tr>", fp);

	git_commit_free(commit);
err:
	git_revwalk_free(w);
	free(stripped_name);

	return ret;
}

int
main(int argc, char *argv[])
{
	FILE *fp;
	char path[PATH_MAX], repodirabs[PATH_MAX + 1];
	const char *repodir;
	int i, ret = 0;

	if (argc < 2) {
		fprintf(stderr, "usage: %s [repodir...]\n", argv[0]);
		return 1;
	}

	/* do not search outside the git repository:
	   GIT_CONFIG_LEVEL_APP is the highest level currently */
	git_libgit2_init();
	for (i = 1; i <= GIT_CONFIG_LEVEL_APP; i++)
		git_libgit2_opts(GIT_OPT_SET_SEARCH_PATH, i, "");
	/* do not require the git repository to be owned by the current user */
	git_libgit2_opts(GIT_OPT_SET_OWNER_VALIDATION, 0);

#ifdef __OpenBSD__
	if (pledge("stdio rpath", NULL) == -1)
		err(1, "pledge");
#endif

	writeheader(stdout);

	for (i = 1; i < argc; i++) {
		repodir = argv[i];
		if (!realpath(repodir, repodirabs))
			err(1, "realpath");

		if (git_repository_open_ext(&repo, repodir,
		    GIT_REPOSITORY_OPEN_NO_SEARCH, NULL)) {
			fprintf(stderr, "%s: cannot open repository\n", argv[0]);
			ret = 1;
			continue;
		}

		/* use directory name as name */
		if ((name = strrchr(repodirabs, '/')))
			name++;
		else
			name = "";

		/* read description or .git/description */
		joinpath(path, sizeof(path), repodir, "description");
		if (!(fp = fopen(path, "r"))) {
			joinpath(path, sizeof(path), repodir, ".git/description");
			fp = fopen(path, "r");
		}
		description[0] = '\0';
		if (fp) {
			if (!fgets(description, sizeof(description), fp))
				description[0] = '\0';
			checkfileerror(fp, "description", 'r');
			fclose(fp);
		}
    if (description[0] == '\0' || strcmp(description, "Unnamed repository; edit this file 'description' to name the repository.\n") == 0)
      fprintf(stderr, "Warning: repository '%s' has no valid description.\n", repodir);

		/* read owner or .git/owner */
		joinpath(path, sizeof(path), repodir, "owner");
		if (!(fp = fopen(path, "r"))) {
			joinpath(path, sizeof(path), repodir, ".git/owner");
			fp = fopen(path, "r");
		}
		owner[0] = '\0';
		if (fp) {
			if (!fgets(owner, sizeof(owner), fp))
				owner[0] = '\0';
			checkfileerror(fp, "owner", 'r');
			fclose(fp);
			owner[strcspn(owner, "\n")] = '\0';
		}
    if (owner[0] == '\0')
      fprintf(stderr, "Warning: repository '%s' has no owner set.\n", repodir);

		writelog(stdout);
	}
	writefooter(stdout);

	/* cleanup */
	git_repository_free(repo);
	git_libgit2_shutdown();

	checkfileerror(stdout, "<stdout>", 'w');

	return ret;
}
