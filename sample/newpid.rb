Namespace.unshare(Namespace::CLONE_NEWPID)

p = Process.fork do
  Namespace.unshare(Namespace::CLONE_NEWNS)

  system "mount --make-private /"
  system "umount /proc"
  system "mount -t proc proc /proc"
  system "sleep 10 &"
  system "sleep 10 &"
  system "sleep 10 &"
  system "ps auxf"
end

p(Process.waitpid2 p)

# system "mount -t proc proc /proc"
