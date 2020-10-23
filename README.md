# Get chrome cookies.

This tool is for getting chrome and other likely browser cookies, while these saved in the 
[Cookies] sqlite database are encrypted.

In default, chrome program path and data path has been set up according to running system.
you can run it without no parameters, especially in OSX.

But if error occured, it seems that you should specify `--chrome` and `--data` parameters manually.

You can also specify `--user` if the username used doesn't match the folders name.

In some chrome likely browser case, eg 360chrome, it maybe failed in default method, 
so I suggest you add `--force` parameter to close all existing chrome session and get
cookies in origin [user-data-dir], instead of creating a temporary directory and start
a new chrome session to avoid affecting users which is the defaut method that without
`--force`.

(Please eat with `EditThisCookie` extension.)
