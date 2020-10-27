# Get chrome cookies.

This tool is for getting chrome and other likely browser cookies(eg. 360chrome, qqbrowser), while these saved in the [Cookies] sqlite database are encrypted.

## Principle

When open chrome by `--remote-debugging-port` parameter, we can connect chrome debugger by websocket, and send debug command 
```json
{"id":1,"method":"Network.getAllCookies"}
```
to get the cookies. For concealment and performance, on Windows platform, we can addtionally add `--headless` to disable showing the GUI window. However, On macos/Linux Platform, Chrome disable to get cookies in diffrent context, and will merge windows in existed window. So I copy the cookies file to a temp user-data directory and open it without `--headless`, and you will see a window flashed.

If anyone else has a better method, please add issus, THX!

## Other

In default, chrome program path and data path has been set up according to running system. you can run it without no parameters, especially in OSX.

But if error occured, it seems that you should specify `--chrome` and `--data` parameters manually.

(Please eat with `EditThisCookie` extension.)
