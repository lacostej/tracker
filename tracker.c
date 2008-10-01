#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <pwd.h>
#include <sys/wait.h>

#define TRACKER_FILE "/var/log/tracker/%s"

enum state {
	no_user, old_user, new_user
};

static struct user {
	enum state state;
	unsigned int uid;
	unsigned long last;
	char *gecos, *login;
	struct user *next;
} *userlist;

static void update_fd(struct user *user, int fd, unsigned int s)
{
	static char buffer[1024];
	unsigned long max, cur, last;
	long left;
	int n;

	n = read(fd, buffer, sizeof(buffer)-1);
	if (n < 0)
		return;
	buffer[n] = 0;
	lseek(fd, 0, SEEK_SET);
	cur = 0;
	max = 60*60;
	last = user->last;
	sscanf(buffer, "%lu %lu %lu", &max, &cur, &last);
	cur += s;

	/* Has the user been logged out more than 8 hours? */
	if (user->last - last >= 8*60*60)
		cur = 0;

	left = max - cur;
	if (left < 0)
		left = 0;
	n = snprintf(buffer, sizeof(buffer), "%lu %lu %lu\n%s\n%lu:%02lu:%02lu left\n",
		max, cur, user->last,
		user->gecos,
		left / 3600, (left / 60) % 60, left % 60);
	/*
	 * We write it with the final '\0' and then truncate it.
	 * This way, even if somebody were to read it concurrently,
	 * the data should always be valid as a string.
	 */
	write(fd, buffer, n+1);
	ftruncate(fd, n);

	/* Over time limit? */
	if (cur > max) {
		int pid = fork();
		if (!pid) {
			if (!setuid(user->uid))
				kill(-1, SIGTERM);
			exit(1);
		}
		/*
		 * Yeah, we set CSIGCHLD to SIG_IGN, so this should
		 * never even succeed reliably anyway, but at least
		 * we'll wait for the child to exit, and not have
		 * potentially lots of children outstanding.
		 */
		if (pid > 0) {
			int status;
			waitpid(pid, &status, 0);
		}
	}
}

static void update_times(struct user *user, struct timeval *now)
{
	static char filename[PATH_MAX];
	int fd;

	snprintf(filename, sizeof(filename), TRACKER_FILE, user->login);
	fd = open(filename, O_RDWR);
	if (fd >= 0) {
		unsigned int s = now->tv_sec - user->last;
		user->last = now->tv_sec;
		update_fd(user, fd, s);
		close(fd);
	}
}

static void report(struct user *user, int login)
{
	fprintf(stderr, "User %d logged %s.\n", user->uid, login < 0 ? "out" : login ? "in" : "on");
}

static void process_users(void)
{
	struct user *p, **pp = &userlist;
	struct timeval tv;

	gettimeofday(&tv, NULL);
	while ((p = *pp) != NULL) {
		switch (p->state) {
		case no_user:
			report(p, -1);
			*pp = p->next;
			free(p->gecos);
			free(p->login);
			free(p);
			continue;

		case old_user:
			update_times(p, &tv);
			break;

		case new_user:
			p->last = tv.tv_sec;
			report(p, 1);
			break;
		}
		p->state = no_user;
		pp = &p->next;
	}
}

static void add_usage(unsigned int uid)
{
	struct user *p = userlist;
	struct passwd *pw;

	while (p) {
		if (p->uid == uid) {
			if (p->state == no_user)
				p->state = old_user;
			return;
		}
		p = p->next;
	}
	pw = getpwuid(uid);
	if (!pw)
		return;
	p = malloc(sizeof(*p));
	memset(p, 0, sizeof(*p));
	p->state = new_user;
	p->uid = uid;
	p->login = strdup(pw->pw_name);
	p->gecos = strdup(pw->pw_gecos);

	p->next = userlist;
	userlist = p;
}

static void check(const char *buf)
{
	int i, nr;
	unsigned src, srcp, dst, dstp, state, dummy, uid;

	nr = sscanf(buf, "%d: %x:%x %x:%x %x %x:%x %x:%x %x %d",
		&i, &src, &srcp, &dst, &dstp,
		&state, &dummy, &dummy, &dummy, &dummy, &dummy,
		&uid);
	if (nr != 12)
		return;
	if (uid < 256)	/* Igore system users (root in particular) */
		return;
	if (state != 1)	/* TCP_ESTABLISHED */
		return;
	if (src == dst)	/* Ignore local addresses */
		return;
	if (!dst || (dst & 255) == 127)	/* Ignore loopback */
		return;
	add_usage(uid);
}

int main(int argc, char **argv)
{
	daemon(0,0);
	signal(SIGCHLD, SIG_IGN);
	for (;;) {
		static char buffer[256];
		FILE *f;

		f = fopen("/proc/net/tcp", "r");
		while (fgets(buffer, sizeof(buffer), f))
			check(buffer);
		fclose(f);
		process_users();
		sleep(5);
	}
}
