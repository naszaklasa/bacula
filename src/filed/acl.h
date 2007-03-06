/*
 * Properties we use for getting and setting ACLs.
 */

#ifndef _BACULA_ACL_
#define _BACULA_ACL_

/* For shorter ACL strings when possible, define BACL_WANT_SHORT_ACLS */
/* #define BACL_WANT_SHORT_ACLS */

/* For numeric user/group ids when possible, define BACL_WANT_NUMERIC_IDS */
/* #define BACL_WANT_NUMERIC_IDS */

/* We support the following types of ACLs */
#define BACL_TYPE_NONE	      0x000
#define BACL_TYPE_ACCESS      0x001
#define BACL_TYPE_DEFAULT     0x002

#define BACL_CAP_NONE	      0x000    /* No special capabilities */
#define BACL_CAP_DEFAULTS     0x001    /* Has default ACLs for directories */
#define BACL_CAP_DEFAULTS_DIR 0x002    /* Default ACLs must be read separately */

/* Set BACL_CAP (always) and BACL_ENOTSUP (when used) for various OS */
#if defined(HAVE_FREEBSD_OS)
#define BACL_CAP	      (BACL_CAP_DEFAULTS|BACL_CAP_DEFAULTS_DIR)
#define BACL_ENOTSUP	      EOPNOTSUPP
#elif defined(HAVE_DARWIN_OS)
#define BACL_CAP	      BACL_CAP_NONE
#define BACL_ENOTSUP	      EOPNOTSUPP
#elif defined(HAVE_HPUX_OS)
#define BACL_CAP	      BACL_CAP_NONE
#define BACL_ENOTSUP	      EOPNOTSUPP
#elif defined(HAVE_IRIX_OS)
#define BACL_CAP	      (BACL_CAP_DEFAULTS|BACL_CAP_DEFAULTS_DIR)
#define BACL_ENOTSUP	      ENOSYS
#elif defined(HAVE_LINUX_OS) 
#define BACL_CAP	      (BACL_CAP_DEFAULTS|BACL_CAP_DEFAULTS_DIR)
#define BACL_ENOTSUP	      ENOTSUP
#elif defined(HAVE_OSF1_OS)
#define BACL_CAP	      (BACL_CAP_DEFAULTS|BACL_CAP_DEFAULTS_DIR)
/* #define BACL_ENOTSUP	      ENOTSUP */     /* Don't know */
#define BACL_CAP	      (BACL_CAP_DEFAULTS|BACL_CAP_DEFAULTS_DIR)
#elif defined(HAVE_SUN_OS)
#define BACL_CAP	      BACL_CAP_DEFAULTS
#else
#define BACL_CAP	      BACL_CAP_NONE  /* nothing special */
#endif

#endif
