#!/bin/bash
# Libft tester v2 - not part of the submission.
# From the repository root:
#   bash tests/run.sh                 all 43 functions
#   bash tests/run.sh ft_split itoa   only these (with or without "ft_")
#   bash tests/run.sh -v              full sanitizer reports on failures
#   bash tests/run.sh -k              keep the build folder (printed at the end)
#   bash tests/run.sh --audit         force the moulinette audit (Makefile, norm, libc calls)
# The audit runs by itself only once all 43 functions are written and clean.
# Every function is built and run alone, with ASan + UBSan + LeakSanitizer,
# and malloc fault injection (-Wl,--wrap=malloc).

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TESTER="$ROOT/tests/tester.c"
CC="${CC:-cc}"
VERBOSE=0
TMO="${LIBFT_TIMEOUT:-30}"
KEEP=0
FORCEAUDIT=0
SELECT=()
for a in "$@"; do
	case "$a" in
		-v) VERBOSE=1 ;;
		-k) KEEP=1 ;;
		--audit) FORCEAUDIT=1 ;;
		-h|--help) sed -n "2,11p" "$0"; exit 0 ;;
		*) a="${a%.c}"; SELECT+=("${a#ft_}") ;;
	esac
done

PART1="isalpha isdigit isalnum isascii isprint strlen memset bzero memcpy
memmove strlcpy strlcat toupper tolower strchr strrchr strncmp memchr memcmp
strnstr atoi calloc strdup"
PART2="substr strjoin strtrim split itoa strmapi striteri putchar_fd putstr_fd
putendl_fd putnbr_fd"
PART3="lstnew lstadd_front lstsize lstlast lstadd_back lstdelone lstclear
lstiter lstmap"
ALL=" $(echo $PART1 $PART2 $PART3) "
for s in "${SELECT[@]}"; do
	case "$ALL" in *" $s "*) ;; *) echo "unknown function: ft_$s"; exit 2 ;; esac
done

# ------------------------------- style --------------------------------------
if [ -t 1 ] && [ -z "$NO_COLOR" ]; then
	R=$'\e[0m'; B=$'\e[1m'; D=$'\e[38;5;245m'; ACC=$'\e[38;5;81m'
	TXT=$'\e[38;5;252m'
	P_OK=$'\e[30;48;5;78m'; P_KO=$'\e[30;48;5;203m'; P_CR=$'\e[97;48;5;125m'
	P_LK=$'\e[30;48;5;221m'; P_UB=$'\e[30;48;5;215m'; P_TM=$'\e[30;48;5;141m'
	P_BD=$'\e[30;48;5;117m'; P_SK=$'\e[38;5;245;48;5;236m'; P_WN=$'\e[30;48;5;221m'
	F_OK=$'\e[38;5;78m'; F_KO=$'\e[38;5;203m'; F_LK=$'\e[38;5;221m'
else
	R=; B=; D=; ACC=; TXT=; P_OK=; P_KO=; P_CR=; P_LK=; P_UB=; P_TM=; P_BD=
	P_WN=; P_SK=; F_OK=; F_KO=; F_LK=
fi

rule() { # rule <width>
	local i; for ((i = 0; i < $1; i++)); do printf '─'; done
}

header() {
	local w=58
	printf '\n  %s╭' "$ACC"; rule $w; printf '╮%s\n' "$R"
	printf '  %s│%s  %sL I B F T%s  %s·  tester v2%s%*s%s│%s\n' "$ACC" "$R" "$B" "$R" "$TXT" "$R" $((w - 25)) '' "$ACC" "$R"
	printf '  %s│%s  %sasan · ubsan · leaksan · malloc fault injection%s%*s%s│%s\n' "$ACC" "$R" "$D" "$R" $((w - 49)) '' "$ACC" "$R"
	printf '  %s╰' "$ACC"; rule $w; printf '╯%s\n' "$R"
}

section() { # section <title>
	printf '\n  %s%s%s %s' "$ACC" "$B" "$1" "$D"; rule $((56 - ${#1})); printf '%s\n' "$R"
}

pill() { # pill <bg> <label>
	printf '%s %-5s %s' "$1" "$2" "$R"
}

# ------------------------------- build --------------------------------------
BUILD="$(mktemp -d /tmp/libft_tester.XXXXXX)"
[ "$KEEP" = 1 ] || trap 'rm -rf "$BUILD"' EXIT
mkdir -p "$BUILD/obj" "$BUILD/bin" "$BUILD/log"

SAN="-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer"
JOBS="$(nproc 2>/dev/null || echo 4)"

wait_slot() { while (($(jobs -rp | wc -l) >= JOBS)); do wait -n; done; }

compile_obj() { # compile_obj <file>
	local f="$1" b
	b="$(basename "$f" .c)"
	$CC -Wall -Wextra -Werror -g $SAN -I "$ROOT" -c "$f" -o "$BUILD/obj/$b.o" \
		2> "$BUILD/obj/$b.err" || rm -f "$BUILD/obj/$b.o"
}

build_bin() { # build_bin <name>
	local n="$1" objs=()
	for o in "$BUILD"/obj/*.o; do [ -f "$o" ] && objs+=("$o"); done
	$CC -g -Wno-unused-function -Wno-unused-variable -Wno-format-security -Werror=implicit-function-declaration $SAN -DT_"$n" -I "$ROOT" \
		"$TESTER" "${objs[@]}" -Wl,--wrap=malloc -o "$BUILD/bin/$n" \
		2> "$BUILD/bin/$n.err" || rm -f "$BUILD/bin/$n"
}

wanted() { # wanted <name>: selected on the command line (or nothing selected)
	[ ${#SELECT[@]} -eq 0 ] && return 0
	local s; for s in "${SELECT[@]}"; do [ "$s" = "$1" ] && return 0; done
	return 1
}

header
printf '\n  %sbuilding...%s' "$D" "$R"
for f in "$ROOT"/ft_*.c; do
	[ -f "$f" ] || continue
	wait_slot
	compile_obj "$f" &
done
wait
for n in $PART1 $PART2 $PART3; do
	wanted "$n" || continue
	[ -f "$BUILD/obj/ft_$n.o" ] || continue
	wait_slot
	build_bin "$n" &
done
wait
printf '\r\e[K'

# ------------------------------- run ----------------------------------------
export ASAN_OPTIONS="exitcode=77:detect_leaks=1:allocator_may_return_null=1:abort_on_error=0:strict_string_checks=1:detect_stack_use_after_return=1:log_path=$BUILD/log/san"
export UBSAN_OPTIONS="exitcode=77:print_stacktrace=1:log_path=$BUILD/log/san"
export LSAN_OPTIONS="exitcode=77:log_path=$BUILD/log/san"

n_ok=0; n_ko=0; n_crash=0; n_leak=0; n_ub=0; n_time=0; n_build=0; n_skip=0
bar=""
total=0

# Where in the student's code each sanitizer stack points (one line per stack).
where() { # where <logfile>
	awk '
	/ERROR: (Address|Leak)Sanitizer/ { sec = "at"; got = 0 }
	/^(READ|WRITE) of size/ { sec = "at"; got = 0 }
	/allocated by thread/ { sec = "allocated at"; got = 0 }
	/freed by thread/ { sec = "freed at"; got = 0 }
	/^(Direct|Indirect) leak/ { sec = "leaked, allocated at"; got = 0 }
	/^ *#[0-9]+ 0x[0-9a-f]+ in ft_[a-z_0-9]+ / {
		if (!got) {
			path = $5; sub(/.*\//, "", path)
			print sec " " path " (" $4 ")"; got = 1
		}
	}' "$1" | awk '!seen[$0]++' | head -4
}

detail() { printf '           %s%s%s\n' "$D" "$1" "$R"; }

report() { # report <status> <name> <checks> <ms>
	local st="$1" n="$2" c="$3" ms="$4" t
	t="$(printf '%d.%02ds' $((ms / 1000)) $((ms % 1000 / 10)))"
	printf '  '
	case "$st" in
		ok)    pill "$P_OK" PASS ;;
		ko)    pill "$P_KO" FAIL ;;
		crash) pill "$P_CR" CRASH ;;
		leak)  pill "$P_LK" LEAK ;;
		ub)    pill "$P_UB" UB ;;
		time)  pill "$P_TM" TIME ;;
		build) pill "$P_BD" BUILD ;;
		skip)  pill "$P_SK" '--' ;;
	esac
	if [ "$st" = skip ]; then
		printf '  %s%-18s not written yet%s\n' "$D" "ft_$n" "$R"
	else
		printf '  %s%-18s%s %s%6s checks%s   %s%s%s\n' "$TXT" "ft_$n" "$R" "$D" "${c:--}" "$R" "$D" "$t" "$R"
	fi
}

fullreport() { # fullreport <file>
	[ "$VERBOSE" = 1 ] && sed 's/^/             /' "$1" | head -45
}

run_one() { # run_one <name>
	local n="$1" bin="$BUILD/bin/$n" start end ms code marker checks fails
	local san="$BUILD/san_$n" l

	if [ ! -f "$ROOT/ft_$n.c" ]; then
		report skip "$n" "" 0; ((n_skip++)); bar+="${D}░${R}"; return
	fi
	if [ ! -f "$BUILD/obj/ft_$n.o" ]; then
		report build "$n" "" 0; ((n_build++)); bar+="${F_KO}█${R}"
		grep -E 'error|warning' "$BUILD/obj/ft_$n.err" | sed -E 's/^.*\/(ft_|libft\.h)/\1/' \
			| head -4 | while IFS= read -r l; do detail "$l"; done
		return
	fi
	if [ ! -f "$bin" ]; then
		report build "$n" "" 0; ((n_build++)); bar+="${F_KO}█${R}"
		grep -E 'error|undefined|implicit' "$BUILD/bin/$n.err" | sed -E 's/^.*\/(ft_|libft\.h)/\1/' \
			| head -3 | while IFS= read -r l; do detail "$l"; done
		grep -q 'implicit' "$BUILD/bin/$n.err" && detail "hint: missing prototype in libft.h?"
		return
	fi

	rm -f "$BUILD"/log/*
	start=$(date +%s%N)
	timeout "$TMO" "$bin" > "$BUILD/out_$n" 2> "$BUILD/err_$n"
	code=$?
	end=$(date +%s%N)
	ms=$(((end - start) / 1000000))
	cat "$BUILD"/log/san.* "$BUILD/err_$n" > "$san" 2>/dev/null
	marker="$(grep '^@@ ' "$BUILD/out_$n" | tail -1)"
	checks="$(echo "$marker" | awk '{print $2}')"
	fails="$(echo "$marker" | awk '{print $3}')"

	if [ "$code" = 124 ]; then
		report time "$n" "" "$ms"; ((n_time++)); bar+="${F_LK}█${R}"
		detail "no answer after $TMO s: infinite loop?"
	elif grep -q 'runtime error' "$san"; then
		report ub "$n" "$checks" "$ms"; ((n_ub++)); bar+="${F_LK}█${R}"
		grep -m1 'runtime error' "$san" | sed -E 's/^.*\/(ft_)/\1/' | while IFS= read -r l; do detail "$l"; done
		fullreport "$san"
	elif grep -q 'ERROR: AddressSanitizer' "$san"; then
		report crash "$n" "$checks" "$ms"; ((n_crash++)); bar+="${F_KO}█${R}"
		detail "$(grep -m1 'ERROR: AddressSanitizer' "$san" | sed -E 's/.*AddressSanitizer: ([^ ]+).*/\1/')"
		where "$san" | while IFS= read -r l; do detail "$l"; done
		grep -m1 -E 'is located .* (after|before|inside)' "$san" | sed -E 's/^ *//' \
			| while IFS= read -r l; do detail "$l"; done
		fullreport "$san"
	elif [ -n "$fails" ] && [ "$fails" -gt 0 ]; then
		report ko "$n" "$checks" "$ms"; ((n_ko++)); bar+="${F_KO}█${R}"
		grep '^  ko| ' "$BUILD/out_$n" | sed 's/^  ko| //' | while IFS= read -r l; do detail "$l"; done
		[ "$fails" -gt 8 ] && detail "... and $((fails - 8)) more of $fails failed checks"
		grep -q 'LeakSanitizer' "$san" && detail "(also leaking memory: fix the logic first)"
	elif grep -q 'LeakSanitizer' "$san"; then
		report leak "$n" "$checks" "$ms"; ((n_leak++)); bar+="${F_LK}█${R}"
		grep -m1 'SUMMARY' "$san" | sed -E 's/SUMMARY: [A-Za-z]+: //' | while IFS= read -r l; do detail "$l"; done
		where "$san" | while IFS= read -r l; do detail "$l"; done
		fullreport "$san"
	elif [ "$code" = 0 ] && [ -n "$marker" ]; then
		report ok "$n" "$checks" "$ms"; ((n_ok++)); bar+="${F_OK}█${R}"
	else
		report crash "$n" "$checks" "$ms"; ((n_crash++)); bar+="${F_KO}█${R}"
		detail "exited with code $code before the end of the test"
		head -3 "$BUILD/err_$n" | while IFS= read -r l; do detail "$l"; done
	fi
}

run_part() { # run_part <title> <names>
	local title="$1" n shown=0
	shift
	for n in $1; do
		wanted "$n" || continue
		[ "$shown" = 0 ] && section "$title" && shown=1
		run_one "$n"
		((total++))
	done
}

run_part "Part 1 · libc" "$PART1"
run_part "Part 2 · additional" "$PART2"
run_part "Part 3 · linked list" "$PART3"

# ------------------------------- audit --------------------------------------
# What the moulinette checks besides the logic. Everything is done on a copy
# (ft_*.c, libft.h, Makefile) in the build folder: the repository is untouched.
n_audit_ko=0
n_audit_warn=0

audit_line() { # audit_line <ok|ko|warn> <label> [detail...]
	local st="$1" label="$2" l
	shift 2
	printf '  '
	case "$st" in
		ok)   pill "$P_OK" PASS ;;
		ko)   pill "$P_KO" FAIL; ((n_audit_ko++)) ;;
		warn) pill "$P_WN" WARN; ((n_audit_warn++)) ;;
	esac
	printf '  %s%s%s\n' "$TXT" "$label" "$R"
	for l in "$@"; do detail "$l"; done
}

run_audit() {
	local A="$BUILD/audit" out nsrc nobj bad extra
	rm -rf "$A"; mkdir -p "$A"
	cp "$ROOT"/ft_*.c "$ROOT"/libft.h "$ROOT"/Makefile "$A"/ 2>/dev/null
	section "Audit · the moulinette checks"

	if [ ! -f "$ROOT/Makefile" ]; then
		audit_line ko "Makefile at the root of the repository"; return
	fi
	audit_line ok "Makefile at the root of the repository"

	bad=""
	for r in all clean fclean re; do
		grep -qE "^$r[[:space:]]*:" "$ROOT/Makefile" || bad+="$r "
	done
	grep -qE '^(\$\(NAME\)|libft\.a)[[:space:]]*:' "$ROOT/Makefile" || bad+='$(NAME) '
	if [ -z "$bad" ]; then audit_line ok "rules all, \$(NAME), clean, fclean, re"
	else audit_line ko "rules all, \$(NAME), clean, fclean, re" "missing: $bad"; fi

	if grep -q -- '-Wall' "$ROOT/Makefile" && grep -q -- '-Wextra' "$ROOT/Makefile" \
		&& grep -q -- '-Werror' "$ROOT/Makefile"; then
		audit_line ok "flags -Wall -Wextra -Werror in the Makefile"
	else audit_line ko "flags -Wall -Wextra -Werror in the Makefile"; fi

	if grep -q -- '-std=c99' "$ROOT/Makefile"; then
		audit_line ko "no -std=c99 (forbidden by the subject)"
	else audit_line ok "no -std=c99 (forbidden by the subject)"; fi

	if grep -qE 'wildcard|\*\.c|\*\.o' "$ROOT/Makefile"; then
		audit_line warn "sources named explicitly (Norm v4.1 III.11)" \
			"wildcard or *.c found: write the list of files before submitting"
	else audit_line ok "sources named explicitly (Norm v4.1 III.11)"; fi

	out="$(cd "$A" && make 2>&1)"
	if [ ! -f "$A/libft.a" ]; then
		audit_line ko "make builds libft.a at the root" \
			"$(echo "$out" | grep -m2 -E '\*\*\*|No rule|error:' | cut -c1-90)"
		return
	fi
	audit_line ok "make builds libft.a at the root"

	out="$(cd "$A" && make 2>&1)"
	if echo "$out" | grep -qE '(^|[[:space:]])(cc|gcc|clang|ar)[[:space:]]'; then
		audit_line ko "second make does not relink" \
			"$(echo "$out" | head -1 | cut -c1-90)"
	else audit_line ok "second make does not relink"; fi

	nsrc=$(ls "$A"/ft_*.c 2>/dev/null | wc -l)
	nobj=$(ar t "$A/libft.a" | wc -l)
	if [ "$nsrc" = "$nobj" ]; then
		audit_line ok "libft.a holds every ft_*.c ($nobj objects)"
	else audit_line ko "libft.a holds every ft_*.c" "$nsrc sources, $nobj objects in the archive"; fi

	# undefined symbols that no object of the archive defines = libc calls
	nm --defined-only "$A/libft.a" 2>/dev/null | awk 'NF==3 {print $3}' | sort -u > "$A/defined.txt"
	extra="$(nm -u "$A/libft.a" 2>/dev/null | awk 'NF==2 && $1=="U" {print $2}' \
		| sort -u | comm -23 - "$A/defined.txt" \
		| grep -vE '^(malloc|free|write|_GLOBAL_OFFSET_TABLE_|__stack_chk_fail)$')"
	if [ -z "$extra" ]; then
		audit_line ok "only malloc, free, write are called from libc"
	else
		audit_line ko "only malloc, free, write are called from libc" \
			"forbidden: $(echo $extra | tr '\n' ' ')"
	fi

	(cd "$A" && make fclean >/dev/null 2>&1)
	if ls "$A"/*.o "$A"/libft.a >/dev/null 2>&1; then
		audit_line ko "fclean removes .o files and libft.a"
	else audit_line ok "fclean removes .o files and libft.a"; fi
	(cd "$A" && make re >/dev/null 2>&1)
	if [ -f "$A/libft.a" ]; then audit_line ok "re rebuilds everything"
	else audit_line ko "re rebuilds everything"; fi

	if ! grep -qE '^#[[:space:]]*ifndef' "$ROOT/libft.h"; then
		audit_line ko "libft.h: include guard"
	elif grep -q 't_list' "$ROOT/libft.h"; then
		audit_line ok "libft.h: include guard and t_list"
	else
		audit_line ok "libft.h: include guard" "t_list not declared yet (needed for part 3)"
	fi

	bad="$(cd "$ROOT" && { git ls-files 2>/dev/null || ls -A; } \
		| grep -vE '^(ft_[a-z_0-9]+\.c|libft\.h|Makefile|README\.md|\.gitignore)$')"
	if [ -z "$bad" ]; then
		audit_line ok "no useless files in the repository"
	else
		audit_line warn "no useless files in the repository (subject: forbidden)" \
			"$(echo $bad | cut -c1-100)" \
			"keep tests/ and other helpers out of the version you submit"
	fi

	if command -v norminette >/dev/null 2>&1; then
		out="$(cd "$ROOT" && norminette ft_*.c libft.h 2>&1 | grep -c 'Error')"
		if [ "$out" = 0 ]; then audit_line ok "norminette: no error"
		else
			audit_line ko "norminette: $out error(s)" \
				"$(cd "$ROOT" && norminette ft_*.c libft.h 2>&1 | grep -A1 -m1 'Error!' | tr '\n' ' ' | cut -c1-100)"
		fi
	else audit_line warn "norminette not installed: not checked"; fi
}

# Only when every function is written and clean (or forced with --audit).
if [ "$FORCEAUDIT" = 1 ]; then
	run_audit
elif [ ${#SELECT[@]} -eq 0 ]; then
	if [ "$n_skip" -eq 0 ] && [ $((n_ko + n_crash + n_leak + n_ub + n_time + n_build)) -eq 0 ]; then
		run_audit
	else
		section "Audit · the moulinette checks"
		printf '  %sruns once the 43 functions are written and clean (or: --audit)%s\n' "$D" "$R"
	fi
fi

# ------------------------------- summary ------------------------------------
bad=$((n_ko + n_crash + n_leak + n_ub + n_time + n_build + n_audit_ko))
printf '\n  %s' "$D"; rule 58; printf '%s\n'  "$R"
printf '\n  %s\n\n' "$bar"
printf '  %s%d passed%s' "$F_OK$B" "$n_ok" "$R"
[ "$n_ko" -gt 0 ] && printf '   %s%d failed%s' "$F_KO$B" "$n_ko" "$R"
[ "$n_crash" -gt 0 ] && printf '   %s%d crashed%s' "$F_KO$B" "$n_crash" "$R"
[ "$n_ub" -gt 0 ] && printf '   %s%d undefined behavior%s' "$F_LK$B" "$n_ub" "$R"
[ "$n_leak" -gt 0 ] && printf '   %s%d leaking%s' "$F_LK$B" "$n_leak" "$R"
[ "$n_time" -gt 0 ] && printf '   %s%d timeout%s' "$F_LK$B" "$n_time" "$R"
[ "$n_build" -gt 0 ] && printf '   %s%d not building%s' "$F_KO$B" "$n_build" "$R"
[ "$n_skip" -gt 0 ] && printf '   %s%d not written yet%s' "$D" "$n_skip" "$R"
[ "$n_audit_ko" -gt 0 ] && printf '   %s%d audit failed%s' "$F_KO$B" "$n_audit_ko" "$R"
[ "$n_audit_warn" -gt 0 ] && printf '   %s%d audit warning(s)%s' "$F_LK$B" "$n_audit_warn" "$R"
printf '   %s(%d functions)%s\n' "$D" "$total" "$R"
if [ "$bad" -eq 0 ]; then
	printf '\n  %s%sEverything written so far is clean: logic, memory, leaks.%s\n\n' "$F_OK" "$B" "$R"
else
	printf '\n  %s%sFix the red ones first.%s %s-v shows the full sanitizer report.%s\n\n' "$F_KO" "$B" "$R" "$D" "$R"
fi
[ "$KEEP" = 1 ] && printf '  %sbuild kept in %s%s\n\n' "$D" "$BUILD" "$R"
[ "$bad" -eq 0 ]
