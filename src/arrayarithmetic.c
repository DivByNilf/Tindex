#include <stdlib.h>

#define MAXADD 1000
#define MAXSUB 1000
#define MAXMUL 500
#define MAXDIVD 1000
#define MAXDIVS 1000
#define MAXEXP 6	// decimals in exponent

int DIVRNDNG = 0;
int DIVFRCTN = 0;

#include <stdio.h>
#define errorf(...) fprintf(stderr, __VA_ARGS__)

char *subtract(char*, char*);			// most functions return allocated memory that must be freed

char *add(char *first, char *second) {
	int i, oldi, firstlen, seclen, fmlen, smlen;
	char buffer, *temp, *p;
	
	if (first != 0) {
		for (fmlen = 0; first[0] == '-'; first++, fmlen++);
	} if (second != 0) {
		for (smlen = 0; second[0] == '-'; second++, smlen++);
	}
	
	if (fmlen % 2) {
		if (smlen % 2) {
			if ((temp = add(first, second)) == 0)
				return temp;
			p = subtract("0", temp);
			free(temp);
			return p;
		} else {
			p = subtract(second, first);
			return p;
		}
	} else if (smlen % 2) {
		p = subtract(first, second);
		return p;
	}
	
	if (first != 0) {
		for (i = 0; first[i] != '\0' && i <= MAXADD; i++);
		firstlen = i;
	} else
		firstlen = 0;
	if (second != 0) {
		for (i = 0; second[i] != '\0' && i <= MAXADD; i++);
		seclen = i;
	} else
		seclen = 0;
	if (firstlen > MAXADD || seclen > MAXADD) {
		errorf("Error: Input too long for addition (max %d decimals).", MAXADD);
		return 0;
	}
	
	if (first != 0) {
		while (first[0] == '0')
			first++, firstlen--;
	}
	if (second != 0) {
		while (second[0] == '0')
			second++, seclen--;
	}
	
	if (firstlen == 0) {
		if (seclen == 0) {
			if ((p = malloc(2)) == 0) {
				errorf("Error: malloc failed");
				return p;
			}
			p[0] = '0', p[1] = '\0';
			return p;
		} else {
			if ((p = malloc(seclen+1)) == 0) {
				errorf("Error: malloc failed");
				return p;
			}
			for (i = 0; i <= seclen; i++)
				p[i] = second[i];
			return p;
		}
	} else if (seclen == 0) {
		if ((p = malloc(firstlen+1)) == 0) {
			errorf("Error: malloc failed");
			return p;
		}
		for (i = 0; i <= firstlen; i++)
			p[i] = first[i];
		return p;
	}
	
	if (firstlen > seclen) {
		temp = malloc(firstlen+1);
		i = firstlen-1;
	}
	else {
		temp = malloc(seclen+1);
		i = seclen-1;
	}
	if (temp == 0) {
		errorf("Error: malloc failed");
		return temp;
	}
	
	for (temp[i+1] = '\0', oldi = i, firstlen--, seclen--, buffer = 0; i >= 0; i--) {
		if (firstlen >= 0) {
			if (seclen >= 0) {
				buffer = first[firstlen--] - '0' + second[seclen--] - '0' + buffer ;
				temp[i] = '0' + buffer % 10;
				buffer /= 10; 
			} else {
				buffer = first[firstlen--] - '0' + buffer;
				temp[i] = '0' + buffer % 10;
				buffer /= 10;
			}
		} else {
			buffer = second[seclen--] - '0' + buffer;
			temp[i] = '0' + buffer % 10;
			buffer /= 10;
		}
	}
	
	if (!buffer) {
		return temp;
	}
	
	if ((p = (char *) malloc(oldi+3)) == 0) {
		errorf("Error: malloc failed");
		free(temp);
		return p;
	}
	p[0] = '1';
	for (i = 0; temp[i] != '\0';) {
		p[++i] = temp[i];
	}
	p[++i] = '\0';
	free(temp);
	return p;
}

char *subtract(char *first, char *second) {
	int i, j, firstlen, seclen, buffer = 0, lcopy, fmlen, smlen;
	char *p, *temp, negative = 0;
	
	if (first != 0) {
		for (fmlen = 0; first[0] == '-'; first++, fmlen++);
	} if (second != 0) {
		for (smlen = 0; second[0] == '-'; second++, smlen++);
	}
	
	if (fmlen % 2) {
		if (smlen % 2) {
			p = subtract(second, first);
			return p;
		} else {
			if ((temp = add(first, second)) == 0)
				return temp;
			p = subtract("0", temp);
			free(temp);
			return p;
		}
	} else if (smlen % 2) {
		p = add(first, second);
		return p;
	}
	
	if (first != 0) {
		for (i = 0; first[i] != '\0' && i <= MAXSUB; i++);
		firstlen = i;
	} else
		firstlen = 0;
	if (second != 0) {
		for (i = 0; second[i] != '\0' && i <= MAXSUB; i++);
		seclen = i;
	} else
		seclen = 0;
	if (firstlen > MAXSUB || seclen > MAXSUB) {
		errorf("Error: Input too long for subtraction (max %d decimals).", MAXSUB);
		return 0;
	}
	
	if (first != 0) {
		while (first[0] == '0')
			first++, firstlen--;
	}
	if (second != 0) {
		while (second[0] == '0')
			second++, seclen--;
	}
	
	if (firstlen == 0) {
		if (seclen == 0) {
			if ((p = malloc(2)) == 0) {
				errorf("Error: malloc failed");
				return p;
			}
			p[0] = '0', p[1] = '\0';
			return p;
		} else {
			if ((p = malloc(seclen+2)) == 0) {
				errorf("Error: malloc failed");
				return p;
			}
			p[0] = '-';
			for (i = 0; i <= seclen; i++)
				p[i+1] = second[i];
			return p;
		}
	} else if (seclen == 0) {
		if ((p = malloc(firstlen+1)) == 0) {
			errorf("Error: malloc failed");
			return p;
		}
		for (i = 0; i <= firstlen; i++)
			p[i] = first[i];
		return p;
	}
		
	if (seclen > firstlen) {		// a numscmp
		negative = 1;
	} else if (seclen == firstlen) {
		for (i = 0; i < seclen; i++) {
			if (second[i] != first[i]) {
				if (second[i] > first[i]) {
					negative = 1;
					break;
				} else
					break;
			}
		}
		if (i == seclen) {
			if ((p = malloc(2)) == 0) {
				errorf("Error: malloc failed");
				return p;
			}
			p[0] = '0', p[1] = '\0';
			return p;
		}
	}
	
	if (negative) {				
		int tlen;
		char *ptemp;
		tlen = firstlen;
		firstlen = seclen;
		seclen = tlen;
		ptemp = first;
		first = second;
		second = ptemp;
	}
	
	if ((temp = (char *) malloc(firstlen+1)) == 0) {
		errorf("Error: malloc failed");
		return temp;
	}	
	
	for (seclen--, lcopy = firstlen - 1; seclen >= 0; seclen--, lcopy--) {
		buffer = first[lcopy] - second[seclen] - buffer;
		if (buffer < 0)
			buffer = 20+buffer;
		temp[lcopy] = '0' + buffer % 10;
		buffer /= 10;
	}
	for (i = lcopy; i >= 0; i--)
		temp[i] = first[i];
	while (buffer) {
		if (lcopy == -1) {
			errorf("Error: subtraction in the negatives.");
			free(temp);
			return 0;
		}
		buffer = temp[lcopy] - '0' - buffer;
		if (buffer < 0)
			buffer = 20+buffer;
		temp[(lcopy--)] = '0' + buffer % 10;
		buffer /= 10;
	}
	
	for (i = 0; temp[i] == '0'; i++);
	if (!negative && i == 0) {	// the amount of numbers is more likely to vary with negative numbers anyway
		temp[firstlen] = '\0';
		return temp;
	}
	
	if ((p = (char *) malloc(firstlen+1-i+negative)) == 0) {
		errorf("Error: malloc failed");
		free(temp);
		return p;
	}
	p[0] = '-';
	for (j = i-negative; i <= firstlen-j; i++)
		p[i-j] = temp[i];
	p[i-j] = '\0';		// note: termination is important
	free(temp);
	return p;
}

char *multiply(char *first, char *second) {
	int i, j, k, firstlen, seclen, buffer = 0, fmlen, smlen;
	char *p, *temp, *temp2;
	
	if (first != 0) {
		for (fmlen = 0; first[0] == '-'; first++, fmlen++);
	} if (second != 0) {
		for (smlen = 0; second[0] == '-'; second++, smlen++);
	}
	
	if ((fmlen % 2) != (smlen % 2)) {
		if ((temp = multiply(first, second)) == 0)
			return temp;
		p = subtract("0", temp);
		free(temp);
		return p;
	}
	
	if (first != 0) {
		for (i = 0; first[i] != '\0' && i <= MAXMUL; i++);
		firstlen = i;
	} else
		firstlen = 0;
	if (second != 0) {
		for (i = 0; second[i] != '\0' && i <= MAXMUL; i++);
		seclen = i;
	} else
		seclen = 0;
	if (firstlen > MAXMUL || seclen > MAXMUL) {
		errorf("Error: Input too long for multiplication (max %d decimals).", MAXMUL);
		return 0;
	}
	
	if (first != 0) {
		while (first[0] == '0')
			first++, firstlen--;
	}
	if (second != 0) {
		while (second[0] == '0')
			second++, seclen--;
	}
	
	if (firstlen == 0 || seclen == 0) {
		if ((p = (char *) malloc(2)) == 0) {
			errorf("Error: malloc failed");
			return p;
		}
		p[0] = '0', p[1] = '\0';
		return p;
	}
	
	if (firstlen == 1 && first[0] == '1') {
		if ((p = malloc(seclen+1)) == 0) {
			errorf("Error: malloc failed");
			return p;
		}
		for (i = 0; i <= seclen; i++)
			p[i] = second[i];
		return p;
	} else if (seclen == 1 && second[0] == '1') {
		if ((p = malloc(firstlen+1)) == 0) {
			errorf("Error: malloc failed");
			return p;
		}
		for (i = 0; i <= firstlen; i++)
			p[i] = first[i];
		return p;
	}
	
	if (seclen > firstlen) {				
		int lentemp;
		char *ptemp;
		lentemp = firstlen;
		firstlen = seclen;
		seclen = lentemp;
		ptemp = first;
		first = second;
		second = ptemp;
	}
	
	if ((temp = malloc(firstlen + seclen + 1)) == 0) { 		// firstlen-1 + seclen-1 + 3 (x = a*10^(firstlen-1) * b*10^(seclen-1) = a*b*10^(firstlen-1+seclen-1) = a*b*10^(firstlen+seclen-2)
		errorf("Error: malloc failed");						// amount of decimals is the order of magnitude plus 1 a*b is less than 100 so at most one extra decimal is added. One space is reserved for terminating null character)
		return temp;
	}
	if ((temp2 = malloc(firstlen+1)) == 0) {
		errorf("Error: malloc failed");	
		free(temp);
		return temp2;
	}
	for (i = firstlen+seclen-1; i >= 0; i--)
		temp[i] = '0';
	 temp[firstlen+seclen] = '\0';
	
	for (i = seclen-1; i >= 0; i--) {
		for (j = firstlen-1; j >= 0; j--) {
			buffer = (second[i] - '0')*(first[j] - '0')+buffer;
			temp2[j+1] = '0' + buffer % 10;
			buffer /= 10;
		}
		temp2[(j--)+1] = '0' + buffer % 10;
		buffer /= 10;
		
		for (k = firstlen-1; k > j; k--) {
			buffer = temp[k+i+1] - '0' + temp2[k+1] - '0' + buffer;
			temp[k+i+1] = '0' + buffer % 10;
			buffer /= 10;
		}
		if (buffer) {
			temp[k+i+1] = '0' + buffer % 10;
			buffer /= 10;
		}
	}
	free(temp2);
	if (temp[0] != '0')
		return temp;
	
	if ((p = malloc(firstlen+seclen)) == 0) {
		errorf("Error: malloc failed");
		free(temp);
		return p;
	}
	for (i = firstlen+seclen-1; i >= 0; i--)
		p[i] = temp[i+1];
	free(temp);
	return p;
}

char *divide(char *first, char *second) {
	int i, j, firstlen, seclen, firstdec, dec, mbuf, remlen, fmlen, smlen;
	char *p, *temp, *remainder, *subbuf, negative, rmndr0;
	
	if (first != 0) {
		for (fmlen = 0; first[0] == '-'; first++, fmlen++);
	} if (second != 0) {
		for (smlen = 0; second[0] == '-'; second++, smlen++);
	}
	
	if ((fmlen % 2) != (smlen % 2)) {
		if ((temp = divide(first, second)) == 0)
			return temp;
		p = subtract("0", temp);
		free(temp);
		return p;
	}
	
	if (first != 0) {
		for (i = 0; first[i] != '\0' && i <= MAXDIVD; i++);
		firstlen = i;
	} else
		firstlen = 0;
	if (second != 0) {
		for (i = 0; second[i] != '\0' && i <= MAXDIVS; i++);
		seclen = i;
	} else
		seclen = 0;
	if (firstlen > MAXDIVD || seclen > MAXDIVS) {
		errorf("Error: Input too long for division (max %d decimals divided and %d decimals divisor).", MAXDIVD, MAXDIVS);
		return 0;
	}
	
	
	if (first != 0) {
		while (first[0] == '0')
			first++, firstlen--;
	}
	if (second != 0) {
		while (second[0] == '0')
			second++, seclen--;
	}
	
	if (seclen == 0) {
		errorf("Error: divide by zero");
		return 0;
	} else if (firstlen == 0) {
		if ((p = malloc(2+DIVFRCTN+!!DIVFRCTN)) == 0) {
			errorf("Error: malloc failed");	
			return p;
		}
		p[0] = '0', p[1+DIVFRCTN+!!DIVFRCTN] = '\0';
		if (DIVFRCTN) {
			p[1] = '.';
			for (i = 1 + DIVFRCTN; i > 1; i--)
				p[i] = '0';
		}
		return p;
	}
	
	if (seclen == 1 && second[0] == '1') {
		if ((p = malloc(firstlen+1+!!DIVFRCTN+DIVFRCTN)) == 0) {
			errorf("Error: malloc failed");	
			return p;
		}
		for (i = 0; i <= firstlen; i++)
			p[i] = first[i];
		if (DIVFRCTN) {
			p[i-1] = '.';
			for (; i < firstlen+DIVFRCTN+1; i++)
				p[i] = '0';
			p[i] = '\0';
		}
		return p;
	}
	
	firstdec = dec = firstlen - seclen;		// firstdec is the highest order of magnitude the result can have (a/b * 10^(x-y))
	
	if (firstdec < -DIVFRCTN-DIVRNDNG) {
		if ((p = malloc(2+DIVFRCTN+!!DIVFRCTN)) == 0) {
			errorf("Error: malloc failed");	
			return p;
		}
		p[0] = '0', p[1+DIVFRCTN+!!DIVFRCTN] = '\0';
		if (DIVFRCTN) {
			p[1] = '.';
			for (i = 1 + DIVFRCTN; i > 1; i--)
				p[i] = '0';
		}
		return p;
	} else if ((firstdec == -DIVFRCTN) && !(DIVRNDNG)) {
		for (i = 0; i < seclen+firstdec; i++) {	// numscmp
			if (second[i] != first[i]) {
				if (second[i] > first[i]) {
					if ((p = malloc(2+DIVFRCTN+!!DIVFRCTN)) == 0) {
						errorf("Error: malloc failed");	
						return p;
					}
					p[0] = '0', p[1+DIVFRCTN+!!DIVFRCTN] = '\0';
					if (DIVFRCTN) {
						p[1] = '.';
						for (i = 1 + DIVFRCTN; i > 1; i--)
							p[i] = '0';
					}
					return p;
				} else
					break;
			}
		}
		if (i == seclen+firstdec) {
			while (i < seclen && second[i] == '0')
				i++;
			if (i == seclen) {
				if ((p = malloc(2)) == 0) {
					errorf("Error: malloc failed");	
					return p;
				}
				p[0] = '0', p[!!DIVFRCTN+DIVFRCTN] = '1',  p[1+DIVFRCTN+!!DIVFRCTN] = '\0';
				if (DIVFRCTN) {
					p[1] = '.';
					for (i = DIVFRCTN; i > 1; i--)
						p[i] = '0';
				}
				return p;
			} else {
				if ((p = malloc(2+DIVFRCTN+!!DIVFRCTN)) == 0) {
					errorf("Error: malloc failed");	
					return p;
				}
				p[0] = '0', p[1+DIVFRCTN+!!DIVFRCTN] = '\0';
				if (DIVFRCTN) {
					p[1] = '.';
					for (i = 1 + DIVFRCTN; i > 1; i--)
						p[i] = '0';
				}
				return p;
				
			}
		}
	}
	
	if (firstdec >= 0) {
		remlen = firstlen+1;	// guaranteed to fit if multiplied by 10 (fits more than 0.1 times), therefore extra number at the left is guaranteed to be 0 after subtracting the quotient
	} else
		remlen = seclen+1;		// fits  0.1 or less times therefore an extra number at the left of the first number isn't enough space since it might need to be multiplied by 10 multiple times
	if ((remainder = malloc(remlen)) == 0) {
		errorf("Error: malloc failed");
		return remainder;
	}
	remainder[0] = '0';
	for (i = 1; i <= firstlen; i++) {
		remainder[i] = first[i-1];		// the decimal place is figured out later
	}
	while (i < remlen)
		remainder[i++] = '0';
	
	if ((temp = malloc(firstdec+1+DIVFRCTN+DIVRNDNG)) == 0) {
		errorf("Error: malloc failed");
		free(remainder);
		return temp;
	} if ((subbuf = malloc(seclen + 1)) == 0) {		// in which how many times the divisor fits in the remainder is tested
		errorf("Error: malloc failed");
		free(remainder);
		free(temp);
		return subbuf;
	}
	
	rmndr0 = 0;
	while (dec >= 0-DIVFRCTN-DIVRNDNG) {
		if (rmndr0) {	// found the last decimal
			temp[firstdec-dec] = '0';
			dec--;
			continue;
		}
		if (remainder[0] == '0') {
			i = (remainder[1] - '0') / (second[0] - '0');	// approximating the maximum times the divisor could fit in the remainder
		} else {
			if (seclen == 1)
				i = (10*(remainder[0] - '0') + (remainder[1] - '0')) / (second[0] - '0');	// in this case it should be exact
			else
				i = (100*(remainder[0] - '0') + 10*(remainder[1] - '0')) / (10*(second[0] - '0') + (second[1] - '0'));	// the maximum should be 10. if the divisor is 37999 then the remainder could be 37998 at max and turn into 379980 in which 37 would fit into 370 ten times.
		}
		
		if (i > 9) {
			if (i > 10) {
				errorf("Error: remainder too large");
				free(remainder);
				free(subbuf);
				free(temp);
				return 0;
			} else
				i = 9;
		}
		
		negative = 1;
		while (i > 0) {
			for (j = seclen, mbuf = 0; j > 0; j--) {	// divisor is multiplied by i into the subbuf
				mbuf = (second[j-1] - '0')*(i)+mbuf;
				subbuf[j] = '0' + mbuf % 10;
				mbuf /= 10;
			}
			subbuf[j] = '0' + mbuf % 10;
			
			for (j = 0; j <= seclen; j++) {
				if (subbuf[j] != remainder[j]) {		// subbuf is compared with the remainder
					if (subbuf[j] > remainder[j])
						break;
					else {
						negative = 0;
						break;
					}
				}
			} if (j == seclen+1) {
				negative = 0;
				while (j < remlen && remainder[j] == '0')
					j++;
				if (j == remlen) {
					rmndr0 = 1;
				}
			} if (!negative)
				break;
			i--;
		}
		
		temp[firstdec-dec] = '0' + i;
		if (i) {
			for (j = seclen, mbuf = 0; j >= 0; j--) {	// the remainder is reduced
				mbuf = remainder[j] - subbuf[j] - mbuf;
				if (mbuf < 0)
					mbuf = 20+mbuf;
				remainder[j] = '0' + mbuf % 10;
				mbuf /= 10;
			}
			if (mbuf) {
				errorf("Error: subtraction in the negatives.");
				free(remainder);
				free(subbuf);
				free(temp);
				return 0;
			}
		}
		if (remainder[0] != '0') {
			errorf("Error: remainder too large");
			free(remainder);	
			free(temp);
			free(subbuf);
			return 0;
		}
		
		for (i = 0; i < remlen-1; i++) {
			remainder[i] = remainder[i+1];
		}
		remainder[remlen-1] = '0';
		dec--;
	}
	mbuf = 0;
	free(remainder);
	free(subbuf);
	
	if (DIVRNDNG) {
		if (temp[firstdec+1+DIVFRCTN] >= '5') {
			mbuf = 1;
			for (i = firstdec+DIVFRCTN; i >= 0; i--) {
				if (temp[i] < '9') {
					temp[i]++;
					mbuf = 0;
					break;
				} else
					temp[i] = '0';	
			}
		}
	}
	if (firstdec >= 0) {
		if (!mbuf) {	// (for example when 99 is divided by 10 and 9.9 is rounded to 10)
			for (i = 0; temp[i] == '0' && i < firstdec; i++);		// change to i <= firstdec to shave off a single zero before a decimal point
		}
		if ((p = malloc(firstdec+1-i+DIVFRCTN+(!!DIVFRCTN)+mbuf+1)) == 0) {
			errorf("Error: malloc failed.");
			free(temp);
			return p;
		}
		
		p[firstdec+1-i+DIVFRCTN+(!!DIVFRCTN)+mbuf] = '\0';
		if (mbuf) {
			i = 0;
			p[0] = '1';
		}
		
		for (j = 0;  j <= firstdec-i && (p[j+mbuf] = temp[j+i]); j++);
		if (DIVFRCTN) {
			p[(j++)+mbuf] = '.';
			for (dec = 1; (p[j+mbuf] = temp[j+i-1]) && dec < DIVFRCTN; dec++, j++);		
		}
	} else {
		if ((p = malloc(DIVFRCTN+3)) == 0) {
			errorf("Error: malloc failed.");
			free(temp);
			return p;
		}
		
		p[DIVFRCTN+2] = '\0';
		p[0] = '0', p[1] = '.';
		
		for (i = 2; i <= -firstdec;)
			p[i++] = '0';
		
		if (mbuf) {
			if (p[i-1] == '.')
				p[i-2] = '1';
			else
				p[i-1] = '1';
		}
		while (i <= DIVFRCTN+1)
			p[i++] = temp[i+firstdec-1];
	}
	
	free(temp);
	return p;
}

char *exponent(char *first, char *second) {
	
	int i, j, firstlen, seclen, mpad;
	char *p, *temp, *scopy, *firstp;
	
	if (second != 0) {
		for (mpad = 0; second[0] == '-'; second++, mpad++);
	}
	
	if (mpad % 2) {
		if ((temp = exponent(first, second)) == 0)
			return temp;
		p = divide("1", temp);
		free(temp);
		return p;
	}
	
	if (first != 0) {
		for (i = 0; first[i] != '\0' && i <= MAXMUL; i++);
		firstlen = i;
	} else
		firstlen = 0;
	if (second != 0) {
		for (i = 0; second[i] != '\0' && i <= MAXEXP; i++);
		seclen = i;
	} else
		seclen = 0;
	if (firstlen > MAXMUL || seclen > MAXEXP) {
		errorf("Error: Input too long for exponentiation (max %d decimals divided and %d decimals divisor).", MAXMUL, MAXEXP);
		return 0;
	}
	
	if (first != 0) {
		for (mpad = 0; first[0] == '-'; first++, mpad++, firstlen--);
		while (first[0] == '0')
			first++, firstlen--;
	} else
		mpad = 0, firstlen = 0;
	
	if (second != 0) {
		while (second[0] == '0')
			second++, seclen--;
	} else
		seclen = 0;
	
	if (seclen == 0) {
		if (firstlen == 0) {
			errorf("Error: divide by zero");
			return 0;
		} else {
			if ((p = (char *) malloc(2)) == 0) {
				errorf("Error: malloc failed");	
				return p;
			}
			p[0] = '1', p[1] = '\0';
			return p;
		}
	} else if (firstlen == 0) {
		if ((p = (char *) malloc(2)) == 0) {
			errorf("Error: malloc failed");	
			return p;
		}
		p[0] = '0', p[1] = '\0';
		return p;
	}
	
	if (seclen == 1 && second[0] == '1') {
		if ((p = malloc(firstlen+1+(mpad % 2))) == 0) {
			errorf("Error: malloc failed");
			return p;
		}
		p[0] = '-';
		for (i = 0, j = (mpad % 2); i <= firstlen; i++)
			p[i+j] = first[i];
		return p;
	} else if (firstlen == 1 && first[0] == '1') {
		if (second[seclen-1] % 2 == 0) {
			if ((p = (char *) malloc(2)) == 0) {
				errorf("Error: malloc failed");	
				return p;
			}
			p[0] = '1', p[1] = '\0';
			return p;
		} else {
			if ((p = (char *) malloc(2+(mpad % 2))) == 0) {
				errorf("Error: malloc failed");	
				return p;
			}
			p[0] = '-', p[0+(mpad % 2)] = '1', p[1+(mpad % 2)] = '\0';
			return p;
		}
	}
	
	if ((scopy = malloc(seclen+1)) == 0) {
		errorf("Error: malloc failed");
		return scopy;
	}
	if ((p = malloc(2)) == 0) {
		errorf("Error: malloc failed");	
		free(scopy);
		return p;
	} p[0] = '1', p[1] = '\0';
	
	for (i = 0; i <= seclen; i++) {
		scopy[i] = second[i];
	}
	
	while(1) {
		for (i = seclen-1; i >= 0; i--) {
			if (scopy[i] >= '1' && scopy[i] <= '9') {	// decrementing scopy
				scopy[i]--;
				break;
			} else if (scopy[i] == '0') {
				scopy[i] = '9';
			} else {
				errorf("Error: invalid number in power");
				free(scopy);
				return 0;
			}
		}
		if (i == -1)
			break;
		else {
			if ((temp = multiply(p, first)) == 0) {
				free(p);
				free(scopy);
				return temp;
			}
			free(p);			
			p = temp;			
		}
	}
	free(scopy);
	if ((mpad % 2) && (second[seclen-1] % 2)) {
		temp = subtract("0", p);
		free(p);
		p = temp;
	}
	return p;
}

unsigned char nscmp(unsigned char *first, unsigned char *second) {		// compares numeric strings -- returns 1 if first number is bigger or 2 if second number is bigger or 0 if they are equal //* could use testing
	int i, firstlen, seclen, fmlen, smlen;
	char temp;
	
	if (first != 0) {
		for (fmlen = 0; first[0] == '-'; first++, fmlen++);
	} if (second != 0) {
		for (smlen = 0; second[0] == '-'; second++, smlen++);
	}
	
	if (fmlen % 2) {
		if (smlen % 2) {
			temp = nscmp(first, second);
			if (temp == 0)
				return temp;
			if (temp == 1)
				return 2;
			if (temp == 2)
				return 1;
			return 3;
		} else {
			return 2;
		}
	} else if (smlen % 2) {
		return 1;
	}
	
	if (first != 0) {
		for (i = 0; first[i] != '\0' && i <= MAXADD; i++);
		firstlen = i;
	} else
		firstlen = 0;
	if (second != 0) {
		for (i = 0; second[i] != '\0' && i <= MAXADD; i++);
		seclen = i;
	} else
		seclen = 0;
	if (firstlen > MAXADD || seclen > MAXADD) {
		errorf("Error: Input too long for addition (max %d decimals).", MAXADD);
		return 0;
	}
	
	if (first != 0) {
		while (first[0] == '0')
			first++, firstlen--;
	}
	if (second != 0) {
		while (second[0] == '0')
			second++, seclen--;
	}
	
	if (firstlen == 0) {
		if (seclen == 0)
			return 0;
		else
			return 2;
	} else if (seclen == 0) {
		return 1;
	}
	
	if (firstlen > seclen)
		return 1;
	else if (seclen > firstlen)
		return 2;
	else {
		for (i = 0; i < firstlen; i++) {
			if (first[i] > second[i])
				return 1;
			if (second[i] > first[i])
				return 2;
		}
		return 0;
	}
}

char *btoc(unsigned char* num) {
	int mlen, len, i;
	char *temp, *p, *buf;
	
	if (num == 0) {
		errorf("Error: no number to convert");
		return 0;
	}
	
	for (mlen = 0; num[0] == 0; num++, mlen++);
	len = (num++)[0];
	while (len > 0 && num[0] == 0)
			num++, len--;
	
	if (len == 0) {
		if ((p = malloc(2)) == 0) {
			errorf("Error: malloc failed.");
			return p;
		}
		p[0] = '0', p[1] = '\0';
		return p;
	}
	
	if ((p = malloc(2)) == 0) {
		errorf("Error: malloc failed.");
		return p;
	}
	p[0] = '0', p[1] = '\0';
	if ((buf = malloc(4)) == 0) {
		errorf("Error: malloc failed.");
		free(p);
		return buf;
	} buf[3] = '\0';	
	
	i = 0;
	while (i < len) {
		buf[0] = '0' + (num[i]/100), buf[1] = '0' + (num[i]/10%10), buf[2] = '0' + (num[i]%10);
		if ((temp = multiply(p, "256")) == 0) {
			free(p);
			free(buf);
			return temp;			
		}
		free(p);
		if ((p = add(temp, buf)) == 0) {
			free(temp);
			free(buf);
			return p;
		}
		free(temp);
		i++;
	} free(buf);
	
	if (mlen % 2) {
		temp = subtract("0", p);
		free(p);
		p = temp;
	}	
	return p;
}

char *itoc(long long num) {		//* could use testing
	int len, i;
	char *p, neg, min;
	long long temp;
	
	if (num == 0) {
		p = malloc(2);
		p[0] = '0', p[1] = '\0';
		return p;
	}
	if (num < 0) {
		neg = 1;
		if (num == -9223372036854775808ULL) {
			num++;
			min = 1;
		} else
			min = 0;
		num = -num;
	} else {
		neg = 0;
		min = 0;
	}
	
	for (temp = num, len = 0; temp > 0; len++, temp /= 10);
	
	if ((p = malloc(len+neg+1)) == 0) {
		errorf("Error: malloc failed.");
		return p;
	}
	p[len+neg] = '\0', p[0] = '-';
	
	for (i = len+neg-1; i >= neg; i--)
		p[i] = '0' + num % 10, num /= 10;
	
	if (min) {
		p[len+neg-1]++;
		if (p[len+neg-1] > '9') {
			free(p);
			return 0;
		}
	}
	return p;
}

char *utoc(unsigned long long num) {
	int len, i;
	char *p;
	long long temp;
	
	if (num == 0) {
		p = malloc(2);
		p[0] = '0', p[1] = '\0';
		return p;
	}
	
	for (temp = num, len = 0; temp > 0; len++, temp /= 10);
	
	if ((p = malloc(len+1)) == 0) {
		errorf("Error: malloc failed.");
		return p;
	}
	p[len] = '\0';
	
	for (i = len-1; i >= 0; i--)
		p[i] = '0' + num % 10, num /= 10;
	
	return p;
}

long long ctoi(char *num) {		//* could use testing esp. with numbers at the limit
	int mlen;
	char cmp;
	long long result;
	
	if (num == 0) {
		errorf("Error: no number to convert");
		return 0;
	}
	for (mlen = 0; num[0] == '-'; num++, mlen++);
	while (num[0] == '0')
		num++;
	
	if ((cmp = nscmp(num, "-9223372036854775808")) == 2)
		return 0;
	if (cmp == 0)
		return -9223372036854775808ULL;
	if ((cmp = nscmp(num, "9223372036854775807")) == 1)
		return 0;
	if (cmp == 0)
		return 9223372036854775807LL;
	
	for (result = 0; num[0] != '\0'; num++) {
		result *= 10;
		result += num[0] - '0';
	}
	
	if (mlen % 2) {
		result *= -1;
	}
	return result;
}

unsigned long long ctou(char *num) {
	int mlen;
	char cmp;
	unsigned long long result;
	
	if (num == 0) {
		errorf("Error: no number to convert");
		return 0;
	}
	for (mlen = 0; num[0] == '-'; num++, mlen++);
	if (mlen != 0)
		return 0;
	while (num[0] == '0')
		num++;
	
	if ((cmp = nscmp(num, "18446744073709551615")) == 1)
		return 0;
	if (cmp == 0)
		return 18446744073709551615ULL;
	
	for (result = 0; num[0] != '\0'; num++) {
		result *= 10;
		result += num[0] - '0';
	}
	
	return result;
}

unsigned char checknum(char *string) {
	int i, maxlen;
	
	while (string[0] == '-')
		string++;
	
	maxlen = MAXADD;
	if (MAXSUB > maxlen)
		maxlen = MAXSUB;
	if (MAXMUL > maxlen)
		maxlen = MAXMUL;
	if (MAXDIVD > maxlen)
		maxlen = MAXDIVD;
	if (MAXDIVS > maxlen)
		maxlen = MAXDIVS;
	for (i = 0; string[0] != '\0' && ++i <= maxlen; string++) {
		if (string[0] == '.')
			break;
		if (string[0] < '0' || string[0] > '9') {
			return 0;			
		}
	}
	if (string[0] == '.' && i <= maxlen) {
		string++;
		while (string[0] != '\0' && ++i <= maxlen) {
			if (string[0] < '0' || string[0] > '9') {
				return 0;			
			}
			string++;
		}
		return 2;
	}
	if (i > maxlen)
		return 0;
	
	return 1;	
}

char *clnnum(char *num) {
	int mlen, len, maxlen, i;
	char *p;
	
	while (num[0] > '9' && num[0] < '0' && num[0] != '-' && num[0] != '.')
		num++;
	while (num[0] == '-' || num[0] == '+')
		if (num[0] == '-')
			mlen++, num++;
		else 
			num++;
	while (num[0] == '0')
		num++;
	
	
	maxlen = MAXADD;
	if (MAXSUB > maxlen)
		maxlen = MAXSUB;
	if (MAXMUL > maxlen)
		maxlen = MAXMUL;
	if (MAXDIVD > maxlen)
		maxlen = MAXDIVD;
	if (MAXDIVS > maxlen)
		maxlen = MAXDIVS;
	
	for (len = -1; num[len+1] != '\0' && ++len < maxlen;) {
		if (num[len] == '.')
			break;
		if (num[len] > '9' && num[len] < '0') {
			errorf("Error: not a number."); 
			return 0;
		}
	}
	
	if (num[len] == '.') {
		for (; num[len+1] != '\0' && ++len < maxlen;) {
			if (num[len] > '9' && num[len] < '0') {
				errorf("Error: not a number."); 
				return 0;
			}
		}
	}
	if (len >= maxlen) {
		errorf("Error: number too long.");
		return 0;		
	}
	if ((p = malloc(len + 1 + (mlen % 2))) == 0) {
		errorf("Error: malloc failed.");
		return p;
	}
	p[0] = '-';
	for (i = 0; i >= len; i++)
		p[i+(mlen%2)] =  num[i];
	return p;
}

