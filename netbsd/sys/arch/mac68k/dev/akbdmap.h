/*	$NetBSD: akbdmap.h,v 1.2 2000/02/14 07:01:45 scottr Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Juergen Hannken-Illjes.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/* XXX This list is incomplete. */

#define KC(n) KS_KEYCODE(n)

static const keysym_t akbd_keydesc_us[] = {
/*  pos      command		normal		shifted */
    KC(0),			KS_a,
    KC(1),			KS_s,
    KC(2),			KS_d,
    KC(3),			KS_f,
    KC(4),			KS_h,
    KC(5),			KS_g,
    KC(6),			KS_z,
    KC(7),			KS_x,
    KC(8),			KS_c,
    KC(9),			KS_v,

    KC(11),			KS_b,
    KC(12),			KS_q,
    KC(13),			KS_w,
    KC(14),			KS_e,
    KC(15),			KS_r,
    KC(16),			KS_y,
    KC(17),			KS_t,
    KC(18),			KS_1,		KS_exclam,
    KC(19),			KS_2,		KS_at,
    KC(20),			KS_3,		KS_numbersign,
    KC(21),			KS_4,		KS_dollar,
    KC(22),			KS_6,		KS_asciicircum,
    KC(23),			KS_5,		KS_percent,
    KC(24),			KS_equal,	KS_plus,
    KC(25),			KS_9,		KS_parenleft,
    KC(26),			KS_7,		KS_ampersand,
    KC(27),			KS_minus,	KS_underscore,
    KC(28),			KS_8,		KS_asterisk,
    KC(29),			KS_0,		KS_parenright,
    KC(30),			KS_bracketright, KS_braceright,
    KC(31),			KS_o,
    KC(32),			KS_u,
    KC(33),			KS_bracketleft,	KS_braceleft,
    KC(34),			KS_i,
    KC(35),			KS_p,
    KC(36),			KS_Return,
    KC(37),			KS_l,
    KC(38),			KS_j,
    KC(39),			KS_apostrophe,	KS_quotedbl,
    KC(40),			KS_k,
    KC(41),			KS_semicolon,	KS_colon,
    KC(42),			KS_backslash,	KS_bar,
    KC(43),			KS_comma,	KS_less,
    KC(44),			KS_slash,	KS_question,
    KC(45),			KS_n,
    KC(46),			KS_m,
    KC(47),			KS_period,	KS_greater,
    KC(48),			KS_Tab,
    KC(49),			KS_space,
    KC(50),			KS_grave,	KS_asciitilde,
    KC(51),			KS_Delete,

    KC(53),			KS_Escape,
    KC(54),			KS_Control_L,
    KC(55),  KS_Cmd,				/* Command */
    KC(56),			KS_Shift_L,
    KC(57),			KS_Caps_Lock,
    KC(58),  KS_Cmd1,				/* Option */
    KC(59),			KS_Left,
    KC(60),			KS_Right,
    KC(61),			KS_Down,
    KC(62),			KS_Up,

    KC(65),			KS_KP_Decimal,
    KC(67),			KS_KP_Multiply,
    KC(69),			KS_KP_Add,
    KC(71),			KS_Clear,
    KC(75),			KS_KP_Divide,
    KC(76),			KS_KP_Enter,
    KC(78),			KS_KP_Subtract,

    KC(81),			KS_KP_Equal,
    KC(82),			KS_KP_0,
    KC(83),			KS_KP_1,
    KC(84),			KS_KP_2,
    KC(85),			KS_KP_3,
    KC(86),			KS_KP_4,
    KC(87),			KS_KP_5,
    KC(88),			KS_KP_6,
    KC(89),			KS_KP_7,

    KC(91),			KS_KP_8,
    KC(92),			KS_KP_9,

    KC(95),			KS_comma,	/* XXX KS_KP_comma */

    KC(106),			KS_KP_Enter,

    KC(127),  KS_Cmd_Debugger,
};

#if 0
static const keysym_t akbd_keydesc_jp[] = {
/*  pos      command		normal		shifted */
    KC(42),			KS_grave,	KS_asciitilde,
    KC(93),			KS_backslash,	KS_bar,
};
#endif

#define KBD_MAP(name, base, map) \
			{ name, base, sizeof(map)/sizeof(keysym_t), map }

static const struct wscons_keydesc akbd_keydesctab[] = {
	KBD_MAP(KB_US,			0,	akbd_keydesc_us),
	{0, 0, 0, 0}
};

#undef KBD_MAP
#undef KC
