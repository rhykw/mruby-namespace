# mruby-namespace   [![Build Status](https://travis-ci.org/rhykw/mruby-namespace.svg?branch=master)](https://travis-ci.org/rhykw/mruby-namespace)
Unshare class
## install by mrbgems
- add conf.gem line to `build_config.rb`

```ruby
MRuby::Build.new do |conf|

    # ... (snip) ...

    conf.gem :github => 'rhykw/mruby-namespace'
end
```
## example
```ruby
> Unshare.getuid
 => 500
> Unshare.getgid
 => 500
>
 => nil
> Unshare.unshare(Unshare::CLONE_NEWUSER|Unshare::CLONE_NEWNET)
 => 0
>
 => nil
> Unshare.getuid
 => 65534
> Unshare.getgid
 => 65534
>
 => nil
> File.open('/proc/self/uid_map','wb+').write('0 500 1')
 => 7
> File.open('/proc/self/setgroups','w+').write('deny')
 => 4
> File.open('/proc/self/gid_map','wb+').write('0 500 1')
 => 7
>
 => nil
> Unshare.getuid
 => 0
> Unshare.getgid
 => 0
>
```

## License
under the MIT License:
- see LICENSE file
