/**
 * rofi-calc
 *
 * MIT/X11 License
 * Copyright (c) 2017 Qball Cow <qball@gmpclient.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <gmodule.h>

#include <rofi/mode.h>
#include <rofi/helper.h>
#include <rofi/mode-private.h>

#include <stdint.h>

#include <muParser.h>
#include <math.h>

using namespace mu;

typedef struct {
    value_type result;
    char *value;
} CALCModeEntry;

/**
 * The internal data structure holding the private data of the TEST Mode.
 */
typedef struct
{
    CALCModeEntry *result;
    unsigned int length_result;
    mu::Parser *p;
    value_type ans;
    value_type afValBuf[100];
    int iVal;
} CALCModePrivateData;


static void get_rofi_calc (  Mode *sw )
{
    /**
     * Get the entries to display.
     * this gets called on plugin initialization.
     */
}
value_type* AddVariable(const char_type *a_szName, void *ud)
{
    CALCModePrivateData *pd = (CALCModePrivateData*)ud;
    if (pd->iVal>=99)
        throw mu::ParserError( ("Variable buffer overflow.") );
    else
        return &(pd->afValBuf[pd->iVal++]);
}


static int IsHexValue(const char_type *a_szExpr, int *a_iPos, value_type *a_fVal)
{
    if (a_szExpr[1]==0 || (a_szExpr[0]!='0' || a_szExpr[1]!='x') )
        return 0;

    uint64_t iVal(0);

    // New code based on streams for UNICODE compliance:
    stringstream_type::pos_type nPos(0);
    stringstream_type ss(a_szExpr + 2);
    ss >> std::hex >> iVal;
    nPos = ss.tellg();

    if (nPos==(stringstream_type::pos_type)0)
        return 1;

    *a_iPos += (int)(2 + nPos);
    *a_fVal = (value_type)iVal;

    return 1;
}
// SI
const value_type kilob = 1024;
const value_type megab = 1024*1024;
const value_type gigab = 1024*1024*1024;
const value_type terab = gigab*1024;
static value_type kilo ( value_type val ) { return 1000*val; }
static value_type mega ( value_type val ) { return 1000*1000*val; }
static value_type giga ( value_type val ) { return 1000*1000*1000*val; }

static value_type kilobyte(value_type val) { return kilob*val; }
static value_type megabyte (value_type val) { return megab*val; }
static value_type gigabyte (value_type val) { return gigab*val; }
static value_type terabyte (value_type val) { return terab*val; }


static int rofi_calc_mode_init ( Mode *sw )
{
    /**
     * Called on startup when enabled (in modi list)
     */
    if ( mode_get_private_data ( sw ) == NULL ) {
        CALCModePrivateData *pd = (CALCModePrivateData*)g_malloc0 ( sizeof ( *pd ) );
        pd->p = new mu::Parser();
        pd->ans = 0;
        pd->p->DefineVar ( "ans", &(pd->ans));
        pd->p->DefineConst("pi", (double)M_PI);
        pd->p->AddValIdent(IsHexValue);
        pd->p->SetVarFactory(AddVariable, pd);

        pd->p->DefineOprtChars("mkgbtMGx");
        pd->p->DefinePostfixOprt("kb", kilobyte);
        pd->p->DefinePostfixOprt("mb", megabyte);
        pd->p->DefinePostfixOprt("gb", gigabyte);
        pd->p->DefinePostfixOprt("tb", terabyte);

        // SI
        pd->p->DefinePostfixOprt("k", kilo);
        pd->p->DefinePostfixOprt("M", mega);
        pd->p->DefinePostfixOprt("G", giga);
        mode_set_private_data ( sw, (void *) pd );
        // Load content.
        get_rofi_calc ( sw );
    }
    return TRUE;
}
static unsigned int rofi_calc_mode_get_num_entries ( const Mode *sw )
{
    const CALCModePrivateData *pd = (const CALCModePrivateData *) mode_get_private_data ( sw );
    return pd->length_result;
}

static ModeMode rofi_calc_mode_result ( Mode *sw, int mretv, char **input, unsigned int selected_line )
{
    ModeMode           retv  = MODE_EXIT;
    CALCModePrivateData *pd = (CALCModePrivateData *) mode_get_private_data ( sw );
    if ( (mretv&(MENU_OK|MENU_CUSTOM_INPUT))){
        if ( input && strlen(*input) > 0 ) {
            try {
                pd->p->SetExpr(*input);
                value_type result = pd->p->Eval();
                pd->ans = result;

                pd->result = (CALCModeEntry*)g_realloc ( pd->result, (pd->length_result+1)*sizeof(CALCModeEntry));
                pd->result[pd->length_result].result = result;
                pd->result[pd->length_result].value  = g_strdup ( *input );
                pd->length_result++;
            } catch ( mu::ParserError e ) {
                pd->result = (CALCModeEntry*)g_realloc ( pd->result, (pd->length_result+1)*sizeof(CALCModeEntry));
                pd->result[pd->length_result].value = g_strdup_printf("<span color='red'>%s</span>", e.GetMsg().c_str());
                pd->result[pd->length_result].result = 0;
                pd->length_result++;

            }
        } else if ( selected_line < pd->length_result ) {
            try {
                char *str = pd->result[pd->length_result-1-selected_line].value;
                pd->p->SetExpr(str);
                value_type result = pd->p->Eval();
                pd->ans = result;

                pd->result = (CALCModeEntry*)g_realloc ( pd->result, (pd->length_result+1)*sizeof(CALCModeEntry));
                pd->result[pd->length_result].result = result;
                pd->result[pd->length_result].value  = g_strdup ( str );
                pd->length_result++;
            } catch ( mu::ParserError e ) {
                pd->result = (CALCModeEntry*)g_realloc ( pd->result, (pd->length_result+1)*sizeof(CALCModeEntry));
                pd->result[pd->length_result].value = g_strdup_printf("<span color='red'>%s</span>", e.GetMsg().c_str());
                pd->result[pd->length_result].result = 0;
                pd->length_result++;

            }

        }
    }
    if ( mretv & MENU_NEXT ) {
        retv = NEXT_DIALOG;
    } else if ( mretv & MENU_PREVIOUS ) {
        retv = PREVIOUS_DIALOG;
    } else if ( mretv & MENU_QUICK_SWITCH ) {
        retv = (ModeMode)( mretv & MENU_LOWER_MASK );
    } else if ( ( mretv & MENU_OK ) ) {
        retv = RESET_DIALOG;
    } else if ( ( mretv & MENU_CUSTOM_INPUT ) ) {
        retv = RESET_DIALOG;
    } else if ( ( mretv & MENU_ENTRY_DELETE ) == MENU_ENTRY_DELETE ) {
        retv = RELOAD_DIALOG;
    }
    return retv;
}

static void rofi_calc_mode_destroy ( Mode *sw )
{
    CALCModePrivateData *pd = (CALCModePrivateData *) mode_get_private_data ( sw );
    if ( pd != NULL ) {
        delete pd->p;
        pd->p = NULL;
        if ( pd->result ) {
            for ( unsigned int i = 0; i < pd->length_result; i++ ){
                g_free (pd->result[i].value);
            }
            g_free ( pd->result );
        }
        g_free ( pd );
        mode_set_private_data ( sw, NULL );
    }
}

static char *rofi_calc_get_completion ( const Mode *sw, unsigned int index )
{
    CALCModePrivateData *pd = (CALCModePrivateData *) mode_get_private_data ( sw );
    if ( pd->result ) {
        CALCModeEntry *e = &(pd->result[pd->length_result-index-1]);
        return g_strdup(e->value);
    }
    return NULL;
}

static char *_get_display_value (
        const Mode *sw,
        unsigned int selected_line,
        int *state,
        G_GNUC_UNUSED GList **attr_list,
        int get_entry )
{
    CALCModePrivateData *pd = (CALCModePrivateData *) mode_get_private_data ( sw );
    if ( state ) {
        (*state) |= 8;
    }

    if ( get_entry ) {
        if ( pd->result ) {
            CALCModeEntry *e = &(pd->result[pd->length_result-selected_line-1]);
            value_type res = fmod(e->result,1);
            if ( abs(res) < 1e-90 ){
                return g_strdup_printf ( "%ld\t<span size='small'>(%s)</span>",(int64_t)(e->result), e->value );
            }
            return g_strdup_printf ( "%e\t<span size='small'>(%s)</span>",e->result, e->value );
        }
    }
    // Only return the string if requested, otherwise only set state.
    return get_entry ? g_strdup("n/a"): NULL;
}

/**
 * @param sw The mode object.
 * @param tokens The tokens to match against.
 * @param index  The index in this plugin to match against.
 *
 * Match the entry.
 *
 * @param returns true when a match.
 */
static int rofi_calc_token_match ( const Mode *sw, rofi_int_matcher **tokens, unsigned int index )
{
    CALCModePrivateData *pd = (CALCModePrivateData *) mode_get_private_data ( sw );

    // Call default matching function.
    return TRUE;
}

static char * rofi_calc_get_message ( const Mode *sw )
{
    CALCModePrivateData *pd = (CALCModePrivateData *) mode_get_private_data ( sw );
    bool pos = pd->ans < 0 ? false:true;
    uint64_t val = (uint64_t)(abs(pd->ans));
    char *str = g_format_size_full ( val, (GFormatSizeFlags)(G_FORMAT_SIZE_LONG_FORMAT|G_FORMAT_SIZE_IEC_UNITS));

    char *str_res = g_markup_printf_escaped (
            "<b>Size:</b>\t%c%s\n"\
            "<b>Hex:</b>\t%c0x%lX",
                pos?' ':'-',str,
                pos?' ':'-',val );

    g_free ( str );
    return str_res;
}


Mode mode = {
    .abi_version        = ABI_VERSION,
    .name               = (char*)"calc",
    .cfg_name_key       =  {'d','i','s','p','l','a','y','-','c', 'a','l','c', 0},
    .display_name       = NULL,
    ._init              = rofi_calc_mode_init,
    ._destroy           = rofi_calc_mode_destroy,
    ._get_num_entries   = rofi_calc_mode_get_num_entries,
    ._result            = rofi_calc_mode_result,
    ._token_match       = rofi_calc_token_match,
    ._get_display_value = _get_display_value,
    ._get_icon          = NULL,
    ._get_completion    = rofi_calc_get_completion,
    ._preprocess_input  = NULL,
    ._get_message       = rofi_calc_get_message,
    .private_data       = NULL,
    .free               = NULL,
    .ed                 = NULL,
};
