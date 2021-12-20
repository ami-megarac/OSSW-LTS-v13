[= AutoGen5 Template h =]
[=

;;;  This file is part of AutoGen.
;;;  Copyright (C) 1992-2016 Bruce Korb - all rights reserved
;;;
;;; AutoGen is free software: you can redistribute it and/or modify it
;;; under the terms of the GNU General Public License as published by the
;;; Free Software Foundation, either version 3 of the License, or
;;; (at your option) any later version.
;;;
;;; AutoGen is distributed in the hope that it will be useful, but
;;; WITHOUT ANY WARRANTY; without even the implied warranty of
;;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
;;; See the GNU General Public License for more details.
;;;
;;; You should have received a copy of the GNU General Public License along
;;; with this program.  If not, see <http://www.gnu.org/licenses/>.

(emit (make-header-guard "mutating"))
(shell "ranged='#if'
guile_range() {
  local lo=${1%%-*}
  local hi=${1##*-}
  if test ${#lo} -eq 0
  then echo \"${ranged} (GUILE_VERSION <= $hi)\"
  elif test ${#hi} -eq 0
  then echo \"${ranged} (GUILE_VERSION >= $lo)\"
  else echo \"${ranged} (GUILE_VERSION >= $lo) && (GUILE_VERSION <= $hi)\"
  fi
  ranged='#elif'
}") =]
[=

FOR invalid     \=]

[= (shell (string-append "guile_range " (get "invalid"))) =]
# error AutoGen does not work with this version of Guile
  choke me.
[=
ENDFOR invalid  =]
[= (out-push-new)  =]
set -- $(sort -n -u <<_EOF_
[=  (join "\n" (stack "iface.i-impl.i-end")) =]
_EOF_
)
v_list="$*"
r_list=$(ix=$# ; while (( ix > 0 )) ; do eval echo \${$ix}
         (( ix = ix - 1 )) ; done)
i_list="[= (join " " (stack "iface.i-name")) =]"
PS4='>${FUNC_NAME:-ag}> '

fill_in() {
  for v in $r_list
  do
    eval f=\"\${${name}_vals[$v]}\"
    if test ${#f} -gt 0
    then val="$f"
    else eval ${name}_vals[$v]=\"$val\"
    fi
  done
}

emit_iface_macro() {
  NM=$(echo $i | tr '[a-z-]' '[A-Z_]')
  eval NM=\"$NM\(\${${i}_args}\)\"
  if test ${#code} -lt 40 -a ${#NM} -lt 22
  then
    printf '# define AG_SCM_%-21s %s\n' "${NM}" "${code}"
  else
    printf '# define AG_SCM_%-21s \\\n    %s\n' "${NM}" "${code}"
  fi
}

emit_iface_type() {
  local nm=ag_scm_$(echo $i | tr '[a-z-]' '[A-Z_]')
  printf '# define %-28s %s\n' $nm "$code"
}

prt_tbl() {
  if='#[= (if (exist? "invalid") "elif" "if  ") =]'
  for v in $v_list
  do
    printf '%s GUILE_VERSION < %s000\n' "$if" "$v"
    if='#elif'

    for i in $i_list
    do
      eval type=\"\${${i}_type}\"
      eval code=\"\${${i}_vals[$v]}\"
      case "$type" in
      ( macro | '' ) emit_iface_macro ;;
      ( typedef )    emit_iface_type  ;;
      esac
    done

    echo
  done
}
[=
(shell (out-pop #t)) =][=

FOR iface       =][=

  (shell (string-append
    "val='" (get "i-impl[].i-code") "'\n"
    (get "i-name") "_args='" (get "i-args") "'\n"
    "name=" (get "i-name") "\n"
    (get "i-name") "_type=" (get "i-type" "macro")

  ))            =][=

  FOR i-impl    =][=

    (shell (string-append
      (get "i-name") "_vals[" (get "i-end")  "]='" (get "i-code") "'"
    ))          =][=

  ENDFOR        =][=

  `fill_in`     =][=

ENDFOR iface    =][=

`prt_tbl`
=]

#else
# error unknown GUILE_VERSION
  choke me.
#endif

#endif /* [= (. header-guard) =][=

# end of guile-iface.tpl =] */
