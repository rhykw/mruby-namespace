# mruby-linux-namespace   [![Build Status](https://travis-ci.org/haconiwa/mruby-linux-namespace.svg?branch=master)](https://travis-ci.org/haconiwa/mruby-linux-namespace)

A mruby gem using unshare/setns/clone to control linux namespaces.

## install by mrbgems

- add conf.gem line to `build_config.rb`

```ruby
MRuby::Build.new do |conf|

  # ... (snip) ...

  conf.gem :github => 'haconiwa/mruby-linux-namespace'
  # mruby-process, mruby-exec, mruby-process-sys are also useful
end
```

## example

```ruby
## Using mruby-process-sys
> Process::Sys.getuid
 => 500
> Process::Sys.getgid
 => 500

> Namespace.unshare(Namespace::CLONE_NEWUSER|Namespace::CLONE_NEWNET)
 => 0

> Process::Sys.getuid
 => 65534 ... unknown
> Process::Sys.getgid
 => 65534

> File.open('/proc/self/uid_map','wb+').write('0 500 1')
 => 7
> File.open('/proc/self/setgroups','w+').write('deny')
 => 4
> File.open('/proc/self/gid_map','wb+').write('0 500 1')
 => 7
>
 => nil
> Process::Sys.getuid
 => 0
> Process::Sys.getgid
 => 0
>
```

### Available APIs

* `Namecpace.unshare(flag)`
* `Namecpace.setns(flag, pid: pid or fd: fd)`
* `Namecpace.clone(flag) { #do something... e.g. exec }` - Note that `clone(2)` support is experimental/Recommended to exec() fast

Please see `sample/*.rb` or `test/*.rb`

## License

under the MIT License: see LICENSE file
