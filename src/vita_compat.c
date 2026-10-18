/* vita_compat.c -- host-service shims the runner core expects from its
 * front-end (on PC these live in the runner's main.c, which this port
 * replaces). Vita versions:
 *
 *   exe_relative(): the PC build resolves data files next to the
 *   executable. Here everything writable (evidence logs, interp
 *   fallback log) goes to ux0:/data/rr3vita/, created by main() at
 *   boot. Nothing the core needs at boot is READ through this path —
 *   glue.c only removes a log through it, runtime_evidence reads and
 *   writes its own evidence files.
 *
 *   readlink(): newlib on Vita has none. runtime_evidence's exe
 *   fingerprint calls readlink("/proc/self/exe") and handles failure
 *   (r <= 0 -> return 0), so a clean -1 is a safe answer.
 */
#include <sys/types.h>
#include <stdio.h>

const char *exe_relative(const char *filename)
{
    static char buf[512];
    snprintf(buf, sizeof buf, "ux0:/data/rr3vita/%s", filename);
    return buf;
}

ssize_t readlink(const char *path, char *buf, size_t bufsiz)
{
    (void)path; (void)buf; (void)bufsiz;
    return -1;
}
