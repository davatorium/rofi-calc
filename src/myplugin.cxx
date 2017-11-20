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
    char *result;
    char *value;
} MYPLUGINModeEntry;
/**
 * The internal data structure holding the private data of the TEST Mode.
 */
typedef struct
{
    gchar **result;
    unsigned int length_result;
    mu::Parser *p;
    value_type ans;
    value_type afValBuf[100];
    int iVal;
} MYPLUGINModePrivateData;


static void get_rofi_calc (  Mode *sw )
{
    /**
     * Get the entries to display.
     * this gets called on plugin initialization.
     */
}
value_type* AddVariable(const char_type *a_szName, void *ud)
{
    MYPLUGINModePrivateData *pd = (MYPLUGINModePrivateData*)ud;
    if (pd->iVal>=99)
        throw mu::ParserError( ("Variable buffer overflow.") );
    else
        return &(pd->afValBuf[pd->iVal++]);
}


static int rofi_calc_mode_init ( Mode *sw )
{
    /**
     * Called on startup when enabled (in modi list)
     */
    if ( mode_get_private_data ( sw ) == NULL ) {
        MYPLUGINModePrivateData *pd = (MYPLUGINModePrivateData*)g_malloc0 ( sizeof ( *pd ) );
        pd->p = new mu::Parser();
        pd->ans = 0;
        pd->p->DefineVar ( "ans", &(pd->ans));
        pd->p->DefineConst("pi", (double)M_PI);
        pd->p->SetVarFactory(AddVariable, pd);
        mode_set_private_data ( sw, (void *) pd );
        // Load content.
        get_rofi_calc ( sw );
    }
    return TRUE;
}
static unsigned int rofi_calc_mode_get_num_entries ( const Mode *sw )
{
    const MYPLUGINModePrivateData *pd = (const MYPLUGINModePrivateData *) mode_get_private_data ( sw );
    return pd->length_result;
}

static ModeMode rofi_calc_mode_result ( Mode *sw, int mretv, char **input, unsigned int selected_line )
{
    ModeMode           retv  = MODE_EXIT;
    MYPLUGINModePrivateData *pd = (MYPLUGINModePrivateData *) mode_get_private_data ( sw );
    if ( input ) {
        try {
            pd->p->SetExpr(*input);
            auto result = pd->p->Eval();
            pd->ans = result;

            pd->result = (gchar **)g_realloc ( pd->result, (pd->length_result+2)*sizeof(gchar*));
            pd->result[pd->length_result]= g_strdup_printf("%lf <span size='small'>(%s)</span>", result,*input);
            pd->result[pd->length_result+1] = NULL;
            pd->length_result++;
        } catch ( mu::ParserError e ) {
            pd->result = (gchar **)g_realloc ( pd->result, (pd->length_result+2)*sizeof(gchar*));
            pd->result[pd->length_result]= g_markup_printf_escaped ( "<span color='red'>%s</span>",e.GetMsg().c_str());
            pd->result[pd->length_result+1] = NULL;
            pd->length_result++;

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
    MYPLUGINModePrivateData *pd = (MYPLUGINModePrivateData *) mode_get_private_data ( sw );
    if ( pd != NULL ) {
        delete pd->p;
        pd->p = nullptr;
        if ( pd->result ) {
            g_strfreev(pd->result);
        }
        g_free ( pd );
        mode_set_private_data ( sw, NULL );
    }
}

static char *_get_display_value ( const Mode *sw, unsigned int selected_line, int *state, G_GNUC_UNUSED GList **attr_list, int get_entry )
{
    MYPLUGINModePrivateData *pd = (MYPLUGINModePrivateData *) mode_get_private_data ( sw );
    if ( state ) {
        (*state) |= 8;
    }

    if ( get_entry ) {
        if ( pd->result ) {
            return g_strdup ( pd->result[pd->length_result-selected_line-1] );
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
    MYPLUGINModePrivateData *pd = (MYPLUGINModePrivateData *) mode_get_private_data ( sw );

    // Call default matching function.
    return TRUE;//helper_token_match ( tokens, pd->array[index]);
}


Mode mode = {
    .abi_version        = ABI_VERSION,
    .name               = (char*)"calc",
    .cfg_name_key       =  {'d','i','s','p','l','a','y','-','m', 'y','p','l','u','g','i', 'n', 0},//"display-rofi_calc",
    .display_name       = NULL,
    ._init              = rofi_calc_mode_init,
    ._destroy           = rofi_calc_mode_destroy,
    ._get_num_entries   = rofi_calc_mode_get_num_entries,
    ._result            = rofi_calc_mode_result,
    ._token_match       = rofi_calc_token_match,
    ._get_display_value = _get_display_value,
    ._get_icon          = NULL,
    ._get_completion    = NULL,
    ._preprocess_input  = NULL,
    ._get_message       = NULL,
    .private_data       = NULL,
    .free               = NULL,
    .ed                 = NULL,
};
