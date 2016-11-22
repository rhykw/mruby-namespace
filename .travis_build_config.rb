MRuby::Build.new do |conf|
  if ENV['CC'] == 'clang'
    toolchain :clang
  else
    toolchain :gcc
  end
  conf.gembox 'default'
  conf.gem '../mruby-linux-namespace'
  unless ENV['CI']
    conf.gem mgem: 'mruby-io'
  end
  conf.enable_test
end
