pid = Process.fork do
  Namespace.unshare(Namespace::CLONE_NEWUTS)
  system "hostname example.mruby.org"
  sleep 3600
end
sleep 1

Namespace.persist_ns(pid, Namespace::CLONE_NEWUTS, "/root/persist_test.nsfile")
Process.kill :INT, pid
p Process.waitpid2(pid)

pid = Process.fork do
  Namespace.setns(Namespace::CLONE_NEWUTS, fd: File.open("/root/persist_test.nsfile").fileno)
  puts "=== persisted hostname:"
  system "hostname"
end

p Process.waitpid2(pid)

puts "=== persisted mount point:"
system "mount | grep nsfile"
system "umount /root/persist_test.nsfile"
puts "=== umounted:"
system "mount | grep nsfile || echo NONE"
