Unshare.getuid
Unshare.getgid

Unshare.unshare(Unshare::CLONE_NEWUSER|Unshare::CLONE_NEWNET)

Unshare.getuid
Unshare.getgid

File.open('/proc/self/uid_map','wb+').write('0 500 1')
File.open('/proc/self/setgroups','w+').write('deny')
File.open('/proc/self/gid_map','wb+').write('0 500 1')

Unshare.getuid
Unshare.getgid
