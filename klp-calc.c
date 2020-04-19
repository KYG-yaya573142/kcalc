#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/kernel.h>
#include <linux/livepatch.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/version.h>

#include "expression.h"

MODULE_LICENSE("Dual MIT/GPL");
MODULE_AUTHOR("National Cheng Kung University, Taiwan");
MODULE_DESCRIPTION("Patch calc kernel module");
MODULE_VERSION("0.1");

void livepatch_nop_cleanup(struct expr_func *f, void *c)
{
    /* suppress compilation warnings */
    (void) f;
    (void) c;
}

int livepatch_nop(struct expr_func *f, vec_expr_t args, void *c)
{
    (void) args;
    (void) c;
    pr_err("function nop is now patched\n");
    return 0;
}

static int GET_FRAC(int n)
{
    int x = n & 15;
    if (x & 8)
        return -(((~x) & 15) + 1);

    return x;
}

static int FP2INT(int n, int d)
{
    while (n && n % 10 == 0) {
        ++d;
        n /= 10;
    }
    if (d == -1) {
        n *= 10;
        --d;
    }

    return ((n << 4) | (d & 15));
}

noinline int livepatch_fib(struct expr_func *f, vec_expr_t args, void *c)
{
    int n = expr_eval(&vec_nth(&args, 0)); /* fixed-point */
    int frac = GET_FRAC(n);                /* frac */
    n >>= 4;                               /* mantissa */

    while (frac) { /* fixed-point -> interger */
        if (frac > 0) {
            n *= 10;
            --frac;
        }
        if (frac < 0) {
            n /= 10;
            ++frac;
        }
    }

    if (n < 2) { /* F(0) = 0, F(1) = 1 */
        return (n << 4);
    }
    long long fib[2];
    unsigned int ndigit = 32 - __builtin_clz(n);
    fib[0] = 0; /* F(k) */
    fib[1] = 1; /* F(k+1) */
    /* fast doubling algorithm */
    for (unsigned int i = 1U << (ndigit - 1); i; i >>= 1) {
        long long k1 = fib[0] * (fib[1] * 2 - fib[0]);
        long long k2 = fib[0] * fib[0] + fib[1] * fib[1];
        if (n & i) {
            fib[0] = k2;
            fib[1] = k1 + k2;
        } else {
            fib[0] = k1;
            fib[1] = k2;
        }
    }
    return FP2INT(fib[0], 0);
}

/* clang-format off */
static struct klp_func funcs[] = {
    {
        .old_name = "user_func_nop",
        .new_func = livepatch_nop,
    },
    {
        .old_name = "user_func_nop_cleanup",
        .new_func = livepatch_nop_cleanup,
    },
    {
        .old_name = "user_func_fib",
        .new_func = livepatch_fib,
    },
    {},
};
static struct klp_object objs[] = {
    {
        .name = "calc",
        .funcs = funcs,
    },
    {},
};
/* clang-format on */

static struct klp_patch patch = {
    .mod = THIS_MODULE,
    .objs = objs,
};

static int livepatch_calc_init(void)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 1, 0)
    return klp_enable_patch(&patch);
#else
    int ret = klp_register_patch(&patch);
    if (ret)
        return ret;
    ret = klp_enable_patch(&patch);
    if (ret) {
        WARN_ON(klp_unregister_patch(&patch));
        return ret;
    }
    return 0;
#endif
}

static void livepatch_calc_exit(void)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 1, 0)
    WARN_ON(klp_unregister_patch(&patch));
#endif
}

module_init(livepatch_calc_init);
module_exit(livepatch_calc_exit);
MODULE_INFO(livepatch, "Y");
