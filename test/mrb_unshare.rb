##
## Unshare Test
##

assert("Unshare.unshare") do
  assert_equal(0, Unshare.unshare(Unshare::CLONE_NEWUSER))
end
