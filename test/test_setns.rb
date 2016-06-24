assert("Namespace.setns") do
  begin
    system "mkdir -p /tmp/foo"
    system "mkdir -p /tmp/bar"
    system "touch /tmp/bar/test.txt"

    pid1 = Process.fork do
      Namespace.unshare(Namespace::CLONE_NEWNS)
      system "mount --make-private /"
      system "mount --bind /tmp/bar /tmp/foo"
      loop {}
    end
    sleep 0.5

    ret = system "test -f /tmp/foo/test.txt"
    assert_false ret

    Namespace.setns(Namespace::CLONE_NEWNS, :pid => pid1)
    ret = system "test -f /tmp/foo/test.txt"
    assert_true ret

    system "umount /tmp/foo"

    Process.kill :TERM, pid1
  ensure
    system "rm -rf /tmp/foo"
    system "rm -rf /tmp/bar"
  end
end
