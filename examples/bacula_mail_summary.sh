#!/bin/sh
# This script is to create a summary of the job notifications from bacula
# and send it to people who care.
#
# For it to work, you need to have all Bacula job report
# mails cc'd to a unix mailbox called 'bacula', but of course you can edit
# as appropriate. This should be run after all backup jobs have finished.
# Tested with bacula-1.38.0

# Contributed by Andrew J. Millar <andrew@alphajuliet.org.uk>

# Use awk to create the report, pass to column to be
# formatted nicely, then on to mail to be sent to
# people who care.
EMAIL_LIST="peoplewhocare@company.com"
awk -F\:\  'BEGIN {
                        print "Client Status Type StartTime EndTime Files Bytes"
                }
        /Client/ {
                CLIENT=$2; sub(/"/, "", CLIENT) ; sub(/".*$/, "", CLIENT)
        }
        /Backup Level/ {
                TYPE=$2 ; sub(/,.*$/, "", TYPE)
        }
        /Start time/ {
                STARTTIME=$2; sub(/.*-.*-.* /, "", STARTTIME)
        }
        /End time/ {
                ENDTIME=$2; sub(/.*-.*-.* /, "", ENDTIME)
        }
        /SD Files Written/ {
                SDFILES=$2
        }
         /SD Bytes Written/ {
                SDBYTES=$2
        }
        /Termination/ {
                TERMINATION=$2 ;
                sub(/Backup/, "", TERMINATION) ;
                printf "%s %s %s %s %s %s %s \n", CLIENT,TERMINATION,TYPE,STARTTIME,ENDTIME,SDFILES,SDBYTES}' /var/spool/mail/bacula | \
        column -t | \
        mail -s "Bacula Summary for `date -d yesterday +%a,\ %D`" ${EMAIL_LIST}
#
# Empty the mailbox
cat /dev/null > /var/spool/mail/bacula
#
# That's all folks
