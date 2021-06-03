#include "/tmp/glibc-2.31/nptl/pthreadP.h"
#include <stdlib.h>
#include <tls.h>
#include <signal.h>
#include <unistd.h>
#include <sysdep-cancel.h>
#include <not-cancel.h>

// vars.c
/* Default thread attributes for the case when the user does not
   provide any.  */
struct pthread_attr __default_pthread_attr attribute_hidden;
/* Mutex protecting __default_pthread_attr.  */
int __default_pthread_attr_lock = LLL_LOCK_INITIALIZER;
/* Flag whether the machine is SMP or not.  */
int __is_smp attribute_hidden;
#ifndef TLS_MULTIPLE_THREADS_IN_TCB
/* Variable set to a nonzero value either if more than one thread runs or ran,
   or if a single-threaded process is trying to cancel itself.  See
   nptl/descr.h for more context on the single-threaded process case.  */
int __pthread_multiple_threads attribute_hidden;
#endif
/* Table of the key information.  */
struct pthread_key_struct __pthread_keys[PTHREAD_KEYS_MAX]
  __attribute__ ((nocommon));
hidden_data_def (__pthread_keys)

// pause_nocancel.c
int
__pause_nocancel (void)
{
#ifdef __NR_pause
  return INLINE_SYSCALL_CALL (pause);
#else
  return INLINE_SYSCALL_CALL (ppoll, NULL, 0, NULL, NULL);
#endif
}
hidden_def (__pause_nocancel)

// lowlevellock.c
#include <sysdep.h>
#include <lowlevellock.h>
#include <atomic.h>
#include <stap-probe.h>

void
__lll_lock_wait_private (int *futex)
{
  if (atomic_load_relaxed (futex) == 2)
    goto futex;

  while (atomic_exchange_acquire (futex, 2) != 0)
    {
    futex:
      LIBC_PROBE (lll_lock_wait_private, 1, futex);
      lll_futex_wait (futex, 2, LLL_PRIVATE); /* Wait if *futex == 2.  */
    }
}


/* This function doesn't get included in libc.  */
#if IS_IN (libpthread)
void
__lll_lock_wait (int *futex, int private)
{
  if (atomic_load_relaxed (futex) == 2)
    goto futex;

  while (atomic_exchange_acquire (futex, 2) != 0)
    {
    futex:
      LIBC_PROBE (lll_lock_wait, 1, futex);
      lll_futex_wait (futex, 2, private); /* Wait if *futex == 2.  */
    }
}
#endif

// tpp.c
#include <assert.h>
#include <atomic.h>
#include <errno.h>
#include <sched.h>
#include <stdlib.h>
#include <atomic.h>


int __sched_fifo_min_prio = -1;
int __sched_fifo_max_prio = -1;

/* We only want to initialize __sched_fifo_min_prio and __sched_fifo_max_prio
   once.  The standard solution would be similar to pthread_once, but then
   readers would need to use an acquire fence.  In this specific case,
   initialization is comprised of just idempotent writes to two variables
   that have an initial value of -1.  Therefore, we can treat each variable as
   a separate, at-least-once initialized value.  This enables using just
   relaxed MO loads and stores, but requires that consumers check for
   initialization of each value that is to be used; see
   __pthread_tpp_change_priority for an example.
 */
void
__init_sched_fifo_prio (void)
{
  atomic_store_relaxed (&__sched_fifo_max_prio,
			__sched_get_priority_max (SCHED_FIFO));
  atomic_store_relaxed (&__sched_fifo_min_prio,
			__sched_get_priority_min (SCHED_FIFO));
}

int
__pthread_tpp_change_priority (int previous_prio, int new_prio)
{
  struct pthread *self = THREAD_SELF;
  struct priority_protection_data *tpp = THREAD_GETMEM (self, tpp);
  int fifo_min_prio = atomic_load_relaxed (&__sched_fifo_min_prio);
  int fifo_max_prio = atomic_load_relaxed (&__sched_fifo_max_prio);

  if (tpp == NULL)
    {
      /* See __init_sched_fifo_prio.  We need both the min and max prio,
         so need to check both, and run initialization if either one is
         not initialized.  The memory model's write-read coherence rule
         makes this work.  */
      if (fifo_min_prio == -1 || fifo_max_prio == -1)
	{
	  __init_sched_fifo_prio ();
	  fifo_min_prio = atomic_load_relaxed (&__sched_fifo_min_prio);
	  fifo_max_prio = atomic_load_relaxed (&__sched_fifo_max_prio);
	}

      size_t size = sizeof *tpp;
      size += (fifo_max_prio - fifo_min_prio + 1)
	      * sizeof (tpp->priomap[0]);
      tpp = calloc (size, 1);
      if (tpp == NULL)
	return ENOMEM;
      tpp->priomax = fifo_min_prio - 1;
      THREAD_SETMEM (self, tpp, tpp);
    }

  assert (new_prio == -1
	  || (new_prio >= fifo_min_prio
	      && new_prio <= fifo_max_prio));
  assert (previous_prio == -1
	  || (previous_prio >= fifo_min_prio
	      && previous_prio <= fifo_max_prio));

  int priomax = tpp->priomax;
  int newpriomax = priomax;
  if (new_prio != -1)
    {
      if (tpp->priomap[new_prio - fifo_min_prio] + 1 == 0)
	return EAGAIN;
      ++tpp->priomap[new_prio - fifo_min_prio];
      if (new_prio > priomax)
	newpriomax = new_prio;
    }

  if (previous_prio != -1)
    {
      if (--tpp->priomap[previous_prio - fifo_min_prio] == 0
	  && priomax == previous_prio
	  && previous_prio > new_prio)
	{
	  int i;
	  for (i = previous_prio - 1; i >= fifo_min_prio; --i)
	    if (tpp->priomap[i - fifo_min_prio])
	      break;
	  newpriomax = i;
	}
    }

  if (priomax == newpriomax)
    return 0;

  /* See CREATE THREAD NOTES in nptl/pthread_create.c.  */
  lll_lock (self->lock, LLL_PRIVATE);

  tpp->priomax = newpriomax;

  int result = 0;

  if ((self->flags & ATTR_FLAG_SCHED_SET) == 0)
    {
      if (__sched_getparam (self->tid, &self->schedparam) != 0)
	result = errno;
      else
	self->flags |= ATTR_FLAG_SCHED_SET;
    }

  if ((self->flags & ATTR_FLAG_POLICY_SET) == 0)
    {
      self->schedpolicy = __sched_getscheduler (self->tid);
      if (self->schedpolicy == -1)
	result = errno;
      else
	self->flags |= ATTR_FLAG_POLICY_SET;
    }

  if (result == 0)
    {
      struct sched_param sp = self->schedparam;
      if (sp.sched_priority < newpriomax || sp.sched_priority < priomax)
	{
	  if (sp.sched_priority < newpriomax)
	    sp.sched_priority = newpriomax;

	  if (__sched_setscheduler (self->tid, self->schedpolicy, &sp) < 0)
	    result = errno;
	}
    }

  lll_unlock (self->lock, LLL_PRIVATE);

  return result;
}

int
__pthread_current_priority (void)
{
  struct pthread *self = THREAD_SELF;
  if ((self->flags & (ATTR_FLAG_POLICY_SET | ATTR_FLAG_SCHED_SET))
      == (ATTR_FLAG_POLICY_SET | ATTR_FLAG_SCHED_SET))
    return self->schedparam.sched_priority;

  int result = 0;

  /* See CREATE THREAD NOTES in nptl/pthread_create.c.  */
  lll_lock (self->lock, LLL_PRIVATE);

  if ((self->flags & ATTR_FLAG_SCHED_SET) == 0)
    {
      if (__sched_getparam (self->tid, &self->schedparam) != 0)
	result = -1;
      else
	self->flags |= ATTR_FLAG_SCHED_SET;
    }

  if ((self->flags & ATTR_FLAG_POLICY_SET) == 0)
    {
      self->schedpolicy = __sched_getscheduler (self->tid);
      if (self->schedpolicy == -1)
	result = -1;
      else
	self->flags |= ATTR_FLAG_POLICY_SET;
    }

  if (result != -1)
    result = self->schedparam.sched_priority;

  lll_unlock (self->lock, LLL_PRIVATE);

  return result;
}

// elf/dl-tunables.h
#include <startup.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <sysdep.h>
#include <fcntl.h>
#include <ldsodefs.h>

#define TUNABLES_INTERNAL 1
#include <elf/dl-tunables.h>

#include <not-errno.h>

#if TUNABLES_FRONTEND == TUNABLES_FRONTEND_valstring
# define GLIBC_TUNABLES "GLIBC_TUNABLES"
#endif

#if TUNABLES_FRONTEND == TUNABLES_FRONTEND_valstring
static char *
tunables_strdup (const char *in)
{
  size_t i = 0;

  while (in[i++] != '\0');
  char *out = __sbrk (i);

  /* For most of the tunables code, we ignore user errors.  However,
     this is a system error - and running out of memory at program
     startup should be reported, so we do.  */
  if (out == (void *)-1)
    _dl_fatal_printf ("sbrk() failure while processing tunables\n");

  i--;

  while (i-- > 0)
    out[i] = in[i];

  return out;
}
#endif

static char **
get_next_env (char **envp, char **name, size_t *namelen, char **val,
	      char ***prev_envp)
{
  while (envp != NULL && *envp != NULL)
    {
      char **prev = envp;
      char *envline = *envp++;
      int len = 0;

      while (envline[len] != '\0' && envline[len] != '=')
	len++;

      /* Just the name and no value, go to the next one.  */
      if (envline[len] == '\0')
	continue;

      *name = envline;
      *namelen = len;
      *val = &envline[len + 1];
      *prev_envp = prev;

      return envp;
    }

  return NULL;
}

#define TUNABLE_SET_VAL_IF_VALID_RANGE(__cur, __val, __type)		      \
({									      \
  __type min = (__cur)->type.min;					      \
  __type max = (__cur)->type.max;					      \
									      \
  if ((__type) (__val) >= min && (__type) (val) <= max)			      \
    {									      \
      (__cur)->val.numval = val;					      \
      (__cur)->initialized = true;					      \
    }									      \
})

static void
do_tunable_update_val (tunable_t *cur, const void *valp)
{
  uint64_t val;

  if (cur->type.type_code != TUNABLE_TYPE_STRING)
    val = *((int64_t *) valp);

  switch (cur->type.type_code)
    {
    case TUNABLE_TYPE_INT_32:
	{
	  TUNABLE_SET_VAL_IF_VALID_RANGE (cur, val, int64_t);
	  break;
	}
    case TUNABLE_TYPE_UINT_64:
	{
	  TUNABLE_SET_VAL_IF_VALID_RANGE (cur, val, uint64_t);
	  break;
	}
    case TUNABLE_TYPE_SIZE_T:
	{
	  TUNABLE_SET_VAL_IF_VALID_RANGE (cur, val, uint64_t);
	  break;
	}
    case TUNABLE_TYPE_STRING:
	{
	  cur->val.strval = valp;
	  break;
	}
    default:
      __builtin_unreachable ();
    }
}

/* Validate range of the input value and initialize the tunable CUR if it looks
   good.  */
static void
tunable_initialize (tunable_t *cur, const char *strval)
{
  uint64_t val;
  const void *valp;

  if (cur->type.type_code != TUNABLE_TYPE_STRING)
    {
      val = _dl_strtoul (strval, NULL);
      valp = &val;
    }
  else
    {
      cur->initialized = true;
      valp = strval;
    }
  do_tunable_update_val (cur, valp);
}

void
__tunable_set_val (tunable_id_t id, void *valp)
{
  tunable_t *cur = &tunable_list[id];

  do_tunable_update_val (cur, valp);
}

#if TUNABLES_FRONTEND == TUNABLES_FRONTEND_valstring
/* Parse the tunable string TUNESTR and adjust it to drop any tunables that may
   be unsafe for AT_SECURE processes so that it can be used as the new
   environment variable value for GLIBC_TUNABLES.  VALSTRING is the original
   environment variable string which we use to make NULL terminated values so
   that we don't have to allocate memory again for it.  */
static void
parse_tunables (char *tunestr, char *valstring)
{
  if (tunestr == NULL || *tunestr == '\0')
    return;

  char *p = tunestr;

  while (true)
    {
      char *name = p;
      size_t len = 0;

      /* First, find where the name ends.  */
      while (p[len] != '=' && p[len] != ':' && p[len] != '\0')
	len++;

      /* If we reach the end of the string before getting a valid name-value
	 pair, bail out.  */
      if (p[len] == '\0')
	return;

      /* We did not find a valid name-value pair before encountering the
	 colon.  */
      if (p[len]== ':')
	{
	  p += len + 1;
	  continue;
	}

      p += len + 1;

      /* Take the value from the valstring since we need to NULL terminate it.  */
      char *value = &valstring[p - tunestr];
      len = 0;

      while (p[len] != ':' && p[len] != '\0')
	len++;

      /* Add the tunable if it exists.  */
      for (size_t i = 0; i < sizeof (tunable_list) / sizeof (tunable_t); i++)
	{
	  tunable_t *cur = &tunable_list[i];

	  if (tunable_is_name (cur->name, name))
	    {
	      /* If we are in a secure context (AT_SECURE) then ignore the tunable
		 unless it is explicitly marked as secure.  Tunable values take
		 precendence over their envvar aliases.  */
	      if (__libc_enable_secure)
		{
		  if (cur->security_level == TUNABLE_SECLEVEL_SXID_ERASE)
		    {
		      if (p[len] == '\0')
			{
			  /* Last tunable in the valstring.  Null-terminate and
			     return.  */
			  *name = '\0';
			  return;
			}
		      else
			{
			  /* Remove the current tunable from the string.  We do
			     this by overwriting the string starting from NAME
			     (which is where the current tunable begins) with
			     the remainder of the string.  We then have P point
			     to NAME so that we continue in the correct
			     position in the valstring.  */
			  char *q = &p[len + 1];
			  p = name;
			  while (*q != '\0')
			    *name++ = *q++;
			  name[0] = '\0';
			  len = 0;
			}
		    }

		  if (cur->security_level != TUNABLE_SECLEVEL_NONE)
		    break;
		}

	      value[len] = '\0';
	      tunable_initialize (cur, value);
	      break;
	    }
	}

      if (p[len] == '\0')
	return;
      else
	p += len + 1;
    }
}
#endif

/* Enable the glibc.malloc.check tunable in SETUID/SETGID programs only when
   the system administrator has created the /etc/suid-debug file.  This is a
   special case where we want to conditionally enable/disable a tunable even
   for setuid binaries.  We use the special version of access() to avoid
   setting ERRNO, which is a TLS variable since TLS has not yet been set
   up.  */
static __always_inline void
maybe_enable_malloc_check (void)
{
  tunable_id_t id = TUNABLE_ENUM_NAME (glibc, malloc, check);
  if (__libc_enable_secure && __access_noerrno ("/etc/suid-debug", F_OK) == 0)
    tunable_list[id].security_level = TUNABLE_SECLEVEL_NONE;
}

/* Initialize the tunables list from the environment.  For now we only use the
   ENV_ALIAS to find values.  Later we will also use the tunable names to find
   values.  */
void
__tunables_init (char **envp)
{
  char *envname = NULL;
  char *envval = NULL;
  size_t len = 0;
  char **prev_envp = envp;

  maybe_enable_malloc_check ();

  while ((envp = get_next_env (envp, &envname, &len, &envval,
			       &prev_envp)) != NULL)
    {
#if TUNABLES_FRONTEND == TUNABLES_FRONTEND_valstring
      if (tunable_is_name (GLIBC_TUNABLES, envname))
	{
	  char *new_env = tunables_strdup (envname);
	  if (new_env != NULL)
	    parse_tunables (new_env + len + 1, envval);
	  /* Put in the updated envval.  */
	  *prev_envp = new_env;
	  continue;
	}
#endif

      for (int i = 0; i < sizeof (tunable_list) / sizeof (tunable_t); i++)
	{
	  tunable_t *cur = &tunable_list[i];

	  /* Skip over tunables that have either been set already or should be
	     skipped.  */
	  if (cur->initialized || cur->env_alias == NULL)
	    continue;

	  const char *name = cur->env_alias;

	  /* We have a match.  Initialize and move on to the next line.  */
	  if (tunable_is_name (name, envname))
	    {
	      /* For AT_SECURE binaries, we need to check the security settings of
		 the tunable and decide whether we read the value and also whether
		 we erase the value so that child processes don't inherit them in
		 the environment.  */
	      if (__libc_enable_secure)
		{
		  if (cur->security_level == TUNABLE_SECLEVEL_SXID_ERASE)
		    {
		      /* Erase the environment variable.  */
		      char **ep = prev_envp;

		      while (*ep != NULL)
			{
			  if (tunable_is_name (name, *ep))
			    {
			      char **dp = ep;

			      do
				dp[0] = dp[1];
			      while (*dp++);
			    }
			  else
			    ++ep;
			}
		      /* Reset the iterator so that we read the environment again
			 from the point we erased.  */
		      envp = prev_envp;
		    }

		  if (cur->security_level != TUNABLE_SECLEVEL_NONE)
		    continue;
		}

	      tunable_initialize (cur, envval);
	      break;
	    }
	}
    }
}

/* Set the tunable value.  This is called by the module that the tunable exists
   in. */
void
__tunable_get_val (tunable_id_t id, void *valp, tunable_callback_t callback)
{
  tunable_t *cur = &tunable_list[id];

  switch (cur->type.type_code)
    {
    case TUNABLE_TYPE_UINT_64:
	{
	  *((uint64_t *) valp) = (uint64_t) cur->val.numval;
	  break;
	}
    case TUNABLE_TYPE_INT_32:
	{
	  *((int32_t *) valp) = (int32_t) cur->val.numval;
	  break;
	}
    case TUNABLE_TYPE_SIZE_T:
	{
	  *((size_t *) valp) = (size_t) cur->val.numval;
	  break;
	}
    case TUNABLE_TYPE_STRING:
	{
	  *((const char **)valp) = cur->val.strval;
	  break;
	}
    default:
      __builtin_unreachable ();
    }

  if (cur->initialized && callback != NULL)
    callback (&cur->val);
}

rtld_hidden_def (__tunable_get_val)

// pthread_mutex_conf.c
#if HAVE_TUNABLES
# define TUNABLE_NAMESPACE pthread
#include "/tmp/glibc-2.31/nptl/pthread_mutex_conf.h"
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>  /* Get STDOUT_FILENO for _dl_printf.  */
#include <elf/dl-tunables.h>

struct mutex_config __mutex_aconf =
{
  /* The maximum number of times a thread should spin on the lock before
  calling into kernel to block.  */
  .spin_count = DEFAULT_ADAPTIVE_COUNT,
};

static void
TUNABLE_CALLBACK (set_mutex_spin_count) (tunable_val_t *valp)
{
  __mutex_aconf.spin_count = (int32_t) (valp)->numval;
}

void
__pthread_tunables_init (void)
{
  TUNABLE_GET (mutex_spin_count, int32_t,
               TUNABLE_CALLBACK (set_mutex_spin_count));
}
#endif
