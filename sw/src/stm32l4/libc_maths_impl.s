  .syntax unified
	.cpu cortex-m4
	.fpu softvfp
	.thumb

.global	__aeabi_uidivmod

__aeabi_uidivmod:
	stmfd	sp!, {r0, r1, ip, lr}
	bl	__aeabi_uidiv
	ldmfd	sp!, {r1, r2, ip, lr}
	mul	r3, r0, r2
	sub	r1, r1, r3
	mov	pc, lr



@ https://github.com/bobbl/libaeabi-cortexm0/blob/master/ldivmod.S

/* Runtime ABI for the ARM Cortex-M0
 * ldivmod.S: 64 bit division (quotient and remainder)
 *
 * Copyright (c) 2012-2017 Jörg Mische <bobbl@gmx.de>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */



@	.syntax unified
@	.text
@	.thumb
@	.cpu cortex-m0



@ {long long quotient, long long remainder}
@ __aeabi_ldivmod(long long numerator, long long denominator)
@
@ Divide r1:r0 by r3:r2 and return the quotient in r1:r0 and the remainder in
@ r3:r2 (all signed)
@
	.thumb_func
        .global __aeabi_ldivmod
__aeabi_ldivmod:

	cmp	r1, #0
	bge	.Lnumerator_pos

	push	{r4, lr}
	movs	r4, #0			@ num = -num
	rsbs	r0, r0, #0
	sbcs	r4, r1
	mov	r1, r4

	cmp	r3, #0
	bge	.Lboth_neg

	movs	r4, #0			@ den = -den
	rsbs	r2, r2, #0
	sbcs	r4, r3
	mov	r3, r4
	bl	__aeabi_uldivmod
	movs	r4, #0			@ rem = -rem
	rsbs	r2, r2, #0
	sbcs	r4, r3
	mov	r3, r4
	pop	{r4, pc}

.Lboth_neg:
	bl	__aeabi_uldivmod
	movs	r4, #0			@ quot = -quot
	rsbs	r0, r0, #0
	sbcs	r4, r1
	mov	r1, r4
	movs	r4, #0			@ rem = -rem
	rsbs	r2, r2, #0
	sbcs	r4, r3
	mov	r3, r4
	pop	{r4, pc}

.Lnumerator_pos:
	cmp	r3, #0
	bge	.Luldivmod

	push	{r4, lr}
	movs	r4, #0			@ den = -den
	rsbs	r2, r2, #0
	sbcs	r4, r3
	mov	r3, r4
	bl	__aeabi_uldivmod
	movs	r4, #0			@ quot = -quot
	rsbs	r0, r0, #0
	sbcs	r4, r1
	mov	r1, r4
	pop	{r4, pc}




@ unsigned long long __udivdi3(unsigned long long num, unsigned long long denom)
@
@ libgcc wrapper: just an alias for __aeabi_uldivmod(), the remainder is ignored
@
	.thumb_func
        .global __udivdi3
__udivdi3:



@ {unsigned long long quotient, unsigned long long remainder}
@ __aeabi_uldivmod(unsigned long long numerator, unsigned long long denominator)
@
@ Divide r1:r0 by r3:r2 and return the quotient in r1:r0 and the remainder
@ in r3:r2 (all unsigned)
@
	.thumb_func
        .global __aeabi_uldivmod
__aeabi_uldivmod:



.Luldivmod:
	cmp	r3, #0
	bne	.L_large_denom
	cmp	r2, #0
	beq	.L_divison_by_0
	cmp	r1, #0
	beq	.L_fallback_32bits



	@ case 1: num >= 2^32 and denom < 2^32
	@ Result might be > 2^32, therefore we first calculate the upper 32
	@ bits of the result. It is done similar to the calculation of the
	@ lower 32 bits, but with a denominator that is shifted by 32.
	@ Hence the lower 32 bits of the denominator are always 0 and the
	@ costly 64 bit shift and sub operations can be replaced by cheap 32
	@ bit operations.

	push	{r4, r5, r6, r7, lr}

	@ shift left the denominator until it is greater than the numerator
	@ denom(r7:r6) = r3:r2 << 32

	movs	r5, #1		@ bitmask
	adds	r7, r2, #0	@ dont shift if denominator would overflow
	bmi	.L_upper_result
	cmp	r1, r7
	blo	.L_upper_result

.L_denom_shift_loop1:
	lsls	r5, #1
	lsls	r7, #1
	bmi	.L_upper_result	@ dont shift if overflow
	cmp	r1, r7
	bhs	.L_denom_shift_loop1

.L_upper_result:
	mov	r3, r1
	mov	r2, r0
	movs	r1, #0		@ upper result = 0
	b	.L_sub_entry1

.L_sub_loop1:
	lsrs	r7, #1		@ denom(r7:r6) >>= 1

.L_sub_entry1:
	cmp	r3, r7
	bcc	.L_dont_sub1	@ if (num>denom)

	subs	r3, r7		@ num -= denom
	orrs	r1, r5		@ result(r7:r6) |= bitmask(r5)
.L_dont_sub1:

	lsrs	r5, #1		@ bitmask(r5) >>= 1
	bne	.L_sub_loop1

	movs	r5, #1
	lsls	r5, #31
	lsls	r6, r7, #31	@ denom(r7:r6) = (r7:0) >> 1
	lsrs	r7, #1		@ dont forget least significant bit!
	b	.L_lower_result



	@ case 2: division by 0
	@ call __aeabi_ldiv0

.L_divison_by_0:
	b	__aeabi_ldiv0



	@ case 3: num < 2^32 and denom < 2^32
	@ fallback to 32 bit division

.L_fallback_32bits:
	mov	r1, r2
	push	{lr}
	bl	__aeabi_uidivmod
	mov	r2, r1
	movs	r1, #0
	movs	r3, #0
	pop	{pc}



	@ case 4: denom >= 2^32
	@ result is smaller than 2^32

.L_large_denom:
	push	{r4, r5, r6, r7, lr}

	mov	r7, r3
	mov	r6, r2
	mov	r3, r1
	mov	r2, r0

	@ Shift left the denominator until it is greater than the numerator

	movs	r1, #0		@ high word of result is 0
	movs	r5, #1		@ bitmask
	adds	r7, #0		@ dont shift if denominator would overflow
	bmi	.L_lower_result
	cmp	r3, r7
	blo	.L_lower_result

.L_denom_shift_loop4:
	lsls	r5, #1
	lsls	r7, #1
	lsls	r6, #1
	adcs	r7, r1		@ r1=0
	bmi	.L_lower_result	@ dont shift if overflow
	cmp	r3, r7
	bhs	.L_denom_shift_loop4



.L_lower_result:
	eors	r0, r0

.L_sub_loop4:
	mov	r4, r3
	cmp	r2, r6
	sbcs	r4, r7
	bcc	.L_dont_sub4	@ if (num>denom)

	subs	r2, r6		@ numerator -= denom
	sbcs	r3, r7
	orrs	r0, r5		@ result(r1:r0) |= bitmask(r5)
.L_dont_sub4:

	lsls	r4, r7, #31	@ denom(r7:r6) >>= 1
	lsrs	r6, #1
	lsrs	r7, #1
	orrs	r6, r4
	lsrs	r5, #1		@ bitmask(r5) >>= 1
	bne	.L_sub_loop4

	pop	{r4, r5, r6, r7, pc}



@ https://github.com/bobbl/libaeabi-cortexm0/blob/master/crt.S

/* Runtime ABI for the ARM Cortex-M0
 * crt.S: C runtime environment
 *
 * Copyright (c) 2012 Jörg Mische <bobbl@gmx.de>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */



@ _start
@
@ Program entry point: call main(), then exit()
@
	.thumb_func
        .global _start
_start:
	bl main
	@ fallthrough to exit()



@ void exit(int status)
@
@ Exit from program: breakpoint 0
@
	.thumb_func
        .global exit
exit:
	bkpt	#0



@ void abort(void)
@
@ Abnormal program termination: breakpoint 1
@
	.thumb_func
        .global abort
abort:
	bkpt	#1



@ int __aeabi_idiv0(int r)
@
@ Handler for 32 bit division by zero
@
	.thumb_func
        .global __aeabi_idiv0
__aeabi_idiv0:



@ long long __aeabi_ldiv0(long long r)
@
@ Handler for 64 bit division by zero
@
	.thumb_func
        .global __aeabi_ldiv0
__aeabi_ldiv0:
	bx	lr



