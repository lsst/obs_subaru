/*
 *      envscan -- do shell-like substitions
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <pwd.h>
#include <stdlib.h>
#include "shCEnvscan.h"
#include "shCGarbage.h"

extern char *getenv(const char *);

/*
static char *copyright[] = {
 "Copyright (c) 1990 Universities Research Association, Inc.",
 "      All Rights Reserved",
 "",
 "This material resulted from work developed under a Government Contract and",
 "is subject to the following license:  The Government retains a paid-up,",
 "nonexclusive, irrevocable worldwide license to reproduce, prepare derivative",
 "works, perform publicly and display publicly by or for the Government,",
 "including the right to distribute to other Government contractors.  Neither",
 "the United States nor the United States Department of Energy, nor any of",
 "their employees, makes any warrenty, express or implied, or assumes any",
 "legal liability or responsibility for the accuracy, completeness, or",
 "usefulness of any information, apparatus, product, or process disclosed, or",
 "represents that its use would not infringe privately owned rights.",
 "",
 "Version 1.0 by Randolph J. Herber; Tue Apr  2 19:18:59 CST 1991"
};
 */

/*
 *   envscan() preforms several substitutions on its argument string:
 *
 *   A parameter is a sequence of letters, digits, or underscores beginning
 *   with a letter or underscore.
 *
 *   A dollar sign can be escaped by preceeding it with a back slash.
 *   This is implemented by simply passing the character after a back slash.
 *
 *   ~parameter   [at the beginning of the argument string]
 *        The parameter is used as a login name to search the passwd file.
 *        If the search is successful, the login directory name is
 *        substituted; otherwise, return a null pointer.
 *   ${parameter}
 *        The value, if any, of the parameter is substituted.  The braces are
 *        required only when parameter is followed by a letter, digit, or
 *        underscore that is not to be interpreted as part of it.
 *   ${parameter:-word}
 *        If parameter is set and is non-null, substitute its value;
 *        otherwise substitute word.
 *   ${parameter:~word}
 *        If parameter is set, substitute its value;
 *        otherwise substitute word.
 *   ${parameter:?word}
 *        If parameter is set and is non-null, substitute its value;
 *        otherwise, return null pointer.
 *   ${parameter:!word}
 *        If parameter is set, substitute its value;
 *        otherwise, return null pointer.
 *   ${parameter:+word}
 *        If parameter is set and is non-null, substitute word;
 *        otherwise substitute nothing.
 */

#define isalphax(x) ((x) == '_' || isalpha(x))
#define isalnumx(x) ((x) == '_' || isalnum(x))
#define LCB '{'
#define RCB '}'


char *
shEnvscan(char *arg)
{
    int length;
    register char *cp, *ep, *vp, *rp = NULL;
    char *result = NULL;
    char holder;
    struct passwd *pp = (struct passwd *)0;

    if(arg) {
	/*
	 * There are two very similiar blocks of code in this routine.
	 * The first block computes the length of the result.
	 * The second block computes the result itself.
	 * They must be kept as parallel code for this procedure to work.
	 */
	length=0;
	cp = arg;
	if(cp[0] == '~') {
	    if(!isalphax(cp[1])) {
		return (char *)0;
	    }
	    for(ep = cp+1; isalnumx(*ep); ++ep) {
	    }
	    holder = *ep;
	    *ep = '\0';
	    pp = getpwnam(cp+1);
	    *ep = holder;
	    if(pp) {
		length += strlen(pp->pw_dir);
		cp = ep;
	    } else {
		return (char *)0;
	    }
	}
	for(; *cp; ++cp) {
	    switch(*cp) {
	    case '\\':
		if(cp[1]) {
		    ++length;
		    ++cp;
		}
		break;
	    case '$':
		if(cp[1] == LCB) {
		    if(!isalphax(cp[2])) {
			return (char *)0;
		    }
		    for(ep = cp+2; isalnumx(*ep); ++ep) {
		    }
		    holder = *ep;
		    *ep = '\0';
		    vp = getenv(cp+2);
		    *ep = holder;
		    if(holder == RCB) {
			cp = ep;
			if(vp) {
			    length += strlen(vp);
			}
		    } else if(holder == ':') {
			holder = ep[1];
			if(holder == '\0' || holder == RCB) {
			    return (char *)0;
			}
			for(cp = ep += 2; *ep && *ep != RCB; ++ep) {
			}
			if(*ep == '\0') {
			    return (char *)0;
			}
			*ep = '\0';
			switch(holder) {
			case '-':
			    length += (vp && *vp) ? strlen(vp) : strlen(cp);
			    break;
			case '~':
			    length += vp ? strlen(vp) : strlen(cp);
			    break;
			case '?':
			    if(vp && *vp) {
				length += strlen(vp);
			    } else {
				*ep = RCB;
				return (char *)0;
			    }
			    break;
			case '!':
			    if(vp) {
				length += strlen(vp);
			    } else {
				*ep = RCB;
				return (char *)0;
			    }
			    break;
			case '+':
			    length += (vp && *vp) ? strlen(cp) : 0;
			    break;
			default:
			    *ep = RCB;
			    return (char *)0;
			}
			*ep = RCB;
			cp = ep;
		    } else {
			return (char *)0;
		    }
		} else if(isalphax(cp[1])) {
		    for(ep = cp+1; isalnumx(*ep); ++ep) {
		    }
		    holder = *ep;
		    *ep = '\0';
		    vp = getenv(cp+1);
		    *ep = holder;
		    cp = ep-1;
		    if(vp) {
			length += strlen(vp);
		    }
		} else {
		    return (char *)0;
		}
		break;
	    default:
		++length;
		break;
	    }
	}
	rp = (char *)shMalloc(length+1);
	if(!rp) {
		return (char *)0;
	}
	result = rp;
	cp = arg;
	if(cp[0] == '~') {
	    if(!isalphax(cp[1])) {
		(void) shFree(result);
		return (char *)0;
	    }
	    for(ep = cp+1; isalnumx(*ep); ++ep) {
	    }
	    if(pp) {
		(void) strcpy(rp,pp->pw_dir);
		rp += strlen(pp->pw_dir);
		cp = ep;
	    } else {
		(void) shFree(result);
		return (char *)0;
	    }
	}
	for(; *cp; ++cp) {
	    switch(*cp) {
	    case '\\':
		if(cp[1]) {
		    *(rp++) = cp[1];
		    ++cp;
		}
		break;
	    case '$':
		if(cp[1] == LCB) {
		    if(!isalphax(cp[2])) {
			(void) shFree(result);
			return (char *)0;
		    }
		    for(ep = cp+2; isalnumx(*ep); ++ep) {
		    }
		    holder = *ep;
		    *ep = '\0';
		    vp = getenv(cp+2);
		    *ep = holder;
		    if(holder == RCB) {
			cp = ep;
			if(vp) {
			    (void) strcpy(rp,vp);
			    rp += strlen(vp);
			}
		    } else if(holder == ':') {
			holder = ep[1];
			if(holder == '\0' || holder == RCB) {
			    (void) shFree(result);
			    return (char *)0;
			}
			for(cp = ep += 2; *ep && *ep != RCB; ++ep) {
			}
			if(*ep == '\0') {
			    (void) shFree(result);
			    return (char *)0;
			}
			*ep = '\0';
			switch(holder) {
			case '-':
			    if(vp && *vp) {
				(void) strcpy(rp,vp);
				rp += strlen(vp);
			    } else {
				(void) strcpy(rp,cp);
				rp += strlen(cp);
			    }
			    break;
			case '~':
			    if(vp) {
				(void) strcpy(rp,vp);
				rp += strlen(vp);
			    } else {
				(void) strcpy(rp,cp);
				rp += strlen(cp);
			    }
			    break;
			case '?':
			    if(vp && *vp) {
				(void) strcpy(rp,vp);
				rp += strlen(vp);
			    } else {
				*ep = RCB;
				(void) shFree(result);
				return (char *)0;
			    }
			    break;
			case '!':
			    if(vp) {
				(void) strcpy(rp,vp);
				rp += strlen(vp);
			    } else {
				*ep = RCB;
				(void) shFree(result);
				return (char *)0;
			    }
			    break;
			case '+':
			    if(vp && *vp) {
				(void) strcpy(rp,cp);
				rp += strlen(cp);
			    }
			    break;
			default:
			    *ep = RCB;
			    (void) shFree(result);
			    return (char *)0;
			}
			*ep = RCB;
			cp = ep;
		    } else {
			(void) shFree(result);
			return (char *)0;
		    }
		} else if(isalphax(cp[1])) {
		    for(ep = cp+1; isalnumx(*ep); ++ep) {
		    }
		    holder = *ep;
		    *ep = '\0';
		    vp = getenv(cp+1);
		    *ep = holder;
		    cp = ep-1;
		    if(vp) {
			(void) strcpy(rp,vp);
			rp += strlen(vp);
		    }
		} else {
		    (void) shFree(result);
		    return (char *)0;
		}
		break;
	    default:
		*(rp++) = *cp;
		break;
	    }
	}
    }
    *rp = '\0';
    return result;
}

void
shEnvfree(char *result)
{
    if(result) {
	shFree(result);
    }
}

#ifdef ENVSCAN_TESTING
main()
{
    char bfr[2048];
    register char *result;

    while(gets(bfr)) {
	printf("\"%s\" is input -------------\n",bfr);
	result = shEnvscan(bfr);
	if(result) {
	    printf("+ %s\n",result);
	    (void) shEnvfree(result);
	} else {
	    printf("- FAILURE\n");
	}
    }
    exit(0);
}
#endif
