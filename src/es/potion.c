/*	SCCS Id: @(#)potion.c	3.4	2002/10/02	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

#ifdef OVLB
boolean notonhead = FALSE;

static NEARDATA int nothing, unkn;
static NEARDATA const char beverages[] = { POTION_CLASS, 0 };

STATIC_DCL long FDECL(itimeout, (long));
STATIC_DCL long FDECL(itimeout_incr, (long,int));
STATIC_DCL void NDECL(ghost_from_bottle);
STATIC_DCL short FDECL(mixtype, (struct obj *,struct obj *));

/* force `val' to be within valid range for intrinsic timeout value */
STATIC_OVL long
itimeout(val)
long val;
{
    if (val >= TIMEOUT) val = TIMEOUT;
    else if (val < 1) val = 0;

    return val;
}

/* increment `old' by `incr' and force result to be valid intrinsic timeout */
STATIC_OVL long
itimeout_incr(old, incr)
long old;
int incr;
{
    return itimeout((old & TIMEOUT) + (long)incr);
}

/* set the timeout field of intrinsic `which' */
void
set_itimeout(which, val)
long *which, val;
{
    *which &= ~TIMEOUT;
    *which |= itimeout(val);
}

/* increment the timeout field of intrinsic `which' */
void
incr_itimeout(which, incr)
long *which;
int incr;
{
    set_itimeout(which, itimeout_incr(*which, incr));
}

void
make_confused(xtime,talk)
long xtime;
boolean talk;
{
	long old = HConfusion;

	if (!xtime && old) {
		if (talk)
		    Te_sientes("menos %s%c ahora.",
			Hallucination ? "tropezad" : "confundid",
			poly_gender()==1? 'a' : 'o');
	}
	if ((xtime && !old) || (!xtime && old)) flags.botl = TRUE;

	set_itimeout(&HConfusion, xtime);
}

void
make_stunned(xtime,talk)
long xtime;
boolean talk;
{
	long old = HStun;

	if (!xtime && old) {
		if (talk)
		    Te_sientes("%s ahora.",
			Hallucination ? "menos tambaleante" : "un poco m�s estable");
	}
	if (xtime && !old) {
		if (talk) {
#ifdef STEED
			if (u.usteed)
				pline("Vacilas en la silla.");
			else
#endif
			pline("%s...", stagger(youmonst.data, "Te bamboleas"));
		}
	}
	if ((!xtime && old) || (xtime && !old)) flags.botl = TRUE;

	set_itimeout(&HStun, xtime);
}

void
make_sick(xtime, cause, talk, type)
long xtime;
const char *cause;	/* sickness cause */
boolean talk;
int type;
{
	long old = Sick;

	if (xtime > 0L) {
	    if (Sick_resistance) return;
	    if (!old) {
		/* newly sick */
		Te_sientes("enferm%c a la muerte.", poly_gender()==1? 'a' : 'o');
	    } else {
		/* already sick */
		if (talk) pline("%s peor.",
			      xtime <= Sick/2L ? "Te sientes mucho" : "Ya te sientes");
	    }
	    set_itimeout(&Sick, xtime);
	    u.usick_type |= type;
	    flags.botl = TRUE;
	} else if (old && (type & u.usick_type)) {
	    /* was sick, now not */
	    u.usick_type &= ~type;
	    if (u.usick_type) { /* only partly cured */
		if (talk) Te_sientes("un poco mejor.");
		set_itimeout(&Sick, Sick * 2); /* approximation */
	    } else {
		if (talk) pline("�Qu� alivio!");
		Sick = 0L;		/* set_itimeout(&Sick, 0L) */
	    }
	    flags.botl = TRUE;
	}

	if (Sick) {
	    exercise(A_CON, FALSE);
	    if (cause) {
		(void) strncpy(u.usick_cause, cause, sizeof(u.usick_cause));
		u.usick_cause[sizeof(u.usick_cause)-1] = 0;
		}
	    else
		u.usick_cause[0] = 0;
	} else
	    u.usick_cause[0] = 0;
}

void
make_vomiting(xtime, talk)
long xtime;
boolean talk;
{
	long old = Vomiting;

	if(!xtime && old)
	    if(talk) Te_sientes("mucho menos nausead%c ahora.",
		    poly_gender()==1? 'a' : 'o');

	set_itimeout(&Vomiting, xtime);
}

static const char vismsg[] = "Tu visi�n parece %s por un momento pero ahora est� %s.";
static const char eyemsg[] = "Tu%s %s %s por un momento.";

void
make_blinded(xtime, talk)
long xtime;
boolean talk;
{
	long old = Blinded;
	boolean u_could_see, can_see_now;
	int eyecnt;
	char buf[BUFSZ];

	/* we need to probe ahead in case the Eyes of the Overworld
	   are or will be overriding blindness */
	u_could_see = !Blind;
	Blinded = xtime ? 1L : 0L;
	can_see_now = !Blind;
	Blinded = old;		/* restore */

	if (u.usleep) talk = FALSE;

	if (can_see_now && !u_could_see) {	/* regaining sight */
	    if (talk) {
		if (Hallucination)
		    pline("�Caramba!  �Todo es c�smico de nuevo!");
		else
		    pline("Puedes ver de nuevo.");
	    }
	} else if (old && !xtime) {
	    /* clearing temporary blindness without toggling blindness */
	    if (talk) {
		if (!haseyes(youmonst.data)) {
		    strange_feeling((struct obj *)0, (char *)0);
		} else if (Blindfolded) {
		    Strcpy(buf, body_part(EYE));
		    eyecnt = eyecount(youmonst.data);
		    pline(eyemsg, plur(eyecnt),
			 (eyecnt == 1) ? buf : makeplural(buf),
			 (eyecnt == 1) ? "pica" : "pican");
		} else {	/* Eyes of the Overworld */
		    pline(vismsg, "volverse m�s clara",
			 Hallucination ? "m�s triste" : "normal");
		}
	    }
	}

	if (u_could_see && !can_see_now) {	/* losing sight */
	    if (talk) {
		if (Hallucination)
		    pline("�Qu� l�o!  �Todo est� oscuro!  �Ay�dame!");
		else
		    pline("Una nube de oscuridad se desciende sobre ti.");
	    }
	    /* Before the hero goes blind, set the ball&chain variables. */
	    if (Punished) set_bc(0);
	} else if (!old && xtime) {
	    /* setting temporary blindness without toggling blindness */
	    if (talk) {
		if (!haseyes(youmonst.data)) {
		    strange_feeling((struct obj *)0, (char *)0);
		} else if (Blindfolded) {
		    Strcpy(buf, body_part(EYE));
		    eyecnt = eyecount(youmonst.data);
		    pline(eyemsg, plur(eyecnt),
			 (eyecnt == 1) ? buf : makeplural(buf),
			 (eyecnt == 1) ? "tembla" : "temblan");
		} else {	/* Eyes of the Overworld */
		    pline(vismsg, "volverse m�s oscura",
			 Hallucination ? "m�s feliz" : "normal");
		}
	    }
	}

	set_itimeout(&Blinded, xtime);

	if (u_could_see ^ can_see_now) {  /* one or the other but not both */
	    flags.botl = 1;
	    vision_full_recalc = 1;	/* blindness just got toggled */
	    if (Blind_telepat || Infravision) see_monsters();
	}
}

boolean
make_hallucinated(xtime, talk, mask)
long xtime;	/* nonzero if this is an attempt to turn on hallucination */
boolean talk;
long mask;	/* nonzero if resistance status should change by mask */
{
	long old = HHallucination;
	boolean changed = 0;
	const char *message, *verb;

	message = (!xtime) ? "Todo %s TAN aburrido ahora." :
			     "�Oh vaya!  �Todo %s tan c�smico!";
	verb = (!Blind) ? "se ve" : "siente";

	if (mask) {
	    if (HHallucination) changed = TRUE;

	    if (!xtime) EHalluc_resistance |= mask;
	    else EHalluc_resistance &= ~mask;
	} else {
	    if (!EHalluc_resistance && (!!HHallucination != !!xtime))
		changed = TRUE;
	    set_itimeout(&HHallucination, xtime);

	    /* clearing temporary hallucination without toggling vision */
	    if (!changed && !HHallucination && old && talk) {
		if (!haseyes(youmonst.data)) {
		    strange_feeling((struct obj *)0, (char *)0);
		} else if (Blind) {
		    char buf[BUFSZ];
		    int eyecnt = eyecount(youmonst.data);

		    Strcpy(buf, body_part(EYE));
		    pline(eyemsg, plur(eyecnt),
			 (eyecnt == 1) ? buf : makeplural(buf),
			 (eyecnt == 1) ? "pica" : "pican");
		} else {	/* Grayswandir */
		    pline(vismsg, "volverse m�s ins�pida", "normal");
		}
	    }
	}

	if (changed) {
	    if (u.uswallow) {
		swallowed(0);	/* redraw swallow display */
	    } else {
		/* The see_* routines should be called *before* the pline. */
		see_monsters();
		see_objects();
		see_traps();
	    }

	    /* for perm_inv and anything similar
	    (eg. Qt windowport's equipped items display) */
	    update_inventory();

	    flags.botl = 1;
	    if (talk) pline(message, verb);
	}
	return changed;
}

STATIC_OVL void
ghost_from_bottle()
{
	struct monst *mtmp = makemon(&mons[PM_GHOST], u.ux, u.uy, NO_MM_FLAGS);
	const char *que;
	boolean fem;

	if (!mtmp) {
		pline("Descubres que la botella est� vac�a.");
		return;
	}
	if (Blind) {
		pline("Al abrir la botella, algo emerge.");
		return;
	}
	que = "espectro";
	if (Hallucination) que = rndmonnam();
	fem = isfeminine(que);
	pline("�Al abrir la botella, un%s %s enorm%c emerge!",
		fem? "a" : "", que, fem? 'a' : 'o');
	if(flags.verbose)
	    pline("Est�s espantad%c a la muerte, y no puedes mover.",
		    poly_gender()==1? 'a' : 'o');
	nomul(-3);
	nomovemsg = "Recubres tu compostura.";
}

/* "Quaffing is like drinking, except you spill more."  -- Terry Pratchett
 */
int
dodrink()
{
	register struct obj *otmp;
	const char *potion_descr;

	if (Strangled) {
		pline("Si no puedes respirar aire, �c�mo puedes beber l�qido?");
		return 0;
	}
	/* Is there a fountain to drink from here? */
	if (IS_FOUNTAIN(levl[u.ux][u.uy].typ) && !Levitation) {
		if(yn("�Beber de la fuente?") == 'y') {
			drinkfountain();
			return 1;
		}
	}
#ifdef SINKS
	/* Or a kitchen sink? */
	if (IS_SINK(levl[u.ux][u.uy].typ)) {
		if (yn("�Beber del fregadero?") == 'y') {
			drinksink();
			return 1;
		}
	}
#endif

	/* Or are you surrounded by water? */
	if (Underwater) {
		if (yn("�Beber el agua alrededor de ti?") == 'y') {
		    pline("�Sabes lo que viva en esta agua?");
			return 1;
		}
	}

	otmp = getobj(beverages, "", "beber");
	if(!otmp) return(0);
	otmp->in_use = TRUE;		/* you've opened the stopper */

#define POTION_OCCUPANT_CHANCE(n) (13 + 2*(n))	/* also in muse.c */

	potion_descr = OBJ_DESCR(objects[otmp->otyp]);
	if (potion_descr) {
	    if (!strcmp(potion_descr, "l�ctea") &&
		    flags.ghost_count < MAXMONNO &&
		    !rn2(POTION_OCCUPANT_CHANCE(flags.ghost_count))) {
		ghost_from_bottle();
		useup(otmp);
		return(1);
	    } else if (!strcmp(potion_descr, "humosa") &&
		    flags.djinni_count < MAXMONNO &&
		    !rn2(POTION_OCCUPANT_CHANCE(flags.djinni_count))) {
		djinni_from_bottle(otmp);
		useup(otmp);
		return(1);
	    }
	}
	return dopotion(otmp);
}

int
dopotion(otmp)
register struct obj *otmp;
{
	int retval;

	otmp->in_use = TRUE;
	nothing = unkn = 0;
	if((retval = peffects(otmp)) >= 0) return(retval);

	if(nothing) {
	    unkn++;
	    pline("Tienes un sentido %s por un momento, y entonces pasa.",
		  Hallucination ? "normal" : "peculiar");
	}
	if(otmp->dknown && !objects[otmp->otyp].oc_name_known) {
		if(!unkn) {
			makeknown(otmp->otyp);
			more_experienced(0,10);
		} else if(!objects[otmp->otyp].oc_uname)
			docall(otmp);
	}
	useup(otmp);
	return(1);
}

int
peffects(otmp)
	register struct obj	*otmp;
{
	register int i, ii, lim;

	switch(otmp->otyp){
	case POT_RESTORE_ABILITY:
	case SPE_RESTORE_ABILITY:
		unkn++;
		if(otmp->cursed) {
		    pline("�Ulch!  ��sta hace que sientes mediocre!");
		    break;
		} else {
		    if (otmp->blessed) {
			if (unfixable_trouble_count(FALSE))
			    pline("�Vaya!  ��sta te hace sentir mejor!");
			else
			    pline("�Vaya!  ��sta te hace sentir magn�fic%c!",
				  poly_gender()==1? 'a' : 'o');
		    } else {
			pline("�Vaya!  ��sta te hace sentir buen%c!",
			      poly_gender()==1? 'a' : 'o');
		    }
		    i = rn2(A_MAX);		/* start at a random point */
		    for (ii = 0; ii < A_MAX; ii++) {
			lim = AMAX(i);
			if (i == A_STR && u.uhs >= 3) --lim;	/* WEAK */
			if (ABASE(i) < lim) {
			    ABASE(i) = lim;
			    flags.botl = 1;
			    /* only first found if not blessed */
			    if (!otmp->blessed) break;
			}
			if(++i >= A_MAX) i = 0;
		    }
		}
		break;
	case POT_HALLUCINATION:
		if (Hallucination || Halluc_resistance) nothing++;
		(void) make_hallucinated(itimeout_incr(HHallucination,
					   rn1(200, 600 - 300 * bcsign(otmp))),
				  TRUE, 0L);
		break;
	case POT_WATER:
		if(!otmp->blessed && !otmp->cursed) {
		    pline("�sta sabe al agua.");
		    u.uhunger += rnd(10);
		    newuhs(FALSE);
		    break;
		}
		unkn++;
		if(is_undead(youmonst.data) || is_demon(youmonst.data) ||
				u.ualign.type == A_CHAOTIC) {
		    if(otmp->blessed) {
			pline("��sta te quema como �cido!");
			exercise(A_CON, FALSE);
			if (u.ulycn >= LOW_PM) {
			    pline("�Tu afinidad con %s desaparece!",
				 makeplural(mons[u.ulycn].mname));
			    if (youmonst.data == &mons[u.ulycn])
				you_unwere(FALSE);
			    u.ulycn = NON_PM;	/* cure lycanthropy */
			}
			losehp(d(2,6), "poci�n de agua bendita", KILLED_BY_AN);
		    } else if(otmp->cursed) {
			Te_sientes("bastante orgullos%c de ti.",
				poly_gender()==1? 'a' : 'o');
			healup(d(2,6),0,0,0);
			if (u.ulycn >= LOW_PM && !Upolyd) you_were();
			exercise(A_CON, TRUE);
		    }
		} else {
		    if(otmp->blessed) {
			Te_sientes("llen%c de pasmo.",
				poly_gender()==1? 'a' : 'o');
			make_sick(0L, (char *) 0, TRUE, SICK_ALL);
			exercise(A_WIS, TRUE);
			exercise(A_CON, TRUE);
			if (u.ulycn >= LOW_PM)
			    you_unwere(TRUE);	/* "Purified" */
			/* make_confused(0L,TRUE); */
		    } else {
			if(u.ualign.type == A_LAWFUL) {
			    pline("��sta te quema como �cido!");
			    losehp(d(2,6), "poci�n de agua maldita",
				KILLED_BY_AN);
			} else
			    Te_sientes("llen%c de temor.",
				    poly_gender()==1? 'a' : 'o');
			if (u.ulycn >= LOW_PM && !Upolyd) you_were();
			exercise(A_CON, FALSE);
		    }
		}
		break;
	case POT_BOOZE:
		unkn++;
		pline("�Ooph!  ��sta sabe a %s%s!",
		      otmp->odiluted ? "diluido " : "",
		      Hallucination ? "vino de diente de le�n" : "fuego l�quido");
		if (!otmp->blessed)
		    make_confused(itimeout_incr(HConfusion, d(3,8)), FALSE);
		/* the whiskey makes us feel better */
		if (!otmp->odiluted) healup(1, 0, FALSE, FALSE);
		u.uhunger += 10 * (2 + bcsign(otmp));
		newuhs(FALSE);
		exercise(A_WIS, FALSE);
		if(otmp->cursed) {
			pline("Desmayas.");
			multi = -rnd(15);
			nomovemsg = "Te levantas con un dolor de cabeza.";
		}
		break;
	case POT_ENLIGHTENMENT:
		if(otmp->cursed) {
			unkn++;
			pline("Tienes un sentido inquieto...");
			exercise(A_WIS, FALSE);
		} else {
			if (otmp->blessed) {
				(void) adjattrib(A_INT, 1, FALSE);
				(void) adjattrib(A_WIS, 1, FALSE);
			}
			Te_sientes("conociente de ti...");
			display_nhwindow(WIN_MESSAGE, FALSE);
			enlightenment(0);
			pline("El sentido se disminue.");
			exercise(A_WIS, TRUE);
		}
		break;
	case SPE_INVISIBILITY:
		/* spell cannot penetrate mummy wrapping */
		if (BInvis && uarmc->otyp == MUMMY_WRAPPING) {
			pline("Te hormigues bajo tu %s.", xname(uarmc));
			break;
		}
		/* FALLTHRU */
	case POT_INVISIBILITY:
		if (Invis || Blind || BInvis) {
		    nothing++;
		} else {
		    self_invis_message();
		}
		if (otmp->blessed) HInvis |= FROMOUTSIDE;
		else incr_itimeout(&HInvis, rn1(15,31));
		newsym(u.ux,u.uy);	/* update position */
		if(otmp->cursed) {
		    pline("Por alguna raz�n, sientes que se sabe tu presencia.");
		    aggravate();
		}
		break;
	case POT_SEE_INVISIBLE:
		/* tastes like fruit juice in Rogue */
	case POT_FRUIT_JUICE:
	    {
		int msg = Invisible && !Blind;

		unkn++;
		if (otmp->cursed)
		    pline("�Yecch!  �sta sabe %s.",
			  Hallucination ? "sobremadura" : "podrida");
		else
		    pline(Hallucination ?
		      "�sta sabe a bebido todo-natural de 10%% real jugo%s de %s." :
		      "�sta sabe a jugo%s de %s.",
			  otmp->odiluted ? " reconstituido" : "",
			  fruitname(FALSE));
		if (otmp->otyp == POT_FRUIT_JUICE) {
		    u.uhunger += (otmp->odiluted ? 5 : 10) * (2 + bcsign(otmp));
		    newuhs(FALSE);
		    break;
		}
		if (!otmp->cursed) {
			/* Tell them they can see again immediately, which
			 * will help them identify the potion...
			 */
			make_blinded(0L,TRUE);
		}
		if (otmp->blessed)
			HSee_invisible |= FROMOUTSIDE;
		else
			incr_itimeout(&HSee_invisible, rn1(100,750));
		set_mimic_blocking(); /* do special mimic handling */
		see_monsters();	/* see invisible monsters */
		newsym(u.ux,u.uy); /* see yourself! */
		if (msg && !Blind) { /* Blind possible if polymorphed */
		    pline("�Puedes ver trav�s de ti, pero est�s visible!");
		    unkn--;
		}
		break;
	    }
	case POT_PARALYSIS:
		if (Free_action)
		    pline("Te atiesas por un momento.");
		else {
		    if (Levitation || Is_airlevel(&u.uz)||Is_waterlevel(&u.uz))
			pline("Est�s suspendid%c sin moci�n.",
				poly_gender()==1? 'a' : 'o');
#ifdef STEED
		    else if (u.usteed)
			pline("�Est�s paralizad%c y no puedes mover!",
				poly_gender()==1? 'a' : 'o');
#endif
		    else {
			const char *pie = body_part(FOOT);
			pline("�Tus %s est�n pegad%cs %s!",
			     makeplural(pie),
			     isfeminine(pie)? 'a' : 'o',
			     al(surface(u.ux, u.uy)));
		    }
		    nomul(-(rn1(10, 25 - 12*bcsign(otmp))));
		    nomovemsg = You_can_move_again;
		    exercise(A_DEX, FALSE);
		}
		break;
	case POT_SLEEPING:
		if(Sleep_resistance || Free_action)
		    pline("Bostezas.");
		else {
		    pline("�Te duermes de repente!");
		    fall_asleep(-rn1(10, 25 - 12*bcsign(otmp)), TRUE);
		}
		break;
	case POT_MONSTER_DETECTION:
	case SPE_DETECT_MONSTERS:
		if (otmp->blessed) {
		    int x, y;

		    if (Detect_monsters) nothing++;
		    unkn++;
		    /* after a while, repeated uses become less effective */
		    if (HDetect_monsters >= 300L)
			i = 1;
		    else
			i = rn1(40,21);
		    incr_itimeout(&HDetect_monsters, i);
		    for (x = 1; x < COLNO; x++) {
			for (y = 0; y < ROWNO; y++) {
			    if (levl[x][y].glyph == GLYPH_INVISIBLE) {
				unmap_object(x, y);
				newsym(x,y);
			    }
			    if (MON_AT(x,y)) unkn = 0;
			}
		    }
		    see_monsters();
		    if (unkn) Te_sientes("sol%c.", poly_gender()==1? 'a' : 'o');
		    break;
		}
		if (monster_detect(otmp, 0))
			return(1);		/* nothing detected */
		exercise(A_WIS, TRUE);
		break;
	case POT_OBJECT_DETECTION:
	case SPE_DETECT_TREASURE:
		if (object_detect(otmp, 0))
			return(1);		/* nothing detected */
		exercise(A_WIS, TRUE);
		break;
	case POT_SICKNESS:
		pline("�Yecch!  Esta poci�n sabe a veneno.");
		if (otmp->blessed) {
		    pline("(Pero de hecho era %s templadamente rancio.)",
			  fruitname(TRUE));
		    if (!Role_if(PM_HEALER)) {
			/* NB: blessed otmp->fromsink is not possible */
			losehp(1, "poci�n templadamente contaminada", KILLED_BY_AN);
		    }
		} else {
		    if(Poison_resistance)
			pline(
		    "(Pero de hecho era %s biol�gicamente contaminado.)",
			      fruitname(TRUE));
		    if (Role_if(PM_HEALER))
			pline("Por fortuna, has sido imunizad%c.",
				poly_gender()==1? 'a' : 'o');
		    else {
			int typ = rn2(A_MAX);

			if (!Fixed_abil) {
			    poisontell(typ);
			    (void) adjattrib(typ,
			    		Poison_resistance ? -1 : -rn1(4,3),
			    		TRUE);
			}
			if(!Poison_resistance) {
			    if (otmp->fromsink)
				losehp(rnd(10)+5*!!(otmp->cursed),
				       "agua contaminada", KILLED_BY);
			    else
				losehp(rnd(10)+5*!!(otmp->cursed),
				       "poci�n contaminada", KILLED_BY_AN);
			}
			exercise(A_CON, FALSE);
		    }
		}
		if(Hallucination) {
			pline("�Eres chocad%c atr�s a ti!",
				poly_gender()==1? 'a' : 'o');
			(void) make_hallucinated(0L,FALSE,0L);
		}
		break;
	case POT_CONFUSION:
		if(!Confusion)
		    if (Hallucination) {
			pline("�Que sentido tropezado!");
			unkn++;
		    } else
			pline("�Huh, Qu�?  �D�nde estoy?");
		else	nothing++;
		make_confused(itimeout_incr(HConfusion,
					    rn1(7, 16 - 8 * bcsign(otmp))),
			      FALSE);
		break;
	case POT_GAIN_ABILITY:
		if(otmp->cursed) {
		    pline("�Ulch!  �Esta poci�n sab�a sucia!");
		    unkn++;
		} else if (Fixed_abil) {
		    nothing++;
		} else {      /* If blessed, increase all; if not, try up to */
		    int itmp; /* 6 times to find one which can be increased. */
		    i = -1;		/* increment to 0 */
		    for (ii = A_MAX; ii > 0; ii--) {
			i = (otmp->blessed ? i + 1 : rn2(A_MAX));
			/* only give "your X is already as high as it can get"
			   message on last attempt (except blessed potions) */
			itmp = (otmp->blessed || ii == 1) ? 0 : -1;
			if (adjattrib(i, 1, itmp) && !otmp->blessed)
			    break;
		    }
		}
		break;
	case POT_SPEED:
		if(Wounded_legs && !otmp->cursed
#ifdef STEED
		   && !u.usteed	/* heal_legs() would heal steeds legs */
#endif
						) {
			heal_legs();
			unkn++;
			break;
		} /* and fall through */
	case SPE_HASTE_SELF:
		if(!Very_fast) /* wwf@doe.carleton.ca */
			pline("Est�s moviendo %sm�s r�pidamente de repente.",
				Fast ? "" : "mucho ");
		else {
			pline("Tus %s ganan nueva energ�a.",
				makeplural(body_part(LEG)));
			unkn++;
		}
		exercise(A_DEX, TRUE);
		incr_itimeout(&HFast, rn1(10, 100 + 60 * bcsign(otmp)));
		break;
	case POT_BLINDNESS:
		if(Blind) nothing++;
		make_blinded(itimeout_incr(Blinded,
					   rn1(200, 250 - 125 * bcsign(otmp))),
			     (boolean)!Blind);
		break;
	case POT_GAIN_LEVEL:
		if (otmp->cursed) {
			unkn++;
			/* they went up a level */
			if((ledger_no(&u.uz) == 1 && u.uhave.amulet) ||
				Can_rise_up(u.ux, u.uy, &u.uz)) {
			    const char *riseup ="�Te levantas, a trav�s %s!";
			    if(ledger_no(&u.uz) == 1) {
			        pline(riseup, del(ceiling(u.ux,u.uy)));
				goto_level(&earth_level, FALSE, FALSE, FALSE);
			    } else {
			        register int newlev = depth(&u.uz)-1;
				d_level newlevel;

				get_level(&newlevel, newlev);
				if(on_level(&newlevel, &u.uz)) {
				    pline("Sab�a mala.");
				    break;
				} else pline(riseup, del(ceiling(u.ux,u.uy)));
				goto_level(&newlevel, FALSE, FALSE, FALSE);
			    }
			}
			else pline("Tienes un sentido inquieto.");
			break;
		}
		pluslvl(FALSE);
		if (otmp->blessed)
			/* blessed potions place you at a random spot in the
			 * middle of the new level instead of the low point
			 */
			u.uexp = rndexp(TRUE);
		break;
	case POT_HEALING:
		Te_sientes("mejor.");
		healup(d(6 + 2 * bcsign(otmp), 4),
		       !otmp->cursed ? 1 : 0, !!otmp->blessed, !otmp->cursed);
		exercise(A_CON, TRUE);
		break;
	case POT_EXTRA_HEALING:
		Te_sientes("mucho mejor.");
		healup(d(6 + 2 * bcsign(otmp), 8),
		       otmp->blessed ? 5 : !otmp->cursed ? 2 : 0,
		       !otmp->cursed, TRUE);
		(void) make_hallucinated(0L,TRUE,0L);
		exercise(A_CON, TRUE);
		exercise(A_STR, TRUE);
		break;
	case POT_FULL_HEALING:
		Te_sientes("completamente curad%c.",
			poly_gender()==1? 'a' : 'o');
		healup(400, 4+4*bcsign(otmp), !otmp->cursed, TRUE);
		/* Restore one lost level if blessed */
		if (otmp->blessed && u.ulevel < u.ulevelmax) {
		    /* when multiple levels have been lost, drinking
		       multiple potions will only get half of them back */
		    u.ulevelmax -= 1;
		    pluslvl(FALSE);
		}
		make_hallucinated(0L,TRUE,0L);
		exercise(A_STR, TRUE);
		exercise(A_CON, TRUE);
		break;
	case POT_LEVITATION:
	case SPE_LEVITATION:
		if (otmp->cursed) HLevitation &= ~I_SPECIAL;
		if(!Levitation) {
			/* kludge to ensure proper operation of float_up() */
			HLevitation = 1;
			float_up();
			/* reverse kludge */
			HLevitation = 0;
			if (otmp->cursed && !Is_waterlevel(&u.uz)) {
	if((u.ux != xupstair || u.uy != yupstair)
	   && (u.ux != sstairs.sx || u.uy != sstairs.sy || !sstairs.up)
	   && (!xupladder || u.ux != xupladder || u.uy != yupladder)
	) {
					pline("Te golpes tu %s contra %s.",
						body_part(HEAD),
						ceiling(u.ux,u.uy));
					losehp(uarmh ? 1 : rnd(10),
						"colisi�n con el techo",
						KILLED_BY);
				} else (void) doup();
			}
		} else
			nothing++;
		if (otmp->blessed) {
		    incr_itimeout(&HLevitation, rn1(50,250));
		    HLevitation |= I_SPECIAL;
		} else incr_itimeout(&HLevitation, rn1(140,10));
		spoteffects(FALSE);	/* for sinks */
		break;
	case POT_GAIN_ENERGY:			/* M. Stephenson */
		{	register int num;
			if(otmp->cursed)
			    Te_sientes("deslustrad%c.",
				poly_gender()==1? 'a' : 'o');
			else
			    pline("Energ�as m�gicas corren trav�s de tu cuerpo.");
			num = rnd(5) + 5 * otmp->blessed + 1;
			u.uenmax += (otmp->cursed) ? -num : num;
			u.uen += (otmp->cursed) ? -num : num;
			if(u.uenmax <= 0) u.uenmax = 0;
			if(u.uen <= 0) u.uen = 0;
			flags.botl = 1;
			exercise(A_WIS, TRUE);
		}
		break;
	case POT_OIL:				/* P. Winner */
		{
			boolean good_for_you = FALSE;

			if (otmp->lamplit) {
			    if (likes_fire(youmonst.data)) {
				pline("Ahh, una bebida refrescante.");
				good_for_you = TRUE;
			    } else {
				pline("Te quemas %s.", the(body_part(FACE)));
				losehp(d(Fire_resistance ? 1 : 3, 4),
				       "poci�n quemante de aceite", KILLED_BY_AN);
			    }
			} else if(otmp->cursed)
			    pline("Esta sabe a aceite de ricino.");
			else
			    pline("�Esa era suave!");
			exercise(A_WIS, good_for_you);
		}
		break;
	case POT_ACID:
		if (Acid_resistance)
			/* Not necessarily a creature who _likes_ acid */
			pline("Esta sabe %s.", Hallucination ? "picante" : "agria");
		else {
			pline("�Esta quema%s!", otmp->blessed ? " un poco" :
					otmp->cursed ? " mucho" : " como �cido");
			losehp(d(otmp->cursed ? 2 : 1, otmp->blessed ? 4 : 8),
					"poci�n de �cido", KILLED_BY_AN);
			exercise(A_CON, FALSE);
		}
		if (Stoned) fix_petrification();
		unkn++; /* holy/unholy water can burn like acid too */
		break;
	case POT_POLYMORPH:
		Te_sientes("un poco %s.",
			Hallucination ? "normal"
				      : feminize("extra�o", poly_gender()==1));
		if (!Unchanging) polyself(FALSE);
		break;
	default:
		impossible("�Qu� poci�n extra�a! (%u)", otmp->otyp);
		return(0);
	}
	return(-1);
}

void
healup(nhp, nxtra, curesick, cureblind)
	int nhp, nxtra;
	register boolean curesick, cureblind;
{
	if (nhp) {
		if (Upolyd) {
			u.mh += nhp;
			if (u.mh > u.mhmax) u.mh = (u.mhmax += nxtra);
		} else {
			u.uhp += nhp;
			if(u.uhp > u.uhpmax) u.uhp = (u.uhpmax += nxtra);
		}
	}
	if(cureblind)	make_blinded(0L,TRUE);
	if(curesick)	make_sick(0L, (char *) 0, TRUE, SICK_ALL);
	flags.botl = 1;
	return;
}

void
strange_feeling(obj,txt)
register struct obj *obj;
register const char *txt;
{
	if(flags.beginner || !txt)
		pline("Tienes un sentido %s por un momento, y entonces pasa.",
		Hallucination ? "normal" : "extra�o");
	else
		pline(txt);

	if(!obj)	/* e.g., crystal ball finds no traps */
		return;

	if(obj->dknown && !objects[obj->otyp].oc_name_known &&
						!objects[obj->otyp].oc_uname)
		docall(obj);
	useup(obj);
}

const char *bottlenames[] = {
	"La botella", "La redoma", "El vaso", "La tarra", "El frasco",
	"La jarra", "La ampolleta"
};


const char *
bottlename()
{
	return bottlenames[rn2(SIZE(bottlenames))];
}

void
potionhit(mon, obj, your_fault)
register struct monst *mon;
register struct obj *obj;
boolean your_fault;
{
	register const char *botlnam = bottlename();
	boolean isyou = (mon == &youmonst);
	int distance;

	if(isyou) {
		distance = 0;
		pline("%s se estrella con tu %s y rompe en pedazos.",
			botlnam, body_part(HEAD));
		losehp(rnd(2), "poci�n tirada", KILLED_BY_AN);
	} else {
		distance = distu(mon->mx,mon->my);
		if (!cansee(mon->mx,mon->my)) pline("�Crash!");
		else {
		    char *mnam = mon_nam(mon);
		    char buf[BUFSZ];

		    if(has_head(mon->data)) {
			Sprintf(buf, "%s %s",
				(notonhead ? "el cuerpo" : "la cabeza"),
				del(mnam));
		    } else {
			Strcpy(buf, mnam);
		    }
		    pline("%s se estrella con %s y rompe en pedazos.",
			   botlnam, buf);
		}
		if(rn2(5) && mon->mhp > 1)
			mon->mhp--;
	}

	/* oil doesn't instantly evaporate */
	if (obj->otyp != POT_OIL && cansee(mon->mx,mon->my))
		pline("%s.", Tobjnam(obj, "se evapora"));

    if (isyou) {
	switch (obj->otyp) {
	case POT_OIL:
		if (obj->lamplit)
		    splatter_burning_oil(u.ux, u.uy);
		break;
	case POT_POLYMORPH:
		Te_sientes("un poco %s.", Hallucination ? "normal" : feminize("extra�o", poly_gender()==1));
		if (!Unchanging && !Antimagic) polyself(FALSE);
		break;
	case POT_ACID:
		if (!Acid_resistance) {
		    pline("�Esta quema%s!", obj->blessed ? " un poco" :
				    obj->cursed ? " mucho" : "");
		    losehp(d(obj->cursed ? 2 : 1, obj->blessed ? 4 : 8),
				    "poci�n de �cido", KILLED_BY_AN);
		}
		break;
	}
    } else {
	boolean angermon = TRUE;

	if (!your_fault) angermon = FALSE;
	switch (obj->otyp) {
	case POT_HEALING:
	case POT_EXTRA_HEALING:
	case POT_FULL_HEALING:
		if (mon->data == &mons[PM_PESTILENCE]) goto do_illness;
		/*FALLTHRU*/
	case POT_RESTORE_ABILITY:
	case POT_GAIN_ABILITY:
 do_healing:
		angermon = FALSE;
		if(mon->mhp < mon->mhpmax) {
		    mon->mhp = mon->mhpmax;
		    if (canseemon(mon))
			pline("%s parece san%c y saludable otra vez.",
				Monnam(mon),
				mon_gender(mon)? 'a' : 'o');
		}
		break;
	case POT_SICKNESS:
		if (mon->data == &mons[PM_PESTILENCE]) goto do_healing;
		if (dmgtype(mon->data, AD_DISE) ||
			   dmgtype(mon->data, AD_PEST) || /* won't happen, see prior goto */
			   resists_poison(mon)) {
		    if (canseemon(mon))
			pline("%s no parece da�ad%c.", Monnam(mon),
				mon_gender(mon)? 'a' : 'o');
		    break;
		}
 do_illness:
		if((mon->mhpmax > 3) && !resist(mon, POTION_CLASS, 0, NOTELL))
			mon->mhpmax /= 2;
		if((mon->mhp > 2) && !resist(mon, POTION_CLASS, 0, NOTELL))
			mon->mhp /= 2;
		if (mon->mhp > mon->mhpmax) mon->mhp = mon->mhpmax;
		if (canseemon(mon))
		    pline("%s parece bastante enferm%c.", Monnam(mon),
			    mon_gender(mon)? 'a' : 'o');
		break;
	case POT_CONFUSION:
	case POT_BOOZE:
		if(!resist(mon, POTION_CLASS, 0, NOTELL))  mon->mconf = TRUE;
		break;
	case POT_INVISIBILITY:
		angermon = FALSE;
		mon_set_minvis(mon);
		break;
	case POT_SLEEPING:
		/* wakeup() doesn't rouse victims of temporary sleep */
		if (sleep_monst(mon, rnd(12), POTION_CLASS)) {
		    pline("%s se duerme.", Monnam(mon));
		    slept_monst(mon);
		}
		break;
	case POT_PARALYSIS:
		if (mon->mcanmove) {
			mon->mcanmove = 0;
			/* really should be rnd(5) for consistency with players
			 * breathing potions, but...
			 */
			mon->mfrozen = rnd(25);
		}
		break;
	case POT_SPEED:
		angermon = FALSE;
		mon_adjust_speed(mon, 1, obj);
		break;
	case POT_BLINDNESS:
		if(haseyes(mon->data)) {
		    register int btmp = 64 + rn2(32) +
			rn2(32) * !resist(mon, POTION_CLASS, 0, NOTELL);
		    btmp += mon->mblinded;
		    mon->mblinded = min(btmp,127);
		    mon->mcansee = 0;
		}
		break;
	case POT_WATER:
		if (is_undead(mon->data) || is_demon(mon->data) ||
			is_were(mon->data)) {
		    if (obj->blessed) {
			pline("�%s de dolor!",
			      Mon_verb_mon(mon,
			          is_silent(mon->data) ? "se retuerce" : "grita",
				  (struct monst *)0));
			mon->mhp -= d(2,6);
			/* should only be by you */
			if (mon->mhp < 1) killed(mon);
			else if (is_were(mon->data) && !is_human(mon->data))
			    new_were(mon);	/* revert to human */
		    } else if (obj->cursed) {
			angermon = FALSE;
			if (canseemon(mon))
			    pline("%s parece m�s san%c.", Monnam(mon),
				    poly_gender()==1? 'a' : 'o');
			mon->mhp += d(2,6);
			if (mon->mhp > mon->mhpmax) mon->mhp = mon->mhpmax;
			if (is_were(mon->data) && is_human(mon->data) &&
				!Protection_from_shape_changers)
			    new_were(mon);	/* transform into beast */
		    }
		} else if(mon->data == &mons[PM_GREMLIN]) {
		    angermon = FALSE;
		    (void)split_mon(mon, (struct monst *)0);
		} else if(mon->data == &mons[PM_IRON_GOLEM]) {
		    if (canseemon(mon))
			pline("%s se oxida.", Monnam(mon));
		    mon->mhp -= d(1,6);
		    /* should only be by you */
		    if (mon->mhp < 1) killed(mon);
		}
		break;
	case POT_OIL:
		if (obj->lamplit)
			splatter_burning_oil(mon->mx, mon->my);
		break;
	case POT_ACID:
		if (!resists_acid(mon) && !resist(mon, POTION_CLASS, 0, NOTELL)) {
		    pline("�%s de dolor!",
			  Mon_verb_mon(mon,
			      is_silent(mon->data) ? "se retuerce" : "grita",
			      (struct monst *)0));
		    mon->mhp -= d(obj->cursed ? 2 : 1, obj->blessed ? 4 : 8);
		    if (mon->mhp < 1) {
			if (your_fault)
			    killed(mon);
			else
			    monkilled(mon, "", AD_ACID);
		    }
		}
		break;
	case POT_POLYMORPH:
		(void) bhitm(mon, obj);
		break;
/*
	case POT_GAIN_LEVEL:
	case POT_LEVITATION:
	case POT_FRUIT_JUICE:
	case POT_MONSTER_DETECTION:
	case POT_OBJECT_DETECTION:
		break;
*/
	}
	if (angermon)
	    wakeup(mon);
	else
	    mon->msleeping = 0;
    }

	/* Note: potionbreathe() does its own docall() */
	if ((distance==0 || ((distance < 3) && rn2(5))) &&
	    (!breathless(youmonst.data) || haseyes(youmonst.data)))
		potionbreathe(obj);
	else if (obj->dknown && !objects[obj->otyp].oc_name_known &&
		   !objects[obj->otyp].oc_uname && cansee(mon->mx,mon->my))
		docall(obj);
	if(*u.ushops && obj->unpaid) {
	        register struct monst *shkp =
			shop_keeper(*in_rooms(u.ux, u.uy, SHOPBASE));

		if(!shkp)
		    obj->unpaid = 0;
		else {
		    (void)stolen_value(obj, u.ux, u.uy,
				 (boolean)shkp->mpeaceful, FALSE);
		    subfrombill(obj, shkp);
		}
	}
	obfree(obj, (struct obj *)0);
}

/* vapors are inhaled or get in your eyes */
void
potionbreathe(obj)
register struct obj *obj;
{
	register int i, ii, isdone, kn = 0;

	switch(obj->otyp) {
	case POT_RESTORE_ABILITY:
	case POT_GAIN_ABILITY:
		if(obj->cursed) {
		    if (!breathless(youmonst.data))
			pline("�Ulch!  �Esta poci�n huele terrible!");
		    else if (haseyes(youmonst.data)) {
			int numeyes = eyecount(youmonst.data);
			pline("�Tu%s %s pica%s!",
			     plur(numeyes),
			     (numeyes == 1) ? body_part(EYE) : makeplural(body_part(EYE)),
			     (numeyes == 1) ? "" : "n");
		    }
		    break;
		} else {
		    i = rn2(A_MAX);		/* start at a random point */
		    for(isdone = ii = 0; !isdone && ii < A_MAX; ii++) {
			if(ABASE(i) < AMAX(i)) {
			    ABASE(i)++;
			    /* only first found if not blessed */
			    isdone = !(obj->blessed);
			    flags.botl = 1;
			}
			if(++i >= A_MAX) i = 0;
		    }
		}
		break;
	case POT_FULL_HEALING:
		if (Upolyd && u.mh < u.mhmax) u.mh++, flags.botl = 1;
		if (u.uhp < u.uhpmax) u.uhp++, flags.botl = 1;
		/*FALL THROUGH*/
	case POT_EXTRA_HEALING:
		if (Upolyd && u.mh < u.mhmax) u.mh++, flags.botl = 1;
		if (u.uhp < u.uhpmax) u.uhp++, flags.botl = 1;
		/*FALL THROUGH*/
	case POT_HEALING:
		if (Upolyd && u.mh < u.mhmax) u.mh++, flags.botl = 1;
		if (u.uhp < u.uhpmax) u.uhp++, flags.botl = 1;
		exercise(A_CON, TRUE);
		break;
	case POT_SICKNESS:
		if (!Role_if(PM_HEALER)) {
			if (Upolyd) {
			    if (u.mh <= 5) u.mh = 1; else u.mh -= 5;
			} else {
			    if (u.uhp <= 5) u.uhp = 1; else u.uhp -= 5;
			}
			flags.botl = 1;
			exercise(A_CON, FALSE);
		}
		break;
	case POT_HALLUCINATION:
		pline("Tienes una visi�n por un momento.");
		break;
	case POT_CONFUSION:
	case POT_BOOZE:
		if(!Confusion)
			Te_sientes("bastante desmayad%c.",
				poly_gender()==1? 'a' : 'o');
		make_confused(itimeout_incr(HConfusion, rnd(5)), FALSE);
		break;
	case POT_INVISIBILITY:
		if (!Blind && !Invis) {
		    kn++;
		    pline("�Por un instante %s!",
			See_invisible ? "pudiste ver trav�s de ti"
			: "no pudiste verte");
		}
		break;
	case POT_PARALYSIS:
		kn++;
		if (!Free_action) {
		    pline("%s parece detenerte.", Something);
		    nomul(-rnd(5));
		    nomovemsg = You_can_move_again;
		    exercise(A_DEX, FALSE);
		} else pline("Te atiesas por un momento.");
		break;
	case POT_SLEEPING:
		kn++;
		if (!Free_action && !Sleep_resistance) {
		    Te_sientes("bastante cansad%c.",
			    poly_gender()==1? 'a' : 'o');
		    nomul(-rnd(5));
		    nomovemsg = You_can_move_again;
		    exercise(A_DEX, FALSE);
		} else pline("Bostezas.");
		break;
	case POT_SPEED:
		if (!Fast) pline("Tus rodillas parecen m�s flexibles ahora.");
		incr_itimeout(&HFast, rnd(5));
		exercise(A_DEX, TRUE);
		break;
	case POT_BLINDNESS:
		if (!Blind && !u.usleep) {
		    kn++;
		    pline("De repente, todo se pone oscuro.");
		}
		make_blinded(itimeout_incr(Blinded, rnd(5)), FALSE);
		if (!Blind && !u.usleep) pline(vision_clears);
		break;
	case POT_WATER:
		if(u.umonnum == PM_GREMLIN) {
		    (void)split_mon(&youmonst, (struct monst *)0);
		} else if (u.ulycn >= LOW_PM) {
		    /* vapor from [un]holy water will trigger
		       transformation but won't cure lycanthropy */
		    if (obj->blessed && youmonst.data == &mons[u.ulycn])
			you_unwere(FALSE);
		    else if (obj->cursed && !Upolyd)
			you_were();
		}
		break;
	case POT_ACID:
	case POT_POLYMORPH:
		exercise(A_CON, FALSE);
		break;
/*
	case POT_GAIN_LEVEL:
	case POT_LEVITATION:
	case POT_FRUIT_JUICE:
	case POT_MONSTER_DETECTION:
	case POT_OBJECT_DETECTION:
	case POT_OIL:
		break;
*/
	}
	/* note: no obfree() */
	if (obj->dknown) {
	    if (kn)
		makeknown(obj->otyp);
	    else if (!objects[obj->otyp].oc_name_known &&
						!objects[obj->otyp].oc_uname)
		docall(obj);
	}
}

STATIC_OVL short
mixtype(o1, o2)
register struct obj *o1, *o2;
/* returns the potion type when o1 is dipped in o2 */
{
	/* cut down on the number of cases below */
	if (o1->oclass == POTION_CLASS &&
	    (o2->otyp == POT_GAIN_LEVEL ||
	     o2->otyp == POT_GAIN_ENERGY ||
	     o2->otyp == POT_HEALING ||
	     o2->otyp == POT_EXTRA_HEALING ||
	     o2->otyp == POT_FULL_HEALING ||
	     o2->otyp == POT_ENLIGHTENMENT ||
	     o2->otyp == POT_FRUIT_JUICE)) {
		struct obj *swp;

		swp = o1; o1 = o2; o2 = swp;
	}

	switch (o1->otyp) {
		case POT_HEALING:
			switch (o2->otyp) {
			    case POT_SPEED:
			    case POT_GAIN_LEVEL:
			    case POT_GAIN_ENERGY:
				return POT_EXTRA_HEALING;
			}
		case POT_EXTRA_HEALING:
			switch (o2->otyp) {
			    case POT_GAIN_LEVEL:
			    case POT_GAIN_ENERGY:
				return POT_FULL_HEALING;
			}
		case POT_FULL_HEALING:
			switch (o2->otyp) {
			    case POT_GAIN_LEVEL:
			    case POT_GAIN_ENERGY:
				return POT_GAIN_ABILITY;
			}
		case UNICORN_HORN:
			switch (o2->otyp) {
			    case POT_SICKNESS:
				return POT_FRUIT_JUICE;
			    case POT_HALLUCINATION:
			    case POT_BLINDNESS:
			    case POT_CONFUSION:
				return POT_WATER;
			}
			break;
		case AMETHYST:		/* "a-methyst" == "not intoxicated" */
			if (o2->otyp == POT_BOOZE)
			    return POT_FRUIT_JUICE;
			break;
		case POT_GAIN_LEVEL:
		case POT_GAIN_ENERGY:
			switch (o2->otyp) {
			    case POT_CONFUSION:
				return (rn2(3) ? POT_BOOZE : POT_ENLIGHTENMENT);
			    case POT_HEALING:
				return POT_EXTRA_HEALING;
			    case POT_EXTRA_HEALING:
				return POT_FULL_HEALING;
			    case POT_FULL_HEALING:
				return POT_GAIN_ABILITY;
			    case POT_FRUIT_JUICE:
				return POT_SEE_INVISIBLE;
			    case POT_BOOZE:
				return POT_HALLUCINATION;
			}
			break;
		case POT_FRUIT_JUICE:
			switch (o2->otyp) {
			    case POT_SICKNESS:
				return POT_SICKNESS;
			    case POT_SPEED:
				return POT_BOOZE;
			    case POT_GAIN_LEVEL:
			    case POT_GAIN_ENERGY:
				return POT_SEE_INVISIBLE;
			}
			break;
		case POT_ENLIGHTENMENT:
			switch (o2->otyp) {
			    case POT_LEVITATION:
				if (rn2(3)) return POT_GAIN_LEVEL;
				break;
			    case POT_FRUIT_JUICE:
				return POT_BOOZE;
			    case POT_BOOZE:
				return POT_CONFUSION;
			}
			break;
	}

	return 0;
}


boolean
get_wet(obj)
register struct obj *obj;
/* returns TRUE if something happened (potion should be used up) */
{
	char Your_buf[BUFSZ];
	const char *vplural;

	if (snuff_lit(obj)) return(TRUE);

	if (obj->greased) {
		grease_protect(obj,(char *)0,&youmonst);
		return(FALSE);
	}
	(void) Shk_Your(Your_buf, obj, (char *)0);
	vplural = obj->quan>1L? "n" : "";
	/* (Rusting shop goods ought to be charged for.) */
	switch (obj->oclass) {
	    case POTION_CLASS:
		if (obj->otyp == POT_WATER) return FALSE;
		/* KMH -- Water into acid causes an explosion */
		if (obj->otyp == POT_ACID) {
			pline("�Hierve vigorosamente!");
			pline("�Eres asid%c en la explosi�n!",
				poly_gender()==1? 'a' : 'o');
			losehp(rnd(10), "qu�mica elementaria", KILLED_BY);
			makeknown(obj->otyp);
			update_inventory();
			return (TRUE);
		}
		pline("%s diluye%s%s.", Your_buf, vplural,
		      obj->odiluted ? " m�s" : "");
		if(obj->unpaid && costly_spot(u.ux, u.uy)) {
		    verbalize("Usted la diluye, usted la paga.");
		    bill_dummy_object(obj);
		}
		if (obj->odiluted) {
			obj->odiluted = 0;
#ifdef UNIXPC
			obj->blessed = FALSE;
			obj->cursed = FALSE;
#else
			obj->blessed = obj->cursed = FALSE;
#endif
			obj->otyp = POT_WATER;
		} else obj->odiluted++;
		update_inventory();
		return TRUE;
	    case SCROLL_CLASS:
		if (obj->otyp != SCR_BLANK_PAPER
#ifdef MAIL
		    && obj->otyp != SCR_MAIL
#endif
		    ) {
			if (!Blind) {
				boolean oq1 = obj->quan == 1L;
				pline("%s pergamino%s se %s.",
					oq1 ? "El" : "Los",
					oq1 ? "" : "s",
					otense(obj, "descolora"));
			}
			if(obj->unpaid && costly_spot(u.ux, u.uy)) {
			    verbalize("Usted lo borra, usted lo paga.");
			    bill_dummy_object(obj);
			}
			obj->otyp = SCR_BLANK_PAPER;
			obj->spe = 0;
			update_inventory();
			return TRUE;
		} else break;
	    case SPBOOK_CLASS:
		if (obj->otyp != SPE_BLANK_PAPER) {

			if (obj->otyp == SPE_BOOK_OF_THE_DEAD) {
	pline("%s de repente se calenta; vapor sube y el libro se queda seco.",
				The(xname(obj)));
			} else {
			    if (!Blind) {
				    boolean oq1 = obj->quan == 1L;
				    pline("%s libro%s se %s.",
					    oq1 ? "El" : "Los",
					    oq1 ? "" : "s",
					    otense(obj, "descolora"));
			    }
			    if(obj->unpaid && costly_spot(u.ux, u.uy)) {
				verbalize("Usted lo borra, usted lo paga.");
			        bill_dummy_object(obj);
			    }
			    obj->otyp = SPE_BLANK_PAPER;
			    update_inventory();
			}
			return TRUE;
		}
		break;
	    case WEAPON_CLASS:
	    /* Just "fall through" to generic rustprone check for now. */
	    /* fall through */
	    default:
		if (!obj->oerodeproof && is_rustprone(obj) &&
		    (obj->oeroded < MAX_ERODE) && !rn2(2)) {
			pline("%s se oxida%s %s.",
			      Your_buf, vplural, obj->oeroded ? "m�s" : "algo");
			obj->oeroded++;
			update_inventory();
			return TRUE;
		} else break;
	}
	pline("%s se pone%s mojad%c.", Your_buf, vplural,
		isfeminine(Your_buf)? 'a' : 'o');
	return FALSE;
}

int
dodip()
{
	register struct obj *potion, *obj;
	struct obj *singlepotion;
	const char *tmp;
	uchar here;
	char allowall[2];
	short mixture;
	char qbuf[QBUFSZ], Your_buf[BUFSZ];
	const char *vplural;

	allowall[0] = ALL_CLASSES; allowall[1] = '\0';
	if(!(obj = getobj(allowall, "", "mojar")))
		return(0);

	here = levl[u.ux][u.uy].typ;
	vplural = obj->quan > 1L? "n" : "";
	/* Is there a fountain to dip into here? */
	if (IS_FOUNTAIN(here)) {
		sprintf(qbuf, "�Mojarlo%s en la fuente?", plur(obj->quan));
		if(yn(qbuf) == 'y') {
			dipfountain(obj);
			return(1);
		}
	} else if (is_pool(u.ux,u.uy)) {
		tmp = waterbody_name(u.ux,u.uy);
		Sprintf(qbuf, "�Mojarlo%s en %s?", plur(obj->quan), tmp);
		if (yn(qbuf) == 'y') {
		    if (Levitation) {
			floating_above(tmp);
#ifdef STEED
		    } else if (u.usteed && !is_swimmer(u.usteed->data) &&
			    P_SKILL(P_RIDING) < P_BASIC) {
			rider_cant_reach(); /* not skilled enough to reach */
#endif
		    } else {
			(void) get_wet(obj);
			if (obj->otyp == POT_ACID) useup(obj);
		    }
		    return 1;
		}
	}

	if(!(potion = getobj(beverages, "en", "mojar")))
		return(0);
	if (potion == obj && potion->quan == 1L) {
		pline("��sta es una botella de poci�n, no una botella Klein!");
		return 0;
	}
	potion->in_use = TRUE;		/* assume it will be used up */
	if(potion->otyp == POT_WATER) {
		boolean useeit = !Blind;
		if (useeit) (void) Shk_Your(Your_buf, obj, (char *)0);
		if (potion->blessed) {
			if (obj->cursed) {
				if (useeit)
				    pline("%s suavemente %s %s.",
					  Your_buf,
					  otense(obj, "alumbra"),
					  feminize(hcolor(NH_AMBER), isfeminine(Your_buf)));
				uncurse(obj);
				obj->bknown=1;
	poof:
				if(!(objects[potion->otyp].oc_name_known) &&
				   !(objects[potion->otyp].oc_uname))
					docall(potion);
				useup(potion);
				return(1);
			} else if(!obj->blessed) {
				if (useeit) {
				    tmp = hcolor(NH_LIGHT_BLUE);
				    pline("%s suavemente %s con una aura %s.",
					  Your_buf,
					  otense(obj, "alumbra"),
					  feminize(tmp, 1));
				}
				bless(obj);
				obj->bknown=1;
				goto poof;
			}
		} else if (potion->cursed) {
			if (obj->blessed) {
				if (useeit)
				    pline("%s %s %s.",
					  Your_buf,
					  otense(obj, "alumbra"),
					  feminize(hcolor((const char *)"marr�n"),
						   isfeminine(Your_buf)));
				unbless(obj);
				obj->bknown=1;
				goto poof;
			} else if(!obj->cursed) {
				if (useeit) {
				    tmp = hcolor(NH_BLACK);
				    pline("%s %s con una aura %s.",
					  Your_buf,
					  otense(obj, "alumbra"),
					  feminize(tmp, 1));
				}
				curse(obj);
				obj->bknown=1;
				goto poof;
			}
		} else
			if (get_wet(obj))
			    goto poof;
	} else if (obj->otyp == POT_POLYMORPH ||
		potion->otyp == POT_POLYMORPH) {
	    /* some objects can't be polymorphed */
	    if (obj->otyp == potion->otyp ||	/* both POT_POLY */
		    obj->otyp == WAN_POLYMORPH ||
		    obj->otyp == SPE_POLYMORPH ||
		    obj == uball || obj == uskin ||
		    obj_resists(obj->otyp == POT_POLYMORPH ?
				potion : obj, 5, 95)) {
		pline(nothing_happens);
	    } else {
	    	boolean was_wep = FALSE, was_swapwep = FALSE, was_quiver = FALSE;
		short save_otyp = obj->otyp;
		/* KMH, conduct */
		u.uconduct.polypiles++;

		if (obj == uwep) was_wep = TRUE;
		else if (obj == uswapwep) was_swapwep = TRUE;
		else if (obj == uquiver) was_quiver = TRUE;

		obj = poly_obj(obj, STRANGE_OBJECT);

		if (was_wep) setuwep(obj);
		else if (was_swapwep) setuswapwep(obj);
		else if (was_quiver) setuqwep(obj);

		if (obj->otyp != save_otyp) {
			makeknown(POT_POLYMORPH);
			useup(potion);
			prinv((char *)0, obj, 0L);
			return 1;
		} else {
			pline("Parece que no pasa nada.");
			goto poof;
		}
	    }
	    potion->in_use = FALSE;	/* didn't go poof */
	    return(1);
	} else if(obj->oclass == POTION_CLASS && obj->otyp != potion->otyp) {
		/* Mixing potions is dangerous... */
		pline("Las pociones se mezclan...");
		/* KMH, balance patch -- acid is particularly unstable */
		if (obj->cursed || obj->otyp == POT_ACID || !rn2(10)) {
			pline("�BUM!  �Estallan!");
			exercise(A_STR, FALSE);
			if (!breathless(youmonst.data) || haseyes(youmonst.data))
				potionbreathe(obj);
			useup(obj);
			useup(potion);
			losehp(rnd(10), "r�faga alqu�mica", KILLED_BY_AN);
			return(1);
		}

		obj->blessed = obj->cursed = obj->bknown = 0;
		if (Blind || Hallucination) obj->dknown = 0;

		if ((mixture = mixtype(obj, potion)) != 0) {
			obj->otyp = mixture;
		} else {
		    switch (obj->odiluted ? 1 : rnd(8)) {
			case 1:
				obj->otyp = POT_WATER;
				break;
			case 2:
			case 3:
				obj->otyp = POT_SICKNESS;
				break;
			case 4:
				{
				  struct obj *otmp;
				  otmp = mkobj(POTION_CLASS,FALSE);
				  obj->otyp = otmp->otyp;
				  obfree(otmp, (struct obj *)0);
				}
				break;
			default:
				if (!Blind)
			  pline("La mezcla deslumbra brillantemente y se evapora.");
				useup(obj);
				useup(potion);
				return(1);
		    }
		}

		obj->odiluted = (obj->otyp != POT_WATER);

		if (obj->otyp == POT_WATER && !Hallucination) {
			pline("La mezcla burbuja%s.",
				Blind ? "" : ", y entonces se clara");
		} else if (!Blind) {
			pline("La mezcla parece %s.",
				feminize(hcolor(OBJ_DESCR(objects[obj->otyp])), TRUE));
		}

		useup(potion);
		return(1);
	}

#ifdef INVISIBLE_OBJECTS
	if (potion->otyp == POT_INVISIBILITY && !obj->oinvis) {
		obj->oinvis = TRUE;
		if (!Blind) {
		    if (!See_invisible) pline("�A d�nde ha ido %s?",
		    		the(xname(obj)));
		    else pline("Noticias un poco de bruma alrededor de %s.",
		    		the(xname(obj)));
		}
		goto poof;
	} else if (potion->otyp == POT_SEE_INVISIBLE && obj->oinvis) {
		obj->oinvis = FALSE;
		if (!Blind) {
		    if (!See_invisible) pline("Ah, �Esto es a donde ha ido %s!",
		    		the(xname(obj)));
		    else pline("La bruma alrededor de %s desaparece.",
		    		the(xname(obj)));
		}
		goto poof;
	}
#endif

	if(is_poisonable(obj)) {
	    if(potion->otyp == POT_SICKNESS && !obj->opoisoned) {
		char buf[BUFSZ];
		if (potion->quan > 1L)
		    Sprintf(buf, "Una de %s", the(xname(potion)));
		else
		    Strcpy(buf, The(xname(potion)));
		pline("%s forma una capa sobre %s.",
		      buf, the(xname(obj)));
		obj->opoisoned = TRUE;
		goto poof;
	    } else if(obj->opoisoned &&
		      (potion->otyp == POT_HEALING ||
		       potion->otyp == POT_EXTRA_HEALING ||
		       potion->otyp == POT_FULL_HEALING)) {
		pline("Una capa se desgasta de %s.", the(xname(obj)));
		obj->opoisoned = 0;
		goto poof;
	    }
	}

	if (potion->otyp == POT_OIL) {
	    boolean wisx = FALSE;
	    if (potion->lamplit) {	/* burning */
		int omat = objects[obj->otyp].oc_material;
		/* the code here should be merged with fire_damage */
		if (catch_lit(obj)) {
		    /* catch_lit does all the work if true */
		} else if (obj->oerodeproof || obj_resists(obj, 5, 95) ||
			   !is_flammable(obj) || obj->oclass == FOOD_CLASS) {
		    pline("%s %s arder por un momento.",
			  Yname2(obj), otense(obj, "parece"));
		} else {
		    if ((omat == PLASTIC || omat == PAPER) && !obj->oartifact)
			obj->oeroded = MAX_ERODE;
		    pline("El aceite ardiente %s %s.",
			    obj->oeroded == MAX_ERODE ? "destruye" : "da�a",
			    yname(obj));
		    if (obj->oeroded == MAX_ERODE) {
			obj_extract_self(obj);
			obfree(obj, (struct obj *)0);
			obj = (struct obj *) 0;
		    } else {
			/* we know it's carried */
			if (obj->unpaid) {
			    /* create a dummy duplicate to put on bill */
			    verbalize("�Usted lo arde, usted lo compra!");
			    bill_dummy_object(obj);
			}
			obj->oeroded++;
		    }
		}
	    } else if (potion->cursed) {
		pline("La poci�n se derrama y cubre tus %s con aceite.",
			  makeplural(body_part(FINGER)));
		incr_itimeout(&Glib, d(2,10));
	    } else if (obj->oclass != WEAPON_CLASS && !is_weptool(obj)) {
		/* the following cases apply only to weapons */
		goto more_dips;
	    /* Oil removes rust and corrosion, but doesn't unburn.
	     * Arrows, etc are classed as metallic due to arrowhead
	     * material, but dipping in oil shouldn't repair them.
	     */
	    } else if ((!is_rustprone(obj) && !is_corrodeable(obj)) ||
			is_ammo(obj) || (!obj->oeroded && !obj->oeroded2)) {
		/* uses up potion, doesn't set obj->greased */
		pline("%s %s con un lustre aceitoso.",
		      Yname2(obj), otense(obj, "destella"));
	    } else {
		const char *nombre = Yname2(obj);
		pline("%s %s menos %s.",
		      nombre, otense(obj, "est�"),
		      feminize(
		          (obj->oeroded && obj->oeroded2) ? "corroido y oxidado" :
			    obj->oeroded ? "oxidado" : "corroido",
			  isfeminine(nombre)));
		if (obj->oeroded > 0) obj->oeroded--;
		if (obj->oeroded2 > 0) obj->oeroded2--;
		wisx = TRUE;
	    }
	    exercise(A_WIS, wisx);
	    makeknown(potion->otyp);
	    useup(potion);
	    return 1;
	}
    more_dips:

	/* Allow filling of MAGIC_LAMPs to prevent identification by player */
	if ((obj->otyp == OIL_LAMP || obj->otyp == MAGIC_LAMP) &&
	   (potion->otyp == POT_OIL)) {
	    /* Turn off engine before fueling, turn off fuel too :-)  */
	    if (obj->lamplit || potion->lamplit) {
		useup(potion);
		explode(u.ux, u.uy, 11, d(6,6), 0, EXPL_FIERY);
		exercise(A_WIS, FALSE);
		return 1;
	    }
	    /* Adding oil to an empty magic lamp renders it into an oil lamp */
	    if ((obj->otyp == MAGIC_LAMP) && obj->spe == 0) {
		obj->otyp = OIL_LAMP;
		obj->age = 0;
	    }
	    if (obj->age > 1000L) {
		const char *nombre = Yname2(obj);
		pline("%s %s llen%c.", nombre, otense(obj, "est�"),
			isfeminine(nombre)? 'a' : 'o');
		potion->in_use = FALSE;	/* didn't go poof */
	    } else {
		pline("Llenas %s con aceite.", yname(obj));
		check_unpaid(potion);	/* Yendorian Fuel Tax */
		obj->age += 2*potion->age;	/* burns more efficiently */
		if (obj->age > 1500L) obj->age = 1500L;
		useup(potion);
		exercise(A_WIS, TRUE);
	    }
	    makeknown(POT_OIL);
	    obj->spe = 1;
	    update_inventory();
	    return 1;
	}

	if ((obj->otyp == UNICORN_HORN || obj->otyp == AMETHYST) &&
	    (mixture = mixtype(obj, potion)) != 0) {
		char oldbuf[BUFSZ], newbuf[BUFSZ];
		short old_otyp = potion->otyp;
		boolean old_dknown = FALSE;
		boolean more_than_one = potion->quan > 1;

		oldbuf[0] = '\0';
		if (potion->dknown) {
		    old_dknown = TRUE;
		    Sprintf(oldbuf, "%s ",
			    feminize(hcolor(OBJ_DESCR(objects[potion->otyp])), TRUE));
		}
		/* with multiple merged potions, split off one and
		   just clear it */
		if (potion->quan > 1L) {
		    singlepotion = splitobj(potion, 1L);
		} else singlepotion = potion;
		
		if(singlepotion->unpaid && costly_spot(u.ux, u.uy)) {
		    verbalize("Usted la usa, usted la compra.");
		    bill_dummy_object(singlepotion);
		}
		singlepotion->otyp = mixture;
		singlepotion->blessed = 0;
		if (mixture == POT_WATER)
		    singlepotion->cursed = singlepotion->odiluted = 0;
		else
		    singlepotion->cursed = obj->cursed;  /* odiluted left as-is */
		singlepotion->bknown = FALSE;
		if (Blind) {
		    singlepotion->dknown = FALSE;
		} else {
		    char qualifier[BUFSZ];

		    singlepotion->dknown = !Hallucination;
		    if (mixture == POT_WATER && singlepotion->dknown)
			Sprintf(newbuf, "se aclara");
		    else
			Sprintf(newbuf, "se hace %s",
				hcolor(OBJ_DESCR(objects[mixture])));
		    if (more_than_one)
			Sprintf(qualifier, "en que mojaste %s ",
				the(xname(obj)));
		    else
			Strcpy(qualifier, "");
		    pline("La poci�n %s%s%s.", oldbuf, qualifier, newbuf);
		    if(!objects[old_otyp].oc_uname &&
			!objects[old_otyp].oc_name_known && old_dknown) {
			struct obj fakeobj;
			fakeobj = zeroobj;
			fakeobj.dknown = 1;
			fakeobj.otyp = old_otyp;
			fakeobj.oclass = POTION_CLASS;
			docall(&fakeobj);
		    }
		}
		obj_extract_self(singlepotion);
		singlepotion = hold_another_object(singlepotion,
					"�Barajas y dejas caer %s!",
					doname(singlepotion), (const char *)0);
		update_inventory();
		return(1);
	}

	pline("Interesante...");
	return(1);
}


void
djinni_from_bottle(obj)
register struct obj *obj;
{
	struct monst *mtmp;
	int chance;

	if(!(mtmp = makemon(&mons[PM_DJINNI], u.ux, u.uy, NO_MM_FLAGS))){
		pline("Descubres que la botella est� vac�a.");
		return;
	}

	if (!Blind) {
		pline("�En una nube de humo, %s emerge!", a_monnam(mtmp));
		pline("%s habla.", Monnam(mtmp));
	} else {
		pline("Hueles humos �cridos.");
		pline("%s habla.", Something);
	}

	chance = rn2(5);
	if (obj->blessed) chance = (chance == 4) ? rnd(4) : 0;
	else if (obj->cursed) chance = (chance == 0) ? rn2(4) : 4;
	/* 0,1,2,3,4:  b=80%,5,5,5,5; nc=20%,20,20,20,20; c=5%,5,5,5,80 */

	switch (chance) {
	case 0 : verbalize("Estoy en su deuda.  �Voy a darl%c un deseo!",
			poly_gender()==1? 'a' : 'e');
		makewish(FALSE);
		mongone(mtmp);
		break;
	case 1 : verbalize("�Gracias por librarme!");
		(void) tamedog(mtmp, (struct obj *)0);
		break;
	case 2 : verbalize("�Usted me ha librado!");
		mtmp->mpeaceful = TRUE;
		set_malign(mtmp);
		break;
	case 3 : verbalize("�A buenas horas!");
		pline("%s se desvanece.", Monnam(mtmp));
		mongone(mtmp);
		break;
	default: verbalize("Me has molestado, �tont%c!",
			poly_gender()==1? 'a' : 'o');
		break;
	}
}

/* clone a gremlin or mold (2nd arg non-null implies heat as the trigger);
   hit points are cut in half (odd HP stays with original) */
struct monst *
split_mon(mon, mtmp)
struct monst *mon,	/* monster being split */
	     *mtmp;	/* optional attacker whose heat triggered it */
{
	struct monst *mtmp2;
	char reason[BUFSZ];

	reason[0] = '\0';
	if (mtmp) {
	    if (mtmp == &youmonst)
		Sprintf(reason, " de tu calor");
	    else
		Sprintf(reason, " del calor %s", del(mon_nam(mtmp)));
	}

	if (mon == &youmonst) {
	    mtmp2 = cloneu();
	    if (mtmp2) {
		mtmp2->mhpmax = u.mhmax / 2;
		u.mhmax -= mtmp2->mhpmax;
		flags.botl = 1;
		pline("�Te multiplicas%s!", reason);
	    }
	} else {
	    mtmp2 = clone_mon(mon, 0, 0);
	    if (mtmp2) {
		mtmp2->mhpmax = mon->mhpmax / 2;
		mon->mhpmax -= mtmp2->mhpmax;
		if (canspotmon(mon))
		    pline("�%s se multiplica%s!", Monnam(mon), reason);
	    }
	}
	return mtmp2;
}

#endif /* OVLB */

/*potion.c*/
