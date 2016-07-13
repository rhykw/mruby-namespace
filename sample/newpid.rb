Namespace.unshare(Namespace::CLONE_NEWPID)

p = Process.fork do
  sleep 1
  puts "exit"
  system "ps auxf"
end

p(Process.waitpid2 p)
