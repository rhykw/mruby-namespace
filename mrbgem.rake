MRuby::Gem::Specification.new('mruby-linux-namespace') do |spec|
  spec.license = 'MIT'
  spec.authors = 'Russel Hunter Yukawa/Uchio Kondo'

  spec.add_dependency 'mruby-sprintf'

  spec.add_test_dependency 'mruby-dir'
  spec.add_test_dependency 'mruby-process'
  spec.add_test_dependency 'mruby-exec', github: 'haconiwa/mruby-exec'

  spec.linker.libraries << 'pthread'
end
