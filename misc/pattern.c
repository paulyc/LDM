#include "config.h"

#include <errno.h>
#include <regex.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "pattern.h"
#include "RegularExpressions.h"


struct Pattern {
    char*       string;
    regex_t     reg;
    int         ignoreCase;
};

static Pattern  MATCH_ALL;


/*
 * Arguments:
 *      pat             Pointer to memory that will be set to point to an
 *                      allocated Pattern.  Set on and only on success.  The
 *                      client should call "pat_free(*pat)" when the pattern
 *                      is no longer needed.
 *      ere             The extended regular expression.  Caller may free upon
 *                      return.  If the expression is a pathological 
 *                      regular-expression, then it will be repaired.
 *      ignoreCase      Whether or not to ignore case in matches.
 *
 * Returns:
 *      NULL    Success.  *pat is set.
 *      else    Error object.  err_code() values:
 *                      1       System error.  *pat not set.
 *                      2       Invalid ERE.  *pat not set.
 */
ErrorObj*
pat_new(
    Pattern** const             pat,
    const char* const           ere,
    const int                   ignoreCase)
{
    ErrorObj*   errObj = NULL;  /* success */
    int         isTrivial = 0 == strcmp(".*", ere);

    if (isTrivial) {
        *pat = &MATCH_ALL;
    }
    else {
        Pattern* const  ptr = (Pattern*)malloc(sizeof(Pattern));

        if (NULL == ptr) {
            errObj = ERR_NEW2(1, NULL, "Couldn't allocate %lu bytes: %s",
                (unsigned long)sizeof(Pattern), strerror(errno));
        }
        else {
            ptr->string = strdup(ere);

            if (NULL == ptr->string) {
                errObj = ERR_NEW2(1, NULL, "Couldn't copy string \"%s\": %s",
                    ere, strerror(errno));
                free(ptr);
            }
            else {
                int     err;

                (void)re_vetSpec(ptr->string);

                if ((err = regcomp(&ptr->reg, ptr->string,
                    REG_EXTENDED | REG_NOSUB | (ignoreCase ? REG_ICASE : 0)))) {

                    char        buf[512];

                    (void)regerror(err, &ptr->reg, buf, sizeof(buf));

                    errObj = ERR_NEW2((REG_ESPACE == err) ? 1 : 2, NULL,
                        "Couldn't compile ERE \"%s\": %s", ptr->string, buf);
                    free(ptr->string);
                    free(ptr);
                }
                else {
                    ptr->ignoreCase = ignoreCase;
                    *pat = ptr;
                }                       /* ERE compiled */
            }                           /* ptr->string allocated */
        }                               /* "ptr" allocated */
    }                                   /* non-trivial ERE */

    return errObj;
}


/*
 * Clones a pattern.
 *
 * Arguments:
 *      dst             Pointer to pointer to be set to the clone.  Set on and
 *                      only on success. Client should call "pat_free(*dst)"
 *                      when the pattern is no longer needed.
 *      src             The pattern to be cloned.
 * Returns:
 *      NULL            Success.
 *      else            Error object.
 */
ErrorObj*
pat_clone(
    Pattern** const             dst,
    const Pattern* const        src)
{
    if(&MATCH_ALL == src) {
        *dst = &MATCH_ALL;
        return NULL;
    }
        
    return pat_new(dst, src->string, src->ignoreCase);
}


/*
 * Arguments:
 *      pat     Pointer to Pattern set by pat_new().
 * Returns:
 *      0       String doesn't match pattern
 *      1       String matches pattern
 */
int
pat_isMatch(
    const Pattern* const        pat,
    const char* const           string)
{
    return
        &MATCH_ALL == pat
            ? 1
            : 0 == regexec(&pat->reg, string, 0, NULL, 0);
}


/*
 * Arguments:
 *      pat     Pointer to Pattern set by pat_new().
 *
 * Returns:
 *      Pointer to the extended regular expression of this pattern.  If the
 *      initializing expression was pathological, then then returned string is
 *      the repaired form of the expression.
 */
const char*
pat_getEre(
    const Pattern* const        pat)
{
    return 
        &MATCH_ALL == pat
            ? ".*"
            : pat->string;
}


/*
 * Releases the resources of a Pattern.
 *
 * Arguments:
 *      pat     Pointer to Pattern.  May be NULL.
 */
void
pat_free(
    Pattern* const      pat)
{
    if (pat && &MATCH_ALL != pat) {
        regfree(&pat->reg);
        free(pat->string);
        free(pat);
    }
}
