##
## Unshare Test
##

## This test passed only run by superuser
assert("Namespace.unshare") do
  assert_equal(0, Namespace.unshare(Namespace::CLONE_NEWNS))
end
