#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#define RGBM_FFTW

#include "rgbm.h"

struct rgbdrv {
#ifdef WIN32
    HMODULE handle;
#else
    void *handle;
#endif
    int (*rgbm_hw_open)(void);
#ifdef RGBM_AGCUP
    int (*rgbm_hw_srgb)(const double *rgb);
#else
    int (*rgbm_hw_pwm)(const double *rgb);
#endif
    void (*rgbm_hw_close)(void);
    struct rgbdrv *next;
};

static struct rgbdrv *drivers = NULL;

static void *getsym(const char *name,
#ifdef WIN32
                    HMODULE handle,
#else
                    void *handle,
#endif
                    const char *symbol)
{
    void *ptr;
#ifdef WIN32
    ptr = GetProcAddress(handle, symbol);
#else
    ptr = dlsym(handle, symbol);
#endif
    if (!ptr) fprintf(stderr, "Failed to find symbol %s in %s driver.\n",
                      symbol, name);
    return ptr;
}

/* Assume that failure to allocate small amounts of memory indicate severe
   problems, and then recovery is not worthwhile. */
static void *verified_malloc(size_t size)
{
    void *ptr;
    ptr = malloc(size);
    if (!ptr) {
        fprintf(stderr, "Memory allocation of %zi bytes failed.\n", size);
        exit(-1);
    }
    return ptr;
}

static struct rgbdrv *load_driver(const char *name)
{
    void *handle;
    char *filename;
    struct rgbdrv *drv;
    unsigned int namelen;

    /* Build DLL file name from driver name */
    namelen = strlen(name);
#ifdef WIN32
    filename = verified_malloc(namelen + 9);
    memcpy(&filename[0], "rgbm", 4);
    memcpy(&filename[4], name, namelen);
    memcpy(&filename[4 + namelen], ".dll", 5);
#else
    filename = verified_malloc(namelen + 13);
    memcpy(&filename[0], "./librgbm", 9);
    memcpy(&filename[9], name, namelen);
    memcpy(&filename[9 + namelen], ".so", 4);
#endif

    /* Try to open driver from current directory first */
#ifdef WIN32
    handle = LoadLibrary(filename);
#else
    handle = dlopen(&filename[0], RTLD_NOW);
    if (!handle) {
        handle = dlopen(&filename[2], RTLD_NOW);
    }
#endif
    free(filename);
    if (!handle) {
#ifdef WIN32
        fprintf(stderr, "Failed to load driver %s\n", name);
#else
        fprintf(stderr, "Failed to load driver %s: %s\n", name, dlerror());
#endif
        return NULL;
    }

    drv = verified_malloc(sizeof(struct rgbdrv));
    drv->handle = handle;
    drv->rgbm_hw_open = getsym(name, handle, "rgbm_hw_open");
    if (!drv->rgbm_hw_open) return NULL;
#ifdef RGBM_AGCUP
    drv->rgbm_hw_srgb = getsym(name, handle, "rgbm_hw_srgb");
    if (!drv->rgbm_hw_srgb) return NULL;
#else
    drv->rgbm_hw_pwm = getsym(name, handle, "rgbm_hw_pwm");
    if (!drv->rgbm_hw_pwm) return NULL;
#endif
    drv->rgbm_hw_close = getsym(name, handle, "rgbm_hw_close");
    if (!drv->rgbm_hw_close) return NULL;
    drv->next = NULL;

    if (!drv->rgbm_hw_open()) {
        free(drv);
        fprintf(stderr, "Failed to initialize driver %s.\n", name);
        return NULL;
    }

    return drv;
}

/* Load drivers using name1;name2 */
static void load_drivers(const char *s)
{
    struct rgbdrv **nextptr = &drivers;
    const char *end, *p = s;
    unsigned int l;

    while (1) {
        end = strchr(p, '+');
        if (end) {
            char *name;
            l = end - p;
            name = verified_malloc(l + 1);
            memcpy(name, p, l);
            name[l] = 0;
            *nextptr = load_driver(name);
            if (*nextptr) nextptr = &((*nextptr)->next);
            p = end + 1;
        } else {
            *nextptr = load_driver(p);
            break;
        }
    }
}

extern char *light_drivers;
int rgbm_hw_open(void)
{
    load_drivers(light_drivers);
    return drivers != NULL;
}

static void close_driver(struct rgbdrv *driver)
{
    driver->rgbm_hw_close();
#ifdef WIN32
    FreeLibrary(driver->handle);
#else
    dlclose(driver->handle);
#endif
    free(driver);
}

#ifdef RGBM_AGCUP
int rgbm_hw_srgb(const double *rgb)
#else
int rgbm_hw_pwm(const double *rgb)
#endif
{
    struct rgbdrv **nextptr = &drivers;
    while (*nextptr) {
        struct rgbdrv *drv = *nextptr;
        int res;
        /* TODO: This could be done in parallel. */
#ifdef RGBM_AGCUP
        res = drv->rgbm_hw_srgb(rgb);
#else
        res = drv->rgbm_hw_pwm(rgb);
#endif
        /* Assume failure means this driver shouldn't be used anymore */
        if (!res) {
            /* TODO: Print message with name. */
            *nextptr = drv->next;
            close_driver(drv);
        } else {
            nextptr = &(drv->next);
        }
    }
    /* Return success as long as at least one driver remains */
    return drivers != NULL;
}

void rgbm_hw_close(void)
{
    struct rgbdrv *drv = drivers;
    while (drv) {
        struct rgbdrv *next = drv->next;
        close_driver(drv);
        drv = next;
    }
}

#if 0
int main(void)
{
    const double rgb[3] = { 0, 0, 2000 };
    load_drivers("lamp;strip");
    rgbm_hw_pwm(rgb);
    rgbm_hw_close();
}
#endif
