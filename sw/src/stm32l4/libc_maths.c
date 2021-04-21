unsigned int __udivmodi(unsigned int num, unsigned int den, unsigned int *rem_p)
{
	unsigned int quot = 0, qbit = 1;

	if (den == 0)
	{
		return 0;
	}

	/*
	 * left-justify denominator and count shift
	 */
	while ((signed int) den >= 0)
	{
		den <<= 1;
		qbit <<= 1;
	}

	while (qbit)
	{
		if (den <= num)
		{
			num -= den;
			quot += qbit;
		}
		den >>= 1;
		qbit >>= 1;
	}

	if (rem_p)
		*rem_p = num;

	return quot;
}

unsigned long __udivmodl(unsigned long num, unsigned long den, unsigned long *rem_p)
{
	unsigned long quot = 0, qbit = 1;

	if (den == 0)
	{
		return 0;
	}

	/*
	 * left-justify denominator and count shift
	 */
	while ((signed long) den >= 0)
	{
		den <<= 1;
		qbit <<= 1;
	}

	while (qbit)
	{
		if (den <= num)
		{
			num -= den;
			quot += qbit;
		}
		den >>= 1;
		qbit >>= 1;
	}

	if (rem_p)
		*rem_p = num;

	return quot;
}

unsigned int __aeabi_uidiv(unsigned int number, unsigned int denominator)
{
    // 32-bit unsigned integer division.
	return __udivmodi(number, denominator, 0);
}

unsigned int __aeabi_uldiv(unsigned long number, unsigned long denominator)
{
    // unsigned long division.
	return __udivmodl(number, denominator, 0);
}

void* memset(void* bufptr, int value, unsigned int size) {
	unsigned char* buf = (unsigned char*) bufptr;
	for (unsigned int i = 0; i < size; i++)
		buf[i] = (unsigned char) value;
	return bufptr;
}
