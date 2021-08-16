#include <stdlib.h>

#include "errorf.h"
#define errorf(...) g_errorf(__VA_ARGS__)

unsigned char *bsubtract(unsigned char*, unsigned char*);

unsigned char *badd(unsigned char *first, unsigned char *second) {			// most functions return allocated memory that must be freed
	int i, oldi, firstlen, seclen, fzlen, szlen, buffer;
	unsigned char *temp, *p;
	
	if (first != 0) {
		for (fzlen = 0; first[0] == 0; first++, fzlen++);
	} if (second != 0) {
		for (szlen = 0; second[0] == 0; second++, szlen++);
	}

	if (fzlen % 2) {
		if (szlen % 2) {
			if ((temp = badd(first, second)) == 0)
				return temp;
			p = bsubtract("\x1\x0", temp);
			free(temp);
			return p;
		} else {
			p = bsubtract(second, first);
			return p;
		}
	} else if (szlen % 2) {
		p = bsubtract(first, second);
		return p;
	}
	
	if (first) {
		firstlen = (first++)[0];
		while (firstlen > 0 && first[0] == 0)
			first++, firstlen--;
	} else
		firstlen = 0;
	if (second) {
		seclen = (second++)[0];
		while (seclen > 0 && second[0] == 0)
			second++, seclen--;
	} else
		seclen = 0;
	
	if (firstlen > 255 || seclen > 255) {
		errorf("Error: binary addition input length error.");
		return 0;
	}
	
	if (firstlen == 0) {
		if (seclen == 0) {
			if ((p = malloc(2)) == 0) {
				errorf("Error: malloc failed");
				return p;
			}
			p[0] = 1, p[1] = 0;
			return p;
		} else {
			if ((p = malloc(seclen+1)) == 0) {
				errorf("Error: malloc failed");
				return p;
			}
			p[0] = seclen;
			for (i = 0; i < seclen; i++)
				p[i+1] = second[i];
			return p;
		}
	} else if (seclen == 0) {
		if ((p = malloc(firstlen+1)) == 0) {
			errorf("Error: malloc failed");
			return p;
		}
		p[0] = firstlen;
		for (i = 0; i < firstlen; i++)
			p[i+1] = first[i];
		return p;
	}
	
	if (firstlen > seclen) {
		temp = malloc(firstlen+1);
		i = firstlen;
	}
	else {
		temp = malloc(seclen+1);
		i = seclen;
	}
	if (temp == 0) {
		errorf("Error: malloc failed");
		return temp;
	}
	
	for (oldi = i, firstlen--, seclen--, buffer = 0; i > 0; i--) {
		if (firstlen >= 0) {
			if (seclen >= 0) {
				buffer = first[firstlen--] + second[seclen--] + buffer;
				temp[i] = buffer % 256;
				buffer /= 256; 
			} else {
				buffer = first[firstlen--] + buffer;
				temp[i] = buffer % 256;
				buffer /= 256;
			}
		} else if (seclen >= 0) {
			buffer = second[seclen--] + buffer;
			temp[i] = buffer % 256;
			buffer /= 256;
		}
	}
	
	if (!buffer) {
		temp[0] = oldi;
		return temp;
	}
	
	p = malloc(oldi+2);
	p[0] = oldi+1, p[1] = 1;
	
	for (i = 1; i <= oldi;) {
		p[++i] = temp[i];
	}	
	free(temp);
	return p;
}

unsigned char *bsubtract(unsigned char *first, unsigned char *second) {
	int i, j, firstlen, seclen, buffer = 0, lcopy, fzlen, szlen;
	unsigned char *p, *temp, negative = 0;
	
	if (first != 0) {
		for (fzlen = 0; first[0] == 0; first++, fzlen++);
	} if (second != 0) {
		for (szlen = 0; second[0] == 0; second++, szlen++);
	}
	
	if (fzlen % 2) {
		if (szlen % 2) {
			p = bsubtract(second, first);
			return p;
		} else {
			if ((temp = badd(first, second)) == 0)
				return temp;
			p = bsubtract("\x1\x0", temp);
			free(temp);
			return p;
		}
	} else if (szlen % 2) {
		p = badd(first, second);
		return p;
	}
	
	if (first) {
		firstlen = (first++)[0];
		while (firstlen > 0 && first[0] == 0)
			first++, firstlen--;
	} else
		firstlen = 0;
	if (second) {
		seclen = (second++)[0];
		while (seclen > 0 && second[0] == 0)
			second++, seclen--;
	} else
		seclen = 0;
	
	if (firstlen > 255 || seclen > 255) {
		errorf("Error: binary subtraction input length error.");
		return 0;
	}
	
	if (firstlen == 0) {
		if (seclen == 0) {
			if ((p = malloc(2)) == 0) {
				errorf("Error: malloc failed");
				return p;
			}
			p[0] = 1, p[1] = 0;
			return p;
		} else {
			if ((p = malloc(seclen+2)) == 0) {
				errorf("Error: malloc failed");
				return p;
			}
			p[0] = 0, p[1] = seclen;
			for (i = 0; i < seclen; i++)
				p[i+2] = second[i];
			return p;
		}
	} else if (seclen == 0) {
		if ((p = malloc(firstlen+1)) == 0) {
			errorf("Error: malloc failed");
			return p;
		}
		p[0] = firstlen;
		for (i = 0; i < firstlen; i++)
			p[i+1] = first[i];
		return p;
	}
		
	if (seclen > firstlen) {
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
			p[0] = 1, p[1] = 0;
			return p;
		}
	}
	
	if (negative) {		
		int lentemp;
		unsigned char *ptemp;
		lentemp = firstlen;
		firstlen = seclen;
		seclen = lentemp;
		ptemp = first;
		first = second;
		second = ptemp;
	}
	
	if ((temp = (unsigned char *) malloc(firstlen+1)) == 0) {
		errorf("Error: malloc failed");
		return temp;
	}
	
	for (seclen--, lcopy = firstlen; seclen >= 0; seclen--, lcopy--) {
		buffer = first[lcopy-1] - second[seclen] - buffer;
		if (buffer < 0)
			buffer = 512+buffer;
		temp[lcopy] = buffer % 256;
		buffer /= 256;
	}
	for (i = lcopy; i > 0; i--)
		temp[i] = first[i-1];
	while (buffer) {
		if (lcopy == 0) {
			errorf("Error: subtraction in the negatives.");
			free(temp);
			return 0;
		}
		buffer = temp[lcopy] - buffer;
		if (buffer < 0)
			buffer = 512+buffer;
		temp[(lcopy--)] = buffer % 256;
		buffer /= 256;
	}
	
	for (i = 1; temp[i] == 0; i++);
	if (!negative && i == 1) {
		temp[0] = firstlen;
		return temp;
	}
	
	if ((p = (unsigned char *) malloc(firstlen+2-i+negative)) == 0) {
		errorf("Error: malloc failed");
		free(temp);
		return p;
	}
	p[0] = 0, p[0+negative] = firstlen-i+1;
	for (j = i-negative-1; i <= firstlen; i++)
		p[i-j] = temp[i];
	free(temp);
	return p;

}

unsigned char *bmultiply(unsigned char *first, unsigned char *second) {
	int i, j, k, firstlen, seclen, buffer = 0, fzlen, szlen;
	unsigned char *p, *temp, *temp2;
	
	if (first != 0) {
		for (fzlen = 0; first[0] == 0; first++, fzlen++);
	} if (second != 0) {
		for (szlen = 0; second[0] == 0; second++, szlen++);
	}
	
	if ((fzlen % 2) != (szlen % 2)) {
		if ((temp = bmultiply(first, second)) == 0)
			return temp;
		p = bsubtract("\x1\x0", temp);
		free(temp);
		return p;
	}
	
	if (first) {
		firstlen = (first++)[0];
		while (firstlen > 0 && first[0] == 0)
			first++, firstlen--;
	} else
		firstlen = 0;
	if (second) {
		seclen = (second++)[0];
		while (seclen > 0 && second[0] == 0)
			second++, seclen--;
	} else
		seclen = 0;
	
	if (firstlen > 255 || seclen > 255) {
		errorf("Error: binary multiplication input length error.");
		return 0;
	}
	if (firstlen+seclen > 256) {
		errorf("Error: result too long");
		return 0;
	}
	
	if (firstlen == 0 || seclen == 0) {
		if ((p = (unsigned char *) malloc(2)) == 0) {
			errorf("Error: malloc failed");
			return p;
		}
		p[0] = 1, p[1] = 0;
		return p;
	}
	
	if (firstlen == 1 && first[0] == 1) {
		if ((p = malloc(seclen+1)) == 0) {
			errorf("Error: malloc failed");
			return p;
		}
		p[0] = seclen;
		for (i = 0; i < seclen; i++)
			p[i+1] = second[i];
		return p;
	} else if (seclen == 1 && second[0] == 1) {
		if ((p = malloc(firstlen+1)) == 0) {
			errorf("Error: malloc failed");
			return p;
		}
		p[0] = firstlen;
		for (i = 0; i < firstlen; i++)
			p[i+1] = first[i];
		return p;
	}
	
	if (seclen > firstlen) {		
		int lentemp;
		unsigned char *ptemp;
		lentemp = firstlen;
		firstlen = seclen;
		seclen = lentemp;
		ptemp = first;
		first = second;
		second = ptemp;
	}
	
	if ((temp = malloc(firstlen + seclen + 1)) == 0) { 		// firstlen-1 + seclen-1 + 3 (x = a*10^(firstlen-1) * b*10^(seclen-1) = a*b*10^(firstlen-1+seclen-1) = a*b*10^(firstlen+seclen-2)
		errorf("Error: malloc failed");
		return temp;
	} if ((temp2 = malloc(firstlen+1)) == 0) {
		errorf("Error: malloc failed");	
		free(temp);
		return temp2;
	}
	for (i = firstlen+seclen; i >= 1; i--)
		temp[i] = 0;
	 temp[0] = firstlen+seclen;
	
	for (i = seclen-1; i >= 0; i--) {	// decremented to 0 because the initial byte is faded away
		for (j = firstlen-1; j >= 0; j--) {
			buffer = (second[i])*(first[j])+buffer;
			temp2[j+1] = buffer % 256;
			buffer /= 256;
		}
		temp2[(j--)+1] = buffer % 256;
		buffer /= 256;
			
		for (k = firstlen-1; k > j; k--) {
			buffer = temp[k+i+2] + temp2[k+1] + buffer;
			temp[k+i+2] = buffer % 256;
			buffer /= 256;
		}
		if (buffer) {
			temp[k+i+2] = buffer % 256;
			buffer /= 256;
		}
	}
	free(temp2);
	if (temp[1] != 0) {
		if (firstlen+seclen > 255) {
			free(temp);
			return 0;
		}
		return temp;
	}
	
	if ((p = malloc(firstlen+seclen)) == 0) {
		errorf("Error: malloc failed");
		free(temp);
		return p;
	} p[0] = firstlen+seclen-1;
	for (i = firstlen+seclen-1; i >= 1; i--)
		p[i] = temp[i+1];
	
	free(temp);
	return p;
}

unsigned char *bdivide(unsigned char *first, unsigned char *second) {
	int i, j, firstlen, seclen, firstdec, dec, mbuf, remlen, fzlen, szlen;
	unsigned char *p, *temp, *remainder, *subbuf, negative, rmndr0;
	
	if (first != 0) {
		for (fzlen = 0; first[0] == 0; first++, fzlen++);
	} if (second != 0) {
		for (szlen = 0; second[0] == 0; second++, szlen++);
	}
	
	if ((fzlen % 2) != (szlen % 2)) {
		if ((temp = bdivide(first, second)) == 0)
			return temp;
		p = bsubtract("\x1\x0", temp);
		free(temp);
		return p;
	}
	
	if (first) {
		firstlen = (first++)[0];
		while (firstlen > 0 && first[0] == 0)
			first++, firstlen--;
	} else
		firstlen = 0;
	if (second) {
		seclen = (second++)[0];
		while (seclen > 0 && second[0] == 0)
			second++, seclen--;
	} else
		seclen = 0;
	
	if (firstlen > 255 || seclen > 255) {
		errorf("Error: binary division input length error.");
		return 0;
	}
	
	if (seclen == 0) {
		errorf("Error: divide by zero");
		return 0;
	} else if (firstlen == 0) {
		if ((p = malloc(2)) == 0) {
			errorf("Error: malloc failed");	
			return p;
		}
		p[0] = 1, p[1] = 0;
		return p;
	}
	
	if (seclen == 1 && second[0] == 1) {
		if ((p = malloc(firstlen+1)) == 0) {
			errorf("Error: malloc failed");	
			return p;
		}
		p[0] = firstlen;
		for (i = 0; i <= firstlen; i++)
			p[i+1] = first[i];
		return p;
	}
	
	firstdec = dec = firstlen - seclen;		// firstdec is the highest order of magnitude the result can have (a/b * 10^(x-y))
	
	if (firstdec < 0) {
		if ((p = malloc(2)) == 0) {
			errorf("Error: malloc failed");	
			return p;
		}
		p[0] = 1, p[1] = 0;
		return p;
	} else if (firstdec == 0) {
		for (i = 0; i < seclen; i++) {	// numscmp
			if (second[i] != first[i]) {
				if (second[i] > first[i]) {
					if ((p = malloc(2)) == 0) {
						errorf("Error: malloc failed");	
						return p;
					}
					p[0] = 1, p[1] = 0;
					return p;
				} else
					break;
			}
		}
		if (i == seclen) {
			if ((p = malloc(2)) == 0) {
				errorf("Error: malloc failed");	
				return p;
			}
			p[0] = 1, p[1] = 1;
			return p;
		}
	}
	remlen = firstlen+1;	// if seclen is bigger it evaluates to 0 before this
	
	if ((remainder = malloc(remlen)) == 0) {
		errorf("Error: malloc failed");
		return remainder;
	}
	remainder[0] = 0;
	for (i = 1; i <= firstlen; i++) {
		remainder[i] = first[i-1];		// the decimal place is figured out later
	}
	
	if ((temp = malloc(firstdec+1)) == 0) {
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
	while (dec >= 0) {
		if (rmndr0) {	// found the last decimal
			temp[firstdec-dec] = 0;
			dec--;
			continue;
		}		
		if (remainder[0] == 0) {
			i = (remainder[1]) / (second[0]);	// approximating the maximum times the divisor could fit in the remainder
		} else {
			if (seclen == 1)
				i = (256*remainder[0] + remainder[1]) / second[0];	// in this case it should be exact
			else
				i = (100*remainder[0] + 10*remainder[1]) / (10*second[0] + second[1]);	// the maximum should be 10. if the divisor is 37999 then the remainder could be 37998 at max and turn into 379980 in which 37 would fit into 370 ten times.
		}
		
		if (i > 255) {
			if (i > 256) {
				errorf("Error: remainder too large");
				free(remainder);
				free(subbuf);
				free(temp);
				return 0;
			} else
				i = 255;
		}
		
		negative = 1;
		while (i > 0) {
			for (j = seclen, mbuf = 0; j > 0; j--) {	// divisor is multiplied by i into the subbuf
				mbuf = second[j-1]*i+mbuf;
				subbuf[j] = mbuf % 256;
				mbuf /= 256;
			}
			subbuf[j] = 0 + mbuf % 256;
			
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
				while (j < remlen && remainder[j] == 0)
					j++;
				if (j == remlen) {
					rmndr0 = 1;
				}
			} if (!negative)
				break;
			i--;
		}
		
		temp[firstdec-dec] = i;
		if (i) {
			for (j = seclen, mbuf = 0; j >= 0; j--) {	// the remainder is reduced
				mbuf = remainder[j] - subbuf[j] - mbuf;
				if (mbuf < 0)
					mbuf = 512+mbuf;
				remainder[j] = mbuf % 256;
				mbuf /= 256;
			}
			if (mbuf) {
				errorf("Error: subtraction in the negatives.");
				free(remainder);
				free(subbuf);
				free(temp);
				return 0;
			}
		}
		if (remainder[0] != 0) {
			errorf("Error: remainder too large");
			free(remainder);	
			free(temp);
			free(subbuf);
			return 0;
		}
		
		for (i = 0; i < remlen-1; i++) {
			remainder[i] = remainder[i+1];
		}
		remainder[remlen-1] = 0;
		dec--;
	}
	free(remainder);
	free(subbuf);
	
	for (i = 0; temp[i] == 0 && i < firstdec; i++);		// change to i <= firstdec to shave off a single zero before a decimal point
	
	if ((p = malloc(firstdec+1-i+1)) == 0) {
		errorf("Error: malloc failed.");
		free(temp);
		return p;
	}
	p[0] = firstdec+1-i;
	
	for (j = 1;  j <= firstdec-i+1 && (p[j] = temp[j+i-1]); j++);
	
	free(temp);
	return p;
}

unsigned char *bexponent(unsigned char *first, unsigned char *second) {
	
	int i, j, firstlen, seclen, mpad;
	unsigned char *p, *temp, *scopy, *firstp;
	
	if (second != 0) {
		for (mpad = 0; second[0] == 0; second++, mpad++);
	}
	
	if (mpad % 2) {
		if ((temp = bexponent(first, second)) == 0)
			return temp;
		p = bdivide("\x1\x1", temp);
		free(temp);
		return p;
	}
	
	if (first) {
		for (mpad = 0; first[0] == 0; first++, mpad++);
		firstlen = first[0];
			for (i = 1; (i <= firstlen) && (first[i] == 0) ; i++);
		if (i == firstlen+1)
			firstlen = 0;
	} else
		firstlen = 0, mpad = 0;
	if (second) {
		seclen = (second++)[0];
		while (seclen > 0 && second[0] == 0)
			second++, seclen--;
	} else
		seclen = 0;
	
	if (firstlen > 255 || seclen > 255) {
		errorf("Error: binary exponentiation input length error.");
		return 0;
	}
	
	if (seclen == 0) {
		if (firstlen == 0) {
			errorf("Error: divide by zero");
			return 0;
		} else {
			if ((p = (unsigned char *) malloc(2)) == 0) {
				errorf("Error: malloc failed");	
				return p;
			}
			p[0] = 1, p[1] = 1;
			return p;
		}
	} else if (firstlen == 0) {
		if ((p = (unsigned char *) malloc(2)) == 0) {
			errorf("Error: malloc failed");	
			return p;
		}
		p[0] = 1, p[1] = 0;
		return p;
	}
	
	if (seclen == 1 && second[0] == 1) {
		if ((p = malloc(firstlen+1+(mpad % 2))) == 0) {
			errorf("Error: malloc failed");
			return p;
		}
		p[0] = 0, p[0+(mpad%2)] = firstlen;
		for (i = 1, j = (mpad % 2); i <= firstlen; i++)
			p[i+j] = first[i];
		return p;
	} else if (firstlen == 1 && first[1] == 1) {
		if (second[seclen-1] % 2 == 0) {
			if ((p = (unsigned char *) malloc(2)) == 0) {
				errorf("Error: malloc failed");	
				return p;
			}
			p[0] = 1, p[1] = 1;
			return p;
		} else {
			if ((p = (unsigned char *) malloc(2+(mpad % 2))) == 0) {
				errorf("Error: malloc failed");	
				return p;
			}
			p[0] = 0, p[0+(mpad % 2)] = 1, p[1+(mpad % 2)] = 1;
			return p;
		}
	}
	
	if ((scopy = malloc(seclen)) == 0) {
		errorf("Error: malloc failed");
		return scopy;
	}
	if ((p = malloc(2)) == 0) {
		errorf("Error: malloc failed");	
		free(scopy);
		return p;
	} p[0] = 1, p[1] = 1;
	
	for (i = 0; i < seclen; i++) {
		scopy[i] = second[i];
	}
	
	while(1) {
		for (i = seclen-1; i >= 0; i--) {
			if (scopy[i] != 0) {
				scopy[i]--;
				break;
			} else if (scopy[i] == 0) {
				scopy[i] = 255;
			} else {
				errorf("Error: exponent error");
				free(scopy);
				return 0;
			}
		}
		if (i == -1)
			break;
		else {
			if ((temp = bmultiply(p, first)) == 0) {
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
		temp = bsubtract("\x1\x0", p);
		free(p);
		p = temp;
	}
	return p;
}

unsigned char bnscmp(unsigned char *first, unsigned char *second) {		// compares numeric strings -- returns 1 if first number is bigger or 2 if second number is bigger or 0 if they are equal -- need to be unsigned or they can get counted as negative
	int i, firstlen, seclen, fzlen, szlen;
	char temp;
	
	if (first != 0) {
		for (fzlen = 0; first[0] == 0; first++, fzlen++);
	} if (second != 0) {
		for (szlen = 0; second[0] == 0; second++, szlen++);
	}
	
	if (fzlen % 2) {
		if (szlen % 2) {
			temp = bnscmp(first, second);
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
	} else if (szlen % 2) {
		return 1;
	}
	
	if (first) {
		firstlen = (first++)[0];
		while (firstlen > 0 && first[0] == 0)
			first++, firstlen--;
	} else
		firstlen = 0;
	if (second) {
		seclen = (second++)[0];
		while (seclen > 0 && second[0] == 0)
			second++, seclen--;
	} else
		seclen = 0;
	
	if (firstlen > 255 || seclen > 255) {
		errorf("Error: binary addition input length error.");
		return 0;
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

unsigned char *bmodulo(unsigned char *first, unsigned char *second) {
	unsigned char *p, *p2, *p3;
	
	p = bsubtract(first, (p3 = bmultiply((p2 = bdivide(first, second)), second)));
	free(p2), free(p3);
	return p;
}

unsigned char *bdivup(unsigned char *first, unsigned char *second) {
	unsigned char *p, *p2;
	p = bdivide(first, second);
	if (bnscmp((p2 = bmodulo(first, second)), "\x1\x0") == 1) {
		free(p2);
		p = badd((p2 = p), "\x1\x1");
		free(p2);
	}
	return p;
}

unsigned char* ctob(char *num) {
	unsigned char *p, *temp, *add;
	int mlen;
	
	if (num == 0) {
		errorf("Error: no number to convert");
		return 0;
	}
	for (mlen = 0; num[0] == '-'; num++, mlen++);
	while (num[0] == '0')
		num++;
	if ((p = malloc(2)) == 0) {
		errorf("Error: malloc failed.");
		return p;
	} p[0] = 1, p[1] = 0;
	if ((add = malloc(2)) == 0) {
		errorf("Error: malloc failed.");
		free(p);
		return add;
	} add[0] = 1, add[1] = 0;
	
	while (num[0] != '\0') {
		add[1] = (num[0] - '0');
		if ((temp = bmultiply(p, "\x1\xa")) == 0) {
			free(p);
			free(add);
			return temp;			
		}
		free(p);
		if ((p = badd(temp, add)) == 0) {
			free(temp);
			free(add);
			return p;
		} free(temp);
		num++;
	}
	free(add);
	
	if (mlen % 2) {
		temp = bsubtract("\x1\x0", p);
		free(p);
		p = temp;
	}
	return p;
}

unsigned char *itob(long long num) {		//* could use testing
	int len, i;
	unsigned char *p, neg, min;
	long long temp;
	
	if (num == 0) {
		p = malloc(2);
		p[0] = 1, p[1] = 0;
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
	
	for (temp = num, len = 0; temp > 0; len++, temp /= 256);
	
	if ((p = malloc(len+neg+1)) == 0) {
		errorf("Error: malloc failed.");
		return p;
	}
	p[0] = 0, p[neg] = len;
	
	for (i = len+neg; i > neg; i--)
		p[i] = num % 256, num /= 256;
	
	if (min) {
		if (p[len+neg-1] == 255) {
			free(p);
			return 0;
		}
		p[len+neg-1]++;
	}
	return p;
}

unsigned char *utob(unsigned long long num) {
	int len, i;
	unsigned char *p;
	long long temp;
	
	if (num == 0) {
		p = malloc(2);
		p[0] = 1, p[1] = 0;
		return p;
	}
	
	for (temp = num, len = 0; temp > 0; len++, temp /= 256);
	
	if ((p = malloc(len+1)) == 0) {
		errorf("Error: malloc failed.");
		return p;
	}
	p[0] = len;
	
	for (i = len; i > 0; i--) {
		p[i] = num % 256, num /= 256;
	}
	
	return p;
}

long long btoi(unsigned char *num) {		//* could use testing esp. with numbers at the limit
	int mlen, len;
	unsigned char cmp, *temp;
	long long result;
	
	if (num == 0) {
		errorf("Error: no number to convert");
		return 0;
	}
	for (mlen = 0; num[0] == 0; num++, mlen++);
	
	temp = ctob("-9223372036854775808");
	if ((cmp = bnscmp(num, temp)) == 2) {
		free(temp);
		return 0;
	} free(temp);
	if (cmp == 0)
		return -9223372036854775808ULL;
	temp = ctob("9223372036854775807");
	if ((cmp = bnscmp(num, temp)) == 1) {
		free(temp);
		return 0;
	} free(temp);
	if (cmp == 0)
		return 9223372036854775807LL;
	
	for (result = 0, len = (num++)[0]; len > 0; len--) {
		result *= 256;
		result += num[0];
	}
	
	if (mlen % 2) {
		result *= -1;
	}
	return result;
}

unsigned long long btou(unsigned char *num) {
	int mlen, len;
	unsigned char cmp, *temp;
	unsigned long long result;
	
	if (num == 0) {
		errorf("Error: no number to convert");
		return 0;
	}
	for (mlen = 0; num[0] == 0; num++, mlen++);
	if (mlen != 0)
		return 0;
	
	temp = ctob("18446744073709551615");
	if ((cmp = bnscmp(num, temp)) == 1) {
		free(temp);
		return 0;
	} free(temp);
	if (cmp == 0)
		return 18446744073709551615ULL;
	
	for (result = 0, len = *num; len > 0; len--) {
		num++;
		result *= 256;
		result += num[0];
	}
	
	return result;
}