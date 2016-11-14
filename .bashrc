# .bashrc

# User specific aliases and functions

alias rm='rm -i'
alias cp='cp -i'
alias mv='mv -i'
alias grep='grep --color'
alias all='chmod 755 *'
alias toker='cd /usr/src/linux-2.4.18-14custom/kernel'
PATH=~/utils:$PATH

# Source global definitions
if [ -f /etc/bashrc ]; then
	. /etc/bashrc
fi
