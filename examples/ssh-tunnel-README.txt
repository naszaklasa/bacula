From: Joshua Kugler <joshua.kugler@uaf.edu>
To: bacula-users@lists.sourceforge.net
Subject: [Bacula-users] SSH tunneling mini-howto
Date: Tue, 13 Dec 2005 16:59:47 -0900

Here is an outline of the steps I took to get ssh tunneling to work for me.

NOTES:
I modified to ssh-tunnel.sh file from CVS because:
1) I didn't need director->client communications encrypted.  My main reason
for using SSH tunneling was so the clients in the DMZ could get back through
the firewall to connect to the storage server.
2) There was a bug in the method it used to get the PID of the tunnel.
It used 'cut -d" " -f1'  The problem was that ps sometimes has a leading space
in front of the PID if PID < 10,000, so cut would return a blank PID. 
Instead I used awk '{ print $1 }' and that worked even with leading spaces.
3) I also took out ssh's 'v' option for production work
4) I added '> /dev/null 2> /dev/null' because for some reason ssh wasn't fully
disconnecting from the terminal, thus the ssh-tunnel script would actually
hang the job
5) I changed it to exit with the status of the SSH command, so the job would
fail right away if the tunnel didn't go up.
6) The $CLIENT is now specified on the command line so it can be specified in
the Run Before Job directive.  As a result, you must specify the client when
you start *and* stop the tunnel.

OK, on to the how to:

1. I placed the attached script in /usr/local/bacula/scripts

2. I modified bacula-dir.conf to have a second Storage directive entry that
referenced the same storage resource in bacula-sd.conf. (Based on a recent 
e-mail, might this be dangerous?  Testing will tell.)

The modified Storage entry looked like this:

Storage {
  Name = herodotus-sd-ops
  Address = localhost
  SDPort = 9103
  Password = "Apassword"
  Device = AdicFastStor22
  Media Type = DLT8000
  Autochanger = yes
  Maximum Concurrent Jobs = 30
}

The Address is set to localhost, because when the tunnel is up, the client
will connect to localhost:9103 in order to connect to the Storage director.

3. In the client configuration, each client that uses this configuration has
these lines added:

Run Before Job =3D "/usr/local/bacula/scripts/ssh-tunnel start FQDN"
Run After Job =3D "/usr/local/bacula/scripts/ssh-tunnel stop FQDN"

=46QDN =3D fully qualifed domain name (i.e. full host name)

And their storage is set to "herodotus-sd-ops" (in our case, OPS is the name
of our DMZ).

4. Now, ssh keys must be created in order for all this to go on unattended.

At the prompt, type:

ssh -b 2048 -t dsa

(if you want less bit strength, you can use a number less than 2048)

When asked where to save the key, specify a location, or accept the default
Just remember the location, because you will have to put it in the script
(replace "/usr/local/bacula/ssh/id_dsa" with your file's location).  Make
sure the user as which bacula runs can read the file.

When asked for a password, leave that blank also as this will be running
unattended.

After it generates the key, it will save a file called id_dsa, and in that
same directory, there will be a file called id_dsa.pub, which is your public
SSH key.

On your backup client, create a user ('bacula' is probably a good choice).  
In that user's home directory, create a directory named '.ssh' (note the
leading dot).  In that directory, copy the id_dsa.pub file you create
earlier.  Once that file is in that user's directory copy it to a file name
"authorized_keys" in that same directory.  If you're not doing this as the
user, make sure the directory and files a owned by that user.  And for good
measure make sure only the user can read them.

5.  Now, the test.  Your keys are generated.  They are in place on the client. 
You've pointed your script to your private key's file (id_dsa).  Now, at the
prompt on your server type:

location_of_script/ssh-tunnel start client.host.name

then type

echo $?

That should be 0, which will mean everything went well.

If you need to debug, remove the redirection on the ssh command and add 'v' to
the switches for verbose output.

If the test went well, reload your modified config, and try running a job.  If
all goes well, the job report will look like it always does, save notices at
the top and bottom letting you know that the tunnel went up and down.

I've also gotten this to work on a Windows box using CopSSH (and OpenSSH
server for Windows), so this isn't a Unix-only solution.

The script is in <bacula-source>/examples/ssh-tunnel.sh
