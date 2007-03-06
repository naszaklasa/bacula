# .bashrc

# User specific aliases and functions

alias rm='rm -i'
alias cp='cp -i'
alias mv='mv -i'
alias dir='ls -l'

PS1='[\u@\h \W]\$ '
LESSCHARSET=latin1
export LESSCHARSET

# Source global definitions
if [ -f /etc/bashrc ]; then
        . /etc/bashrc
fi
