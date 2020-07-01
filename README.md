# Get chrome cookies.
## （and other likely browser）

In default, chrome program path and data path has been set up according to running system.

If running error, it seems that you should specify `--chrome` and `--data` parameters manually.

In some chrome likely browser case, eg 360chrome, it maybe failed in default method, 
so I suggest you add `--force` parameter to close all existing chrome session and get
cookies in origin [user-data-dir], instead of creating a temporary directory and start
a new chrome session to avoid affecting users which is the defaut method that without
`--force`.

(Please eat with `EditThisCookie` extension.)
