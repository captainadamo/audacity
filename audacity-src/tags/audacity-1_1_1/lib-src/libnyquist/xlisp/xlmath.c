/* xlmath - xlisp built-in arithmetic functions */
/*	Copyright (c) 1985, by David Michael Betz
        All Rights Reserved
        Permission is granted for unrestricted non-commercial use	*/

#include "xlisp.h"
#include <math.h>

/* external variables */
extern LVAL s_true;

/* forward declarations */
FORWARD LOCAL LVAL unary(int fcn);
FORWARD LOCAL LVAL binary(int fcn);
FORWARD LOCAL LVAL predicate(int fcn);
FORWARD LOCAL LVAL compare(int fcn);
FORWARD LOCAL void badiop(void);
FORWARD LOCAL void badfop(void);


/* binary functions */
LVAL xadd(void)    { return (binary('+')); } /* + */
LVAL xsub(void)    { return (binary('-')); } /* - */
LVAL xmul(void)    { return (binary('*')); } /* * */
LVAL xdiv(void)    { return (binary('/')); } /* / */
LVAL xrem(void)    { return (binary('%')); } /* rem */
LVAL xmin(void)    { return (binary('m')); } /* min */
LVAL xmax(void)    { return (binary('M')); } /* max */
LVAL xexpt(void)   { return (binary('E')); } /* expt */
LVAL xlogand(void) { return (binary('&')); } /* logand */
LVAL xlogior(void) { return (binary('|')); } /* logior */
LVAL xlogxor(void) { return (binary('^')); } /* logxor */

/* xgcd - greatest common divisor */
LVAL xgcd(void)
{
    FIXTYPE m,n,r;
    LVAL arg;

    if (!moreargs())			/* check for identity case */
        return (cvfixnum((FIXTYPE)0));
    arg = xlgafixnum();
    n = getfixnum(arg);
    if (n < (FIXTYPE)0) n = -n;		/* absolute value */
    while (moreargs()) {
        arg = xlgafixnum();
        m = getfixnum(arg);
        if (m < (FIXTYPE)0) m = -m;	/* absolute value */
        for (;;) {			/* euclid's algorithm */
            r = m % n;
            if (r == (FIXTYPE)0)
                break;
            m = n;
            n = r;
        }
    }
    return (cvfixnum(n));
}

/* binary - handle binary operations */
LOCAL LVAL binary(int fcn)
{
    FIXTYPE ival,iarg;
    FLOTYPE fval,farg;
    LVAL arg;
    int mode;

    /* get the first argument */
    arg = xlgetarg();

    /* set the type of the first argument */
    if (fixp(arg)) {
        ival = getfixnum(arg);
        mode = 'I';
    }
    else if (floatp(arg)) {
        fval = getflonum(arg);
        mode = 'F';
    }
    else
        xlerror("bad argument type",arg);

    /* treat a single argument as a special case */
    if (!moreargs()) {
        switch (fcn) {
        case '-':
            switch (mode) {
            case 'I':
                ival = -ival;
                break;
            case 'F':
                fval = -fval;
                break;
            }
            break;
        case '/':
            switch (mode) {
            case 'I':
                checkizero(ival);
                ival = 1 / ival;
                break;
            case 'F':
                checkfzero(fval);
                fval = 1.0 / fval;
                break;
            }
        }
    }

    /* handle each remaining argument */
    while (moreargs()) {

        /* get the next argument */
        arg = xlgetarg();

        /* check its type */
        if (fixp(arg)) {
            switch (mode) {
            case 'I':
                iarg = getfixnum(arg);
                break;
            case 'F':
                farg = (FLOTYPE)getfixnum(arg);
                break;
            }
        }
        else if (floatp(arg)) {
            switch (mode) {
            case 'I':
                fval = (FLOTYPE)ival;
                farg = getflonum(arg);
                mode = 'F';
                break;
            case 'F':
                farg = getflonum(arg);
                break;
            }
        }
        else
            xlerror("bad argument type",arg);

        /* accumulate the result value */
        switch (mode) {
        case 'I':
            switch (fcn) {
            case '+':	ival += iarg; break;
            case '-':	ival -= iarg; break;
            case '*':	ival *= iarg; break;
            case '/':	checkizero(iarg); ival /= iarg; break;
            case '%':	checkizero(iarg); ival %= iarg; break;
            case 'M':	if (iarg > ival) ival = iarg; break;
            case 'm':	if (iarg < ival) ival = iarg; break;
            case '&':	ival &= iarg; break;
            case '|':	ival |= iarg; break;
            case '^':	ival ^= iarg; break;
            default:	badiop();
            }
            break;
        case 'F':
            switch (fcn) {
            case '+':	fval += farg; break;
            case '-':	fval -= farg; break;
            case '*':	fval *= farg; break;
            case '/':	checkfzero(farg); fval /= farg; break;
            case 'M':	if (farg > fval) fval = farg; break;
            case 'm':	if (farg < fval) fval = farg; break;
            case 'E':	fval = pow(fval,farg); break;
            default:	badfop();
            }
                break;
        }
    }

    /* return the result */
    switch (mode) {
    case 'I':	return (cvfixnum(ival));
    case 'F':	return (cvflonum(fval));
    }
}

/* checkizero - check for integer division by zero */
void checkizero(FIXTYPE iarg)
{
    if (iarg == 0)
        xlfail("division by zero");
}

/* checkfzero - check for floating point division by zero */
void checkfzero(FLOTYPE farg)
{
    if (farg == 0.0)
        xlfail("division by zero");
}

/* checkfneg - check for square root of a negative number */
void checkfneg(FLOTYPE farg)
{
    if (farg < 0.0)
        xlfail("square root of a negative number");
}

/* unary functions */
LVAL xlognot(void) { return (unary('~')); } /* lognot */
LVAL xabs(void)    { return (unary('A')); } /* abs */
LVAL xadd1(void)   { return (unary('+')); } /* 1+ */
LVAL xsub1(void)   { return (unary('-')); } /* 1- */
LVAL xsin(void)    { return (unary('S')); } /* sin */
LVAL xcos(void)    { return (unary('C')); } /* cos */
LVAL xtan(void)    { return (unary('T')); } /* tan */
LVAL xexp(void)    { return (unary('E')); } /* exp */
LVAL xsqrt(void)   { return (unary('R')); } /* sqrt */
LVAL xfix(void)    { return (unary('I')); } /* truncate */
LVAL xfloat(void)  { return (unary('F')); } /* float */
LVAL xrand(void)   { return (unary('?')); } /* random */

/* unary - handle unary operations */
LOCAL LVAL unary(int fcn)
{
    FLOTYPE fval;
    FIXTYPE ival;
    LVAL arg;

    /* get the argument */
    arg = xlgetarg();
    xllastarg();

    /* check its type */
    if (fixp(arg)) {
        ival = getfixnum(arg);
        switch (fcn) {
        case '~':	ival = ~ival; break;
        case 'A':	ival = (ival < 0 ? -ival : ival); break;
        case '+':	ival++; break;
        case '-':	ival--; break;
        case 'I':	break;
        case 'F':	return (cvflonum((FLOTYPE)ival));
        case '?':	ival = (FIXTYPE)osrand((int)ival); break;
        default:	badiop();
        }
        return (cvfixnum(ival));
    }
    else if (floatp(arg)) {
        fval = getflonum(arg);
        switch (fcn) {
        case 'A':	fval = (fval < 0.0 ? -fval : fval); break;
        case '+':	fval += 1.0; break;
        case '-':	fval -= 1.0; break;
        case 'S':	fval = sin(fval); break;
        case 'C':	fval = cos(fval); break;
        case 'T':	fval = tan(fval); break;
        case 'E':	fval = exp(fval); break;
        case 'R':	checkfneg(fval); fval = sqrt(fval); break;
        case 'I':	return (cvfixnum((FIXTYPE)fval));
        case 'F':	break;
        default:	badfop();
        }
        return (cvflonum(fval));
    }
    else
        xlerror("bad argument type",arg);
}

/* unary predicates */
LVAL xminusp(void) { return (predicate('-')); } /* minusp */
LVAL xzerop(void)  { return (predicate('Z')); } /* zerop */
LVAL xplusp(void)  { return (predicate('+')); } /* plusp */
LVAL xevenp(void)  { return (predicate('E')); } /* evenp */
LVAL xoddp(void)   { return (predicate('O')); } /* oddp */

/* predicate - handle a predicate function */
LOCAL LVAL predicate(int fcn)
{
    FLOTYPE fval;
    FIXTYPE ival;
    LVAL arg;

    /* get the argument */
    arg = xlgetarg();
    xllastarg();

    /* check the argument type */
    if (fixp(arg)) {
        ival = getfixnum(arg);
        switch (fcn) {
        case '-':	ival = (ival < 0); break;
        case 'Z':	ival = (ival == 0); break;
        case '+':	ival = (ival > 0); break;
        case 'E':	ival = ((ival & 1) == 0); break;
        case 'O':	ival = ((ival & 1) != 0); break;
        default:	badiop();
        }
    }
    else if (floatp(arg)) {
        fval = getflonum(arg);
        switch (fcn) {
        case '-':	ival = (fval < 0); break;
        case 'Z':	ival = (fval == 0); break;
        case '+':	ival = (fval > 0); break;
        default:	badfop();
        }
    }
    else
        xlerror("bad argument type",arg);

    /* return the result value */
    return (ival ? s_true : NIL);
}

/* comparison functions */
LVAL xlss(void) { return (compare('<')); } /* < */
LVAL xleq(void) { return (compare('L')); } /* <= */
LVAL xequ(void) { return (compare('=')); } /* = */
LVAL xneq(void) { return (compare('#')); } /* /= */
LVAL xgeq(void) { return (compare('G')); } /* >= */
LVAL xgtr(void) { return (compare('>')); } /* > */

/* compare - common compare function */
LOCAL LVAL compare(int fcn)
{
    FIXTYPE icmp,ival,iarg;
    FLOTYPE fcmp,fval,farg;
    LVAL arg;
    int mode;

    /* get the first argument */
    arg = xlgetarg();

    /* set the type of the first argument */
    if (fixp(arg)) {
        ival = getfixnum(arg);
        mode = 'I';
    }
    else if (floatp(arg)) {
        fval = getflonum(arg);
        mode = 'F';
    }
    else
        xlerror("bad argument type",arg);

    /* handle each remaining argument */
    for (icmp = TRUE; icmp && moreargs(); ival = iarg, fval = farg) {

        /* get the next argument */
        arg = xlgetarg();

        /* check its type */
        if (fixp(arg)) {
            switch (mode) {
            case 'I':
                iarg = getfixnum(arg);
                break;
            case 'F':
                farg = (FLOTYPE)getfixnum(arg);
                break;
            }
        }
        else if (floatp(arg)) {
            switch (mode) {
            case 'I':
                fval = (FLOTYPE)ival;
                farg = getflonum(arg);
                mode = 'F';
                break;
            case 'F':
                farg = getflonum(arg);
                break;
            }
        }
        else
            xlerror("bad argument type",arg);

        /* compute result of the compare */
        switch (mode) {
        case 'I':
            icmp = ival - iarg;
            switch (fcn) {
            case '<':	icmp = (icmp < 0); break;
            case 'L':	icmp = (icmp <= 0); break;
            case '=':	icmp = (icmp == 0); break;
            case '#':	icmp = (icmp != 0); break;
            case 'G':	icmp = (icmp >= 0); break;
            case '>':	icmp = (icmp > 0); break;
            }
            break;
        case 'F':
            fcmp = fval - farg;
            switch (fcn) {
            case '<':	icmp = (fcmp < 0.0); break;
            case 'L':	icmp = (fcmp <= 0.0); break;
            case '=':	icmp = (fcmp == 0.0); break;
            case '#':	icmp = (fcmp != 0.0); break;
            case 'G':	icmp = (fcmp >= 0.0); break;
            case '>':	icmp = (fcmp > 0.0); break;
            }
            break;
        }
    }

    /* return the result */
    return (icmp ? s_true : NIL);
}

/* badiop - bad integer operation */
LOCAL void badiop(void)
{
    xlfail("bad integer operation");
}

/* badfop - bad floating point operation */
LOCAL void badfop(void)
{
    xlfail("bad floating point operation");
}
