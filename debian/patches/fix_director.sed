s~Job[ ]+= "/etc/bacula/scripts/~Job = "/usr/lib/bacula/~
s~user = bacula; password = ""~user = @db_user@; password = "@db_pswd@"~
s~"/etc/bacula/scripts/make_catalog_backup bacula bacula"~"/etc/bacula/scripts/make_catalog_backup -u<user> -p<password>"~
