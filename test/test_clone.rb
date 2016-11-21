assert("Namespace.clone") do
  begin
    system "mkdir -p /tmp/clone.res"
    system "rm -rf /tmp/clone.res/*"

    system "ls -l /proc/self/ns/pid > /tmp/clone.res/parent.pid.txt"
    system "ls -l /proc/self/ns/mnt > /tmp/clone.res/parent.mnt.txt"

    pid = Namespace.clone(Namespace::CLONE_NEWPID) do
      system "ls -l /proc/self/ns/pid > /tmp/clone.res/cloned.pid.txt"
      system "ls -l /proc/self/ns/mnt > /tmp/clone.res/cloned.mnt.txt"
    end
    Process.waitpid pid

    ret = system "diff -q /tmp/clone.res/parent.mnt.txt /tmp/clone.res/cloned.mnt.txt >/dev/null"
    assert_true ret

    ret = system "diff -q /tmp/clone.res/parent.pid.txt /tmp/clone.res/cloned.pid.txt >/dev/null"
    assert_false ret
  end
end
