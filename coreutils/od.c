/* vi: set sw=4 ts=4: */
/*
 * od implementation for busybox
 * Based on code from util-linux v 2.11l
 *
 * Copyright (c) 1990
 * The Regents of the University of California.  All rights reserved.
 *
 * Licensed under GPLv2 or later, see file LICENSE in this source tree.
 *
 * Original copyright notice is retained at the end of this file.
 */
//config:config OD
//config:	bool "od (11 kb)"
//config:	default y
//config:	help
//config:	od is used to dump binary files in octal and other formats.

//applet:IF_OD(APPLET(od, BB_DIR_USR_BIN, BB_SUID_DROP))

//kbuild:lib-$(CONFIG_OD) += od.o

//usage:#if !ENABLE_DESKTOP
//usage:#define od_trivial_usage
//usage:       "[-abcdeFfhiloxsv] [-A RADIX] [-j BYTES] [-N BYTES] [-t TYPE] [-w[BYTES]] [FILE]..."
// We also support -BDOHXIL, but they are not documented in coreutils 9.1
// manpage/help, so don't show them either.
//usage:#define od_full_usage "\n\n"
//usage:       "Print FILE (or stdin) unambiguously, as octal bytes by default"
//usage:#endif

#include "libbb.h"
#if ENABLE_DESKTOP
/* This one provides -t (busybox's own build script needs it) */
#include "od_bloaty.c"
#else

#include "dump.h"

static void
odoffset(dumper_t *dumper, int argc, char ***argvp)
{
	char *num, *p;
	int base;
	char *end;

	/*
	 * The offset syntax of od(1) was genuinely bizarre.  First, if
	 * it started with a plus it had to be an offset.  Otherwise, if
	 * there were at least two arguments, a number or lower-case 'x'
	 * followed by a number makes it an offset.  By default it was
	 * octal; if it started with 'x' or '0x' it was hex.  If it ended
	 * in a '.', it was decimal.  If a 'b' or 'B' was appended, it
	 * multiplied the number by 512 or 1024 byte units.  There was
	 * no way to assign a block count to a hex offset.
	 *
	 * We assumes it's a file if the offset is bad.
	 */
	p = **argvp;

	if (!p) {
		/* hey someone is probably piping to us ... */
		return;
	}

	if ((*p != '+')
		&& (argc < 2
			|| (!isdigit(p[0])
				&& ((p[0] != 'x') || !isxdigit(p[1])))))
		return;

	base = 0;
	/*
	 * skip over leading '+', 'x[0-9a-fA-f]' or '0x', and
	 * set base.
	 */
	if (p[0] == '+')
		++p;
	if (p[0] == 'x' && isxdigit(p[1])) {
		++p;
		base = 16;
	} else if (p[0] == '0' && p[1] == 'x') {
		p += 2;
		base = 16;
	}

	/* skip over the number */
	if (base == 16)
		for (num = p; isxdigit(*p); ++p)
			continue;
	else
		for (num = p; isdigit(*p); ++p)
			continue;

	/* check for no number */
	if (num == p)
		return;

	/* if terminates with a '.', base is decimal */
	if (*p == '.') {
		if (base)
			return;
		base = 10;
	}

	dumper->dump_skip = strtol(num, &end, base ? base : 8);

	/* if end isn't the same as p, we got a non-octal digit */
	if (end != p)
		dumper->dump_skip = 0;
	else {
		if (*p) {
			if (*p == 'b') {
				dumper->dump_skip *= 512;
				++p;
			} else if (*p == 'B') {
				dumper->dump_skip *= 1024;
				++p;
			}
		}
		if (*p)
			dumper->dump_skip = 0;
		else {
			++*argvp;
			/*
			 * If the offset uses a non-octal base, the base of
			 * the offset is changed as well.  This isn't pretty,
			 * but it's easy.
			 */
#define TYPE_OFFSET 7
			{
				char x_or_d;
				if (base == 16) {
					x_or_d = 'x';
					goto DO_X_OR_D;
				}
				if (base == 10) {
					x_or_d = 'd';
 DO_X_OR_D:
					if (dumper->fshead && dumper->fshead->nextfs) {
						dumper->fshead->nextfu->fmt[TYPE_OFFSET]
							= dumper->fshead->nextfs->nextfu->fmt[TYPE_OFFSET]
							= x_or_d;
					}
				}
			}
		}
	}
}

// bb_dump_add():
// A format string contains format units separated by [optional] whitespace.
// A format unit contains up to three items: an iteration count, a byte count,
// and a format.
// The iteration count is an optional integer (default 1).
// Each format is applied iteration count times.
// The byte count is an optional integer. It defines the number
// of bytes to be interpreted by each iteration of the format.
// If an iteration count and/or a byte count is specified, a slash must be
// placed after the iteration count and/or before the byte count
// to disambiguate them.
// The printf-style format is required and must be surrounded by " "s.
// (Below, each string contains two format units)
struct od_format {
	struct od_format *next;
	const char *fmt;
	unsigned bcnt;
};

static const struct {
	const char *fmt;
	unsigned bcnt;
} od_format_defs[] ALIGN_PTR = {
	{ " %3_u",    1 },                         /* 0: a */
	{ " %06o",   2 },                         /* 1: B (undocumented in od), o */
	{ " %03o",   1 },                         /* 2: b, -t o1 */
	{ " %3_c",   1 },                         /* 3: c */
	{ " %5u",    2 },                         /* 4: d, -t u2 */
	{ " %10u",   4 },                         /* 5: D, -t u4 */
	{ " %24.14e", 8 },                        /* 6: e (undocumented in od), F */
	{ " %15.7e", 4 },                         /* 7: f */
	{ " %08x",   4 },                         /* 8: H, X, -t x4 */
	{ " %04x",   2 },                         /* 9: h, x, -t x2 */
	{ " %11d",   4 },                         /* 10: i, -t d4 */
	{ " %011o",  4 },                         /* 11: O, -t o4 */
	{ " %6d",    2 },                         /* 12: s, -t d2 */
	{ " %02x",   1 },                         /* 13: -t x1 */
	{ " %4d",    1 },                         /* 14: -t d1 */
	{ " %3u",    1 },                         /* 15: -t u1 */
	/* -I,L,l: depend on word width of the arch (what is "long"?) */
#if ULONG_MAX > 0xffffffff
	{ " %20lld", 8 },                         /* 16: I, L, l */
#define L_ 16
#else
	/* 32-bit arch: -I,L,l are the same as -i */
#define L_ 10
#endif
};

static const char od_opts[] ALIGN1 = "aBbcDdeFfHhIiLlOoXxsv";

static const char od_o2si[] ALIGN1 = {
	0, 1, 2, 3, 5,     /* aBbcD */
	4, 6, 6, 7, 8,     /* deFfH */
	9, L_, 10, L_, L_, /* hIiLl */
	11, 1, 8, 9, 12    /* OoXxs */
};

static const struct suffix_mult od_byte_suffixes[] ALIGN_SUFFIX = {
	{ "b", 512 },
	{ "KB", 1000 },
	{ "K", 1024 },
	{ "KiB", 1024 },
	{ "MB", 1000000 },
	{ "M", 1024*1024 },
	{ "MiB", 1024*1024 },
	{ "GB", 1000000*1000 },
	{ "G", 1024*1024*1024 },
	{ "GiB", 1024*1024*1024 },
	{ "", 0 }
};

static void
od_append_format(struct od_format **head, struct od_format **tail, unsigned idx)
{
	struct od_format *f = xmalloc(sizeof(*f));

	f->next = NULL;
	f->fmt = od_format_defs[idx].fmt;
	f->bcnt = od_format_defs[idx].bcnt;
	if (*tail)
		(*tail)->next = f;
	else
		*head = f;
	*tail = f;
}

static unsigned
od_parse_size(const char **sp, unsigned default_size, int floating)
{
	const char *s = *sp;
	char *end;
	unsigned long size;

	if (!floating && *s && strchr("CSIL", *s)) {
		static const unsigned char csil_size[] ALIGN1 = {
			sizeof(char), sizeof(short), sizeof(int), sizeof(long)
		};
		size = csil_size[strchr("CSIL", *s) - "CSIL"];
		*sp = s + 1;
		return size;
	}
	if (floating && *s && strchr("FDL", *s)) {
		static const unsigned char fdl_size[] ALIGN1 = {
			sizeof(float), sizeof(double), sizeof(long double)
		};
		size = fdl_size[strchr("FDL", *s) - "FDL"];
		*sp = s + 1;
		return size;
	}
	if (!isdigit(*s))
		return default_size;

	errno = 0;
	size = strtoul(s, &end, 0);
	if (errno || end == s || size > 8)
		bb_error_msg_and_die("invalid type size '%s'", s);
	*sp = end;
	return size;
}

static void
od_append_type(struct od_format **head, struct od_format **tail, const char *type)
{
	while (*type) {
		unsigned size;

		switch (*type++) {
		case 'a':
			od_append_format(head, tail, 0);
			break;
		case 'c':
			od_append_format(head, tail, 3);
			break;
		case 'd':
			size = od_parse_size(&type, sizeof(int), 0);
			if (size == 1)
				od_append_format(head, tail, 14);
			else if (size == 2)
				od_append_format(head, tail, 12);
			else if (size == 4)
				od_append_format(head, tail, 10);
#if ULONG_MAX > 0xffffffff
			else if (size == 8)
				od_append_format(head, tail, L_);
#endif
			else
				bb_error_msg_and_die("%u-byte signed decimal type is not supported", size);
			break;
		case 'f':
			size = od_parse_size(&type, sizeof(double), 1);
			if (size == 4)
				od_append_format(head, tail, 7);
			else if (size == 8)
				od_append_format(head, tail, 6);
			else
				bb_error_msg_and_die("%u-byte floating point type is not supported", size);
			break;
		case 'o':
			size = od_parse_size(&type, sizeof(int), 0);
			if (size == 1)
				od_append_format(head, tail, 2);
			else if (size == 2)
				od_append_format(head, tail, 1);
			else if (size == 4)
				od_append_format(head, tail, 11);
			else
				bb_error_msg_and_die("%u-byte octal type is not supported", size);
			break;
		case 'u':
			size = od_parse_size(&type, sizeof(int), 0);
			if (size == 1)
				od_append_format(head, tail, 15);
			else if (size == 2)
				od_append_format(head, tail, 4);
			else if (size == 4)
				od_append_format(head, tail, 5);
			else
				bb_error_msg_and_die("%u-byte unsigned decimal type is not supported", size);
			break;
		case 'x':
			size = od_parse_size(&type, sizeof(int), 0);
			if (size == 1)
				od_append_format(head, tail, 13);
			else if (size == 2)
				od_append_format(head, tail, 9);
			else if (size == 4)
				od_append_format(head, tail, 8);
			else
				bb_error_msg_and_die("%u-byte hexadecimal type is not supported", size);
			break;
		default:
			bb_error_msg_and_die("invalid character '%c' in type string '%s'",
				type[-1], type);
		}
		if (*type == 'z')
			bb_simple_error_msg_and_die("the 'z' type suffix is not supported");
	}
}

static const char *
od_required_arg(int argc, char **argv, int *argno, const char *arg)
{
	if (arg && arg[0])
		return arg;
	if (++*argno >= argc)
		bb_show_usage();
	return argv[*argno];
}

#if ENABLE_LONG_OPTS
static int
od_long_match(const char *arg, const char *name, const char **value)
{
	unsigned len = strlen(name);

	if (strncmp(arg, name, len) != 0)
		return 0;
	if (arg[len] == '=') {
		*value = arg + len + 1;
		return 1;
	}
	if (arg[len] == '\0') {
		*value = NULL;
		return 1;
	}
	return 0;
}
#endif

static void
od_parse_address_radix(const char *radix, char *address_base, unsigned *address_pad)
{
	if (radix[0] == '\0' || radix[1] != '\0')
		bb_error_msg_and_die("bad output address radix '%s' (must be [doxn])", radix);
	switch (radix[0]) {
	case 'd':
		*address_base = 'd';
		*address_pad = 7;
		break;
	case 'o':
		*address_base = 'o';
		*address_pad = 7;
		break;
	case 'x':
		*address_base = 'x';
		*address_pad = 6;
		break;
	case 'n':
		*address_base = '\0';
		*address_pad = 0;
		break;
	default:
		bb_error_msg_and_die("bad output address radix '%c' (must be [doxn])", radix[0]);
	}
}

static void
od_add_address_format(dumper_t *dumper, char address_base, unsigned address_pad, int final)
{
	char fmt[sizeof("\"%07.7_Ao\\n\"")];

	if (!address_base)
		return;
	if (final)
		sprintf(fmt, "\"%%0%u.%u_A%c\\n\"", address_pad, address_pad, address_base);
	else
		sprintf(fmt, "\"%%0%u.%u_a%c\"", address_pad, address_pad, address_base);
	bb_dump_add(dumper, fmt);
}

static void
od_add_address_pad(dumper_t *dumper, unsigned address_pad)
{
	char fmt[sizeof("\"       \"")];

	if (!address_pad)
		return;
	fmt[0] = '"';
	memset(fmt + 1, ' ', address_pad);
	fmt[address_pad + 1] = '"';
	fmt[address_pad + 2] = '\0';
	bb_dump_add(dumper, fmt);
}

static void
od_add_data_format(dumper_t *dumper, const struct od_format *f, unsigned bytes_per_line)
{
	char fmt[sizeof("4294967295/4294967295 \" %24.14e\"\"\\n\"")];
	unsigned reps;

	if (!bytes_per_line || bytes_per_line % f->bcnt != 0)
		bb_error_msg_and_die("invalid output width %u for %u-byte format",
			bytes_per_line, f->bcnt);
	reps = bytes_per_line / f->bcnt;
	sprintf(fmt, "%u/%u \"%s\"\"\\n\"", reps, f->bcnt, f->fmt);
	bb_dump_add(dumper, fmt);
}

int od_main(int argc, char **argv) MAIN_EXTERNALLY_VISIBLE;
int od_main(int argc, char **argv)
{
	int first = 1;
	int i;
	char address_base = 'o';
	unsigned address_pad = 7;
	unsigned bytes_per_line = 16;
	struct od_format *formats = NULL;
	struct od_format *formats_tail = NULL;
	struct od_format *fmt;
	dumper_t *dumper = alloc_dumper();

	for (i = 1; i < argc; ++i) {
		char *arg = argv[i];
		const char *val;
		char *p;

		if (arg[0] != '-' || arg[1] == '\0')
			break;
		if (arg[1] == '-' && arg[2] == '\0') {
			++i;
			break;
		}
#if ENABLE_LONG_OPTS
		if (arg[1] == '-') {
			arg += 2;
			if (strcmp(arg, "help") == 0)
				bb_show_usage();
			if (strcmp(arg, "version") == 0) {
				puts(bb_banner);
				return EXIT_SUCCESS;
			}
			if (od_long_match(arg, "address-radix", &val)) {
				od_parse_address_radix(od_required_arg(argc, argv, &i, val),
					&address_base, &address_pad);
			} else if (od_long_match(arg, "skip-bytes", &val)) {
				dumper->dump_skip = xstrtoull_range_sfx(
					od_required_arg(argc, argv, &i, val),
					0, 0, OFF_T_MAX, od_byte_suffixes);
			} else if (od_long_match(arg, "read-bytes", &val)) {
				dumper->dump_length = xstrtou_range_sfx(
					od_required_arg(argc, argv, &i, val),
					0, 0, INT_MAX, od_byte_suffixes);
			} else if (od_long_match(arg, "format", &val)) {
				od_append_type(&formats, &formats_tail,
					od_required_arg(argc, argv, &i, val));
			} else if (od_long_match(arg, "output-duplicates", &val)) {
				if (val)
					bb_show_usage();
				dumper->dump_vflag = ALL;
			} else if (od_long_match(arg, "width", &val)) {
				bytes_per_line = val ? xstrtou_range_sfx(val, 0, 1, INT_MAX,
					od_byte_suffixes) : 32;
			} else if (od_long_match(arg, "traditional", &val)) {
				if (val)
					bb_show_usage();
			} else {
				bb_show_usage();
			}
			continue;
		}
#endif

		for (p = arg + 1; *p; ++p) {
			if (*p == 'A') {
				val = od_required_arg(argc, argv, &i, p + 1);
				od_parse_address_radix(val, &address_base, &address_pad);
				break;
			}
			if (*p == 'j') {
				val = od_required_arg(argc, argv, &i, p + 1);
				dumper->dump_skip = xstrtoull_range_sfx(val, 0, 0, OFF_T_MAX,
					od_byte_suffixes);
				break;
			}
			if (*p == 'N') {
				val = od_required_arg(argc, argv, &i, p + 1);
				dumper->dump_length = xstrtou_range_sfx(val, 0, 0, INT_MAX,
					od_byte_suffixes);
				break;
			}
			if (*p == 't') {
				val = od_required_arg(argc, argv, &i, p + 1);
				od_append_type(&formats, &formats_tail, val);
				break;
			}
			if (*p == 'w') {
				bytes_per_line = p[1] ? xstrtou_range_sfx(p + 1, 0, 1,
					INT_MAX, od_byte_suffixes) : 32;
				break;
			}
			if (*p == 'v') {
				dumper->dump_vflag = ALL;
			} else if ((val = strchr(od_opts, *p)) != NULL) {
				od_append_format(&formats, &formats_tail,
					(unsigned)od_o2si[val - od_opts]);
			} else {
				bb_show_usage();
			}
		}
	}

	if (!formats)
		od_append_format(&formats, &formats_tail, 1); /* -o format is default */

	for (fmt = formats; fmt; fmt = fmt->next) {
		if (first) {
			first = 0;
			od_add_address_format(dumper, address_base, address_pad, 1);
			od_add_address_format(dumper, address_base, address_pad, 0);
		} else {
			od_add_address_pad(dumper, address_pad);
		}
		od_add_data_format(dumper, fmt, bytes_per_line);
	}

	dumper->od_eofstring = "\n";

	argc -= i;
	argv += i;

	odoffset(dumper, argc, &argv);

	return bb_dump_dump(dumper, argv);
}
#endif /* !ENABLE_DESKTOP */

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ''AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
