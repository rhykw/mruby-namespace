##
## Unshare Test
##

assert("Namespace.unshare") do
  assert_equal(0, Namespace.unshare(Unshare::CLONE_NEWUSER))
end
