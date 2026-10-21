/* Libft tester v2 - not part of the submission.
** One binary per function, built with ASan + UBSan + LeakSanitizer.
** Run from the repository root:  bash tests/run.sh [ft_name ...]
**
** Protocol with run.sh: failures are printed as "  ko| <message>" and the
** last line is "@@ <checks> <failures>". Sanitizer reports go to a log file.
*/
#include <ctype.h>
#include <fcntl.h>
#include <limits.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "libft.h"

#define MAX_SHOWN 8
#define COUNT(a) ((int)(sizeof(a) / sizeof((a)[0])))

/* ------------------------------ framework ------------------------------ */

static int	g_checks;
static int	g_fails;

static void	check(int ok, const char *fmt, ...)
	__attribute__((format(printf, 2, 3)));

static void	check(int ok, const char *fmt, ...)
{
	va_list	ap;

	g_checks++;
	if (ok)
		return ;
	g_fails++;
	if (g_fails > MAX_SHOWN)
		return ;
	printf("  ko| ");
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
	printf("\n");
}

/* Escaped, truncated rendering of n bytes (rotating buffers). */
static const char	*show(const void *p, size_t n)
{
	static char			buf[8][220];
	static int			w;
	const unsigned char	*s;
	char				*o;
	size_t				i;
	size_t				k;

	o = buf[w++ & 7];
	if (p == NULL)
		return (strcpy(o, "(null)"));
	s = p;
	k = 0;
	for (i = 0; i < n && k < 180; i++)
	{
		if (s[i] == '\n')
			k += sprintf(o + k, "\\n");
		else if (s[i] == '\t')
			k += sprintf(o + k, "\\t");
		else if (s[i] == '\\')
			k += sprintf(o + k, "\\\\");
		else if (s[i] < 32 || s[i] > 126)
			k += sprintf(o + k, "\\x%02x", s[i]);
		else
			o[k++] = s[i];
	}
	if (i < n)
		k += sprintf(o + k, "...");
	o[k] = '\0';
	return (o);
}
#define S(x) show((x), (x) ? strlen(x) : 0)

static int	sgn(long x)
{
	return ((x > 0) - (x < 0));
}

/* Allocation used by the tester itself: never fails, never counted. */
void		*__real_malloc(size_t n);

static void	*xalloc(size_t n)
{
	return (__real_malloc(n ? n : 1));
}

/* Exact-size copies: any read or write past the end is caught by ASan. */
static char	*xdup(const char *s)
{
	size_t	n;
	char	*p;

	n = strlen(s) + 1;
	p = __real_malloc(n);
	memcpy(p, s, n);
	return (p);
}

static void	*xbytes(const void *s, size_t n)
{
	void	*p;

	p = __real_malloc(n);
	memcpy(p, s, n);
	return (p);
}

/* malloc fault injection: only the libft code goes through __wrap_malloc. */
static int	g_inj_on;
static int	g_inj_cnt;
static int	g_inj_at;

void	*__wrap_malloc(size_t n)
{
	if (g_inj_on && ++g_inj_cnt == g_inj_at)
		return (NULL);
	return (__real_malloc(n));
}

static void	inject_on(int k)
{
	g_inj_cnt = 0;
	g_inj_at = k;
	g_inj_on = 1;
}

static int	inject_off(void)
{
	g_inj_on = 0;
	return (g_inj_cnt >= g_inj_at);
}

/* ------------------------------ ctype ---------------------------------- */

static void	ctype_bool(const char *n, int (*mine)(int), int (*real)(int))
{
	int	c;
	int	a;
	int	b;

	for (c = -1; c <= 255; c++)
	{
		a = mine(c);
		b = (real(c) != 0);
		check(a == b, "%s(%d) = %d, expected exactly %d", n, c, a, b);
	}
}

static void	ctype_conv(const char *n, int (*mine)(int), int (*real)(int))
{
	int	c;

	for (c = -1; c <= 255; c++)
		check(mine(c) == real(c), "%s(%d) = %d, libc says %d", n, c,
			mine(c), real(c));
}

#define REAL(f) static int r_##f(int c) { return (f(c)); }
REAL(isalpha) REAL(isdigit) REAL(isalnum) REAL(isascii) REAL(isprint)
REAL(toupper) REAL(tolower)

#ifdef T_isalpha
static void	run_test(void) { ctype_bool("ft_isalpha", ft_isalpha, r_isalpha); }
#endif
#ifdef T_isdigit
static void	run_test(void) { ctype_bool("ft_isdigit", ft_isdigit, r_isdigit); }
#endif
#ifdef T_isalnum
static void	run_test(void) { ctype_bool("ft_isalnum", ft_isalnum, r_isalnum); }
#endif
#ifdef T_isascii
static void	run_test(void) { ctype_bool("ft_isascii", ft_isascii, r_isascii); }
#endif
#ifdef T_isprint
static void	run_test(void) { ctype_bool("ft_isprint", ft_isprint, r_isprint); }
#endif
#ifdef T_toupper
static void	run_test(void) { ctype_conv("ft_toupper", ft_toupper, r_toupper); }
#endif
#ifdef T_tolower
static void	run_test(void) { ctype_conv("ft_tolower", ft_tolower, r_tolower); }
#endif

/* ------------------------------ strlen --------------------------------- */

#ifdef T_strlen
static void	run_test(void)
{
	static const char	*tab[] = {"", "a", "ab", "hello world", "  42\n\t",
		"\xff\xfe\x80", "tab\there", "ab\0cd"};
	char				*p;
	size_t				i;
	int					k;

	for (k = 0; k < COUNT(tab); k++)
	{
		p = xdup(tab[k]);
		check(ft_strlen(p) == strlen(p), "strlen(\"%s\") = %zu, expected %zu",
			show(p, strlen(p)), ft_strlen(p), strlen(p));
		free(p);
	}
	for (i = 0; i < 70; i++)
	{
		p = xalloc(i + 1);
		memset(p, 'x', i);
		p[i] = '\0';
		check(ft_strlen(p) == i, "strlen of %zu x's = %zu", i, ft_strlen(p));
		free(p);
	}
	p = xalloc((1 << 20) + 1);
	memset(p, 'y', 1 << 20);
	p[1 << 20] = '\0';
	check(ft_strlen(p) == (1 << 20), "strlen of 1 MiB string = %zu",
		ft_strlen(p));
	free(p);
}
#endif

/* ------------------------- strchr / strrchr ---------------------------- */

#if defined(T_strchr) || defined(T_strrchr)
static char	*r_strchr(const char *s, int c) { return (strchr(s, c)); }
static char	*r_strrchr(const char *s, int c) { return (strrchr(s, c)); }

static void	chr_loop(const char *n, char *(*mine)(const char *, int),
		char *(*real)(const char *, int))
{
	static const char	*tab[] = {"", "a", "aaa", "hello", "hello world",
		"42 Paris\n", "abcabcabc", "\xff\xfe\x80", "x\x7f", "a b  c"};
	char				*p;
	char				*a;
	char				*b;
	int					k;
	int					c;

	for (k = 0; k < COUNT(tab); k++)
	{
		p = xdup(tab[k]);
		for (c = -300; c <= 600; c++)
		{
			a = mine(p, c);
			b = real(p, c);
			check(a == b, "%s(\"%s\", %d) -> %s, libc -> %s", n, S(p), c,
				a ? show(a, strlen(a)) : "NULL", b ? show(b, strlen(b)) : "NULL");
		}
		free(p);
	}
}
#endif
#ifdef T_strchr
static void	run_test(void) { chr_loop("ft_strchr", ft_strchr, r_strchr); }
#endif
#ifdef T_strrchr
static void	run_test(void) { chr_loop("ft_strrchr", ft_strrchr, r_strrchr); }
#endif

/* ------------------------------ strncmp -------------------------------- */

#ifdef T_strncmp
static void	run_test(void)
{
	static const char	*tab[] = {"", "a", "abc", "abd", "abcd", "ABC", "\xff",
		"\x80", "a\xff", "test\x80", "test", "Hello", "hello"};
	static const size_t	ns[] = {0, 1, 2, 3, 4, 5, 6, 7, 100, SIZE_MAX};
	char				*x;
	char				*y;
	int					i;
	int					j;
	int					k;

	for (i = 0; i < COUNT(tab); i++)
		for (j = 0; j < COUNT(tab); j++)
		{
			x = xdup(tab[i]);
			y = xdup(tab[j]);
			for (k = 0; k < COUNT(ns); k++)
				check(sgn(ft_strncmp(x, y, ns[k])) == sgn(strncmp(x, y, ns[k])),
					"strncmp(\"%s\", \"%s\", %zu) = %d, libc %d (sign must match, bytes are unsigned)",
					S(x), S(y), ns[k], ft_strncmp(x, y, ns[k]),
					strncmp(x, y, ns[k]));
			free(x);
			free(y);
		}
}
#endif

/* ------------------------------ memset / bzero ------------------------- */

#ifdef T_memset
static void	run_test(void)
{
	static const int	vals[] = {0, 'A', 127, 128, 255, 256 + 66, -1, INT_MAX,
		INT_MIN};
	unsigned char		a[40];
	unsigned char		b[40];
	unsigned char		*p;
	void				*ret;
	size_t				off;
	size_t				n;
	int					v;

	for (v = 0; v < COUNT(vals); v++)
	{
		for (off = 0; off < 6; off += 5)
			for (n = 0; n + off <= 40; n++)
			{
				memset(a, '#', 40);
				memset(b, '#', 40);
				ret = ft_memset(a + off, vals[v], n);
				memset(b + off, vals[v], n);
				check(ret == a + off, "memset c=%d n=%zu: return is not dest",
					vals[v], n);
				check(memcmp(a, b, 40) == 0,
					"memset(buf+%zu, %d, %zu): wrong bytes", off, vals[v], n);
			}
		for (n = 0; n <= 33; n++)
		{
			p = xalloc(n);
			ret = ft_memset(p, vals[v], n);
			check(ret == p, "memset exact buffer n=%zu: return is not dest", n);
			free(p);
		}
	}
}
#endif

#ifdef T_bzero
static void	run_test(void)
{
	unsigned char	a[40];
	unsigned char	*p;
	size_t			off;
	size_t			n;
	size_t			i;
	int				bad;

	for (off = 0; off < 6; off += 5)
		for (n = 0; n + off <= 40; n++)
		{
			memset(a, '#', 40);
			ft_bzero(a + off, n);
			bad = 0;
			for (i = 0; i < 40; i++)
				if ((i >= off && i < off + n && a[i] != 0)
					|| ((i < off || i >= off + n) && a[i] != '#'))
					bad = 1;
			check(!bad, "bzero(buf+%zu, %zu): not zeroed or wrote outside",
				off, n);
		}
	for (n = 0; n <= 33; n++)
	{
		p = xalloc(n);
		memset(p, 0x55, n);
		ft_bzero(p, n);
		for (i = 0; i < n; i++)
			if (p[i] != 0)
				break ;
		check(i == n, "bzero exact buffer n=%zu: byte %zu not zero", n, i);
		free(p);
	}
}
#endif

/* ------------------------------ memcpy --------------------------------- */

#ifdef T_memcpy
static void	run_test(void)
{
	unsigned char	pat[64];
	unsigned char	big[100];
	unsigned char	*src;
	unsigned char	*dst;
	void			*ret;
	size_t			n;
	size_t			i;

	for (i = 0; i < 64; i++)
		pat[i] = (unsigned char)(i * 7 + (i % 5 == 0 ? 0 : 3));
	for (n = 0; n <= 64; n++)
	{
		src = xbytes(pat, n);
		dst = xalloc(n);
		memset(dst, 0x55, n);
		ret = ft_memcpy(dst, src, n);
		check(ret == dst, "memcpy n=%zu: return is not dest", n);
		check(memcmp(dst, pat, n) == 0,
			"memcpy n=%zu: wrong bytes (does it stop at a '\\0'?)", n);
		free(src);
		free(dst);
		memset(big, 0x55, 100);
		src = xbytes(pat, n);
		ft_memcpy(big + 16, src, n);
		check(memcmp(big + 16, pat, n) == 0, "memcpy into the middle, n=%zu", n);
		for (i = 0; i < 100; i++)
			if ((i < 16 || i >= 16 + n) && big[i] != 0x55)
				break ;
		check(i == 100, "memcpy n=%zu: touched byte %zu outside the zone", n, i);
		free(src);
	}
}
#endif

/* ------------------------------ memmove -------------------------------- */

#ifdef T_memmove
static void	run_test(void)
{
	static const size_t	ns[] = {0, 1, 2, 3, 7, 8, 16, 17, 24};
	unsigned char		a[40];
	unsigned char		b[40];
	unsigned char		*s;
	unsigned char		*d;
	void				*ret;
	size_t				i;
	size_t				si;
	size_t				di;
	int					k;

	for (si = 0; si < 16; si++)
		for (di = 0; di < 16; di++)
			for (k = 0; k < COUNT(ns); k++)
			{
				if (si + ns[k] > 40 || di + ns[k] > 40)
					continue ;
				for (i = 0; i < 40; i++)
					a[i] = b[i] = (unsigned char)(i + 1);
				ret = ft_memmove(a + di, a + si, ns[k]);
				memmove(b + di, b + si, ns[k]);
				check(ret == a + di, "memmove dst+%zu src+%zu n=%zu: return",
					di, si, ns[k]);
				check(memcmp(a, b, 40) == 0,
					"memmove dst+%zu src+%zu n=%zu: wrong bytes (overlap direction)",
					di, si, ns[k]);
			}
	for (i = 0; i <= 40; i++)
	{
		for (si = 0; si < 40; si++)
			a[si] = (unsigned char)(si * 3);
		s = xbytes(a, i);
		d = xalloc(i);
		ft_memmove(d, s, i);
		check(memcmp(d, a, i) == 0, "memmove between exact buffers n=%zu", i);
		free(s);
		free(d);
	}
}
#endif

/* ------------------------------ memchr / memcmp ------------------------ */

#ifdef T_memchr
static void	run_test(void)
{
	static const unsigned char	data[20] = {'h', 'e', 'l', 'l', 'o', 0, 'w',
		'o', 'r', 0xff, 'l', 'd', 0x80, '!', 'a', 'b', 'c', 'a', 0x7f, 0};
	unsigned char				*p;
	size_t						n;
	int							c;

	for (n = 0; n <= 20; n++)
	{
		p = xbytes(data, n);
		for (c = -300; c <= 600; c++)
			check(ft_memchr(p, c, n) == memchr(p, c, n),
				"memchr(buf, %d, %zu) = %p, libc %p", c, n, ft_memchr(p, c, n),
				memchr(p, c, n));
		free(p);
	}
}
#endif

#ifdef T_memcmp
static void	run_test(void)
{
	static const unsigned char	a[][9] = {"abcdefgh", "abcdefgh", "abc\0defg",
		"\xff" "bcdefgh", "a\200cdefgh", "abcdefgh", "\0\0\0\0\0\0\0\1",
		"abcdefgh", "\x7f" "bcdefgh", "\x01\x02\x03\x04\x05\x06\x07\x08"};
	static const unsigned char	b[][9] = {"abcdefgh", "abcdefgX", "abc\0dezg",
		"\x01" "bcdefgh", "a\0cdefgh", "Abcdefgh", "\0\0\0\0\0\0\0\2",
		"abcdefg", "\x80" "bcdefgh", "\x01\x02\x03\x04\x05\x06\x07\xff"};
	unsigned char				*x;
	unsigned char				*y;
	size_t						n;
	int							i;

	for (i = 0; i < COUNT(a); i++)
		for (n = 0; n <= 8; n++)
		{
			x = xbytes(a[i], n);
			y = xbytes(b[i], n);
			check(sgn(ft_memcmp(x, y, n)) == sgn(memcmp(x, y, n)),
				"memcmp pair %d n=%zu = %d, libc %d (sign must match, bytes are unsigned)",
				i, n, ft_memcmp(x, y, n), memcmp(x, y, n));
			free(x);
			free(y);
		}
}
#endif

/* ------------------------------ strlcpy / strlcat ---------------------- */

#if defined(T_strlcpy) || defined(T_strlcat)
static size_t	ref_strlcpy(char *d, const char *s, size_t size)
{
	size_t	l;
	size_t	c;

	l = strlen(s);
	if (size)
	{
		c = (l >= size) ? size - 1 : l;
		memcpy(d, s, c);
		d[c] = '\0';
	}
	return (l);
}

static size_t	ref_strlcat(char *d, const char *s, size_t size)
{
	size_t	dl;
	size_t	sl;
	size_t	c;

	dl = 0;
	while (dl < size && d[dl])
		dl++;
	sl = strlen(s);
	if (dl == size)
		return (size + sl);
	c = (sl > size - dl - 1) ? size - dl - 1 : sl;
	memcpy(d + dl, s, c);
	d[dl + c] = '\0';
	return (dl + sl);
}
#endif

#ifdef T_strlcpy
static void	run_test(void)
{
	static const char	*srcs[] = {"", "a", "ab", "hello", "abcdefg", "\xff\x80z"};
	char				*s;
	char				*d;
	char				*r;
	size_t				size;
	size_t				a;
	size_t				b;
	int					k;

	for (k = 0; k < COUNT(srcs); k++)
		for (size = 0; size <= 10; size++)
		{
			s = xdup(srcs[k]);
			d = xalloc(size);
			r = xalloc(size);
			memset(d, 'X', size);
			memset(r, 'X', size);
			a = ft_strlcpy(d, s, size);
			b = ref_strlcpy(r, s, size);
			check(a == b, "strlcpy(dst, \"%s\", %zu) returned %zu, expected %zu",
				S(s), size, a, b);
			check(memcmp(d, r, size) == 0,
				"strlcpy(dst, \"%s\", %zu): dst=\"%s\" expected \"%s\"", S(s),
				size, show(d, size), show(r, size));
			free(s);
			free(d);
			free(r);
		}
	s = xalloc(5001);
	memset(s, 'q', 5000);
	s[5000] = '\0';
	d = xalloc(5001);
	check(ft_strlcpy(d, s, 5001) == 5000 && strcmp(d, s) == 0,
		"strlcpy of a 5000 chars string");
	free(s);
	free(d);
}
#endif

#ifdef T_strlcat
static void	run_test(void)
{
	size_t	size;
	size_t	dl;
	size_t	sl;
	size_t	cap;
	size_t	a;
	size_t	b;
	char	*d;
	char	*r;
	char	*s;

	for (size = 0; size <= 10; size++)
		for (dl = 0; dl <= 12; dl++)
			for (sl = 0; sl <= 6; sl++)
			{
				cap = (dl >= size ? dl : size) + 1;
				d = xalloc(cap);
				r = xalloc(cap);
				memset(d, 'X', cap);
				memset(d, 'D', dl);
				d[dl] = '\0';
				memcpy(r, d, cap);
				s = xalloc(sl + 1);
				memcpy(s, "abcdef", sl);
				s[sl] = '\0';
				a = ft_strlcat(d, s, size);
				b = ref_strlcat(r, s, size);
				check(a == b,
					"strlcat(dst[len %zu], \"%s\", %zu) returned %zu, expected %zu%s",
					dl, S(s), size, a, b,
					dl >= size ? " (dst longer than dstsize: return dstsize + len(src), write nothing)" : "");
				check(memcmp(d, r, cap) == 0,
					"strlcat(dst[len %zu], \"%s\", %zu): dst=\"%s\" expected \"%s\"",
					dl, S(s), size, show(d, cap), show(r, cap));
				free(d);
				free(r);
				free(s);
			}
}
#endif

/* ------------------------------ strnstr -------------------------------- */

#ifdef T_strnstr
static char	*ref_strnstr(const char *h, const char *n, size_t len)
{
	size_t	nl;
	size_t	i;

	nl = strlen(n);
	if (nl == 0)
		return ((char *)h);
	for (i = 0; h[i] && i + nl <= len; i++)
		if (strncmp(h + i, n, nl) == 0)
			return ((char *)h + i);
	return (NULL);
}

static void	run_test(void)
{
	static const char	*hs[] = {"", "a", "aaab", "abcabcd", "hello world",
		"lorem ipsum dolor sit amet", "mississippi"};
	static const char	*ns[] = {"", "a", "aa", "aab", "abcd", "ss", "issip",
		"ppi", "world", "xyz", "mississippi", "mississippix", "o w"};
	char				*h;
	char				*n;
	char				*a;
	char				*b;
	size_t				len;
	int					i;
	int					j;

	for (i = 0; i < COUNT(hs); i++)
		for (j = 0; j < COUNT(ns); j++)
		{
			h = xdup(hs[i]);
			n = xdup(ns[j]);
			for (len = 0; len <= strlen(h) + 2; len++)
			{
				a = ft_strnstr(h, n, len);
				b = ref_strnstr(h, n, len);
				check(a == b, "strnstr(\"%s\", \"%s\", %zu) -> offset %ld, expected %ld (-1 = NULL)",
					S(h), S(n), len, a ? (long)(a - h) : -1L,
					b ? (long)(b - h) : -1L);
			}
			a = ft_strnstr(h, n, SIZE_MAX);
			b = ref_strnstr(h, n, SIZE_MAX);
			check(a == b, "strnstr(\"%s\", \"%s\", SIZE_MAX) differs", S(h), S(n));
			free(h);
			free(n);
		}
}
#endif

/* ------------------------------ atoi ----------------------------------- */

#ifdef T_atoi
static void	run_test(void)
{
	static const char	*tab[] = {"0", "42", "-42", "+42", "   42",
		" \t\n\v\f\r42", "2147483647", "-2147483648", "42abc", "abc", "",
		"--42", "+-42", "-+42", "  -0012", "4 2", " + 42", "-", "+", "  -",
		"0000", "-0", "+0", "\t-2147483648xyz", "9", "-9", "0x1A", "1e5",
		"\v42", "\f-42", " 12 34", "00000000000000000042",
		"-00000000000000002147483648", "2147483647abc", "\xff" "42",
		"42\xff", " ---1", "1-1", "2147483646", "-2147483647"};
	char				*p;
	int					i;

	for (i = 0; i < COUNT(tab); i++)
	{
		p = xdup(tab[i]);
		check(ft_atoi(p) == atoi(p), "atoi(\"%s\") = %d, libc %d", S(p),
			ft_atoi(p), atoi(p));
		free(p);
	}
}
#endif

/* ------------------------------ calloc / strdup ------------------------ */

#ifdef T_calloc
static void	expect_null(void *p, const char *msg)
{
	check(p == NULL, "%s", msg);
	free(p);
}

static void	run_test(void)
{
	static const size_t	pairs[][2] = {{0, 0}, {0, 8}, {8, 0}, {1, 1}, {3, 7},
		{250, 4}, {1000, 1000}, {1, 65536}, {65536, 1}};
	unsigned char		*p;
	size_t				i;
	int					k;

	for (k = 0; k < COUNT(pairs); k++)
	{
		p = ft_calloc(pairs[k][0], pairs[k][1]);
		check(p != NULL, "calloc(%zu, %zu) returned NULL", pairs[k][0],
			pairs[k][1]);
		if (p == NULL)
			continue ;
		for (i = 0; i < pairs[k][0] * pairs[k][1]; i++)
			if (p[i] != 0)
				break ;
		check(i == pairs[k][0] * pairs[k][1],
			"calloc(%zu, %zu): byte %zu is not zero", pairs[k][0],
			pairs[k][1], i);
		memset(p, 0xAB, pairs[k][0] * pairs[k][1]);
		free(p);
	}
	expect_null(ft_calloc(SIZE_MAX, 2), "calloc(SIZE_MAX, 2) must be NULL");
	expect_null(ft_calloc(2, SIZE_MAX), "calloc(2, SIZE_MAX) must be NULL");
	expect_null(ft_calloc(SIZE_MAX, SIZE_MAX),
		"calloc(SIZE_MAX, SIZE_MAX) must be NULL");
	expect_null(ft_calloc(SIZE_MAX / 2 + 1, 2),
		"calloc(SIZE_MAX/2+1, 2) must be NULL (overflow wraps to 0)");
	expect_null(ft_calloc(2, SIZE_MAX / 2 + 1),
		"calloc(2, SIZE_MAX/2+1) must be NULL (overflow wraps to 0)");
	expect_null(ft_calloc((size_t)1 << 32, (size_t)1 << 32),
		"calloc(2^32, 2^32) must be NULL (overflow wraps to 0)");
	expect_null(ft_calloc(SIZE_MAX / 2, 3),
		"calloc(SIZE_MAX/2, 3) must be NULL (overflow)");
	inject_on(1);
	p = ft_calloc(4, 4);
	if (inject_off())
		check(p == NULL, "calloc must return NULL when malloc fails");
	free(p);
}
#endif

#ifdef T_strdup
static void	run_test(void)
{
	static const char	*tab[] = {"", "a", "hello", "hello world\n\t42",
		"\xff\x80\x7f"};
	char				*s;
	char				*d;
	char				*big;
	int					i;

	for (i = 0; i < COUNT(tab); i++)
	{
		s = xdup(tab[i]);
		d = ft_strdup(s);
		check(d != NULL, "strdup(\"%s\") returned NULL", S(s));
		if (d)
		{
			check(strcmp(d, s) == 0, "strdup(\"%s\") = \"%s\"", S(s),
				show(d, strlen(d)));
			check(d != s, "strdup(\"%s\") returned the same pointer", S(s));
			free(d);
		}
		free(s);
	}
	big = xalloc(100001);
	memset(big, 'z', 100000);
	big[100000] = '\0';
	d = ft_strdup(big);
	check(d && strcmp(d, big) == 0, "strdup of 100000 chars");
	free(d);
	free(big);
	s = xdup("fail me");
	inject_on(1);
	d = ft_strdup(s);
	if (inject_off())
		check(d == NULL, "strdup must return NULL when malloc fails");
	free(d);
	free(s);
}
#endif

/* ------------------------------ substr --------------------------------- */

#ifdef T_substr
static void	run_test(void)
{
	static const char		*tab[] = {"", "a", "hello", "hello world, 42!"};
	static const size_t		lens[] = {SIZE_MAX, SIZE_MAX - 1, 1000};
	static const unsigned	starts[] = {UINT_MAX, UINT_MAX - 1, 1000};
	char					*s;
	char					*r;
	char					exp[64];
	size_t					sl;
	size_t					st;
	size_t					ln;
	int						k;

	for (k = 0; k < COUNT(tab); k++)
	{
		s = xdup(tab[k]);
		sl = strlen(s);
		for (st = 0; st <= sl + 2; st++)
			for (ln = 0; ln <= sl + 2; ln++)
			{
				r = ft_substr(s, (unsigned int)st, ln);
				if (st >= sl)
					exp[0] = '\0';
				else
				{
					memcpy(exp, s + st, (ln > sl - st ? sl - st : ln));
					exp[(ln > sl - st ? sl - st : ln)] = '\0';
				}
				check(r != NULL && strcmp(r, exp) == 0,
					"substr(\"%s\", %zu, %zu) = %s, expected \"%s\"", S(s), st,
					ln, r ? show(r, strlen(r)) : "NULL", exp);
				free(r);
			}
		for (st = 0; st < 3; st++)
			for (ln = 0; ln < 3; ln++)
			{
				r = ft_substr(s, starts[st], lens[ln]);
				check(r != NULL && *r == '\0',
					"substr(\"%s\", %u, %zu) must be an empty string", S(s),
					starts[st], lens[ln]);
				free(r);
				r = ft_substr(s, 0, lens[ln]);
				check(r != NULL && strcmp(r, s) == 0,
					"substr(\"%s\", 0, %zu) must be the whole string", S(s),
					lens[ln]);
				free(r);
			}
		free(s);
	}
	s = xdup("fail me");
	inject_on(1);
	r = ft_substr(s, 1, 3);
	if (inject_off())
		check(r == NULL, "substr must return NULL when malloc fails");
	free(r);
	free(s);
}
#endif

/* ------------------------------ strjoin -------------------------------- */

#ifdef T_strjoin
static void	run_test(void)
{
	static const char	*tab[] = {"", "a", "hello", "world !", "\xff\x80"};
	char				*x;
	char				*y;
	char				*r;
	char				*e;
	char				*big;
	int					i;
	int					j;

	for (i = 0; i < COUNT(tab); i++)
		for (j = 0; j < COUNT(tab); j++)
		{
			x = xdup(tab[i]);
			y = xdup(tab[j]);
			r = ft_strjoin(x, y);
			e = xalloc(strlen(x) + strlen(y) + 1);
			strcpy(e, x);
			strcat(e, y);
			check(r != NULL && strcmp(r, e) == 0,
				"strjoin(\"%s\", \"%s\") = %s, expected \"%s\"", S(x), S(y),
				r ? show(r, strlen(r)) : "NULL", show(e, strlen(e)));
			check(r != x && r != y, "strjoin returned one of its arguments");
			check(strcmp(x, tab[i]) == 0 && strcmp(y, tab[j]) == 0,
				"strjoin modified an argument");
			free(e);
			free(r);
			free(x);
			free(y);
		}
	big = xalloc(50001);
	memset(big, 'b', 50000);
	big[50000] = '\0';
	r = ft_strjoin(big, big);
	check(r && strlen(r) == 100000 && r[99999] == 'b', "strjoin of 2 x 50000");
	free(r);
	free(big);
	x = xdup("left");
	y = xdup("right");
	for (i = 1; i < 5; i++)
	{
		inject_on(i);
		r = ft_strjoin(x, y);
		if (inject_off())
			check(r == NULL, "strjoin must return NULL when malloc fails");
		free(r);
	}
	free(x);
	free(y);
}
#endif

/* ------------------------------ strtrim -------------------------------- */

#ifdef T_strtrim
static char	*ref_trim(const char *s, const char *set)
{
	size_t	a;
	size_t	b;
	char	*r;

	a = 0;
	while (s[a] && strchr(set, s[a]) && s[a])
		a++;
	b = strlen(s);
	while (b > a && strchr(set, s[b - 1]))
		b--;
	r = xalloc(b - a + 1);
	memcpy(r, s + a, b - a);
	r[b - a] = '\0';
	return (r);
}

static void	run_test(void)
{
	static const char	*sets[] = {"", " ", "a", "ab", "ba ", "abab", "xyz"};
	static const char	*hand[][3] = {{"  abc  ", " ", "abc"},
	{"\t\n  hello \n\t", " \n\t", "hello"}, {"abcba", "ab", "c"},
	{"xxhixx", "x", "hi"}, {"hello", "xyz", "hello"}, {"xxxx", "x", ""},
	{"", "x", ""}, {"a", "a", ""}, {"hello", "", "hello"},
	{"   a   b   ", " ", "a   b"}, {"1234abc4321", "4321", "abc"}};
	static const char	alpha[] = "ab ";
	char				*s;
	char				*t;
	char				*r;
	char				*e;
	int					i;
	int					len;
	int					n;
	int					m;
	int					cnt;

	for (i = 0; i < COUNT(hand); i++)
	{
		s = xdup(hand[i][0]);
		t = xdup(hand[i][1]);
		r = ft_strtrim(s, t);
		check(r != NULL && strcmp(r, hand[i][2]) == 0,
			"strtrim(\"%s\", \"%s\") = %s, expected \"%s\"", S(s), S(t),
			r ? show(r, strlen(r)) : "NULL", hand[i][2]);
		free(r);
		free(s);
		free(t);
	}
	for (len = 0; len <= 5; len++)
	{
		cnt = 1;
		for (i = 0; i < len; i++)
			cnt *= 3;
		for (n = 0; n < cnt; n++)
		{
			s = xalloc(len + 1);
			for (i = 0, m = n; i < len; i++, m /= 3)
				s[i] = alpha[m % 3];
			s[len] = '\0';
			for (i = 0; i < COUNT(sets); i++)
			{
				t = xdup(sets[i]);
				r = ft_strtrim(s, t);
				e = ref_trim(s, t);
				check(r != NULL && strcmp(r, e) == 0,
					"strtrim(\"%s\", \"%s\") = %s, expected \"%s\"", S(s), S(t),
					r ? show(r, strlen(r)) : "NULL", e);
				free(r);
				free(e);
				free(t);
			}
			free(s);
		}
	}
	s = xdup("  fail me  ");
	t = xdup(" ");
	for (i = 1; i < 4; i++)
	{
		inject_on(i);
		r = ft_strtrim(s, t);
		if (inject_off())
			check(r == NULL, "strtrim must return NULL when malloc fails");
		free(r);
	}
	free(s);
	free(t);
}
#endif

/* ------------------------------ split ---------------------------------- */

#ifdef T_split
typedef struct s_scase
{
	const char	*s;
	char		c;
	const char	*exp[16];
}	t_scase;

static void	free_split(char **r, int max)
{
	int	i;

	for (i = 0; i < max && r[i]; i++)
		free(r[i]);
	free(r);
}

static void	run_test(void)
{
	static const t_scase	cases[] = {
	{"hello world", ' ', {"hello", "world"}},
	{"  hello   world  ", ' ', {"hello", "world"}},
	{"abc", ' ', {"abc"}}, {"", ' ', {NULL}}, {"     ", ' ', {NULL}},
	{"a", 'a', {NULL}}, {"aaa", 'a', {NULL}}, {"a", ' ', {"a"}},
	{"a b c d e", ' ', {"a", "b", "c", "d", "e"}},
	{"  a", ' ', {"a"}}, {"a  ", ' ', {"a"}}, {" a ", ' ', {"a"}},
	{"a,b,,c,", ',', {"a", "b", "c"}}, {",,,", ',', {NULL}},
	{"abc", '\0', {"abc"}}, {"", '\0', {NULL}},
	{"tab\tsep\tok", '\t', {"tab", "sep", "ok"}},
	{"line1\nline2\n\nline3", '\n', {"line1", "line2", "line3"}},
	{"xAxxBxxxCxxxxD", 'x', {"A", "B", "C", "D"}},
	{"a\xff" "b", '\xff', {"a", "b"}},
	{"lorem ipsum dolor sit amet, consectetur adipiscing elit. Sed non risus.",
		' ', {"lorem", "ipsum", "dolor", "sit", "amet,", "consectetur",
			"adipiscing", "elit.", "Sed", "non", "risus."}}};
	char					**r;
	char					*in;
	char					*big;
	int						k;
	int						i;
	int						ne;

	for (k = 0; k < COUNT(cases); k++)
	{
		in = xdup(cases[k].s);
		r = ft_split(in, cases[k].c);
		ne = 0;
		while (cases[k].exp[ne])
			ne++;
		check(r != NULL, "split(\"%s\", '%c') returned NULL", S(in),
			cases[k].c >= 32 ? cases[k].c : '?');
		if (r)
		{
			for (i = 0; i < ne; i++)
			{
				if (r[i] == NULL)
				{
					check(0, "split(\"%s\", '%c'): only %d words, expected %d",
						S(in), cases[k].c >= 32 ? cases[k].c : '?', i, ne);
					break ;
				}
				check(strcmp(r[i], cases[k].exp[i]) == 0,
					"split(\"%s\", '%c')[%d] = \"%s\", expected \"%s\"", S(in),
					cases[k].c >= 32 ? cases[k].c : '?', i,
					show(r[i], strlen(r[i])), cases[k].exp[i]);
			}
			if (i == ne)
				check(r[ne] == NULL,
					"split(\"%s\", '%c'): array not NULL-terminated after %d words",
					S(in), cases[k].c >= 32 ? cases[k].c : '?', ne);
			free_split(r, i == ne ? ne : i);
		}
		free(in);
	}
	big = xalloc(2001);
	for (i = 0; i < 1000; i++)
	{
		big[2 * i] = 'w';
		big[2 * i + 1] = ' ';
	}
	big[2000] = '\0';
	r = ft_split(big, ' ');
	for (i = 0; r && i < 1000 && r[i]; i++)
		;
	check(r && i == 1000 && r[1000] == NULL, "split of 1000 one-letter words");
	if (r)
		free_split(r, i);
	free(big);
	in = xdup("ab cd ef gh");
	for (k = 1; k < 12; k++)
	{
		inject_on(k);
		r = ft_split(in, ' ');
		if (inject_off())
			check(r == NULL,
				"split must return NULL when malloc #%d fails (and free what it already allocated)",
				k);
		else
			free_split(r, 4);
	}
	free(in);
}
#endif

/* ------------------------------ itoa ----------------------------------- */

#ifdef T_itoa
static void	itoa_one(int n)
{
	char	exp[32];
	char	*r;

	snprintf(exp, sizeof(exp), "%d", n);
	r = ft_itoa(n);
	check(r != NULL && strcmp(r, exp) == 0, "itoa(%d) = %s, expected \"%s\"",
		n, r ? show(r, strlen(r)) : "NULL", exp);
	free(r);
}

static void	run_test(void)
{
	unsigned int	seed;
	long			p;
	int				n;
	int				i;
	char			*r;

	for (n = -1100; n <= 1100; n++)
		itoa_one(n);
	itoa_one(INT_MIN);
	itoa_one(INT_MIN + 1);
	itoa_one(INT_MAX);
	itoa_one(INT_MAX - 1);
	for (p = 1; p <= 1000000000; p *= 10)
	{
		itoa_one((int)p);
		itoa_one((int)p - 1);
		itoa_one((int)p + 1);
		itoa_one(-(int)p);
		itoa_one(-(int)p - 1);
		itoa_one(-(int)p + 1);
	}
	seed = 12345;
	for (i = 0; i < 20000; i++)
	{
		seed = seed * 1103515245u + 12345u;
		itoa_one((int)seed);
	}
	for (i = 1; i < 4; i++)
	{
		inject_on(i);
		r = ft_itoa(-42);
		if (inject_off())
			check(r == NULL, "itoa must return NULL when malloc fails");
		free(r);
	}
}
#endif

/* ------------------------------ strmapi / striteri --------------------- */

#if defined(T_strmapi) || defined(T_striteri)
static unsigned int	g_calls;
static unsigned int	g_order_ok;

static void	order_reset(void)
{
	g_calls = 0;
	g_order_ok = 1;
}

static void	order_note(unsigned int i)
{
	if (i != g_calls)
		g_order_ok = 0;
	g_calls++;
}
#endif

#ifdef T_strmapi
static char	f_add(unsigned int i, char c)
{
	order_note(i);
	return ((char)(c + i));
}

static char	f_upper(unsigned int i, char c)
{
	order_note(i);
	return ((char)toupper((unsigned char)c));
}

static char	f_zero_at_1(unsigned int i, char c)
{
	order_note(i);
	return (i == 1 ? '\0' : c);
}

static void	run_test(void)
{
	char	*s;
	char	*r;
	char	*big;
	size_t	i;

	s = xdup("abc");
	order_reset();
	r = ft_strmapi(s, f_add);
	check(r != NULL && strcmp(r, "ace") == 0, "strmapi(\"abc\", c+i) = %s",
		r ? show(r, strlen(r)) : "NULL");
	check(g_calls == 3 && g_order_ok, "f must be called 3 times with i=0,1,2 in order (called %u)",
		g_calls);
	check(strcmp(s, "abc") == 0, "strmapi modified the source string");
	free(r);
	free(s);
	s = xdup("");
	order_reset();
	r = ft_strmapi(s, f_add);
	check(r != NULL && *r == '\0' && g_calls == 0,
		"strmapi on \"\" must return \"\" without calling f");
	free(r);
	free(s);
	s = xdup("Hello, World 42\xff");
	order_reset();
	r = ft_strmapi(s, f_upper);
	check(r != NULL && strcmp(r, "HELLO, WORLD 42\xff") == 0,
		"strmapi(toupper) = %s", r ? show(r, strlen(r)) : "NULL");
	free(r);
	free(s);
	s = xdup("abcd");
	order_reset();
	r = ft_strmapi(s, f_zero_at_1);
	check(r != NULL && memcmp(r, "a\0cd\0", 5) == 0 && g_calls == 4,
		"strmapi must call f for every index even if f returns '\\0' (calls=%u)",
		g_calls);
	free(r);
	free(s);
	big = xalloc(5001);
	memset(big, 'a', 5000);
	big[5000] = '\0';
	order_reset();
	r = ft_strmapi(big, f_add);
	for (i = 0; r && i < 5000; i++)
		if (r[i] != (char)('a' + (unsigned int)i))
			break ;
	check(r && i == 5000 && r[5000] == '\0' && g_calls == 5000 && g_order_ok,
		"strmapi on 5000 chars (failed at index %zu)", i);
	free(r);
	free(big);
	s = xdup("fail me");
	inject_on(1);
	r = ft_strmapi(s, f_upper);
	if (inject_off())
		check(r == NULL, "strmapi must return NULL when malloc fails");
	free(r);
	free(s);
}
#endif

#ifdef T_striteri
static char	*g_base;
static int	g_ptr_ok;

static void	f_add(unsigned int i, char *c)
{
	order_note(i);
	if (c != g_base + i)
		g_ptr_ok = 0;
	*c = (char)(*c + i);
}

static void	f_upper(unsigned int i, char *c)
{
	order_note(i);
	*c = (char)toupper((unsigned char)*c);
}

static void	run_test(void)
{
	char	*s;
	char	*big;
	size_t	i;

	s = xdup("abc");
	g_base = s;
	g_ptr_ok = 1;
	order_reset();
	ft_striteri(s, f_add);
	check(strcmp(s, "ace") == 0, "striteri(\"abc\", c+i) = \"%s\"",
		show(s, strlen(s)));
	check(g_calls == 3 && g_order_ok, "f must be called 3 times with i=0,1,2 in order (called %u)",
		g_calls);
	check(g_ptr_ok, "f must receive the ADDRESS of s[i], not a copy");
	free(s);
	s = xdup("");
	order_reset();
	ft_striteri(s, f_upper);
	check(g_calls == 0, "striteri on \"\" must not call f (called %u)", g_calls);
	free(s);
	s = xdup("Hello, World 42");
	ft_striteri(s, f_upper);
	check(strcmp(s, "HELLO, WORLD 42") == 0, "striteri(toupper) = \"%s\"",
		show(s, strlen(s)));
	free(s);
	big = xalloc(5001);
	memset(big, 'a', 5000);
	big[5000] = '\0';
	g_base = big;
	g_ptr_ok = 1;
	order_reset();
	ft_striteri(big, f_add);
	for (i = 0; i < 5000; i++)
		if (big[i] != (char)('a' + (unsigned int)i))
			break ;
	check(i == 5000 && g_calls == 5000 && g_order_ok && g_ptr_ok,
		"striteri on 5000 chars (failed at index %zu)", i);
	free(big);
}
#endif

/* ------------------------------ put*_fd -------------------------------- */

#if defined(T_putchar_fd) || defined(T_putstr_fd) || defined(T_putendl_fd) \
	|| defined(T_putnbr_fd)
static char	g_cap_path[64];

static int	cap_start(void)
{
	int	fd;

	strcpy(g_cap_path, "/tmp/libft_tester_XXXXXX");
	fd = mkstemp(g_cap_path);
	return (fd);
}

/* Reads back what was written, closes and removes the file. */
static size_t	cap_end(int fd, char *buf, size_t max)
{
	ssize_t	r;

	lseek(fd, 0, SEEK_SET);
	r = read(fd, buf, max);
	close(fd);
	unlink(g_cap_path);
	return (r < 0 ? 0 : (size_t)r);
}
#endif

#ifdef T_putchar_fd
static void	run_test(void)
{
	static char	buf[512];
	size_t		got;
	int			fd;
	int			c;

	fd = cap_start();
	ft_putchar_fd('a', fd);
	got = cap_end(fd, buf, sizeof(buf));
	check(got == 1 && buf[0] == 'a', "putchar_fd('a'): wrote %zu bytes", got);
	fd = cap_start();
	ft_putchar_fd('\n', fd);
	got = cap_end(fd, buf, sizeof(buf));
	check(got == 1 && buf[0] == '\n', "putchar_fd('\\n'): wrote %zu bytes", got);
	fd = cap_start();
	ft_putchar_fd('\0', fd);
	got = cap_end(fd, buf, sizeof(buf));
	check(got == 1 && buf[0] == '\0', "putchar_fd('\\0') must write 1 byte (wrote %zu)",
		got);
	fd = cap_start();
	for (c = 0; c < 256; c++)
		ft_putchar_fd((char)c, fd);
	got = cap_end(fd, buf, sizeof(buf));
	for (c = 0; c < 256 && got == 256; c++)
		if ((unsigned char)buf[c] != c)
			break ;
	check(got == 256 && c == 256, "putchar_fd of all 256 byte values (got %zu bytes, first bad %d)",
		got, c);
	ft_putchar_fd('x', -1);
	check(1, "survives an invalid fd");
}
#endif

#ifdef T_putstr_fd
static void	run_test(void)
{
	static const char	*tab[] = {"", "a", "hello", "hello\n", "tab\there",
		"\xff\x80 high", "ab\0cd"};
	static char			buf[200100];
	char				*big;
	size_t				got;
	int					fd;
	int					i;

	for (i = 0; i < COUNT(tab); i++)
	{
		fd = cap_start();
		ft_putstr_fd((char *)tab[i], fd);
		got = cap_end(fd, buf, sizeof(buf));
		check(got == strlen(tab[i]) && memcmp(buf, tab[i], got) == 0,
			"putstr_fd(\"%s\"): wrote %zu bytes, expected %zu", show(tab[i],
				strlen(tab[i])), got, strlen(tab[i]));
	}
	big = xalloc(150001);
	memset(big, 'k', 150000);
	big[150000] = '\0';
	fd = cap_start();
	ft_putstr_fd(big, fd);
	got = cap_end(fd, buf, sizeof(buf));
	check(got == 150000 && memcmp(buf, big, got) == 0,
		"putstr_fd of 150000 chars wrote %zu bytes", got);
	free(big);
	ft_putstr_fd("x", -1);
}
#endif

#ifdef T_putendl_fd
static void	run_test(void)
{
	static const char	*tab[] = {"", "a", "hello", "hello\n", "\xff\x80 high"};
	static char			buf[256];
	char				exp[64];
	size_t				got;
	int					fd;
	int					i;

	for (i = 0; i < COUNT(tab); i++)
	{
		fd = cap_start();
		ft_putendl_fd((char *)tab[i], fd);
		got = cap_end(fd, buf, sizeof(buf));
		snprintf(exp, sizeof(exp), "%s\n", tab[i]);
		check(got == strlen(exp) && memcmp(buf, exp, got) == 0,
			"putendl_fd(\"%s\"): got \"%s\" (%zu bytes), expected \"%s\"",
			show(tab[i], strlen(tab[i])), show(buf, got), got,
			show(exp, strlen(exp)));
	}
	ft_putendl_fd("x", -1);
}
#endif

#ifdef T_putnbr_fd
static void	putnbr_one(int n)
{
	static char	buf[64];
	char		exp[32];
	size_t		got;
	int			fd;

	fd = cap_start();
	ft_putnbr_fd(n, fd);
	got = cap_end(fd, buf, sizeof(buf));
	snprintf(exp, sizeof(exp), "%d", n);
	check(got == strlen(exp) && memcmp(buf, exp, got) == 0,
		"putnbr_fd(%d): got \"%s\", expected \"%s\"", n, show(buf, got), exp);
}

static void	run_test(void)
{
	unsigned int	seed;
	int				n;
	int				i;

	for (n = -1100; n <= 1100; n++)
		putnbr_one(n);
	putnbr_one(INT_MIN);
	putnbr_one(INT_MIN + 1);
	putnbr_one(INT_MAX);
	putnbr_one(INT_MAX - 1);
	putnbr_one(1000000000);
	putnbr_one(-1000000000);
	seed = 777;
	for (i = 0; i < 2000; i++)
	{
		seed = seed * 1103515245u + 12345u;
		putnbr_one((int)seed);
	}
	ft_putnbr_fd(42, -1);
}
#endif

/* ------------------------------ linked list ---------------------------- */

#if defined(T_lstnew) || defined(T_lstadd_front) || defined(T_lstsize) \
	|| defined(T_lstlast) || defined(T_lstadd_back) || defined(T_lstdelone) \
	|| defined(T_lstclear) || defined(T_lstiter) || defined(T_lstmap)
static int	g_del_count;
static int	g_del_order[64];
static int	g_live;

static int	*mkint(int v)
{
	int	*p;

	p = __real_malloc(sizeof(int));
	*p = v;
	g_live++;
	return (p);
}

static void	del_int(void *p)
{
	if (g_del_count < 64)
		g_del_order[g_del_count] = *(int *)p;
	g_del_count++;
	g_live--;
	free(p);
}

static void	del_noop(void *p)
{
	(void)p;
	g_del_count++;
}

static t_list	*mknode(void *content)
{
	t_list	*n;

	n = __real_malloc(sizeof(t_list));
	n->content = content;
	n->next = NULL;
	return (n);
}

/* list holding ints 1..n built by hand (independent of the code under test) */
static t_list	*mklist(int n)
{
	t_list	*head;
	t_list	*cur;
	int		i;

	head = NULL;
	cur = NULL;
	for (i = 1; i <= n; i++)
	{
		if (!head)
			head = cur = mknode(mkint(i));
		else
		{
			cur->next = mknode(mkint(i));
			cur = cur->next;
		}
	}
	return (head);
}

static void	freelist(t_list *l, int free_content)
{
	t_list	*nx;

	while (l)
	{
		nx = l->next;
		if (free_content)
		{
			free(l->content);
			g_live--;
		}
		free(l);
		l = nx;
	}
}

static int	list_is(t_list *l, int n, int mul)
{
	int	i;

	for (i = 1; i <= n; i++)
	{
		if (l == NULL || l->content == NULL || *(int *)l->content != i * mul)
			return (0);
		l = l->next;
	}
	return (l == NULL);
}
#endif

#ifdef T_lstnew
static void	run_test(void)
{
	int		x;
	t_list	*n;
	int		i;

	x = 42;
	n = ft_lstnew(&x);
	check(n != NULL, "lstnew returned NULL");
	if (n)
	{
		check(n->content == &x, "lstnew: content must be the pointer given (no copy)");
		check(n->next == NULL, "lstnew: next must be NULL");
		free(n);
	}
	n = ft_lstnew(NULL);
	check(n != NULL && n->content == NULL && n->next == NULL,
		"lstnew(NULL) must give a valid node with content NULL");
	free(n);
	for (i = 1; i < 3; i++)
	{
		inject_on(i);
		n = ft_lstnew(&x);
		if (inject_off())
			check(n == NULL, "lstnew must return NULL when malloc fails");
		free(n);
	}
}
#endif

#ifdef T_lstadd_front
static void	run_test(void)
{
	t_list	*l;
	t_list	*a;
	t_list	*b;
	t_list	*c;

	l = NULL;
	a = mknode(NULL);
	ft_lstadd_front(&l, a);
	check(l == a && a->next == NULL, "lstadd_front on an empty list (*lst == NULL)");
	b = mknode(NULL);
	ft_lstadd_front(&l, b);
	check(l == b && b->next == a && a->next == NULL,
		"lstadd_front: new head must point to the old head");
	c = mknode(NULL);
	ft_lstadd_front(&l, c);
	check(l == c && c->next == b && b->next == a && a->next == NULL,
		"lstadd_front: chain of three");
	free(a);
	free(b);
	free(c);
}
#endif

#ifdef T_lstsize
static void	run_test(void)
{
	t_list	*l;
	int		n;

	check(ft_lstsize(NULL) == 0, "lstsize(NULL) must be 0 (got %d)",
		ft_lstsize(NULL));
	for (n = 1; n <= 50; n++)
	{
		l = mklist(n);
		check(ft_lstsize(l) == n, "lstsize of a %d nodes list = %d", n,
			ft_lstsize(l));
		freelist(l, 1);
	}
	l = mklist(10000);
	check(ft_lstsize(l) == 10000, "lstsize of 10000 nodes = %d", ft_lstsize(l));
	freelist(l, 1);
}
#endif

#ifdef T_lstlast
static void	run_test(void)
{
	t_list	*l;
	t_list	*t;
	int		n;

	check(ft_lstlast(NULL) == NULL, "lstlast(NULL) must be NULL");
	for (n = 1; n <= 20; n++)
	{
		l = mklist(n);
		for (t = l; t->next; t = t->next)
			;
		check(ft_lstlast(l) == t, "lstlast of %d nodes: wrong node", n);
		freelist(l, 1);
	}
}
#endif

#ifdef T_lstadd_back
static void	run_test(void)
{
	t_list	*l;
	t_list	*n;
	int		i;

	l = NULL;
	n = mknode(mkint(1));
	ft_lstadd_back(&l, n);
	check(l == n && n->next == NULL, "lstadd_back on an empty list (*lst == NULL)");
	for (i = 2; i <= 30; i++)
		ft_lstadd_back(&l, mknode(mkint(i)));
	check(list_is(l, 30, 1), "lstadd_back x30: order must be 1..30, NULL-terminated");
	freelist(l, 1);
}
#endif

#ifdef T_lstdelone
static void	run_test(void)
{
	t_list	*l;
	t_list	*second;

	l = mklist(2);
	second = l->next;
	g_del_count = 0;
	ft_lstdelone(l, del_int);
	check(g_del_count == 1 && g_del_order[0] == 1,
		"lstdelone: del must be called exactly once on the content (called %d)",
		g_del_count);
	check(second->next == NULL && *(int *)second->content == 2,
		"lstdelone must not touch the following nodes");
	freelist(second, 1);
	l = mknode(NULL);
	g_del_count = 0;
	ft_lstdelone(l, del_noop);
	check(g_del_count == 1, "lstdelone with a NULL content: del called %d times",
		g_del_count);
}
#endif

#ifdef T_lstclear
static void	run_test(void)
{
	t_list	*l;
	int		i;

	l = mklist(10);
	g_del_count = 0;
	ft_lstclear(&l, del_int);
	check(l == NULL, "lstclear: *lst must be NULL afterwards");
	check(g_del_count == 10, "lstclear: del called %d times, expected 10",
		g_del_count);
	for (i = 0; i < 10 && i < g_del_count; i++)
		if (g_del_order[i] != i + 1)
			break ;
	check(i == 10, "lstclear: del must be called head to tail (bad at %d)", i);
	l = mklist(1);
	g_del_count = 0;
	ft_lstclear(&l, del_int);
	check(l == NULL && g_del_count == 1, "lstclear on a single node");
	l = NULL;
	g_del_count = 0;
	ft_lstclear(&l, del_int);
	check(l == NULL && g_del_count == 0, "lstclear on an empty list");
}
#endif

#ifdef T_lstiter
static int	g_iter_n;

static void	f_inc(void *p)
{
	g_iter_n++;
	(*(int *)p)++;
}

static void	run_test(void)
{
	t_list	*l;
	t_list	*e;

	l = mklist(8);
	g_iter_n = 0;
	ft_lstiter(l, f_inc);
	check(g_iter_n == 8, "lstiter: f called %d times, expected 8", g_iter_n);
	check(list_is(l, 8, 1) == 0 && *(int *)l->content == 2
		&& *(int *)l->next->content == 3 && list_is(l, 8, 1) == 0,
		"lstiter: f must get node->content (every value +1)");
	freelist(l, 1);
	e = NULL;
	g_iter_n = 0;
	ft_lstiter(e, f_inc);
	check(g_iter_n == 0, "lstiter on NULL must do nothing");
}
#endif

#ifdef T_lstmap
static void	*f_double(void *p)
{
	return (mkint(*(int *)p * 2));
}

static void	run_test(void)
{
	t_list	*l;
	t_list	*m;
	t_list	*a;
	t_list	*b;
	int		k;
	int		base;

	l = mklist(7);
	base = g_live;
	m = ft_lstmap(l, f_double, del_int);
	check(list_is(m, 7, 2), "lstmap: new list must hold 2,4,..,14 in order");
	check(list_is(l, 7, 1), "lstmap must not modify the original list");
	for (a = l, b = m; a && b; a = a->next, b = b->next)
		if (a == b || a->content == b->content)
			break ;
	check(a == NULL && b == NULL, "lstmap: nodes must be new, not the originals");
	freelist(m, 1);
	freelist(l, 1);
	check(ft_lstmap(NULL, f_double, del_int) == NULL, "lstmap(NULL) must be NULL");
	l = mklist(5);
	base = g_live;
	for (k = 1; k <= 6; k++)
	{
		g_del_count = 0;
		inject_on(k);
		m = ft_lstmap(l, f_double, del_int);
		if (inject_off())
		{
			check(m == NULL, "lstmap must return NULL when malloc #%d fails", k);
			check(g_live == base,
				"lstmap, malloc #%d fails: %d content(s) created by f never passed to del",
				k, g_live - base);
			g_live = base;
		}
		else
		{
			freelist(m, 1);
			break ;
		}
	}
	freelist(l, 1);
}
#endif

/* ------------------------------ main ----------------------------------- */

int	main(void)
{
	setvbuf(stdout, NULL, _IONBF, 0);
	run_test();
	printf("@@ %d %d\n", g_checks, g_fails);
	return (g_fails != 0);
}
