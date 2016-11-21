(1..3).to_a.map do |i|
  Namespace.clone(Namespace::CLONE_NEWNS|Namespace::CLONE_NEWPID) do
    puts "Process: ##{i}"
    system "ls -l /proc/self/ns/pid"
    exec '/bin/sleep', (i * 5).to_s
  end
end

loop do
  begin
    p(Process.waitpid2(-1))
  rescue => e
    if e.message.include? "waitpid failed"
      puts "All over successfully"
      exit 0
    else
      raise e
    end
  end
end
