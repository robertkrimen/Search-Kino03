#include "xs/XSBind.h"

#include "Search/Kino03/KinoSearch/Analysis/Tokenizer.h"
#include "Search/Kino03/KinoSearch/Analysis/Token.h"
#include "Search/Kino03/KinoSearch/Analysis/Inversion.h"
#include "Search/Kino03/KinoSearch/Util/Host.h"
#include "Search/Kino03/KinoSearch/Util/MemManager.h"
#include "Search/Kino03/KinoSearch/Util/StringHelper.h"

static void
S_set_token_re_but_not_pattern(kino_Tokenizer *self, void *token_re);

static void
S_set_pattern_from_token_re(kino_Tokenizer *self, void *token_re);

kino_Tokenizer*
kino_Tokenizer_init(kino_Tokenizer *self, const kino_CharBuf *pattern)
{
    SV    *token_re_sv;

    kino_Analyzer_init((kino_Analyzer*)self);
    #define DEFAULT_PATTERN "\\w+(?:['\\x{2019}]\\w+)*"
    self->pattern = pattern 
                  ? Kino_CB_Clone(pattern)
                  : kino_CB_new_from_trusted_utf8(DEFAULT_PATTERN,
                      sizeof(DEFAULT_PATTERN) - 1);

    /* Acquire a compiled regex engine for matching one token. */
    token_re_sv = kino_Host_callback_nat(&KINO_TOKENIZER,
        "compile_token_re", 1, KINO_ARG_STR("pattern", self->pattern));
    S_set_token_re_but_not_pattern(self, SvRV(token_re_sv));

    return self;
}

static void
S_set_token_re_but_not_pattern(kino_Tokenizer *self, void *token_re)
{
    MAGIC *magic = NULL;
    if (SvMAGICAL((SV*)token_re))
        magic = mg_find((SV*)token_re, PERL_MAGIC_qr); 
    if (!magic)
        THROW("token_re is not a qr// entity");
    if (self->token_re) ReREFCNT_dec(((REGEXP*)self->token_re));
    self->token_re = magic->mg_obj;
    (void)ReREFCNT_inc(((REGEXP*)self->token_re));
}

static void
S_set_pattern_from_token_re(kino_Tokenizer *self, void *token_re)
{
    SV *rv = newRV((SV*)token_re);
    STRLEN len = 0;
    char *ptr = SvPVutf8((SV*)rv, len);
    Kino_CB_Copy_Str(self->pattern, ptr, len);
    SvREFCNT_dec(rv);
}

void
kino_Tokenizer_set_token_re(kino_Tokenizer *self, void *token_re)
{
    S_set_token_re_but_not_pattern(self, token_re);
    /* Set pattern as a side effect. */
    S_set_pattern_from_token_re(self, token_re);
}

void
kino_Tokenizer_destroy(kino_Tokenizer *self)
{
    KINO_DECREF(self->pattern);
    ReREFCNT_dec(((REGEXP*)self->token_re));
    KINO_FREE_OBJ(self);
}

void
kino_Tokenizer_tokenize_str(kino_Tokenizer *self, const char *string, 
                            size_t string_len, kino_Inversion *inversion)
{
    chy_u32_t  num_code_points = 0;
    SV        *wrapper    = sv_newmortal();
    REGEXP    *rx         = (REGEXP*)self->token_re;
    char      *string_beg = (char*)string;
    char      *string_end = string_beg + string_len;
    char      *string_arg = string_beg;


    /* Fake up an SV wrapper to feed to the regex engine. */
    sv_upgrade(wrapper, SVt_PV);
    SvREADONLY_on(wrapper);
    SvLEN(wrapper) = 0;
    SvUTF8_on(wrapper);

    /* Wrap the string in an SV to please the regex engine. */
    SvPVX(wrapper) = string_beg;
    SvCUR_set(wrapper, string_len);
    SvPOK_on(wrapper);

    while (
        pregexec(rx, string_arg, string_end, string_arg, 1, wrapper, 1)
    ) {
#if ((PERL_VERSION > 9) || (PERL_VERSION == 9 && PERL_SUBVERSION >= 5))
        char *const start_ptr = string_arg + rx->offs[0].start;
        char *const end_ptr   = string_arg + rx->offs[0].end;
#else 
        char *const start_ptr = string_arg + rx->startp[0];
        char *const end_ptr   = string_arg + rx->endp[0];
#endif
        chy_u32_t start, end;

        /* Get start and end offsets in Unicode code points. */
        for( ; string_arg < start_ptr; num_code_points++) {
            string_arg += KINO_STRHELP_UTF8_SKIP[(chy_u8_t)*string_arg];
            if (string_arg > string_end)
                THROW("scanned past end of '%s'", string_beg);
        }
        start = num_code_points;
        for( ; string_arg < end_ptr; num_code_points++) {
            string_arg += KINO_STRHELP_UTF8_SKIP[(chy_u8_t)*string_arg];
            if (string_arg > string_end)
                THROW("scanned past end of '%s'", string_beg);
        }
        end = num_code_points;

        /* Add a token to the new inversion. */
        Kino_Inversion_Append(inversion,
            kino_Token_new(
                start_ptr,
                (end_ptr - start_ptr),
                start,
                end,
                1.0f, /* boost always 1 for now */
                1     /* position increment */
            )
        );
    }
}

/* Copyright 2005-2009 Marvin Humphrey
 *
 * This program is free software; you can redistribute it and/or modify
 * under the same terms as Perl itself.
 */

