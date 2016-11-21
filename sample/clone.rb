pid = Namespace.clone(Namespace::CLONE_NEWNS|Namespace::CLONE_NEWPID) do
  puts "=== Do remount by mount -t proc proc /proc && ps auxf"
  exec '/bin/bash'
end
puts "Spawn OK: #{pid}"
p Process.waitpid2(pid)
