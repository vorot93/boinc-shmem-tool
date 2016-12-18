# boinc-shmem-tool
Front-end to [BOINC application API](https://github.com/volunode/boinc-app-api). Poll and edit memory shared with BOINC applications.

## Rationale
BOINC projects rely on individual applications for crunching itself. They are downloaded to volunteer's computer and are run by a BOINC client.

The communication between client and applications is done via C struct (`SHARED_MEM`) mapped to disk file (`boinc_mmap_file`) and shared between processes. While fast it presents several significant challenges:
* Loading struct data from disk is dangerous. Any process with same priveleges may potentially exploit C string handling.
* It is not possible to reimplement IPC protocol in any language except C and C++.
* Even with C/C++ this approach is fragile since **any change to said struct will break the protocol**.

This program is a black box that isolates impact from malicious applications and allows the clients to use stream-based I/O, a universal Unix communication mechanism.

## How it works
The client (or a prying user) launches this tool with first argument being the location of `boinc_mmap_file`.

boinc-shmem-tool uses standard streams as illustrated in the following scheme:
```
                                                ABI boundary              shmem-tool
                                                      |
    /---<-----<-----<--\                  /-<-1024-byte string-<--|     Process command       |---<---<--- stdin
App   1024-byte string      SHARED_MEM                |           |                           |
    \--->----->----->--/ (mapped to disk) \->-1024-byte string->--| Form response with result |--->--->--- stdout
                                                      |
```

For example:

Assuming the following message is available in shared memory's `app_status` channel:
```
<current_cpu_time>7.176276e+03</current_cpu_time>
<checkpoint_cpu_time>7.129676e+03</checkpoint_cpu_time>
<fraction_done>1.002963e-01</fraction_done>
```

The user can access it by entering into stdin:
```
{"action": "view", "channel":"app_status"}
```

And it will be pushed to stdout as:
```
{"action": "view", "channel":"app_status", "ok":true, "message":"<current_cpu_time>7.176276e+03</current_cpu_time>\n<checkpoint_cpu_time>7.129676e+03</checkpoint_cpu_time>\n<fraction_done>1.002963e-01</fraction_done>"}
```

### Available commands
* **view** - display the message stored in channel if available.
* **receive** - same as above, clear the channel afterwards.
* **delete** - set channel to empty.
* **send** - send message into channel, fail if already contains one.
* **force** - force send message, overwrite existing message.


## Requirements
* C++14
* boinc-app-api
* JsonCpp

## Installing
```
$ meson build
$ cd build
$ ninja
# ninja install
```
