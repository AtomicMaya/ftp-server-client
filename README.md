# FTP Server Client Implementation over TCP/IP in C #

Welcome to whatever the hell this was.

Long story short, this repo has **1859** lines of C code, so to whoever has to read this, good luck to you.

![Good code by XKCD](https://imgs.xkcd.com/comics/good_code.png)

In the context of the evaluation, I'm aiming for a solid 7, and I'm aware that quantity doesn't equate quality.

Which is why I tried to make this as solid as possible. I've tried breaking it on purpose, by accident, by making my cat sit on my laptop and others, it didn't work.

## Navigation ##

- [Environment](#environment)
- [How to use it](#how-to-use-it)
- [WARNING](#warning) (<--------- READ THIS, seriously)
- [SHA256 of the files](#sha256-of-the-files)
- [Credits](#credits)

## Environment ##

Running on WSL-Ubuntu 18.04.3 LTS, this works all of the time, but my some other WSL machines with strong firewalls can block sockets, so keep that in mind.

Using the `gcc` compiler version `7.4.0` (`Ubuntu 7.4.0-1ubuntu1~18.04.1`) and `make`.

## How to use it ##

The project has the following file structure (from the project root):

```bash
- client
 \_ files
    \_ ...
 \_ ,..
 \_ main.c
- common
 \_ utils.c
 \_ ...
- light_server
 \_ main.c
 \_ ...
- server
 \_ files
    \_ ...
 \_ ...
- .gitignore
- Makefile
- README.md
```

Before running the project, use the `Makefile` (from the root directory of the application) ! :

```bash
$> make
```

To run the project (from the root directory) :

Terminal 1:

```bash
$> ./light_server/ftp_lserver
```

Terminal 2..n:

```bash
$> ./client/ftp_client
SERVER IP $> 127.0.0.1
PORT $> 4242
...
```

## WARNING ##

Do not attempt to use `./server/ftp_server`, it requires a second argument which is a file descriptor (which by all accounts would not be open, thus unhappy segfaults).

Take the time to whitelist `ftp_client`, `ftp_lserver` and `ftp_server` in all your firewall systems. 

Note for Comodo IS users, unblock executables for all security controls and restart the applications.

## SHA256 of the files ##

```bash
>$ make sha
sha256sum client/*.h client/*.c common/*.h common/*.c server/*.h server/*.c light_server/*.h light_server/*.c
028510d6e49045ce3a8928aedbdcde53cec1afec41c175e25bbea88087314606  client/builtins.h
463f544ecfb0163eedb5013b8096c9e581e3dab84150c5ce6e012bbac1706abf  client/lineReader.h
4687ff442e8c39b9acd87819064d68140779f55a1236945760acbc67a668ea67  client/builtins.c
539e1c2dab872101e6357f120e00abc4982f1efdb716bf86c79e482d761c2236  client/lineReader.c
cc3092f1bd11256913d40890413e221381049550949838098a7c7a8190cf0e84  client/main.c
09d8c964944895a15d826fd602e5502c90bdb478c8fcfce865232b325b5a21ff  common/crc.h
e3b4d9d5bcd1e20a188c95a0319d8a6b08aad8734fc0ff0e32c542637220c472  common/fileHandler.h
5633d6f0c14c0c461a6d6f27969b06353babb48baf5c1c75220db2577ebde01a  common/utils.h
e0a56f64d61f324794894f6119e8315a6e5c6e55f5d8b64e04c654c5b7de099f  common/crc.c
157548c4a9944e1e5083eec2a8d0feb142e3b470aaa04f34ec722b19e14f407f  common/fileHandler.c
957f661411f1fe525ed0c0b2b8ad24f490204433cdd8cadfc2821a1b527855d5  common/utils.c
54dd692c010b0c3cb32810b8b670c4272b45bb31f58b3edeb031d623f629c45b  server/builtins.h
3b122daf6d40d3b2e0e6d296c58de8a77a64d022c8154ed780108503b477db0d  server/builtins.c
77221c2a50c09c44e93846876fa16e9f33e72cf74dadd35d242a1e4600c064d0  server/main.c
de563597266699b9bf63c9faabd94a8bc6f4dc6c97908d864ccfcc10c4ac717c  light_server/llist.h
5cf7af3abf00e21e24bd1f9680533a0feb3b6cd4fa24a01601954b25bdb7ff45  light_server/main.h
7294ac393d245a4a19f695cd25967cf6dd295924e7eae1fcc04bfcef7d67dd43  light_server/llist.c
6fc8c3b77f4012c4660f6682205deabaf2157fc40e23be7b300ef28efb9da50a  light_server/main.c
```

## Credits ##

Credits to xkcd for the image.
