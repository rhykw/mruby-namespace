##
## Unshare Test
##

assert("Namespace.unshare") do
  assert_equal(0, Namespace.unshare(Namespace::CLONE_NEWUSER))
end
