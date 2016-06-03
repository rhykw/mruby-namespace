##
## Unshare Test
##

assert("Unshare#hello") do
  t = Unshare.new "hello"
  assert_equal("hello", t.hello)
end

assert("Unshare#bye") do
  t = Unshare.new "hello"
  assert_equal("hello bye", t.bye)
end

assert("Unshare.hi") do
  assert_equal("hi!!", Unshare.hi)
end
