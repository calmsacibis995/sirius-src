########################################################################
#                                                                      #
#               This software is part of the ast package               #
#           Copyright (c) 1982-2007 AT&T Knowledge Ventures            #
#                      and is licensed under the                       #
#                  Common Public License, Version 1.0                  #
#                      by AT&T Knowledge Ventures                      #
#                                                                      #
#                A copy of the License is available at                 #
#            http://www.opensource.org/licenses/cpl1.0.txt             #
#         (with md5 checksum 059e8cd6165cb4c31e351f2b69388fd9)         #
#                                                                      #
#              Information and Software Systems Research               #
#                            AT&T Research                             #
#                           Florham Park NJ                            #
#                                                                      #
#                  David Korn <dgk@research.att.com>                   #
#                                                                      #
########################################################################
function err_exit
{
	print -u2 -n "\t"
	print -u2 -r ${Command}[$1]: "${@:2}"
	let Errors+=1
}
alias err_exit='err_exit $LINENO'

Command=${0##*/}
integer Errors=0
alias foo='print hello'
if	[[ $(foo) != hello ]]
then	err_exit 'foo, where foo is alias for "print hello" failed'
fi
if	[[ $(foo world) != 'hello world' ]]
then	err_exit 'foo world, where foo is alias for "print hello" failed'
fi
alias foo='print hello '
alias bar=world
if	[[ $(foo bar) != 'hello world' ]]
then	err_exit 'foo bar, where foo is alias for "print hello " failed'
fi
if	[[ $(foo \bar) != 'hello bar' ]]
then	err_exit 'foo \bar, where foo is alias for "print hello " failed'
fi
alias bar='foo world'
if	[[ $(bar) != 'hello world' ]]
then	err_exit 'bar, where bar is alias for "foo world" failed'
fi
if	[[ $(alias bar) != "bar='foo world'" ]]
then	err_exit 'alias bar, where bar is alias for "foo world" failed'
fi
unalias foo  || err_exit  "unalias foo failed"
alias foo 2> /dev/null  && err_exit "alias for non-existent alias foo returns true"
unset bar
alias bar="print foo$bar"
bar=bar
if	[[ $(bar) != foo ]]
then	err_exit 'alias bar, where bar is alias for "print foo$bar" failed'
fi
unset bar
alias bar='print hello'
if	[[ $bar != '' ]]
then	err_exit 'alias bar cause variable bar to be set'
fi
alias !!=print
if	[[ $(!! hello 2>/dev/null) != hello ]]
then	err_exit 'alias for !!=print not working'
fi
alias foo=echo 
if	[[ $(print  "$(foo bar)" ) != bar  ]]
then	err_exit 'alias in command substitution not working'
fi
( unalias foo)
if	[[ $(foo bar 2> /dev/null)  != bar  ]]
then	err_exit 'alias not working after unalias in subshell'
fi
builtin -d rm 2> /dev/null
if	whence rm > /dev/null
then	[[ ! $(alias -t | grep rm= ) ]] && err_exit 'tracked alias not set'
	PATH=$PATH
	[[ $(alias -t | grep rm= ) ]] &&  err_exit 'tracked alias not cleared'
fi
exit $((Errors))
