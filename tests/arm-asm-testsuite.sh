#!/bin/sh

set -e

# Note: "{r3}" is definitely different--but would complicate the assembler.

state="`mktemp -d`"
cat ../arm-tok.h | \
  grep DEF_ASM | \
  grep -v 'not useful' | \
  grep -v '#define' | \
  grep -v '/[*]' | \
  grep -v 'DEF_ASM_CONDED_WITH_SUFFIX(x' | \
  sed -e 's;^[ ]*DEF_ASM_CONDED_VFP_F32_F64[^(]*(\(.*\)).*$; DEF_ASM_CONDED(\1.f32)\
 DEF_ASM_CONDED(\1.f64);g' | \
  sed -e 's;^[ ]*DEF_ASM[^(]*(\(.*\)).*$;\1;g' -e 's;, ;.;g' | \
  egrep -v '^((r|c|p|s|d)[0-9]+|fp|ip|sp|lr|pc|asl|apsr_nzcv|fpsid|fpscr|fpexc)$' | while read s
do
	as_opts=""
	if [ "${s#v}" != "${s}" ]
	then
		if grep -q "CONFIG_arm_vfp=yes" ../config.mak
		then
			as_opts="${as_opts} -mfpu=vfp"
		else
			echo "note: skipping VFP instruction: $s (because VFP is disabled)">&2
			continue
		fi
	fi
	ok=0
	for args in "r3, r4, r5, r6" \
	            "r3, r4, r5" \
	            "r3, r4, r5, asl #7" \
	            "r3, r4, r5, lsl #7" \
	            "r3, r4, r5, asr #7" \
	            "r3, r4, r5, lsr #7" \
	            "r3, r4, r5, ror #7" \
	            "r3, r4, r5, rrx" \
	            "r3, r4, r5, asl r6" \
	            "r3, r4, r5, lsl r6" \
	            "r3, r4, r5, asr r6" \
	            "r3, r4, r5, lsr r6" \
	            "r3, r4, r5, ror r6" \
	            "r3, r4, #5, asl #7" \
	            "r3, r4, #5, lsl #7" \
	            "r3, r4, #5, asr #7" \
	            "r3, r4, #5, lsr #7" \
	            "r3, r4, #5, ror #7" \
	            "r3, r4, #5, rrx" \
	            "r3, #5, r4" \
	            "r3, #4, #8" \
	            "r3, r4, asl #5" \
	            "r3, r4, lsl #5" \
	            "r3, r4, asr #5" \
	            "r3, r4, lsr #5" \
	            "r3, r4, ror #5" \
	            "r3, r4, ror #8" \
	            "r3, r4, asl r5" \
	            "r3, r4, lsl r5" \
	            "r3, r4, asr r5" \
	            "r3, r4, lsr r5" \
	            "r3, r4, ror r5" \
	            "r3, r4, ror #8" \
	            "r3, r4, ror #16" \
	            "r3, r4, ror #24" \
	            "r3, r4, rrx" \
	            "r3, #4, asl #5" \
	            "r3, #4, lsl #5" \
	            "r3, #4, asr #5" \
	            "r3, #4, lsr #5" \
	            "r3, #4, ror #5" \
	            "r3, r4, rrx" \
	            "r3, r4" \
	            "r3" \
	            "{r3,r4,r5}" \
	            "{r3,r5,r4}" \
	            "r2!, {r3,r4,r5}" \
	            "r2!, {r3,r5,r4}" \
	            "r2, {r3,r4,r5}" \
	            "r2, {r3,r5,r4}" \
	            "r2, [r3, r4]" \
	            "r2, [r3, r4]!" \
	            "r2, [r3, -r4]" \
	            "r2, [r3, -r4]!" \
	            "r2, [r3], r4" \
	            "r2, [r3], -r4" \
	            "r2, [r3]" \
	            "r2, r3, [r4, lsl# 2]" \
	            "r2, [r3, r4, lsr# 1]" \
	            "r2, [r3, r4, lsr# 2]!" \
	            "r2, [r3, -r4, ror# 3]" \
	            "r2, [r3, -r4, lsl# 1]!" \
	            "r2, [r3], r4, lsl# 3" \
	            "r2, [r3], -r4, asr# 31" \
	            "r2, [r3], -r4, asl# 1" \
	            "r2, [r3], -r4, rrx" \
	            "r2, [r3]" \
	            "r2, r3, [r4]" \
	            "r2, [r3, #4]" \
	            "r2, [r3, #-4]" \
	            "r2, [r3, #0x45]" \
	            "r2, [r3, #-0x45]" \
	            "r2, r3, #4" \
	            "r2, r3, #-4" \
	            "p10, #7, c2, c0, c1, #4" \
	            "p10, #7, r2, c0, c1, #4" \
	            "p10, #0, c2, c0, c1, #4" \
	            "p10, #0, r2, c0, c1, #4" \
	            "r2, #4" \
	            "r2, #-4" \
	            "r2, #0xEFFF" \
	            "r3, #0x0000" \
	            "r4, #0x0201" \
	            "r4, #0xFFFFFF00" \
	            "r2, #-4" \
	            "p10, #7, c2, c0, c1, #4" \
	            "p10, #7, r2, c0, c1, #4" \
	            "#4" \
	            "#-4" \
                    "p5, c2, [r3]" \
                    "p5, c3, [r4]" \
                    "p5, c2, [r3, #4]" \
                    "p5, c2, [r3, #-4]" \
                    "p5, c2, [r3, #0x45]" \
                    "p5, c2, [r3, #-0x45]" \
                    "s2, [r3]" \
                    "s3, [r4]" \
                    "s2, [r3, #4]" \
                    "s2, [r3, #-4]" \
                    "s2, [r3, #0x45]" \
                    "s2, [r3, #-0x45]" \
                    "r1, {d3-d4}" \
                    "r1!, {d3-d4}" \
                    "r2, {d4-d15}" \
                    "r3!, {d4-d15}" \
                    "r3!, {d4}" \
                    "r2, {s4-s31}" \
                    "r3!, {s4}" \
                    "{d3-d4}" \
                    "{d4-d15}" \
                    "{d4}" \
                    "{s4-s31}" \
                    "{s4}" \
                    "s2, s3, s4" \
                    "s2, s3" \
                    "d2, d3, d4" \
                    "d2, d3" \
                    "s2, #0" \
                    "d2, #0" \
                    "s3, #0.0" \
                    "d3, #0.0" \
                    "s4, #-0.1796875" \
                    "d4, #0.1796875" \
                    "r2, r3, d1" \
                    "d1, r2, r3" \
                    "s1, r2" \
                    "r2, s1" \
                    "r2, fpexc" \
                    "r2, fpscr" \
                    "r2, fpsid" \
                    "apsr_nzcv, fpscr" \
                    "fpexc, r2" \
                    "fpscr, r2" \
                    "fpsid, r2" \
                    "s3, d4" \
                    "d4, s3" \
	            ""
	do
		#echo ".syntax unified" > a.s
		err="`mktemp --suffix=-stderr.log`"
		as_object="${state}/as-$s $args.o"
		tcc_object="${state}/tcc-$s $args.o"
		expected="${state}/expected-$s $args"
		got="${state}/got-$s $args"
		if echo "$s $args" | "${CROSS_COMPILE}as" -mlittle-endian ${as_opts} -o "${as_object}" - 2>"${err}"
		then
			cat "${err}"
			rm -f "${err}"
			total_count=`expr $total_count + 1`
			"${CROSS_COMPILE}objdump" -S "${as_object}" |grep "^[ ]*0:" >"${expected}"

			#echo '__asm__("'"$s ${args}"'");' > "${csource}"
			if echo '__asm__("'"$s ${args}"'");'| ${TCC} -o "${tcc_object}" -c -
			then
				"${CROSS_COMPILE}objdump" -S "${tcc_object}" |grep "^[ ]*0:" >"${got}"
				if diff -u "${got}" "${expected}"
				then
					touch "${state}/ok-$s $args"
				else
					echo "warning: '$s $args' did not work in tcc (see above)">&2
				fi
			else
				rm -f "${tcc_object}"
				echo "warning: '$s $args' did not work in tcc">&2
			fi
			ok=1
		else # GNU as can't do it either--so we don't care
			rm -f "${as_object}"
		fi
		rm -f "${err}"
	done
	if [ "${ok}" -eq "0" ]
	then
		echo "warning: $s could not be used.">&2
		continue
	fi
done

successful_count="$(ls -1 "${state}/ok-"* |wc -l)"
total_count="$(ls -1 "${state}/as-"*.o |wc -l)"
echo "${successful_count} of ${total_count} tests succeeded.">&2
if [ "${successful_count}" -eq "${total_count}" ]
then
	rm -rf "${state}"
	exit 0
else
	status=0
	for s in "${state}/as-"*.o
	do
		test="$(basename "$s")"
		test="${test%.o}"
		test="${test#as-}"
		t="${state}/ok-${test}"
		if [ ! -f "$t" ]
		then
			case "${test}" in
			"bl r3"|"b r3"|"mov r2, #0xEFFF"|"mov r4, #0x0201")
				known_failure=" (known failure)"
				;;
			"vmov.f32 r2, r3, d1"|"vmov.f32 d1, r2, r3") # GNU as bug
				known_failure=" (known failure)"
				;;
			*)
				known_failure=""
				status=1
				;;
			esac
			echo "Failed test: ${test}${known_failure}">&2
		fi
	done
	rm -rf "${state}"
	exit "${status}"
fi
