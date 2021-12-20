#ifndef MUTATING_GUILE_IFACE_H_GUARD
#define MUTATING_GUILE_IFACE_H_GUARD 1

#if (GUILE_VERSION >= 200000) && (GUILE_VERSION <= 200003)
# error AutoGen does not work with this version of Guile
  choke me.

#elif (GUILE_VERSION <= 106999)
# error AutoGen does not work with this version of Guile
  choke me.

#elif GUILE_VERSION < 201000
# define AG_SCM_IS_PROC(_p)           scm_is_true( scm_procedure_p(_p))
# define AG_SCM_LIST_P(_l)            scm_is_true( scm_list_p(_l))
# define AG_SCM_PAIR_P(_p)            scm_is_true( scm_pair_p(_p))
# define AG_SCM_TO_LONG(_v)           scm_to_long(_v)
# define AG_SCM_TO_ULONG(_v)          ((unsigned long)scm_to_ulong(_v))

#else
# error unknown GUILE_VERSION
  choke me.
#endif

#endif /* MUTATING_GUILE_IFACE_H_GUARD */
